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

#include "ItSearchBar.h"
#include "ItWindow.h"

ItSearchBar::ItSearchBar(ItWindow *parent) : QWidget(parent),
    findEdit(new QComboBox(this)),
    replEdit(new QComboBox(this)),
    searchTypeSel(new QComboBox(this)),
    searchSideSel(new QComboBox(this)),
    toggleButton(new QToolButton(this))
{
    window = parent;
  QGridLayout *fullLayout = new QGridLayout(this);
  //fullLayout->setContentsMargins(2, 0, 2, 0);
  toggleButton->installEventFilter(this);
  findEdit->installEventFilter(this);
  replEdit->installEventFilter(this);
  searchTypeSel->installEventFilter(this);
  searchSideSel->installEventFilter(this);

  // hidebutton
  QToolButton *hideButton = new QToolButton(this);
  hideButton->installEventFilter(this);
  hideButton->setAutoRaise(true);
  hideButton->setIcon(QIcon(":/images/16/close.png"));
  connect(hideButton, SIGNAL(clicked()), this, SLOT(hide()));
  // labels
  QLabel *fLabel = new QLabel(tr("Search:"));
  fullLayout->addWidget(fLabel, 0, 1, Qt::AlignRight | Qt::AlignVCenter);
  QLabel *rLabel = new QLabel(tr("Replace:"));
  fullLayout->addWidget(rLabel, 1, 1, Qt::AlignRight | Qt::AlignVCenter);
  // parameters
  searchTypeSel->addItem(tr("Substring (a=A)"), SubStr);
  searchTypeSel->addItem(tr("Substring (a<>A)"), SubStrCS);
  searchTypeSel->addItem(tr("Regular exp. (a=A)"), RegExp);
  searchTypeSel->addItem(tr("Regular exp. (a<>A)"), RegExpCS);
  searchTypeSel->addItem(tr("Element ID (reg.exp.)"), ElementId);
  searchTypeSel->addItem(tr("Empty segment"), EmptySeg);
  searchTypeSel->addItem(tr("Segment <> 1"), Non1Seg);
  connect(searchTypeSel, SIGNAL(currentIndexChanged(int)), this, SLOT(searchTypeChanged(int)));
  searchSideSel->addItem(tr("left"), Left);
  searchSideSel->addItem(tr("right"), Right);
  searchSideSel->addItem(tr("both"), Both);
  // query boxes
  findEdit->setEditable(true);
  replEdit->setEditable(true);
  // buttons
  findNextButton = new QPushButton(QIcon(":/images/16/go-down-search.png"), tr("&Next"), this);
  findNextButton->installEventFilter(this);
  findPrevButton = new QPushButton(QIcon(":/images/16/go-up-search.png"), tr("&Previous"), this);
  findPrevButton->installEventFilter(this);
  findAllButton = new QPushButton(QIcon(), tr("Find &all"), this);
  findAllButton->installEventFilter(this);
  connect(findNextButton, SIGNAL(clicked()), parent, SLOT(findNext()));
  connect(findPrevButton, SIGNAL(clicked()), parent, SLOT(findPrev()));
  connect(this, SIGNAL(findNext()), parent, SLOT(findNext()));
  connect(this, SIGNAL(findPrev()), parent, SLOT(findPrev()));
  connect(this, SIGNAL(replFind()), parent, SLOT(replFind()));
  connect(findAllButton, SIGNAL(clicked()), parent, SLOT(setFilter()));
  QFrame * line = new QFrame(this);
  line->setFrameShape(QFrame::VLine);
  line->setFrameShadow(QFrame::Sunken);
  fullLayout->addWidget(line, 0, 7, 2, 1);
  fullLayout->addWidget(findAllButton, 0, 8);
  replaceButton = new QPushButton(QIcon(), tr("&Replace"), this);
  replaceButton->installEventFilter(this);
  replFindButton = new QPushButton(QIcon(), tr("Find && replace"), this);
  replFindButton->installEventFilter(this);
  replAllButton = new QPushButton(QIcon(), tr("Replace all"), this);
  replAllButton->installEventFilter(this);
  connect(replaceButton, SIGNAL(clicked()), parent, SLOT(replace()));
  connect(replFindButton, SIGNAL(clicked()), parent, SLOT(replFind()));
  connect(replAllButton, SIGNAL(clicked()), parent, SLOT(replaceAll()));
  replaceButton->setEnabled(false);
  // togglebutton
  toggleButton->setAutoRaise(true);
  toggleButton->setIcon(QIcon(":/images/16/arrow-down-double.png"));
  connect(toggleButton, SIGNAL(clicked()), this, SLOT(toggleMode()));

  fullLayout->addWidget(hideButton, 0, 0, 2, 1,  Qt::AlignLeft | Qt::AlignTop);
  fullLayout->addWidget(searchTypeSel, 0, 2);
  fullLayout->addWidget(findEdit, 0, 3);
  fullLayout->addWidget(searchSideSel, 0, 4);
  fullLayout->addWidget(findNextButton, 0, 5);
  fullLayout->addWidget(findPrevButton, 0, 6);
  fullLayout->addWidget(replEdit, 1, 3);
  fullLayout->setColumnStretch(3,1);
  fullLayout->addWidget(replFindButton, 1, 5);
  fullLayout->addWidget(replaceButton, 1, 6);
  fullLayout->addWidget(replAllButton, 1, 8);
  fullLayout->addWidget(toggleButton, 0, 9, 2, 1,  Qt::AlignRight | Qt::AlignTop);

  setLayout(fullLayout);
  mode = Replace;
  showSearch();
  hide();
}

ItSearchBar::~ItSearchBar() {
}

void ItSearchBar::toggleMode()
{
  QGridLayout * l = static_cast<QGridLayout*>(layout());
  if (mode==Replace) {
    l->itemAtPosition(1,1)->widget()->hide();
    l->itemAtPosition(1,3)->widget()->hide();
    l->itemAtPosition(1,5)->widget()->hide();
    l->itemAtPosition(1,6)->widget()->hide();
    l->itemAtPosition(1,8)->widget()->hide();
    toggleButton->setIcon(QIcon(":/images/16/arrow-up-double.png"));
    mode = Search;
    window->enableHtmlView(true);
  } else {
    if (!ensureNoHtmlView())
        return;
    l->itemAtPosition(1,1)->widget()->show();
    l->itemAtPosition(1,3)->widget()->show();
    l->itemAtPosition(1,5)->widget()->show();
    l->itemAtPosition(1,6)->widget()->show();
    l->itemAtPosition(1,8)->widget()->show();
    toggleButton->setIcon(QIcon(":/images/16/arrow-down-double.png"));
    mode = Replace;
    window->enableHtmlView(false);
  }
}

bool ItSearchBar::ensureNoHtmlView() {
    if (!window->view->htmlView()) {
        return true;
    } else {
        QMessageBox::StandardButton resp = QMessageBox::question(this, tr("Mode"),
          tr("Replace functions cannot work in the HTML view mode. Do you want to turn it off?"),
          QMessageBox::Ok|QMessageBox::Cancel);
        if (resp == QMessageBox::Ok) {
            window->toggleHtmlView();
            return true;
        } else {
            return false;
        }
    }
}

void ItSearchBar::showSearch()
{
  showAs(Search);
}

void ItSearchBar::showReplace()
{
  showAs(Replace);
}

void ItSearchBar::showAs(barMode startMode)
{
  if (startMode != mode)
    toggleMode();
  show();
  findEdit->setFocus();
}

ItSearchBar::searchType ItSearchBar::getSearchType()
{
  return (searchType)searchTypeSel->itemData(searchTypeSel->currentIndex()).toUInt();
}

ItSearchBar::searchSide ItSearchBar::getSearchSide()
{
  return (searchSide)searchSideSel->itemData(searchSideSel->currentIndex()).toUInt();
}

QString ItSearchBar::getSearchString()
{
  return findEdit->currentText();
}

QString ItSearchBar::getReplacementString()
{
  return replEdit->currentText();
}

void ItSearchBar::addCurrentQuery()
{
  if (findEdit->findText(getSearchString()) == -1)
    findEdit->addItem(getSearchString());
}

void ItSearchBar::hide()
{
    showSearch();
  QWidget::hide();
  enableReplace(false);
  emit hiding();
}

bool ItSearchBar::emptySearch()
{
  if (getSearchString().isEmpty() && (getSearchType()!=Bookmark && getSearchType()!=EmptySeg && getSearchType()!=Non1Seg))
    return true;
  else
    return false;
}

bool ItSearchBar::eventFilter(QObject *obj, QEvent *ev) {
    QWidget *editor = qobject_cast<QWidget*>(obj);
    if (!editor)
        return false;
    if (ev->type() == QEvent::KeyPress) {
        //qDebug()<<"obj "<<obj<<"key "<<static_cast<QKeyEvent *>(ev)->key();
        switch (static_cast<QKeyEvent *>(ev)->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (obj==findEdit) {
                emit findNext();
                return true;
            } else if (obj==replEdit) {
                emit replFind();
                return true;
            } else {
                return false;
            }
        case Qt::Key_Escape:
            hide();
            return true;
        default:
            return false;
        return false;
        }
    }
    if (ev->type() == QEvent::ShortcutOverride) {
        //qDebug()<<"obj "<<obj <<"override"<<static_cast<QKeyEvent *>(ev)->key();
        switch (static_cast<QKeyEvent *>(ev)->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            ev->accept();
            return true;
        case Qt::Key_Escape:
            ev->accept();
            return true;
        default:
            return false;
        }
    } else
      return false;
      //return QStyledItemDelegate::eventFilter(obj, ev);
}

void ItSearchBar::searchTypeChanged(int index)
{
  switch (getSearchType()) {
  case Bookmark:
  case EmptySeg:
  case Non1Seg:
    findEdit->setEnabled(false);
    replEdit->setEnabled(false);
    findAllButton->setEnabled(false);
    replaceButton->setEnabled(false);
    replFindButton->setEnabled(false);
    replAllButton->setEnabled(false);
    break;
  case ElementId:
    findEdit->setEnabled(true);
    replEdit->setEnabled(false);
    findAllButton->setEnabled(false);
    replaceButton->setEnabled(false);
    replFindButton->setEnabled(false);
    replAllButton->setEnabled(false);
    break;
  default:
    findEdit->setEnabled(true);
    replEdit->setEnabled(true);
    findAllButton->setEnabled(true);
    replaceButton->setEnabled(true);
    replFindButton->setEnabled(true);
    replAllButton->setEnabled(true);
  }
}

void ItSearchBar::enableReplace(bool enable)
{
  replaceButton->setEnabled(enable);
}
