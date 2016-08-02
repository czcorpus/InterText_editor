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

#ifndef IT_SEGMENT_VIEW_H
#define IT_SEGMENT_VIEW_H

#include <QtWidgets>
#include "ItSegmentDelegate.h"
#include "ItAlignmentModel.h"
#include "ItCommands.h"

class ItAlignmentView;

class ItSegmentView : public QListView
{
	Q_OBJECT

public:
  QModelIndex index;
  ItPlainTextEdit * myeditor;
  ItSegmentView(QWidget * parent, ItAlignmentView * parAlView);
	~ItSegmentView();
    //int heightHint();
  void setHtmlView(bool set);
  bool isEditing();
  void setEditor(ItPlainTextEdit * editor);
  QSize sizeHint() const;
  void 	setGeometry(const QRect &rect);
  void adjustGeometry();
public slots:
  void closeAnyEditor();
  void closeEditor(QWidget* editor,QAbstractItemDelegate::EndEditHint hint);
  void resizeRowToContents(int row);
  void autoOpenEditor(QAbstractItemDelegate::EndEditHint hint);
signals:
	void editingStarted();
	void editingFinished();
    void editingCancelled();
	void cursorChanged();
  void focusChanged();
  void sizeHintChanged(int row);
  void wantBeClosed(QWidget * widget, QAbstractItemDelegate::EndEditHint hint = QAbstractItemDelegate::NoHint);
protected:
	void keyPressEvent (QKeyEvent * event);
  void focusOutEvent( QFocusEvent * event );
  void focusInEvent( QFocusEvent * event );
    void currentChanged ( const QModelIndex & current, const QModelIndex & previous );
   // bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *event);
protected slots:
	void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void commitData (QWidget * editor );
  void commitData (QWidget * editor , QAbstractItemDelegate::EndEditHint hint );
  void mayCloseEditor ( QWidget * editor, QAbstractItemDelegate::EndEditHint hint = QAbstractItemDelegate::NoHint );
private:
  void handleCloseHint(QAbstractItemDelegate::EndEditHint hint);
    //bool editorOpen;
    ItSegmentDelegate * delegate;
    ItAlignmentView * alView;
    QAbstractItemDelegate::EndEditHint autoEditHint;
};

#endif
