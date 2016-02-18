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

#ifndef IT_SEGMENT_DELEGATE_H
#define IT_SEGMENT_DELEGATE_H

#include <QtWidgets>
#include <QtXml>
#include "ItAbstractDelegate.h"
// #include "ItSegmentView.h"
//#include "ItPlainTextEdit.h"

class ItSegmentView;
class ItAlignmentView;

class ItSegmentDelegate : public ItAbstractDelegate
{
	Q_OBJECT

public:
  ItSegmentDelegate(QObject *parent, ItAlignmentView * aview);
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void setEditorData(QWidget *editor, const QModelIndex &index) const;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const;
protected:
  bool eventFilter(QObject *obj, QEvent *ev);
  ItAlignmentView * alview;
//private slots:
//  void onClose ( QWidget * editor, QAbstractItemDelegate::EndEditHint hint = NoHint );
};

#endif
