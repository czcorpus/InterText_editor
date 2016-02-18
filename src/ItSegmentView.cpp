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

#include "ItSegmentView.h"
#include "ItAlignmentView.h"

ItSegmentView::ItSegmentView(QWidget * parent, ItAlignmentView *parAlView) : QListView(parent) {
  delegate = new ItSegmentDelegate(this, parAlView);
  alView = parAlView;
  myeditor = 0;
  autoEditHint = QAbstractItemDelegate::NoHint;
  setItemDelegate(delegate);
  disconnect(delegate, SIGNAL(commitData(QWidget*)), this, SLOT(commitData(QWidget*)));
  connect(delegate, SIGNAL(commitData(QWidget*,QAbstractItemDelegate::EndEditHint)), this, SLOT(commitData(QWidget*,QAbstractItemDelegate::EndEditHint)));
  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
  setResizeMode(QListView::Adjust);
}

ItSegmentView::~ItSegmentView() {
    //qDebug() << rectForIndex(model()->index(0,0,rootIndex()));
	delete this->itemDelegate();
}

void ItSegmentView::keyPressEvent(QKeyEvent * event)
{
  //qDebug() << "Segment key" << event->key();
	
	/*if ((event->key() == Qt::Key_E) || (event->key() == Qt::Key_Return) || (event->key() == Qt::Key_Enter))
    edit(currentIndex(),QAbstractItemView::EditKeyPressed,event);
	else if (event->key() == Qt::Key_Backspace) {
		QModelIndex previndex = model()->index(currentIndex().row()-1, currentIndex().column(), currentIndex().parent());
		if (!previndex.isValid()) return;
		ItAlignmentModel * almodel = static_cast<ItAlignmentModel*>(model());
		if (!almodel->canMerge(previndex)) return;
		if (QMessageBox::question(this, tr("Merging elements"), tr("Are you sure you want to merge the element with the previous one?"), QMessageBox::Ok|QMessageBox::Abort)==QMessageBox::Ok) {
			ItAlignmentModel * almodel = static_cast<ItAlignmentModel*>(model());
			setCurrentIndex(previndex);
			MergeCommand * merge = new MergeCommand(almodel, previndex);
			almodel->undoStack->push(merge);
		}
	} else if (event->key() == Qt::Key_P) {
		ItAlignmentModel * almodel = static_cast<ItAlignmentModel*>(model());
		if (!almodel->canSplitParent(currentIndex()))
			return;
		if (QMessageBox::question(this, tr("Splitting parents"), tr("Are you sure you want to create a new parent (paragraph) at the current element? (New paragraph break will be added.)"), QMessageBox::Ok|QMessageBox::Abort)==QMessageBox::Ok) {
			SplitParentCommand * split = new SplitParentCommand(almodel, currentIndex());
			almodel->undoStack->push(split);
		}
	} else if (event->key() == Qt::Key_D) {
		ItAlignmentModel * almodel = static_cast<ItAlignmentModel*>(model());
		if (!almodel->canMergeParent(currentIndex()))
			return;
		if (QMessageBox::question(this, tr("Merging parents"), tr("Are you sure you want to merge the parent (paragraph) of the current element to the previous one? (Last paragraph break will be deleted.)"), QMessageBox::Ok|QMessageBox::Abort)==QMessageBox::Ok) {
			MergeParentCommand * merge = new MergeParentCommand(almodel, currentIndex());
			almodel->undoStack->push(merge);
		}
	} else*/
		QListView::keyPressEvent(event);
}

/*int ItSegmentView::heightHint() {
	int height=0;
	for (int i=0; i<index.model()->rowCount(index); i++) {
		height = height + rectForIndex(model()->index(i,0,rootIndex())).height();
		//qDebug() << i << rectForIndex(model()->index(i,0,rootIndex())).height();
	}
    //qDebug() << "sum " << height;
	return  height;
}*/


void ItSegmentView::currentChanged ( const QModelIndex & current, const QModelIndex & previous )
{
	QAbstractItemView::currentChanged(current, previous);
    emit cursorChanged();
}

void ItSegmentView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight) {
	QListView::dataChanged(topLeft, bottomRight);
}

void ItSegmentView::focusOutEvent( QFocusEvent * event )
{
  QWidget::focusOutEvent(event);
  emit focusChanged();
}

void ItSegmentView::focusInEvent( QFocusEvent * event )
{
  QWidget::focusInEvent(event);
  emit focusChanged();
  if (autoEditHint == QAbstractItemDelegate::EditNextItem) {
      autoEditHint = QAbstractItemDelegate::NoHint;
      QModelIndex first = model()->index(0,0,rootIndex());
      if (first.isValid()) {
          setCurrentIndex(first);
          edit(first);
      }
  } else if (autoEditHint == QAbstractItemDelegate::EditPreviousItem) {
      autoEditHint = QAbstractItemDelegate::NoHint;
      int lastR = model()->rowCount(rootIndex()) - 1;
      QModelIndex last = model()->index(lastR,0,rootIndex());
      if (last.isValid()) {
          setCurrentIndex(last);
          edit(last);
      }
  }
}

void ItSegmentView::setHtmlView(bool set)
{
  delegate->setHtmlView(set);
}

void ItSegmentView::autoOpenEditor(QAbstractItemDelegate::EndEditHint hint)
{
    autoEditHint = hint;
}

void ItSegmentView::closeEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint)
{
  // override Qt's strange decisions -> the user decides!
  mayCloseEditor(editor, hint);
  /*for (int i=0; i<model()->rowCount(rootIndex()); i++) {
      resizeRowToContents(i);
  }*/
}

void ItSegmentView::commitData ( QWidget * editor )
{
  // override Qt's strange decisions -> the user decides!
  mayCloseEditor(editor);
}

void ItSegmentView::commitData ( QWidget * editor, QAbstractItemDelegate::EndEditHint hint  )
{
  // override Qt's strange decisions -> the user decides!
  mayCloseEditor(editor, hint);
}

// override Qt's strange decisions -> the user decides!
// but do not ask twice! (commitData is always followed by closeEditor)
void ItSegmentView::mayCloseEditor ( QWidget * editor, QAbstractItemDelegate::EndEditHint hint )
{
    // ignore Qt's decisions!
    //if (hint==QAbstractItemDelegate::SubmitModelCache)
    //    hint = QAbstractItemDelegate::NoHint;
    ItPlainTextEdit * texted = static_cast<ItPlainTextEdit*>(editor);
    //qDebug()<<(alView->getAutoSaveElement() == AutoAsk)<<(texted->haveAsked==AutoAsk)<<texted->hasBeenChanged();
    if (alView->getAutoSaveElement() == AutoAsk && texted->haveAsked==AutoAsk && texted->hasBeenChanged()) {//qDebug()<<"!!!";
        ItQuestionDialog * qd = new ItQuestionDialog(this);
        qd->exec();
        if (qd->result() == QDialog::Accepted) {
            if (qd->getRememberChoice())
                alView->setAutoSaveElement(AutoYes);
            texted->haveAsked = AutoYes;
            QListView::commitData(editor);
        } else if (qd->getRememberChoice()) {
            alView->setAutoSaveElement(AutoNo);
            texted->haveAsked = AutoNo;
        } else {
            texted->haveAsked = AutoNo;
        }
    } else if (alView->getAutoSaveElement() == AutoYes || texted->haveAsked==AutoYes) {//qDebug()<<"saving";
        QListView::commitData(editor);
    }
    int row = texted->index.row();
    myeditor = 0;
    if ((hint==QAbstractItemDelegate::EditNextItem && row==model()->rowCount(rootIndex())-1) ||
            (hint==QAbstractItemDelegate::EditPreviousItem && row==0)) {
        QListView::closeEditor(editor, QAbstractItemDelegate::NoHint);
        if (rootIndex().row()==model()->rowCount()-1)
            hint = QAbstractItemDelegate::NoHint;
        emit wantBeClosed(this, hint);
    } else {
        //QListView::closeEditor(editor, hint);
        QListView::closeEditor(editor, QAbstractItemDelegate::NoHint);
        adjustGeometry();
        handleCloseHint(hint);
    }
    adjustGeometry();
}

void ItSegmentView::handleCloseHint(QAbstractItemDelegate::EndEditHint hint)
{

    // this is just a copy of a closing part from QAbstractItemView::closeEditor in order to call this object's methods...

    // The EndEditHint part
    //QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect
    //                                            | d->selectionBehaviorFlags();
    switch (hint) {
    case QAbstractItemDelegate::EditNextItem: {
        QModelIndex index = moveCursor(MoveNext, Qt::NoModifier);
        if (index.isValid()) {
            QPersistentModelIndex persistent(index);
            setCurrentIndex(persistent);
            //d->selectionModel->setCurrentIndex(persistent, flags);
            // currentChanged signal would have already started editing
            if (index.flags() & Qt::ItemIsEditable
                && (!(editTriggers() & QAbstractItemView::CurrentChanged)))
                edit(persistent);
        } break; }
    case QAbstractItemDelegate::EditPreviousItem: {
        QModelIndex index = moveCursor(MovePrevious, Qt::NoModifier);
        if (index.isValid()) {
            QPersistentModelIndex persistent(index);
            setCurrentIndex(persistent);
            //d->selectionModel->setCurrentIndex(persistent, flags);
            // currentChanged signal would have already started editing
            if (index.flags() & Qt::ItemIsEditable
                && (!(editTriggers() & QAbstractItemView::CurrentChanged)))
                edit(persistent);
        } break; }
    case QAbstractItemDelegate::SubmitModelCache:
        model()->submit();
        //d->model->submit();
        break;
    case QAbstractItemDelegate::RevertModelCache:
        model()->revert();
        //d->model->revert();
        break;
    default:
        break;
    }
}

void ItSegmentView::setEditor(ItPlainTextEdit * editor)
{
    myeditor = editor;
}

void ItSegmentView::closeAnyEditor()
{
  if (myeditor)
    closeEditor(myeditor, QAbstractItemDelegate::NoHint);
}

QSize ItSegmentView::sizeHint() const
{
    return QSize(width(), height());
}

void ItSegmentView::setGeometry(const QRect &rect)
{
    //qDebug()<<"Setting initial geometry"<<rect.width()<<rect.height()<<rect.x()<<rect.y();
    //qDebug()<<"Height for width:"<<heightForWidth(rect.width());
    QListView::setGeometry(rect);
    adjustGeometry();
    //resize(rect.width(), rect.height());
}

void ItSegmentView::adjustGeometry()
{
    int i = 0;
    int height=0;
    for (i=0; i<index.model()->rowCount(index); i++) {
        //height += delegate->sizeHint(QStyleOptionViewItem(), model()->index(i,0,rootIndex())).height();
        height = height + rectForIndex(model()->index(i,0,rootIndex())).height();
        //qDebug() << i << rectForIndex(model()->index(i,0,rootIndex())).size()<<size();
    }
    height += 6;
    //qDebug() << height << i;
    if (height > 2*rect().height()) height = rect().height()+2; // avoid initial crazy scaling
    //QRect newrect(rect());
    QSize nsize(size());
    nsize.setHeight(height);
    resize(nsize);
        //newrect.setHeight(height);
        //qDebug()<<"Updating geometry";
        //QListView::setGeometry(newrect);
    //}
    //qDebug()<<"Emitting size hint change";
    emit sizeHintChanged(index.row());
    //qDebug()<<"Emitted.Done.";

}

void ItSegmentView::resizeRowToContents(int row)
{//qDebug()<<"resize row"<<row;
    // just tired of this Qt size management... tell me if you know how it actually works...
    if (myeditor && myeditor->index.row()==row) {
        //QRect r = rectForIndex(model()->index(row, 0, rootIndex()));qDebug()<<r;
        //myeditor->setGeometry(QRect(r.x(), r.y(), myeditor->width(), myeditor->sizeHint().height()));
        myeditor->resize(QSize(myeditor->width(), myeditor->sizeHint().height()));
    }
    adjustGeometry();
    //setGeometry(rect());
    //emit sizeHintChanged(index.row());
}

/*bool ItSegmentView::edit(const QModelIndex &index, EditTrigger trigger, QEvent *event) {
    if (QListView::edit(index, trigger, event)) {
      editorOpen = true;
      //emit editingStarted();
      return true;
    } else {
      return false;
    }
}

void ItSegmentView::closeEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint) {
  QListView::closeEditor(editor, QAbstractItemDelegate::NoHint);
  editorOpen = false;
  //emit editingFinished();
}

bool ItSegmentView::isEditing() {
    return editorOpen;
}*/
