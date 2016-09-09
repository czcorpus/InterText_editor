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

#ifndef IT_DOCUMENT_H
#define IT_DOCUMENT_H

#include <QtXml>
#include "ItElement.h"

#define NO_IDS_IN_DOCUMENT -2

class ItDocument
{
public:
    ItDocument();
    ~ItDocument();
    bool load(const QString &filename);
    bool setXml(QByteArray &xml);
    bool setXml(QString &xml);
    QString getXml();
    bool save(const QString &filename);
    bool hasIndex();
    void destroyIndex(QStringList alignedIds);
    int numLevels();
    QString numPrefix();
    QString numParentPrefix();
    void setNumLevels(int &value);
    void setNumPrefix(QString &prefix);
    void setNumParentPrefix(QString &prefix);
    void setIdNamespaceURI(QString &uri);
    ItElement* elementById(QString &id);
    void detectIdSystem(QStringList alignedIds);
    void collectElements(QList<ItElement*> * elements, QStringList &elNames, QDomNode node = QDomNode());
    QString errorMessage;
    QDomDocument getDomDoc();

private:
    int numbering_levels;
    QString numbering_prefix;
    QString numbering_parent_prefix;
    QString idNamespaceURI;
    QString filename;
    QString alignables;
    QDomDocument doc;
    QHash<QString,ItElement*> index;
    bool createIndex();
    bool scanNode(QDomNode node);
    void setUnknownNumbering(int numbering = 0);
    //QDomElement searchById(QDomElement &e, QString &id);
};

#endif
