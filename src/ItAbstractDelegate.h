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

#ifndef IT_ABSTRACTDELEGATE_H
#define IT_ABSTRACTDELEGATE_H

#include <QtWidgets>
#include "ItCommon.h"
#include "ItAlignmentModel.h"
#include "ItQuestionDialog.h"
#include "ItPlainTextEdit.h"

class ItAbstractDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    //static AutoState autoSaveElement;
    explicit ItAbstractDelegate(QObject *parent = 0);
    //ItSegmentView * segview;
    //QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    //void setEditorData(QWidget *editor, const QModelIndex &index) const;
    //void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    bool getHtmlView() const;
    void setHtmlView(bool set);
    //static void setAutoSaveElement(AutoState value);
    //static AutoState getAutoSaveElement();
signals:
    void commitData(QWidget *editor, QAbstractItemDelegate::EndEditHint hint = QAbstractItemDelegate::NoHint);
    void insertNextRequested();
public slots:
    //void onClose ( QWidget * editor, QAbstractItemDelegate::EndEditHint hint = NoHint );
    void editorCloseRequested(QWidget * editor, QAbstractItemDelegate::EndEditHint hint = QAbstractItemDelegate::NoHint, bool insertNext = false);
protected:
    QWidget * myparent;
    //bool eventFilter(QObject *obj, QEvent *ev);
    //void registerSegView(ItSegmentView * view);
private:
    bool htmlView;
    void createContents(QTextDocument * doc, const QStyleOptionViewItem & option, const QModelIndex & index) const;
private slots:
};

#endif // ITABSTRACTDELEGATE_H
