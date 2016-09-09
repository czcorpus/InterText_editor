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

#include <QtGui>
#include <QtXml>

#include "itdomitem.h"
#include "itdommodel.h"

ItDomModel::ItDomModel(ItAlignment *alignment, aligned_doc doc, QObject *parent)
    : QAbstractItemModel(parent)
{
    al = alignment;
    docNum = doc;
    QDomDocument domDocument = al->getDocument(docNum)->getDomDoc();
    rootItem = new ItDomItem(domDocument, 0);
    atomicElementNames = al->getAlignableElementnames(docNum);
}

ItDomModel::~ItDomModel()
{
    delete rootItem;
}

int ItDomModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 3;
}

QVariant ItDomModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    ItDomItem *item = static_cast<ItDomItem*>(index.internalPointer());

    QDomNode node = item->node();
    //QStringList attributes;

    switch (index.column()) {
    case 0:
        if (item->getType() == ItDomItem::Element && !atomicElementNames.contains(item->parent()->node().nodeName()))
            return node.nodeName();
        else
            return QVariant();
    case 1:
        if (item->getType() == ItDomItem::Attribute)
            return node.nodeName();
        else if (atomicElementNames.contains(item->parent()->node().nodeName()))
            return tr("#text");
        else
            return QVariant();
    case 2:
        if (item->getType()==ItDomItem::Element && atomicElementNames.contains(item->parent()->node().nodeName()))
            return item->parent()->getInnerXml();
        else
            return node.nodeValue();//.split("\n").join(" ");
    default:
        return QVariant();
    }
}

Qt::ItemFlags ItDomModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;
    ItDomItem *item = static_cast<ItDomItem*>(index.internalPointer());
    QDomNode node = item->node();

    if (index.column() == 2) {
        if (item->getType() == ItDomItem::Attribute) {
            if (node.nodeName() != "id" && !node.nodeName().startsWith("it_"))
                return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
            else
                return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        } else if (item->getType()==ItDomItem::Element) {
            //if (atomicElementNames.contains(item->parent()->node().nodeName()))
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            /*else
                return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;*/
        } else
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    } else {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
}

QVariant ItDomModel::headerData(int section, Qt::Orientation orientation,
                                int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0:
            return tr("Element");
        case 1:
            return tr("Attribute");
        case 2:
            return tr("Value");
        default:
            return QVariant();
        }
    }

    return QVariant();
}

QModelIndex ItDomModel::index(int row, int column, const QModelIndex &parent)
const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    ItDomItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<ItDomItem*>(parent.internalPointer());

    ItDomItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex ItDomModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    ItDomItem *childItem = static_cast<ItDomItem*>(child.internalPointer());
    ItDomItem *parentItem = childItem->parent();

    if (!parentItem || parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int ItDomModel::rowCount(const QModelIndex &parent) const
{

    if (parent.column() > 0)
        return 0;

    ItDomItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<ItDomItem*>(parent.internalPointer());

    if (atomicElementNames.contains(parentItem->node().nodeName()))
        return parentItem->node().attributes().count() + 1;
    else {
        int childCnt = 0;
        if (parentItem->getType() == ItDomItem::Element)
            childCnt = parentItem->node().childNodes().count();
        return parentItem->node().attributes().count() + childCnt;
    }
}


bool ItDomModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.column()!=2)
        return false;

    if (index.isValid() && index.parent().isValid() && role == Qt::EditRole && value.toString()!=data(index,Qt::EditRole)) {
        QString text = value.toString();
        setValue(index, text);
        return true;
    }
    return false;
}

QDomNode ItDomModel::getRootNode()
{
    return rootItem->node();
}

bool ItDomModel::setValue(const QModelIndex &index, const QString &value)
{
    if (!index.isValid())
        return false;

    ItDomItem *item = static_cast<ItDomItem*>(index.internalPointer());
    /*if (item->getType() != ItDomItem::Attribute)
        return false;*/

    item->setValue(value);
    al->setDocDepCTime(docNum, QDateTime::currentDateTime());
    //emit dataChanged(index, index);
    //emit layoutChanged();
    return true;
}

bool ItDomModel::addAttribute(const QModelIndex &index, const QString &name)
{
    if (!index.isValid())
        return false;

    ItDomItem *item = static_cast<ItDomItem*>(index.internalPointer());
    if (item->getType() != ItDomItem::Element)
        return false;
    QDomElement node = QDomElement();
    node.setAttribute(name, "");
    QDomAttr attr = node.attributeNode(name);
    if (!item->insertChild(attr))
        return false;
    al->setDocDepCTime(docNum, QDateTime::currentDateTime());
    emit layoutChanged();
    return true;
}

bool ItDomModel::delAttribute(const QModelIndex &index, const QString &name)
{
    int row = 0;
    ItDomItem *item = 0;
    bool found = false;
    QModelIndex attrIndex;
    while (row < rowCount(index)) {
        attrIndex = this->index(row,0,index);
        item = static_cast<ItDomItem*>(attrIndex.internalPointer());
        if (!item->getType() == ItDomItem::Attribute)
            return false;
        if (item->node().nodeName() == name) {
            found = true;
            break;
        }
    }
    if (!found)
        return false;
    return cutItem(attrIndex);
}

bool ItDomModel::addChildEl(const QModelIndex &parent, int beforePos, const QString &name)
{
    if (!parent.isValid())
        return false;

    ItDomItem *item = static_cast<ItDomItem*>(parent.internalPointer());
    if (item->getType() != ItDomItem::Element)
        return false;
    QDomElement node = QDomElement();
    node.setTagName(name);
    if (!item->insertChild(node, beforePos))
        return false;
    al->setDocDepCTime(docNum, QDateTime::currentDateTime());
    emit layoutChanged();
    return true;
}

bool ItDomModel::cutItem(const QModelIndex &index)
{
    if (!index.isValid() || !index.parent().isValid())
        return false;

    ItDomItem *item = static_cast<ItDomItem*>(index.parent().internalPointer());
    if (item->getType() != ItDomItem::Element)
        return false;
    lastRemovedNode = item->cutChild(index.row());
    al->setDocDepCTime(docNum, QDateTime::currentDateTime());
    emit layoutChanged();
    return true;
}

bool ItDomModel::pasteItem(const QModelIndex &parent, int beforePos)
{
    if (!parent.isValid())
        return false;

    if (!lastRemovedNode.isAttr() && !lastRemovedNode.isElement())
        return false;

    ItDomItem *item = static_cast<ItDomItem*>(parent.internalPointer());
    if (item->getType() != ItDomItem::Element)
        return false;

    if (!item->insertChild(lastRemovedNode, beforePos))
        return false;
    lastRemovedNode = QDomNode();
    al->setDocDepCTime(docNum, QDateTime::currentDateTime());
    emit layoutChanged();
    return true;
}

aligned_doc ItDomModel::getDocNum()
{
    return docNum;
}

QDomNode ItDomModel::getPasteNode()
{
    return lastRemovedNode;
}

void ItDomModel::setPasteNode(QDomNode &node)
{
    lastRemovedNode = node;
}
