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

#include "ItElement.h"

ItElement::ItElement(QDomElement el)
{
    element = el;
}

QString ItElement::getContents(bool prepend)
{
    QString string;
    QTextStream* str = new QTextStream(&string);
    element.save(*str,-1);
    delete str;
    string.remove(QRegExp("^<[^>]+>"));
    string.remove(QRegExp("</[^>]+>$"));
    if (prepend) {
        string = ' ' + string;
        if (first()) string = QString(QChar(0x25BA))+string;
        else string = QString(QChar(0x25A0))+string;
        //if (first()) string = QString::fromUtf8("‣‣")+string;
        //else string = QString::fromUtf8("‣")+string;
    }
    return string;
}

bool ItElement::updateContents(QString &string, bool trackChanges)
{
    QDomElement replacement = makeDuplicate(string);
    if (replacement.nodeName().isEmpty())
        return false;
    element.parentNode().replaceChild(replacement,element);
    element = replacement;
    if (trackChanges)
        replEnsure();
    return true;
}

void ItElement::replEnsure()
{
    if (!element.hasAttribute("it_tc_repl"))
        element.setAttribute("it_tc_repl", 1);
}

void ItElement::replInc()
{
    if (!element.hasAttribute("it_tc_repl"))
        element.setAttribute("it_tc_repl", 2);
    else
        element.setAttribute("it_tc_repl", element.attribute("it_tc_repl").toInt()+1);
}

int ItElement::repl()
{
    return element.attribute("it_tc_repl", "1").toInt();
}

void ItElement::setRepl(int val)
{
    element.setAttribute("it_tc_repl", val);
}

void ItElement::delRepl()
{
    if (element.hasAttribute("it_tc_repl"))
        element.removeAttribute("it_tc_repl");
}

QString ItElement::parbr()
{
    return element.attribute("it_tc_parbr","");
}

void ItElement::setParbr(QString val)
{
    if (val.isEmpty()) {
        if (element.hasAttribute("it_tc_parbr"))
            element.removeAttribute("it_tc_parbr");
    } else
        element.setAttribute("it_tc_parbr", val);
}

bool ItElement::isVirgin()
{
    return !element.hasAttribute("it_tc_repl");
}

void ItElement::setVirgin()
{
    element.removeAttribute("it_tc_repl");
}

ItElement * ItElement::clone(QString &newcontents, bool trackChanges) {
    QDomElement copy = makeDuplicate(newcontents);
    if (copy.isNull())
        return 0;
    if (trackChanges) {
        copy.setAttribute("it_tc_repl",0);
        copy.removeAttribute("it_tc_parbr");
    }
    element.parentNode().insertAfter(copy,element);
    return new ItElement(copy);
}

QDomElement ItElement::makeDuplicate(QString &newcontents) {
    QDomDocument tmp;
    QString string = QString("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<text>%1</text>\n").arg(newcontents.trimmed());
    if (!tmp.setContent(string)) return QDomElement();
    QDomElement newel = tmp.documentElement();
    // make newel have the same name and attributes as the original node
    newel.setTagName(element.tagName());
    QDomNamedNodeMap atts = element.attributes();
    for (int i = 0; i < atts.count(); i++) {
        newel.setAttribute(atts.item(i).toAttr().name(), atts.item(i).toAttr().value());
    }
    return element.ownerDocument().importNode(newel,true).toElement();
}

QString ItElement::getID(QString &namespaceURI)
{
    if (namespaceURI.isEmpty()) {
        return element.attribute("id", "");
    } else {
        return element.attributeNS(namespaceURI, "id", "");
    }
}

void ItElement::setID(const QString &id, QString &namespaceURI) {
    if (namespaceURI.isEmpty()) {
        element.setAttribute("id", id);
    } else {
        element.setAttributeNS(namespaceURI, "id", id);
    }
}

void ItElement::setParentID(const QString &id, QString &namespaceURI) {
    if (namespaceURI.isEmpty()) {
        element.parentNode().toElement().setAttribute("id", id);
    } else {
        element.parentNode().toElement().setAttributeNS(namespaceURI, "id", id);
    }
}

QString ItElement::getParentID(QString &namespaceURI)
{
    if (namespaceURI.isEmpty()) {
        return element.parentNode().toElement().attribute("id", "");
    } else {
        return element.parentNode().toElement().attributeNS(namespaceURI, "id", "");
    }
}

bool ItElement::first()
{
    if (element==element.parentNode().firstChildElement()) return true;
    else return false;
}

QDomElement ItElement::getParent() {
    return element.parentNode().toElement();
}

QList<QDomElement> ItElement::getDomElPath()
{
    QList<QDomElement> path;
    QDomElement el = this->element;
    path.prepend(el);
    while (el.parentNode().isElement()) {
        el = el.parentNode().toElement();
        path.prepend(el);
    }
    return path;
}
