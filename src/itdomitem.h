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

#ifndef ITDOMITEM_H
#define ITDOMITEM_H

#include <QDomNode>
#include <QHash>

class ItDomItem
{
public:
    enum Type { Attribute = 0, Element };

    ItDomItem(QDomNode &node, int row, ItDomItem *parent = 0);
    ~ItDomItem();
    ItDomItem *child(int i);
    ItDomItem *parent();
    QDomNode node() const;
    int row();
    Type getType();
    QString getInnerXml();
    void setValue(const QString &value);
    QDomNode cutChild(int i);
    bool insertChild(QDomNode &node, int pos = 0);

protected:
    void incRow();
    void decRow();

private:
    QDomNode domNode;
    Type type;
    QList<ItDomItem*> childItems;
    ItDomItem *parentItem;
    int rowNumber;

};

#endif // ITDOMITEM_H
