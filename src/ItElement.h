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

#ifndef IT_ELEMENT_H
#define IT_ELEMENT_H

#include <QtWidgets>
#include <QtXml>

class ItElement
{
public:
    QDomElement element;
    ItElement(QDomElement el);
    QString getContents(bool prepend = true);
    bool updateContents(QString &string, bool trackChanges = false);
    ItElement * clone(QString &newcontents, bool trackChanges = false);
    QString getID(QString &namespaceURI);
    QString getParentID(QString &namespaceURI);
    QDomElement getParent();
    int num;
    void setID(const QString &id, QString &namespaceURI);
    void setParentID(const QString &id, QString &namespaceURI);
    bool first();
    // tracking changes
    int repl();
    void setRepl(int val);
    void delRepl();
    QString parbr();
    void setParbr(QString val);
    bool isVirgin();
    void setVirgin();
    QList<QDomElement> getDomElPath();

private:
    QDomElement makeDuplicate(QString &newcontents);
    // tracking changes
    void replEnsure();
    void replInc();
};

#endif
