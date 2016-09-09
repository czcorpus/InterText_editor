/*  Copyright (c) 2010-2016 Pavel Vondřička (Pavel.Vondricka@korpus.cz)
 *  Copyright (c) 2010-2016 Charles University in Prague, Faculty of Arts,
 *                          Institute of the Czech National Corpus
 *
 *  This file is part of InterText Editor.
 *
 *  InterText Editor is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  InterText Editor is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with InterText Editor.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "ItDocument.h"

ItDocument::ItDocument() {
    numbering_levels = -1;
    numbering_prefix = ":";
    numbering_parent_prefix = "";
    idNamespaceURI = "";
}

ItDocument::~ItDocument() {
    //qDeleteAll(index); // use destroyIndex() before allowing the editor to split/merge elements!
}

void ItDocument::destroyIndex(QStringList alignedIds) {
    for (int i=0; i < alignedIds.size(); i++) // first remove items with linked elements
        index.remove(alignedIds.at(i));
    qDeleteAll(index); // delete all unused elements
    index.clear(); // clear the index
}

bool ItDocument::load(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        errorMessage = QObject::tr("Error opening file '%1':\n%2").arg(filename, file.errorString());
        return false;
    }
    QFileInfo fi(filename);
    QString errorMsg; int errorLine, errorColumn;
    if (!doc.setContent(&file, true, &errorMsg, &errorLine, &errorColumn)) {
        file.close();
        errorMessage = QObject::tr("Error parsing XML document '%1' at line %2, column %3: %4.").arg(fi.fileName(), QString().setNum(errorLine), QString().setNum(errorColumn), errorMsg);
        return false;
    }
    file.close();
    if (!createIndex()) {
        errorMessage = QObject::tr("Error indexing document '%1': %2").arg(fi.fileName(), errorMessage);
        return false;
    }
    return true;
}

bool ItDocument::setXml(QByteArray &xml)
{
    QString errorMsg; int errorLine, errorColumn;
    if (!doc.setContent(xml, true, &errorMsg, &errorLine, &errorColumn)) {
        errorMessage = QObject::tr("Error parsing XML document at line %1, column %2: %3.").arg(QString::number(errorLine), QString::number(errorColumn), errorMsg);
        return false;
    }
    // fix encoding in the XML header - the contents are now in unicode anyway, and they will be exported in UTF-8 by default
    if (doc.firstChild().isProcessingInstruction()) {
        doc.firstChild().toProcessingInstruction()
                .setData(doc.firstChild().toProcessingInstruction().data().replace(
                             QRegExp("encoding\\s*=\\s*[\"'][^\"']*[\"']", Qt::CaseInsensitive), "encoding=\"UTF-8\""));
    }
    return true;
}

bool ItDocument::setXml(QString &xml)
{
    QString errorMsg; int errorLine, errorColumn;
    if (!doc.setContent(xml, true, &errorMsg, &errorLine, &errorColumn)) {
        errorMessage = QObject::tr("Error parsing XML document at line %1, column %2: %3.").arg(QString::number(errorLine), QString::number(errorColumn), errorMsg);
        return false;
    }
    return true;
}

bool ItDocument::save(const QString &filename) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        errorMessage = QObject::tr("Error saving file '%1':\n%2").arg(filename, file.errorString());
        return false;
    }
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << doc.toString(2);
    file.close();
    return true;
}

QString ItDocument::getXml()
{
    QString xml;
    QTextStream out(&xml);
    out.setCodec("UTF-8");
    out << doc.toString(2);
    return xml;
}

bool ItDocument::createIndex()
{
    //QDomElement* docElem = &(doc.documentElement());
    //qDebug() << "Scanning document";
    return scanNode(doc.documentElement());
}

bool ItDocument::scanNode(QDomNode node)
{
    ItElement* itElement;
    QDomElement e = node.toElement();
    //qDebug() << "New element";
    if (!e.isNull()) {
        //QDomElement* element = static_cast<QDomElement*>(&node);
        //QDomElement element = QDomElement (node);
        QDomAttr a = e.attributeNode("id");
        QString id = "";
        if (!a.isNull() && a.namespaceURI()==idNamespaceURI) {
            id =  e.attribute("id","");
        }
        //if (!a.isNull() && a.namespaceURI()!=idNamespaceURI) {qDebug()<<"URI: "<<a.namespaceURI(); }
        //qDebug() << "Adding id:" << id;
        if (id != "") {
            if (index.contains(id)) {
                errorMessage = QObject::tr("Duplicate id '%1' for element '%2'.").arg(id, e.tagName());
                return false;
            }
            itElement = new ItElement(e);
            index[id] = itElement;
        }
        QDomNodeList children = e.childNodes();
        for (unsigned int i=0; i<children.length(); ++i) {
            if (!scanNode(children.at(i)))
                return false;
        }
    }
    return true;
}

void ItDocument::collectElements(QList<ItElement*> * elements, QStringList &elNames, QDomNode node)
{
    if (node.isNull()) {
        collectElements(elements, elNames, doc.documentElement());
        return;
    } else {
        QDomElement e = node.toElement();
        if (!e.isNull()) {
            if (elNames.contains(e.tagName(), Qt::CaseInsensitive)) {
                elements->append(new ItElement(e));
            } else {
                QDomNodeList children = e.childNodes();
                for (unsigned int i=0; i<children.length(); ++i) {
                    collectElements(elements, elNames, children.at(i));
                }
            }
        }
    }
}

void ItDocument::detectIdSystem(QStringList alignedIds) {
    //QStringList keys = index.keys();
    /* Check first ID */
    if (alignedIds.isEmpty()) return;
    QString temp;
    QString id = "";
    //while (id=="" || !index.contains(id))
    id = alignedIds.takeFirst();
    ItElement * e = elementById(id);
    if (e==0) {
        setUnknownNumbering(NO_IDS_IN_DOCUMENT);
        return;
    }
    QString parent_id = e->getParentID(idNamespaceURI);
    if (id.startsWith(parent_id)) {
        //qDebug() << "First ID starts with parent's ID: 2-level(?).";
        numbering_parent_prefix = parent_id;
        numbering_parent_prefix.remove(QRegExp("[0-9]+$"));
        if (numbering_parent_prefix.size()<parent_id.size()) {
            numbering_levels = 2;
            numbering_prefix = id;
            numbering_prefix.remove(QRegExp(QString("^%1").arg(parent_id)));
            temp = numbering_prefix;
            numbering_prefix.remove(QRegExp("[0-9]+$"));
            if (numbering_prefix.size()==temp.size()) {
                //qDebug() << "No number in the first ID (2-level):" << id << numbering_prefix << temp;
                setUnknownNumbering();
                return;
            }
        } else {
            //qDebug() << "Parents are not numbered: 1-level.";
            numbering_levels = 1;
            numbering_parent_prefix = "";
        }
    } else {
        //qDebug() << "First ID does not start with parent's ID: 1-level(?)." << id << parent_id;
        numbering_prefix = id;
        numbering_prefix.remove(QRegExp("[0-9]+$"));
        if (numbering_prefix.size()==id.size()) {
            //qDebug() << "No number in the first ID (1-level):" << id;
            setUnknownNumbering();
            return;
        }
        numbering_levels = 1;
        numbering_parent_prefix = "";
    }
    //qDebug() << "Verifying all aligned elements.";
    /* Verify all IDs */
    while (!alignedIds.isEmpty()) {
        id = alignedIds.takeFirst();
        if (id=="" || !index.contains(id)) {
            setUnknownNumbering();
            return;
        } // continue;
        //qDebug() << id;
        if (numbering_levels==1) {
            //temp = id;
            if (id.remove(QRegExp("[0-9]+$"))!=numbering_prefix) {
                //qDebug() << "No number in the ID (1-level):" << temp << id << numbering_prefix;
                setUnknownNumbering();
                return;
            }
        } else {
            parent_id = index.value(id)->getParentID(idNamespaceURI);
            //temp = id;
            if (id.remove(QRegExp("[0-9]+$")).remove(QRegExp(QString(numbering_prefix).append("$")))!=parent_id) {
                //qDebug() << "No parent ID" << parent_id << "or prefix" << numbering_prefix << "or number in the ID (2-level):" << temp;
                setUnknownNumbering();
                return;
            }
            //temp = parent_id;
            if (parent_id.remove(QRegExp("[0-9]+$"))!=numbering_parent_prefix) {
                //qDebug() << "No number in the parent ID (2-level):" << temp;
                setUnknownNumbering();
                return;
            }
        }
    }
    /* check for one common parent - revised: why not?? */
    /*if (numbering_levels==2 && first_parent_id==parent_id) {
    //qDebug() << "There is only one common parent! (1-level)";
    numbering_levels = 1;
    numbering_prefix = parent_id;
  }*/
}

void ItDocument::setUnknownNumbering(int numbering) {
    numbering_levels = numbering;
    numbering_prefix = "";
    numbering_parent_prefix = "";
}

ItElement* ItDocument::elementById(QString &id)
{
    if (index.isEmpty())
        createIndex();
    return index.value(id,0);
}

/*QDomElement ItDocument::searchById(QDomElement &e, QString &id)
{
    if (e.attribute("id","") == id) return e;
    else {
        QDomNodeList children = e.childNodes();
        for (unsigned int i=0; i<children.length(); ++i) {
            QDomElement child = children.at(i).toElement();
            if (!child.isNull()) {
                QDomElement found = searchById(child,id);
                if (!found.isNull()) return found;
            }
        }
    }
    return QDomElement();
}*/


int ItDocument::numLevels() {
    return numbering_levels;
}

QString ItDocument::numPrefix() {
    return numbering_prefix;
}

QString ItDocument::numParentPrefix() {
    return numbering_parent_prefix;
}

void ItDocument::setNumLevels(int &value) {
    numbering_levels = value;
}

void ItDocument::setNumPrefix(QString &prefix) {
    numbering_prefix = prefix;
}

void ItDocument::setNumParentPrefix(QString &prefix) {
    numbering_parent_prefix = prefix;
}

void ItDocument::setIdNamespaceURI(QString &uri) {
    idNamespaceURI = uri;
}

bool ItDocument::hasIndex()
{
    return !index.isEmpty();
}

QDomDocument ItDocument::getDomDoc()
{
    return doc;
}
