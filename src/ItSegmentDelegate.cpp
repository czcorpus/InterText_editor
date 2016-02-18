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

#include "ItSegmentDelegate.h"
#include "ItSegmentView.h"
#include "ItAlignmentView.h"

ItSegmentDelegate::ItSegmentDelegate(QObject *parent, ItAlignmentView *aview) : ItAbstractDelegate(parent) {
    alview = aview;
    //connect(this, SIGNAL(closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)), this, SLOT(onClose(QWidget*,QAbstractItemDelegate::EndEditHint)));
}

QWidget *ItSegmentDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  ItSegmentView * segview = static_cast<ItSegmentView*>(parent->parentWidget());
  ItPlainTextEdit * editor = new ItPlainTextEdit(parent, alview->getEditorKeys());
  editor->index = index;
  //editor->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  //editor->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
  segview->setEditor(editor);
  editor->setFocusPolicy(Qt::StrongFocus);
  connect(editor, SIGNAL(closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)), this,
          SLOT(editorCloseRequested(QWidget*,QAbstractItemDelegate::EndEditHint)));
  connect(editor, SIGNAL(sizeHintChanged(int)), segview, SLOT(resizeRowToContents(int)));
  return editor;
}

void ItSegmentDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QString text = index.model()->data(index, Qt::EditRole).toString();
    ItPlainTextEdit *texted = static_cast<ItPlainTextEdit*>(editor);
	texted->setPlainText(text);
    texted->resetChangeFlag();
}

void ItSegmentDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	QPlainTextEdit *texted = static_cast<ItPlainTextEdit*>(editor);
  QString text = texted->toPlainText();
	model->setData(index, text, Qt::EditRole);
}

void ItSegmentDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  ItPlainTextEdit * teditor = static_cast<ItPlainTextEdit*>(editor);
  //QRect nrect(option.rect);
  //nrect.setHeight(nrect.height()+18);
  //teditor->setGeometry(option.rect);
  QRect nrect(option.rect);
  //nrect.setSize(teditor->sizeHint());
  nrect.setHeight(nrect.height()+teditor->fontInfo().pixelSize()+3);
  teditor->setGeometry(nrect);
  //QWidget *myparent = static_cast<QWidget*>(parent());
  //ItSegmentView * segview = static_cast<ItSegmentView*>(myparent->parentWidget());
  //ItAlignmentModel * mymodel = static_cast<ItAlignmentModel*>(segview->model());
  //mymodel->externalDataChange();
  //QStyledItemDelegate::updateEditorGeometry(editor, option, index);
  //ItSegmentView * segview = static_cast<ItSegmentView*>(parent());
  //editor->setGeometry(option.rect.adjusted(0,0,0,16));
  //segview->adjustSize();
}

bool ItSegmentDelegate::eventFilter(QObject *obj, QEvent *ev) {
	QWidget *editor = qobject_cast<QWidget*>(obj);
	if (!editor)
			return false;
	if (ev->type() == QEvent::KeyPress) {
        ItPlainTextEdit *texted = static_cast<ItPlainTextEdit*>(editor);
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

        /*switch (static_cast<QKeyEvent *>(ev)->key()) {
        case Qt::Key_F2:*/

        if (keycode == alview->getEditorKeys().saveExit) {
            texted->haveAsked = AutoYes;
            emit commitData(editor);
            return true;
        } else if (keycode == alview->getEditorKeys().discardExit) {
            texted->haveAsked = AutoNo;
            emit closeEditor(editor, QAbstractItemDelegate::NoHint);
            return true;
        } else if (keycode == alview->getEditorKeys().saveNext) {
            texted->haveAsked = AutoYes;
            emit commitData(editor, QAbstractItemDelegate::EditNextItem);
            return true;
        } else if (keycode == alview->getEditorKeys().savePrev) {
            texted->haveAsked = AutoYes;
            emit commitData(editor, QAbstractItemDelegate::EditPreviousItem);
            return true;
        } else {
            return false;
		}
    } /*else if (ev->type() == QEvent::FocusOut) {
        emit closeEditor(editor, QAbstractItemDelegate::NoHint);
        return true;
    }*/ else
      return false;
		//	return QStyledItemDelegate::eventFilter(obj, ev);
}

QSize ItSegmentDelegate::sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const
{//qDebug()<<"Asking sizehint from SegmentDelegate";
    //QWidget *myparent = static_cast<QWidget*>(parent());
    ItSegmentView * segview = static_cast<ItSegmentView*>(myparent);
    if (segview->myeditor!=0 && segview->myeditor->index == index) {
        return segview->myeditor->sizeHint();}
    else
        return ItAbstractDelegate::sizeHint(option, index);
}
