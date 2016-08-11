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

#include "ItAlignmentView.h"
#include <cmath>
#include "ItWindow.h"

ItAlignmentView::ItAlignmentView(QWidget * parent) : QTableView(parent) {
    window = static_cast<ItWindow*>(parent);
  delegate = new ItAlignmentDelegate(this);
  setItemDelegate(delegate);
  disconnect(delegate, SIGNAL(commitData(QWidget*)), this, SLOT(commitData(QWidget*)));
  connect(delegate, SIGNAL(commitData(QWidget*,QAbstractItemDelegate::EndEditHint)), this, SLOT(commitData(QWidget*,QAbstractItemDelegate::EndEditHint)));
  connect(delegate, SIGNAL(insertNextRequested()), this, SLOT(insertNextRequested()), Qt::QueuedConnection);
  setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  setMouseTracking(true);
  showControls = OnMove;
  controlsHideTimeOut = 300;
  editorOpen = false;
  segview = 0;
  txteditor = 0;
  itmodel = 0;
  //nextToResize = 0;
  hNon11 = true;
  hMarked = true;
  setShowGrid(false);
  resizerow = 0;
  lfirstrow = 0;
  llastrow = 0;
  resizeNextTime = false;
  keepMarginNextTime = true;
  skipMargin = 1;
  timer.start(250);
  nexthint = QAbstractItemDelegate::NoHint;

  floatControl = new ItFloatControls(this);
  floatControl->hide();
  setSizeControls(TINY);
  connect(floatControl, SIGNAL(sglUp(int,int)), this, SLOT(shift(int,int)));
  connect(floatControl, SIGNAL(sglDown(int,int)), this, SLOT(pop(int,int)));
  connect(floatControl, SIGNAL(txtDown(int,int)), this, SLOT(moveDown(int,int)));
  connect(floatControl, SIGNAL(txtUp(int,int)), this, SLOT(moveUp(int,int)));
  connect(floatControl, SIGNAL(bothUp(int)), this, SLOT(moveBothUp(int)));
  connect(floatControl, SIGNAL(bothDown(int)), this, SLOT(moveBothDown(int)));

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));

}

ItAlignmentView::~ItAlignmentView()
{
    delete floatControl;
}

void ItAlignmentView::createContextMenu()
{
    ctxMenu.clear();
    QAction *a;
    foreach (a, window->contextMenuCurActions) {
        if (a)
            ctxMenu.addAction(a);
        else
            ctxMenu.addSeparator();
    }
}

void ItAlignmentView::setShowControls(ShowControlsType mode)
{
    floatControl->hide();
    showControls = mode;
}

void ItAlignmentView::setSizeControls(int size)
{
    sizeControls = size;
    floatControl->setIconSize(size);
}

ShowControlsType ItAlignmentView::getShowControls()
{
    return showControls;
}

int ItAlignmentView::getSizeControls()
{
    return sizeControls;
}

void ItAlignmentView::setModel(QAbstractItemModel * model) {
  if (model==0) {
    hide();
    itmodel = 0;
    segview = 0;
    txteditor = 0;
    disconnect(&timer, SIGNAL(timeout()), this, SLOT(resizeRows()));
    return;
  }
  updateRowSize();
  itmodel = static_cast<ItAlignmentModel*>(model);
  QTableView::setModel(model);
  itmodel->setHighlNon11(hNon11);
  itmodel->setHighlMarked(hMarked);
  setWordWrap(true);
	editorOpen = false;
  setSelectionMode(QAbstractItemView::SingleSelection);
  horizontalHeader()->setSectionResizeMode(0,QHeaderView::Fixed);
  horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
  horizontalHeader()->setSectionResizeMode(2,QHeaderView::Stretch);
  horizontalHeader()->setSectionResizeMode(3,QHeaderView::Fixed);
  horizontalHeader()->resizeSection(0,24);
  horizontalHeader()->resizeSection(3,24);
  //nextToResize = 0;
  //verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  verticalHeader()->setSectionResizeMode(QHeaderView::Interactive);
  setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
  connect(itmodel, SIGNAL(focusOnChange(QModelIndex)), this, SLOT(setCurrentIndex(QModelIndex)));
  connect(&timer, SIGNAL(timeout()), this, SLOT(resizeRows()));
  connect(itmodel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(dataChanged(QModelIndex,QModelIndex)));
  //setShowGrid(false);
  //cacheAllSizeHints();
  show();
  setFocus();
  //resizeRowsToContents();
}

void ItAlignmentView::setAutoSaveElement(AutoState value) {
    autoSaveElement = value;
}

AutoState ItAlignmentView::getAutoSaveElement() {
    return autoSaveElement;
}

void ItAlignmentView::keyPressEvent(QKeyEvent * event)
{
  //ItAlignmentModel * m;
  //QModelIndex i;
  //m = static_cast<ItAlignmentModel*>(model());
	//qDebug() << "Alignment key" << event->key();
	
  /*if (currentIndex().column()==1 || currentIndex().column()==2) {
		if (event->key() == Qt::Key_Return) {
			m->undoStack->push(new MoveDownCommand(m, currentIndex()));
			setCurrentIndex(m->index(currentIndex().row()+1, currentIndex().column()));
		} else if (event->key() == Qt::Key_Backspace) {
			m->undoStack->push(new MoveUpCommand(m, currentIndex()));
			setCurrentIndex(m->index(currentIndex().row()-1, currentIndex().column()));
		} else if (event->key() == Qt::Key_Up && (event->modifiers() & Qt::ControlModifier) ) {
      m->undoStack->push(new ShiftCommand(m, currentIndex()));*/
			//setCurrentIndex(m->index(currentIndex().row()-1, currentIndex().column()));
			/*int orc = m->rowCount();
			if (m->shift(currentIndex())) {
        if (currentIndex().row() == orc-1) {
					i = m->index(currentIndex().row()-1, currentIndex().column(), currentIndex().parent());
					setCurrentIndex(i);
        } else {
					setCurrentIndex(currentIndex());
				}
			}*/
    /*} else if (event->key() == Qt::Key_Down && (event->modifiers() & Qt::ControlModifier) ) {
			m->undoStack->push(new PopCommand(m, currentIndex()));
    } else if (event->key() == Qt::Key_E) {
        edit(currentIndex(),QAbstractItemView::EditKeyPressed,event);
		} else {
			QTableView::keyPressEvent(event);
		}
	} else if (currentIndex().column()==0) {
		if (event->key() == Qt::Key_Return) {
			m->undoStack->push(new ToggleMarkCommand(m, currentIndex()));
		} else {
			QTableView::keyPressEvent(event);
		}
	} else if (currentIndex().column()==3) {
		if (event->key() == Qt::Key_Return) {
			m->undoStack->push(new ToggleStatCommand(m, currentIndex()));
		} else {
			QTableView::keyPressEvent(event);
        }*/
    int curCol=1;
    if (currentIndex().isValid())
        curCol = currentIndex().column();
    if (event->key() == Qt::Key_PageDown) {
        int nextrow = rowAt(height())-1;
        if (nextrow>model()->rowCount()-1 || nextrow < 0)
            nextrow = model()->rowCount()-1;
        QModelIndex nextidx = model()->index(nextrow, curCol, QModelIndex());
        QModelIndex viewidx = model()->index(nextrow-skipMargin, curCol, QModelIndex());
        setCurrentIndex(nextidx);
        scrollTo(viewidx, QAbstractItemView::PositionAtTop);
    } else if (event->key() == Qt::Key_PageUp) {
        int nextrow = rowAt(0)-1;
        if (nextrow<0)
            nextrow = 0;
        QModelIndex nextidx = model()->index(nextrow, curCol, QModelIndex());
        QModelIndex viewidx = model()->index(nextrow+skipMargin, curCol, QModelIndex());
        setCurrentIndex(nextidx);
        scrollTo(viewidx, QAbstractItemView::PositionAtBottom);
    } else {
        QTableView::keyPressEvent(event);
    }
}

void ItAlignmentView::moveUp(int row, int doc) {
  if (model()==0) return;
  ItAlignmentModel * m;
  m = static_cast<ItAlignmentModel*>(model());
  QModelIndex myidx;
  if (row == INVALID_ROW) {
      myidx = currentIndex();
  } else {
      myidx = m->index(row, doc+1, QModelIndex());
  }
  m->undoStack->push(new MoveUpCommand(m, myidx));
  //setCurrentIndex(m->index(currentIndex().row()-1, currentIndex().column()));
}

void ItAlignmentView::moveDown(int row, int doc) {
  if (model()==0) return;
  ItAlignmentModel * m;
  m = static_cast<ItAlignmentModel*>(model());
  QModelIndex myidx;
  if (row == INVALID_ROW) {
      myidx = currentIndex();
  } else {
      myidx = m->index(row, doc+1, QModelIndex());
  }
  m->undoStack->push(new MoveDownCommand(m, myidx));
  //setCurrentIndex(m->index(currentIndex().row()+1, currentIndex().column()));
}

void ItAlignmentView::moveBothUp(int row) {
  if (model()==0) return;
  ItAlignmentModel * m;
  m = static_cast<ItAlignmentModel*>(model());
  int crow, ccol;
  if (row == INVALID_ROW) {
      crow = currentIndex().row();
      ccol = currentIndex().column();
  } else {
      crow = row;
      ccol = 1;
  }
  int ocol = 2;
  if (ccol==2)
    ocol= 1;
  else
    ccol = 1;
  m->undoStack->beginMacro("Move both up");
    m->undoStack->push(new MoveUpCommand(m, m->index(crow,ocol), true));
    m->undoStack->push(new MoveUpCommand(m, m->index(crow,ccol)));
  m->undoStack->endMacro();
  //setCurrentIndex(m->index(currentIndex().row()-1, currentIndex().column()));
}

void ItAlignmentView::moveBothDown(int row) {
  if (model()==0) return;
  ItAlignmentModel * m;
  m = static_cast<ItAlignmentModel*>(model());
  int crow, ccol;
  if (row == INVALID_ROW) {
      crow = currentIndex().row();
      ccol = currentIndex().column();
  } else {
      crow = row;
      ccol = 1;
  }
  int ocol = 2;
  if (ccol==2)
    ocol = 1;
  else {
    ccol = 1;
  }
  m->undoStack->beginMacro("Move both up");
    m->undoStack->push(new MoveDownCommand(m, m->index(crow,ocol), true));
    m->undoStack->push(new MoveDownCommand(m, m->index(crow,ccol)));
  m->undoStack->endMacro();
  //setCurrentIndex(m->index(currentIndex().row()+1, currentIndex().column()));
}

void ItAlignmentView::shift(int row, int doc) {
  if (model()==0) return;
  if (segview!=0) closeEditor(segview, QAbstractItemDelegate::NoHint);
  if (txteditor!=0) closeEditor(txteditor, QAbstractItemDelegate::NoHint);
  ItAlignmentModel * m;
  m = static_cast<ItAlignmentModel*>(model());
  QModelIndex myidx;
  if (row == INVALID_ROW) {
      myidx = currentIndex();
  } else {
      myidx = m->index(row, doc+1, QModelIndex());
  }
  m->undoStack->push(new ShiftCommand(m, myidx));
	//if (currentIndex().row()==model()->rowCount()-1 && model()->rowCount(currentIndex())==0) {
  /*if (!model()->hasIndex(currentIndex().row(), currentIndex().column(), currentIndex().parent()))
    setCurrentIndex(model()->index(currentIndex().row()-1, currentIndex().column(), currentIndex().parent()));*/
}

void ItAlignmentView::pop(int row, int doc) {
  if (model()==0) return;
	if (segview!=0) closeEditor(segview, QAbstractItemDelegate::NoHint);
    if (txteditor!=0) closeEditor(txteditor, QAbstractItemDelegate::NoHint);
  ItAlignmentModel * m;
  m = static_cast<ItAlignmentModel*>(model());
  QModelIndex myidx;
  if (row == INVALID_ROW) {
      myidx = currentIndex();
  } else {
      myidx = m->index(row, doc+1, QModelIndex());
  }
  m->undoStack->push(new PopCommand(m, myidx));
}

void ItAlignmentView::swapSegments(int row, int doc)
{
    if (model()==0) return;
      if (segview!=0) closeEditor(segview, QAbstractItemDelegate::NoHint);
      if (txteditor!=0) closeEditor(txteditor, QAbstractItemDelegate::NoHint);
    ItAlignmentModel * m;
    m = static_cast<ItAlignmentModel*>(model());
    QModelIndex myidx;
    if (row == INVALID_ROW) {
        myidx = currentIndex();
    } else {
        myidx = m->index(row, doc+1, QModelIndex());
    }
    m->undoStack->push(new SwapCommand(m, myidx));
}

void ItAlignmentView::insertElement() {
    if (model()==0) return;
    if (segview!=0) closeEditor(segview, QAbstractItemDelegate::NoHint);
    if (txteditor!=0) closeEditor(txteditor, QAbstractItemDelegate::NoHint);
    ItAlignmentModel * m;
    m = static_cast<ItAlignmentModel*>(model());
    QModelIndex idx = this->currentIndex();
    if (!m->insert(idx)) {
        QMessageBox::critical(this, tr("Insert element"), tr("No preceding element to split. Please, create some manually."));
        return;
    }
    setCurrentIndex(idx);
    this->resizeRowToContents(idx.row());
    //this->setCurrentIndex(m->index(0, m->rowCount(idx)-1, idx));
    openEditor();
    if (segview) {
        segview->setCurrentIndex(m->index(m->rowCount(idx)-1, 0, idx));
        segview->edit(segview->currentIndex());
    }
}

void ItAlignmentView::toggleMark() {
  if (model()==0) return;
  ItAlignmentModel * m;
  m = static_cast<ItAlignmentModel*>(model());
  m->undoStack->push(new ToggleMarkCommand(m, currentIndex()));
}

void ItAlignmentView::toggleStat() {
  if (model()==0) return;
  ItAlignmentModel * m;
  m = static_cast<ItAlignmentModel*>(model());
  m->undoStack->push(new ToggleStatCommand(m, currentIndex()));
}

void ItAlignmentView::confirmAll() {
  if (model()==0) return;
  ItAlignmentModel * m;
  m = static_cast<ItAlignmentModel*>(model());
  m->undoStack->push(new ConfirmCommand(m, currentIndex()));
}

bool ItAlignmentView::openEditor() {
    if (segview!=0) {
		segview->edit(segview->currentIndex());
		return true;
    } else {
        if (currentIndex().isValid() && (currentIndex().column()==1 || currentIndex().column()==2)) {
            edit(currentIndex());
			return true;
		} else {
			return false;
		}
	}
}

void ItAlignmentView::splitParent() {
	if (model()==0) return;
	QModelIndex cur;
	if (segview!=0)
		cur = segview->currentIndex();
	else
		cur = currentIndex();
    if (QMessageBox::question(this, tr("Splitting parents"), tr("Are you sure you want to split the current paragraph? (i.e. create a new paragraph starting with this element)"), QMessageBox::Ok|QMessageBox::Abort)==QMessageBox::Ok) {
		ItAlignmentModel * m;
		m = static_cast<ItAlignmentModel*>(model());
		m->undoStack->push(new SplitParentCommand(m, cur));
	}
}

void ItAlignmentView::mergeParent() {
	if (model()==0) return;
	QModelIndex cur;
	if (segview!=0)
		cur = segview->currentIndex();
	else
		cur = currentIndex();
    if (QMessageBox::question(this, tr("Merging parents"), tr("Are you sure you want to merge the paragraph with the previous one? (i.e. delete the current paragraph.)"), QMessageBox::Ok|QMessageBox::Abort)==QMessageBox::Ok) {
		ItAlignmentModel * m;
		m = static_cast<ItAlignmentModel*>(model());
		m->undoStack->push(new MergeParentCommand(m, cur));
	}
}

void ItAlignmentView::edit(const QModelIndex &index) {
  QTableView::edit(index);
}

bool ItAlignmentView::edit(const QModelIndex &index, EditTrigger trigger, QEvent *event) {
  if (QTableView::edit(index, trigger, event)) {
    editorOpen = true;
    floatControl->hide();
    emit editingStarted();
    return true;
  } else {
    return false;
  }
}
void ItAlignmentView::mouseMoveEvent ( QMouseEvent * event ) {
    if (showControls == OnMove) {
        int row = rowAt(event->y());
        int col = columnAt(event->x());
        if (!editorOpen && (col==1 || col==2)) {
            int ypos = rowViewportPosition(row)+(rowHeight(row)/2);
            int xpos = verticalHeader()->width() + columnViewportPosition(2)-(floatControl->width()/2);
            floatControl->move(xpos, ypos);
            floatControl->setRow(row);
            if (model()->data(model()->index(row, 1, QModelIndex()), Qt::DisplayRole).toString().isEmpty())
                floatControl->setEnabledLeft(false);
            else
                floatControl->setEnabledLeft(true);
            if (model()->data(model()->index(row, 2, QModelIndex()), Qt::DisplayRole).toString().isEmpty())
                floatControl->setEnabledRight(false);
            else
                floatControl->setEnabledRight(true);
            floatControl->show(controlsHideTimeOut);
        } else {
            floatControl->hide();
        }
    }
}

void ItAlignmentView::mousePressEvent(QMouseEvent * event)
{
    QTableView::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        ItAlignmentModel * m = static_cast<ItAlignmentModel*>(model());
        if (currentIndex().column()==0) {
            m->undoStack->push(new ToggleMarkCommand(m, currentIndex()));
            //m->toggleMark(currentIndex());
        } else if (currentIndex().column()==3) {
            m->undoStack->push(new ToggleStatCommand(m, currentIndex()));
            //m->toggleStat(currentIndex());
        }
        if (showControls == OnClick) {
            int row = currentIndex().row();
            int col = currentIndex().column();
            if (!editorOpen && (col==1 || col==2)) {
                if (floatControl->row() == row && floatControl->isVisible()) {
                    floatControl->hide();
                } else {
                    int ypos = rowViewportPosition(row)+(rowHeight(row)/2);
                    int xpos = verticalHeader()->width() + columnViewportPosition(2)-(floatControl->width()/2);
                    floatControl->move(xpos, ypos);
                    floatControl->setRow(row);
                    if (model()->data(model()->index(row, 1, QModelIndex()), Qt::DisplayRole).toString().isEmpty())
                        floatControl->setEnabledLeft(false);
                    else
                        floatControl->setEnabledLeft(true);
                    if (model()->data(model()->index(row, 2, QModelIndex()), Qt::DisplayRole).toString().isEmpty())
                        floatControl->setEnabledRight(false);
                    else
                        floatControl->setEnabledRight(true);
                    floatControl->show();
                }
            } else {
                floatControl->hide();
            }
        } else {
            floatControl->hide();
        }
    }
}

void ItAlignmentView::wheelEvent(QWheelEvent * event)
{
    QTableView::wheelEvent(event);
    floatControl->hide();
}

void ItAlignmentView::leaveEvent(QEvent * event)
{
    QTableView::leaveEvent(event);
    floatControl->hide();
}

void ItAlignmentView::showContextMenu(const QPoint& pos)
{
    QPoint globalPos = viewport()->mapToGlobal(pos);
    setCurrentIndex(model()->index(rowAt(pos.y()), columnAt(pos.x()), QModelIndex()));

    ctxMenu.exec(globalPos);
}

void ItAlignmentView::currentChanged ( const QModelIndex & current, const QModelIndex & previous )
{
    QAbstractItemView::currentChanged(current, previous);
    emit cursorChanged();
    //updateRowSize();
    //resizeRows();
    keepMargin();
    //qDebug() << this->rowAt( 0 ) << "-" << this->rowAt( this->height() );
    //qDebug() << this->columnAt( 0 ) << "-" << this->columnAt( this->width() );
	//qDebug() << "Height for" << current.column() << current.row() << "is" << calcItemHeight(current);
}

void ItAlignmentView::keepMargin()
{//qDebug()<<"keep";
  int clrow = rowAt(height());
  if (clrow<0)
    clrow = model()->rowCount();
  QModelIndex current = currentIndex();
  if (current.row() < rowAt(0)+skipMargin) {
    int vrow = current.row()-skipMargin;
    if (vrow<0)
        vrow = 0;
    QModelIndex viewidx = model()->index(vrow, current.column(), QModelIndex());
    scrollTo(viewidx, QAbstractItemView::PositionAtTop);
  } else if (current.row()+skipMargin > clrow-2) {
    int vrow = current.row()+skipMargin;
    if (vrow>model()->rowCount()-1 || vrow < 0)
        vrow = model()->rowCount()-1;
    QModelIndex viewidx = model()->index(vrow, current.column(), QModelIndex());
    scrollTo(viewidx, QAbstractItemView::PositionAtBottom);//qDebug()<<"Bottom"<<vrow;
  }
  keepMarginNextTime = true;
}

/*void ItAlignmentView::cacheAllSizeHints()
{
	updateSizeHints(0,-1);
}

void ItAlignmentView::updateSizeHints(int from, int to)
{
	ItAlignmentModel * m = static_cast<ItAlignmentModel*>(model());
	QModelIndex idx;
	if (to==-1) to = m->rowCount()-1;
	for (int i=from; i <= to; ++i) {
		idx = m->index(i,1);
		m->resetSize(idx);
		m->cacheSize(idx,QSize(visualRect(idx).width(),calcItemHeight(idx)));
		//m->cacheSize(idx,sizeHintForIndex(idx));
	}
	for (int i=from; i <= to; ++i) {
		idx = m->index(i,2);
		m->resetSize(idx);
		m->cacheSize(idx,QSize(visualRect(idx).width(),calcItemHeight(idx)));
	}
}

int ItAlignmentView::calcItemHeight(const QModelIndex &index)
{
	QStyleOptionViewItemV2 option = viewOptions();

	option.rect = visualRect(index);
	option.features = option.features |  QStyleOptionViewItemV2::WrapText;
	//qDebug() << "rect" << option.rect.width() << (option.features & QStyleOptionViewItemV2::WrapText);
	return itemDelegate(index)->sizeHint(option, index).height();
}*/

void ItAlignmentView::updateRowSize()
{
    resizerow = 0;
    lfirstrow = 0;
    llastrow = 0;
  /*ItAlignmentModel * m = static_cast<ItAlignmentModel*>(model());
	int cur = currentIndex().row();
	int first = cur - BACKWARD_RESIZE_ROWS;
	if (first<0) first = 0;
	int last = cur + FORWARD_RESIZE_ROWS;
	if (last >= m->rowCount()) last = m->rowCount()-1;
	for (int i = first; i <= last; ++i)
    resizeRowToContents(i);*/
}

void ItAlignmentView::resizeRows()
{
  int firstrow = rowAt(0);
  int lastrow = rowAt(height());
  /*bool toscroll = false;
  if (currentIndex().row()<=lastrow || currentIndex().row()>=firstrow)
      toscroll = true;*/
  if (lastrow<0)
      lastrow = model()->rowCount();
  /*if (resizerow>=llastrow && (lfirstrow!=firstrow || llastrow<lastrow)) {
    if (resizerow<firstrow || currow>lastrow)
      resizerow = firstrow;
    lfirstrow = firstrow;
    llastrow = lastrow;
    //timer.start(0); // resize everythin visible as quickly as possible
  }*/
  if (firstrow!=lfirstrow || lastrow!=llastrow) {
    lfirstrow = firstrow;
    llastrow = lastrow;
    resizerow = lastrow+1; // wait for next cycle to test for stable view (no resizing while scrolling!)
    resizeNextTime = true;
  } else if (resizeNextTime) {//qDebug()<<lfirstrow<<"-"<<llastrow<<" / "<<firstrow<<"-"<<lastrow;
    resizerow = firstrow;
    resizeNextTime = false;
  }
  /*else {
    timer.start(100); // slow down to save CPU load
  }*/
  while (resizerow<=llastrow) { // just resize everything visible immediately!
    resizeRowToContents(resizerow);//qDebug()<<"r"<<resizerow;
    resizerow++;
    if (resizerow>llastrow) {//qDebug()<<"resized";
      if (keepMarginNextTime) {
        keepMargin();
        keepMarginNextTime = false;
      }
      /*lfirstrow = rowAt(0); // this block makes things hang when editing something at the bottom of the view!
      llastrow = rowAt(height());
      if (llastrow<0)
          llastrow = model()->rowCount();
      resizerow = lastrow+1;*/
    }
  }
  //if (toscroll)
  //   scrollTo(currentIndex()); // blocks scrolling, actually :-(
  /*if (nextToResize<model()->rowCount()) {
    resizeRowToContents(nextToResize);
    nextToResize++;
    timer.start(0);
  } else {
    timer.start(100);
  }*/
}

/*void ItAlignmentView::resizeEvent ( QResizeEvent * event )
{
  QAbstractItemView::resizeEvent(event);
	if (model()) {
		ItAlignmentModel * m = static_cast<ItAlignmentModel*>(model());
		m->resetAllSizes();
  }
}*/

void ItAlignmentView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight) {
  QTableView::dataChanged(topLeft, bottomRight);
  //QModelIndex start, stop;
  //if (topLeft.parent().isValid()) start = topLeft.parent(); else start = topLeft;
  //if (bottomRight.parent().isValid()) stop = bottomRight.parent(); else stop = bottomRight;
  //int last = bottomRight.row();
  //if (last > topLeft.row()+FORWARD_RESIZE_ROWS) {
  //    last = topLeft.row()+FORWARD_RESIZE_ROWS;
  //    resizeNextTime = true;
  //}
  int first = topLeft.row();
  int last = bottomRight.row();
  if (first<rowAt(0))
    first = rowAt(0);
  if (last>rowAt(height()))
    last = rowAt(height());//qDebug() << first << last;
  for (int i = first; i <= last; ++i)
      resizeRowToContents(i);
  /*if (last<nextToResize-1)
      nextToResize = last+1;*/
}

void ItAlignmentView::closeEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint)
{//qDebug()<<"CLOSE"<<hint;
    // override Qt's strange decisions -> the user decides!
    mayCloseEditor(editor, hint);
}

void ItAlignmentView::commitData ( QWidget * editor )
{//qDebug()<<"COMMIT NOHINT";
    //it always runs it again with a hint anyway!
    return;
    // override Qt's strange decisions -> the user decides!
    //mayCloseEditor(editor);
}

void ItAlignmentView::commitData ( QWidget * editor, QAbstractItemDelegate::EndEditHint hint )
{//qDebug()<<"COMMIT"<<hint;
    // override Qt's strange decisions -> the user decides!
    mayCloseEditor(editor, hint);
}

// override Qt's strange decisions -> the user decides!
// but do not ask twice! (commitData is always followed by closeEditor)
void ItAlignmentView::mayCloseEditor ( QWidget * editor, QAbstractItemDelegate::EndEditHint hint )
{
    int row = 0;
    bool commited = false;
    ItPlainTextEdit * texted = 0;
    if (QString(editor->metaObject()->className())=="ItPlainTextEdit") {
        texted = static_cast<ItPlainTextEdit*>(editor);
        row = texted->index.row();
        if (autoSaveElement == AutoAsk && texted->haveAsked==AutoAsk && texted->hasBeenChanged()) {
            ItQuestionDialog * qd = new ItQuestionDialog(this);
            qd->exec();
            if (qd->result() == QDialog::Accepted) {
                if (qd->getRememberChoice())
                    autoSaveElement = AutoYes;
                texted->haveAsked = AutoYes;
                QTableView::commitData(editor);
                commited = true;
                QTableView::closeEditor(editor, QAbstractItemDelegate::NoHint);
                dataChanged(texted->index, texted->index);
            } else if (qd->getRememberChoice()) {
                autoSaveElement = AutoNo;
                texted->haveAsked = AutoNo;
                QTableView::closeEditor(editor, QAbstractItemDelegate::NoHint);
            } else {
                texted->haveAsked = AutoNo;
                QTableView::closeEditor(editor, QAbstractItemDelegate::NoHint);
            }
        } else if (autoSaveElement == AutoYes || texted->haveAsked==AutoYes) {
            QTableView::commitData(editor);
            commited = true;
            QTableView::closeEditor(editor, QAbstractItemDelegate::NoHint);
            dataChanged(texted->index, texted->index);
        } else {
            QTableView::closeEditor(editor, QAbstractItemDelegate::NoHint);
        }
        setEditor(0);
    } else {
        if (segview) {
            segview->closeAnyEditor();
            dataChanged(segview->index, segview->index);
        }
      QTableView::closeEditor(editor, QAbstractItemDelegate::NoHint);
      setSegView(0);
    }
    ItAlignmentModel * m = static_cast<ItAlignmentModel*>(model());
    if (commited)
        m->commitInsert();
    else
        m->cancelInsert();
    if (texted)
        handleCloseHint(hint, texted->insertNext);
    else
        handleCloseHint(hint);
    resizeRowToContents(row);
}

void ItAlignmentView::handleCloseHint(QAbstractItemDelegate::EndEditHint hint, bool insertNext)
{
    editorOpen = false;
    emit editingFinished();

    // this is just a copy of a closing part from QAbstractItemView::closeEditor in order to call this object's methods...

    // The EndEditHint part
    //QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect
    //                                            | d->selectionBehaviorFlags();
    QModelIndex index;
    switch (hint) {
    case QAbstractItemDelegate::EditNextItem: {
        index = moveCursor(MoveNext, Qt::NoModifier);
        if (index.isValid()) {
            QPersistentModelIndex persistent(index);
            setCurrentIndex(persistent);
            //d->selectionModel->setCurrentIndex(persistent, flags);
            // currentChanged signal would have already started editing
            if (insertNext) {
                insertElement();
            } else if (index.flags() & Qt::ItemIsEditable
                && (!(editTriggers() & QAbstractItemView::CurrentChanged))) {
                nexthint = hint;
                edit(persistent);
            }
            else
                nexthint = QAbstractItemDelegate::NoHint;
        } break; }
    case QAbstractItemDelegate::EditPreviousItem: {
        QModelIndex index = moveCursor(MovePrevious, Qt::NoModifier);
        if (index.isValid()) {
            QPersistentModelIndex persistent(index);
            setCurrentIndex(persistent);
            //d->selectionModel->setCurrentIndex(persistent, flags);
            // currentChanged signal would have already started editing
            if (index.flags() & Qt::ItemIsEditable
                && (!(editTriggers() & QAbstractItemView::CurrentChanged))) {
                nexthint = hint;
                edit(persistent);
            }
            else
                nexthint = QAbstractItemDelegate::NoHint;
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

void ItAlignmentView::insertNextRequested()
{
    insertElement();
}


bool ItAlignmentView::isEditing() {
  return editorOpen;
}

void ItAlignmentView::setSegView(ItSegmentView * cursegview) {
  segview = cursegview;
    emit segViewChanged(cursegview);
}

void ItAlignmentView::setEditor(ItPlainTextEdit * cureditor) {
  txteditor = cureditor;
  //emit segViewChanged(cursegview);
}

void ItAlignmentView::resizeRowToContents( int row )
{//if (txteditor) qDebug()<<txteditor<<txteditor->index.row()<<txteditor->sizeHint().height()<<sizeHintForRow(row);
    // DO IT HERE _OR_ IN THE DELEGATE'S sizeHint()... not both
    /*if (segview!=0 && segview->index.row() == row && segview->sizeHint().height()>sizeHintForRow(row)) {//qDebug()<<"setting"<<segview->sizeHint().height();
        setRowHeight(row, segview->sizeHint().height());}
    else if (txteditor!=0 && txteditor->index.row() == row && txteditor->sizeHint().height()>sizeHintForRow(row)) {
        //qDebug()<<"setting"<<txteditor->sizeHint().height()<<"at"<<row;
        setRowHeight(row, txteditor->sizeHint().height());}
    else*/
        QTableView::resizeRowToContents(row);
}

/*void ItAlignmentView::setRowHeight(int row, int height)
{
    qDebug()<<"nastavuji"<<row<<"na"<<height;
    QTableView::setRowHeight(row, height);
}*/

/*void ItAlignmentView::updateRowHeight(QModelIndex index, int height)
{
    if (sizeHintForRow(index.row())<height)
        setRowHeight(index.row(), height);
}*/

void ItAlignmentView::focusOutEvent( QFocusEvent * event )
{
  QWidget::focusOutEvent(event);
  emit focusChanged();
}

void ItAlignmentView::focusInEvent( QFocusEvent * event )
{
  QWidget::focusInEvent(event);
  emit focusChanged();
}

void ItAlignmentView::focusIndex(QModelIndex idx)
{
  if (idx.parent().isValid())
      idx = idx.parent();
  setCurrentIndex(idx);
  int row = idx.row()-skipMargin;
  if (row<0)
      row=0;
  QModelIndex viewidx = model()->index(row, idx.column());
  scrollTo(viewidx, QAbstractItemView::PositionAtTop);
}

void ItAlignmentView::setHtmlView(bool set)
{
  delegate->setHtmlView(set);
  updateRowSize();
  reset();
  /*if (segview!=0) {
    segview->setHtmlView(set);
    segview->update();
  }*/
}

bool ItAlignmentView::htmlView()
{
  return delegate->getHtmlView();
}

void ItAlignmentView::setFont( const QFont &f )
{
  QTableView::setFont(f);
  if (segview!=0)
    segview->setFont(f);
}

void ItAlignmentView::optimizeSize(QStatusBar * statusBar)
{
    updateRowSize();
/*
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif

  statusBar->showMessage(tr("Calculating optimal table dimensions (this takes some time, sorry)..."));
  resizeRowsToContents();
  statusBar->clearMessage();

#ifndef QT_NO_CURSOR
  QApplication::restoreOverrideCursor();
#endif
*/
}

bool ItAlignmentView::realign(int fromPos, int toPos, QList<QStringList> alignedIDs [2])
{
  ItAlignmentModel * m = static_cast<ItAlignmentModel*>(model());
  RealignCommand * cmd = new RealignCommand(m, fromPos, toPos, alignedIDs, LINK_AUTO);
  m->undoStack->push(cmd);
  if (!cmd->m_success) {
    m->undoStack->undo();
    return false;
  }
  updateRowSize();
  return true;
}


void ItAlignmentView::setHighlNon11(bool set)
{
  hNon11 = set;
  if (itmodel)
    itmodel->setHighlNon11(set);
}

void ItAlignmentView::setHighlMarked(bool set)
{
  hMarked = set;
  if (itmodel)
    itmodel->setHighlMarked(set);
}

bool ItAlignmentView::highlNon11()
{
  return hNon11;
}

bool ItAlignmentView::highlMarked()
{
  return hMarked;
}

void ItAlignmentView::moveText()
{
    if (!currentIndex().isValid())
        return;
    if (currentIndex().parent().isValid())
        return;
    int currow = currentIndex().row();
    int ver = currentIndex().column()-1;
    int minrow = itmodel->getPrevNonemptyRow(currentIndex());
    int newrow = QInputDialog::getInt(this, tr("Move text"),
                              tr("Move text in version '%1' beginning at position %2 to position:").arg(itmodel->alignment->info.ver[ver].name, QString::number(currow+1)),
                              currow+1, minrow+1, itmodel->rowCount())-1;
    int moveBy = newrow - currow;
    if (!moveBy)
        return;
    int steps = abs(moveBy);
    itmodel->undoStack->beginMacro("Move text");
    for (int i=0; i<steps; i++) {
        if (moveBy>0)
            itmodel->undoStack->push(new MoveDownCommand(itmodel, currentIndex()));
        else
            itmodel->undoStack->push(new MoveUpCommand(itmodel, currentIndex()));
    }
    itmodel->undoStack->endMacro();
    return;
}

QModelIndex ItAlignmentView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    if (cursorAction==QTableView::MoveNext)
        cursorAction = QTableView::MoveDown;
    else if (cursorAction==QTableView::MovePrevious)
        cursorAction = QTableView::MoveUp;
    return QTableView::moveCursor(cursorAction, modifiers);
}

void ItAlignmentView::setEditorKeys(EditorKeys keys)
{
  edKeys = keys;
}

EditorKeys ItAlignmentView::getEditorKeys() const
{
  return edKeys;
}
