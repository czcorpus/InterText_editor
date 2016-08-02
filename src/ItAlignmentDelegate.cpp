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

#include "ItAlignmentDelegate.h"
#include "ItAlignmentView.h"

ItAlignmentDelegate::ItAlignmentDelegate(QObject *parent) : ItAbstractDelegate(parent) {
    alview = static_cast<ItAlignmentView*>(parent);
  //segview = 0;
    //mytxtedit = 0;
    //connect(this, SIGNAL(closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)), this, SLOT(onClose(QWidget*,QAbstractItemDelegate::EndEditHint)));
}

QWidget *ItAlignmentDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  if (!index.isValid() || (!index.parent().isValid() && (index.column()!=1 && index.column()!=2)))
      return 0;
  //ItAlignmentView * alview = static_cast<ItAlignmentView*>(parent->parentWidget());
  if (index.model()->rowCount(index)==1) {
    //alview->setRowHeight(index.row(), alview->rowHeight(index.row())+16);
      alview->nexthint = QAbstractItemDelegate::NoHint;
      ItPlainTextEdit * editor = new ItPlainTextEdit(parent, alview->getEditorKeys());
    //editor->setFocusPolicy(Qt::StrongFocus); // opening within segview does not work
    editor->index = index;
    connect(editor, SIGNAL(closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)),
            this, SLOT(editorCloseRequested(QWidget*,QAbstractItemDelegate::EndEditHint)));
    connect(editor, SIGNAL(sizeHintChanged(int)), alview, SLOT(resizeRowToContents(int)));
/*    connect(editor->saveExit, SIGNAL(activated()), this, SLOT(slotSaveExit()));
    connect(editor->discardExit, SIGNAL(activated()), this, SLOT(slotDiscardExit()));
    connect(editor->saveNext, SIGNAL(activated()), this, SLOT(slotSaveNext()));
    connect(editor->savePrev, SIGNAL(activated()), this, SLOT(slotSavePrev()));*/
    //mytxtedit = editor;
    alview->setEditor(editor);
    return editor;
  } else {
		//qDebug() << "Creating editor";
    ItAlignmentModel * model = static_cast<ItAlignmentModel*>(alview->model());
    //int add=(model->rowCount(index))*16;
    //alview->setRowHeight(index.row(), alview->rowHeight(index.row())+add);
    ItSegmentView * view = new ItSegmentView(parent, alview);
    //registerSegView(view);
    view->index = index;
    view->setModel(model);
    view->setHtmlView(getHtmlView());
    view->setFont(alview->font());
    view->setRootIndex(model->index(index.row(),index.column(),QModelIndex()));
    //int add=(view->model()->rowCount(view->rootIndex()))*8;
    //view->setGeometry(option.rect.adjusted(0,0,0,add));
    view->setWordWrap(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    //view->horizontalHeader()->setResizeMode(0,QHeaderView::ResizeToContents);
    //view->verticalHeader()->setResizeMode(0,QHeaderView::ResizeToContents);
    //view->horizontalHeader()->setHidden(true);
    //view->verticalHeader()->setHidden(true);
    view->setFocusPolicy(Qt::StrongFocus);
    //alview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    view->setCurrentIndex(model->index(0,0,index));
    //if (model->rowCount(index)==1)
    //	view->edit(model->index(0,0,index));
    alview->setSegView(view);
    //alview->resizeRowToContents(index.row());
    connect(view, SIGNAL(sizeHintChanged(int)), alview, SLOT(resizeRowToContents(int)));
    connect(view, SIGNAL(wantBeClosed(QWidget*,QAbstractItemDelegate::EndEditHint, bool)),
            this, SLOT(editorCloseRequested(QWidget*,QAbstractItemDelegate::EndEditHint, bool)));
    connect(view, SIGNAL(editingCancelled()), model, SLOT(undo()), Qt::QueuedConnection);
    if (alview->nexthint != QAbstractItemDelegate::NoHint)
        view->autoOpenEditor(alview->nexthint);
    alview->nexthint = QAbstractItemDelegate::NoHint;
    return view;
  }
}

/*void ItAlignmentDelegate::registerSegView(ItSegmentView * view) {
  segview = view;
}*/

void ItAlignmentDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    ItAbstractDelegate::setEditorData(editor, index);
    if (QString(editor->metaObject()->className())=="ItPlainTextEdit") {
        ItPlainTextEdit * texted = static_cast<ItPlainTextEdit*>(editor);
        texted->resetChangeFlag();
    }
}


void ItAlignmentDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if (QString(editor->metaObject()->className()) == "ItPlainTextEdit") {
    ItPlainTextEdit *texted = static_cast<ItPlainTextEdit*>(editor);
		QString text = texted->toPlainText();
		model->setData(model->index(0,0,index), text, Qt::EditRole);
	}
}

bool ItAlignmentDelegate::eventFilter(QObject *obj, QEvent *ev) {
    QWidget *editor = qobject_cast<QWidget*>(obj);
    if (!editor)
        return false;
    //qDebug()<<QString(editor->metaObject()->className())<<ev->type();
    if (ev->type() == QEvent::KeyPress) {
        //switch (static_cast<QKeyEvent *>(ev)->key()) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(ev);
        int keyInt = keyEvent->key();
        Qt::Key key = static_cast<Qt::Key>(keyInt);
        if (key == Qt::Key_unknown) {
            return false;
        }
        // the user have clicked just and only the special keys Ctrl, Shift, Alt, Meta.
        if(key == Qt::Key_Control ||
                key == Qt::Key_Shift ||
                key == Qt::Key_Alt ||
                key == Qt::Key_Meta){
            return false;
        }

        // check for a combination of user clicks
        Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
        if(modifiers & Qt::ShiftModifier)
            keyInt += Qt::SHIFT;
        if(modifiers & Qt::ControlModifier)
            keyInt += Qt::CTRL;
        if(modifiers & Qt::AltModifier)
            keyInt += Qt::ALT;
        if(modifiers & Qt::MetaModifier)
            keyInt += Qt::META;

        QKeySequence keycode = QKeySequence(keyInt);

        if (keycode == alview->getEditorKeys().saveExit) { //: //Qt::Key_F2:
            if (QString(editor->metaObject()->className()) == "ItPlainTextEdit") {
              ItPlainTextEdit *texted = static_cast<ItPlainTextEdit*>(editor);
              texted->haveAsked = AutoYes;
            }
            emit commitData(editor);
            return true;
        } else if (keycode == alview->getEditorKeys().discardExit) {//: //Qt::Key_Escape:
            if (QString(editor->metaObject()->className()) == "ItPlainTextEdit") {
              ItPlainTextEdit *texted = static_cast<ItPlainTextEdit*>(editor);
              texted->haveAsked = AutoNo;
            }
            emit closeEditor(editor, QAbstractItemDelegate::NoHint);
            return true;
        } else if (keycode == alview->getEditorKeys().saveNext) { //: //Qt::Key_Tab:
            if (QString(editor->metaObject()->className()) == "ItPlainTextEdit") {
              ItPlainTextEdit *texted = static_cast<ItPlainTextEdit*>(editor);
              texted->haveAsked = AutoYes;
            }
            emit commitData(editor, QAbstractItemDelegate::EditNextItem);
            return true;
        } else if (keycode == alview->getEditorKeys().savePrev) {//: //Qt::Key_Backtab:
            if (QString(editor->metaObject()->className()) == "ItPlainTextEdit") {
              ItPlainTextEdit *texted = static_cast<ItPlainTextEdit*>(editor);
              texted->haveAsked = AutoYes;
            }
            emit commitData(editor, QAbstractItemDelegate::EditPreviousItem);
            return true;
        } else if (keycode == alview->getEditorKeys().saveInsertNext) { //: //Ctrl+Tab:
            if (QString(editor->metaObject()->className()) == "ItPlainTextEdit") {
              ItPlainTextEdit *texted = static_cast<ItPlainTextEdit*>(editor);
              texted->haveAsked = AutoYes;
              texted->insertNext = true;
            }
            emit commitData(editor, QAbstractItemDelegate::EditNextItem);
            return true;
        } else {
            return false;
        }
    }/* else if (ev->type() == QEvent::FocusOut) {
        if (QString(editor->metaObject()->className())=="ItPlainTextEdit") {
            emit closeEditor(editor, QAbstractItemDelegate::NoHint);
            return true;
        }
        return false;
    }*/ else
        return false;
    //return QStyledItemDelegate::eventFilter(obj, ev);
}

QSize ItAlignmentDelegate::sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    ItAlignmentView * alview = static_cast<ItAlignmentView*>(myparent);
    if (alview->txteditor!=0 && alview->txteditor->index == index) {//qDebug()<<"TXTED:"<<alview->txteditor->sizeHint();
        return alview->txteditor->sizeHint();}
    else if (alview->segview!=0 && alview->segview->index == index) {//qDebug()<<"SEGVW:"<<alview->segview->sizeHint();
        return alview->segview->sizeHint();}
    else
        return ItAbstractDelegate::sizeHint(option, index);
}
