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

#include <QtXml>

#include "itdomitem.h"

ItDomItem::ItDomItem(QDomNode &node, int row, ItDomItem *parent)
{
    domNode = node;
    // Record the item's location within its parent.
    rowNumber = row;
    parentItem = parent;
    type = ItDomItem::Element;
    if (node.isAttr())
        type = ItDomItem::Attribute;
}

ItDomItem::~ItDomItem()
{
    for (int i = 0; i < childItems.size() ; i++)
        delete childItems[i];
}

QDomNode ItDomItem::node() const
{
    return domNode;
}

ItDomItem *ItDomItem::parent()
{
    return parentItem;
}

ItDomItem *ItDomItem::child(int i)
{
    if (childItems.size()>0)
        return childItems[i];

    int attrCnt = domNode.attributes().count();
    int childCnt = 0;
    if (type == Element)
        childCnt = domNode.childNodes().count();

    for (int n = 0; n < attrCnt; n++) {
        QDomNode childNode = domNode.attributes().item(n);
        ItDomItem *childItem = new ItDomItem(childNode, n, this);
        childItems.append(childItem);
    }

    for (int n = attrCnt; n < attrCnt+childCnt; n++) {
        QDomNode childNode = domNode.childNodes().item(n-attrCnt);
        ItDomItem *childItem = new ItDomItem(childNode, n, this);
        childItems.append(childItem);
    }
    return childItems[i];
}

int ItDomItem::row()
{
    return rowNumber;
}

void ItDomItem::incRow()
{
    rowNumber++;
}

void ItDomItem::decRow()
{
    rowNumber--;
}

ItDomItem::Type ItDomItem::getType()
{
    return type;
}

QString ItDomItem::getInnerXml()
{
    QString string;
    QTextStream* str = new QTextStream(&string);
    node().save(*str,-1);
    delete str;
    string.remove(QRegExp("^<[^>]+>"));
    string.remove(QRegExp("</[^>]+>$"));
    return string;
}

void ItDomItem::setValue(const QString &value)
{
    domNode.setNodeValue(value);
}

QDomNode ItDomItem::cutChild(int i)
{
    if (type == Attribute)
        return QDomNode();

    ItDomItem * itchild = childItems.takeAt(i);
    QDomNode node = itchild->node();
    if (itchild->getType() == Attribute) {
        domNode.toElement().removeAttribute(node.nodeName());
    } else {
        domNode.removeChild(node);
    }
    delete itchild;
    for (int j=i; j<childItems.size(); j++) {
        childItems[j]->decRow();
    }
    return node;
}

bool ItDomItem::insertChild(QDomNode &node, int pos)
{
    if (type == Attribute)
        return false;

    QDomElement el = domNode.toElement();

    if (node.isAttr()) {
        pos = domNode.attributes().count();
        if (el.hasAttribute(node.nodeName()))
            return false;

        el.setAttributeNode(node.toAttr());
    } else {
        if (pos < domNode.attributes().count())
            return false;

        QDomNode tmp;
        if (pos < childItems.count()) {
            if (( tmp = el.insertBefore(node, childItems.at(pos)->node()) )==QDomNode())
                return false;
        } else {
            pos = childItems.count() - 1;
            if (( tmp = el.insertAfter(node, childItems.at(pos)->node()) )==QDomNode())
                return false;
        }
    }
    ItDomItem * newitchild = new ItDomItem(node, pos, this);
    childItems.insert(pos, newitchild);
    for (int j=pos+1; j<childItems.size(); j++) {
        childItems[j]->incRow();
    }
    return true;
}
