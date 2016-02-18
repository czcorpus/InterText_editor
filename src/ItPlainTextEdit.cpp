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

#include "ItPlainTextEdit.h"


ItPlainTextEdit::ItPlainTextEdit(QWidget *parent, EditorKeys keys) : QPlainTextEdit(parent) {
  this->fitting_height = 0;
  haveAsked = AutoAsk;
  index = QModelIndex();
  QSizePolicy sp;
  sp.setHeightForWidth(true);
  sp.setHorizontalPolicy(QSizePolicy::Fixed);
  sp.setVerticalPolicy(QSizePolicy::Preferred);
  setSizePolicy(sp);
  //connect(this, SIGNAL(textChanged()), this, SLOT(fitHeightToDocument()));
  textChangeFlag = false; // FIXME: how about using undo stack?
  connect(this, SIGNAL(textChanged()), this, SLOT(setTextChangeFlagTrue()));

  splitAct = new QAction(tr("Split"), this);
  //splitAct->setShortcut(Qt::CTRL|Qt::Key_S);
  splitAct->setStatusTip(tr("Split element at this point."));
  connect(splitAct, SIGNAL(triggered()), this, SLOT(splitAtCursor()));
  addAction(splitAct);

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint &)));
}

void ItPlainTextEdit::showContextMenu(const QPoint &pt)
{
    QMenu *menu = createStandardContextMenu();
    QAction * first = menu->actions().at(0);
    menu->insertAction(first, splitAct);
    menu->insertSeparator(first);
    menu->exec(mapToGlobal(pt));
    delete menu;
}

ItPlainTextEdit::~ItPlainTextEdit()
{

}

/*void ItPlainTextEdit::setPlainText(const QString &text)
{
    QPlainTextEdit::setPlainText(text);
    emit sizeHintChanged(index.row());
}*/


QSize ItPlainTextEdit::sizeHint() const {//qDebug()<<"Size hint asked.";
    //return QSize(width(), height());
    return QPlainTextEdit::sizeHint();
    //document()->setPageSize(QSizeF(rect().width(),999));qDebug()<<document()->size().toSize();
    //return document()->size().toSize();//childrenRegion().boundingRect().size();
    /*QSize sizehint = QPlainTextEdit::sizeHint();
  QSize document_size(document()->size().toSize());qDebug()<<"Doc"<<document()->size().toSize();
  int fitting_height = document_size.height() * (fontInfo().pixelSize()+3)+100;
  sizehint.setHeight(fitting_height);
  qDebug()<<"Size hint asked"<<sizehint;*/
    //return sizehint;
}

/*void ItPlainTextEdit::fitHeightToDocument() {
    /*document()->setTextWidth(viewport()->width());qDebug()<<"WP"<<viewport()->width();
    QSize document_size(document()->size().toSize());qDebug()<<"Doc"<<document()->size().toSize()<<document()->pageSize().toSize()<<document()->lineCount();
    fitting_height = document_size.height() * (fontInfo().pixelSize()+3);
    qDebug()<<document_size.height()<<fitting_height<<childrenRegion().boundingRect().size();*/
    /*QSize size(childrenRegion().boundingRect().size());
    int height = size.height() + contentsMargins().bottom() + contentsMargins().top();
    resize(width(), height);
    //emit sizeHintChanged(height);
    //updateGeometry();
}*/

void ItPlainTextEdit::setTextChangeFlagTrue() {
    textChangeFlag = true;
}

void ItPlainTextEdit::resetChangeFlag() {
    textChangeFlag = false;
}

bool ItPlainTextEdit::hasBeenChanged() {
    return textChangeFlag;
}

void ItPlainTextEdit::mousePressEvent(QMouseEvent *mouseEvent) {
    if (Qt::RightButton == mouseEvent->button()) {
        QTextCursor textCursor = cursorForPosition(mouseEvent->pos());
        //textCursor.select(QTextCursor::WordUnderCursor);
        setTextCursor(textCursor);
        //QString word = textCursor.selectedText();
        /*QTextCharFormat highlight;
        highlight.setBackground(QBrush(QColor("red")));
        textCursor.setCharFormat(highlight);*/

        //qDebug() << word;
    }
    QPlainTextEdit::mousePressEvent(mouseEvent);
}

void ItPlainTextEdit::splitAtCursor()
{
    QTextCursor c = textCursor();
    c.insertText("\n\n");
    haveAsked = AutoYes;
    emit closeEditor(this, QAbstractItemDelegate::NoHint);
}


void ItPlainTextEdit::setGeometry(const QRect &rect)
{//qDebug()<<"Set geometry";
    /*qDebug()<<"Setting initial geometry"<<rect.width()<<rect.height()<<rect.x()<<rect.y();
    //qDebug()<<"Height for width:"<<heightForWidth(rect.width());*/
    QPlainTextEdit::setGeometry(rect);
    //fitHeightToDocument();
    //QPlainTextEdit::setGeometry(QRect(x(), y(), width(), height));
    /*QRect nrect(rect);
    nrect.setHeight(rect.height()+(fontInfo().pixelSize()+3));
    QPlainTextEdit::setGeometry(nrect);*/
    /*fitHeightToDocument();
    updateGeometry();*/
    //if (height <= 2*rect.height()) {
    /*    QRect newrect(rect);
        newrect.setHeight(sizeHint().height());
        qDebug()<<"Updating geometry"<<sizeHint().height();
        QPlainTextEdit::setGeometry(newrect);*/
    //}
    //qDebug()<<"Emitting size hint change"<<index.row();
    emit sizeHintChanged(index.row());
    //qDebug()<<"Emitted.Done.";
}

//void ItPlainTextEdit::updateGeometry()
//{//qDebug()<<"Update geometry";
    /*QSize size(childrenRegion().boundingRect().size());
    int height = size.height() + contentsMargins().bottom() + contentsMargins().top();
qDebug()<<size.height()<<contentsMargins().top()<<contentsMargins().bottom()<<contentsRect().height();
QRect nrect(x(), y(), width(), height);
    QPlainTextEdit::setGeometry(nrect);*/ //*(fontInfo().pixelSize()+3));
    /*int height = sizeHint().height();
    int width = rect().width();
    qDebug()<<"Updating geometry"<<width<<height;
    resize(width, height);*/
    //QPlainTextEdit::setGeometry(nrect);
    /*QPlainTextEdit::updateGeometry();*/
    //emit sizeHintChanged(index.row());
//}

/*bool ItPlainTextEdit::event( QEvent * e )
{
    //qDebug()<<"Event"<<QString(this->metaObject()->className())<<e->type();;
    //if (e->type() == QEvent::ContentsRectChange) {qDebug()<<"Content size changed.";
    //    fitHeightToDocument();}
    return QPlainTextEdit::event(e);
}*/
