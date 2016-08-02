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

#ifndef ITPLAINTEXTEDIT_H
#define ITPLAINTEXTEDIT_H

#include <QtWidgets>
#include "ItCommon.h"

class ItPlainTextEdit : public QPlainTextEdit
{
  Q_OBJECT
  int fitting_height;

public:
  AutoState haveAsked;
  bool insertNext;
  QModelIndex index;
  ItPlainTextEdit(QWidget *parent, EditorKeys keys);
  ~ItPlainTextEdit();
  QSize sizeHint() const;
  bool hasBeenChanged();
  void setGeometry(const QRect &rect);
  QShortcut * saveExit;
  QShortcut * discardExit;
  QShortcut * saveNext;
  QShortcut * savePrev;
public slots:
  //void fitHeightToDocument();
  void resetChangeFlag();
  void showContextMenu(const QPoint &pt);
  void splitAtCursor();
  //void setPlainText(const QString &text);
  //void updateGeometry();
signals:
  void closeEditor(QWidget * editor, QAbstractItemDelegate::EndEditHint hint = QAbstractItemDelegate::NoHint);
  void sizeHintChanged(int row);
protected:
  bool textChangeFlag;
  void mousePressEvent(QMouseEvent *mouseEvent);
  //bool event ( QEvent * e );
  QAction * splitAct;
protected slots:
  void setTextChangeFlagTrue();
};

#endif // ITPLAINTEXTEDIT_H
