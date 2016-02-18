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

#ifndef ITDOMMODEL_H
#define ITDOMMODEL_H


#include <QAbstractItemModel>
#include <QDomDocument>
#include <QModelIndex>
#include <QVariant>
#include "itdomitem.h"
#include "ItAlignment.h"

class ItDomModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ItDomModel(ItAlignment * alignment, aligned_doc doc, QObject *parent = 0);
    ~ItDomModel();

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QDomNode getRootNode();
    aligned_doc getDocNum();
    QDomNode getPasteNode();
    void setPasteNode(QDomNode &node);

    bool setValue(const QModelIndex &index, const QString &value);
    bool addAttribute(const QModelIndex &index, const QString &name);
    bool delAttribute(const QModelIndex &index, const QString &name);
    bool addChildEl(const QModelIndex &parent, int beforePos, const QString &name);
    bool cutItem(const QModelIndex &index);
    bool pasteItem(const QModelIndex &parent, int beforePos = 0);

    ItAlignment * al;

private:
    aligned_doc docNum;
    ItDomItem *rootItem;
    QStringList atomicElementNames;
    QDomNode lastRemovedNode;
};

#endif // ITDOMMODEL_H
