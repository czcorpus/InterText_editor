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

#include "ItAbstractDelegate.h"
#include "ItSegmentView.h"

//AutoState ItAbstractDelegate::autoSaveElement = AutoAsk;

ItAbstractDelegate::ItAbstractDelegate(QObject *parent) : QStyledItemDelegate(parent) {
    myparent = static_cast<QWidget*>(parent);
  //colWidth[0] = 0;
  //colWidth[1] = 0;
}

void ItAbstractDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  //int origheight = option.rect.height();
  //ItAlignmentView * alview = static_cast<ItAlignmentView*>(parent());
  //ItSegmentView * segview = static_cast<ItSegmentView*>(editor);
  //int add=(index->model()->rowCount(index->model()->rootIndex()))*8;
  if (QString(editor->metaObject()->className())=="ItSegmentView") {
    ItSegmentView * segview = static_cast<ItSegmentView*>(editor);
    //int height = segview->heightHint()+2;
    //qDebug() << "orig" << origheight;
    //if (origheight>height) height = origheight;
    segview->setGeometry(option.rect);
    //editor->setGeometry(QRect(option.rect.x(), option.rect.y(), option.rect.width(), height));
    //qDebug() << "geometry";
    //alview->setRowHeight(index.row(), height);
    //qDebug() << "row";
  } else if (QString(editor->metaObject()->className())=="ItPlainTextEdit") {
      ItPlainTextEdit * txteditor = static_cast<ItPlainTextEdit*>(editor);
      txteditor->setGeometry(option.rect);
  } else
      editor->setGeometry(option.rect);
}

void ItAbstractDelegate::paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
  Q_ASSERT(index.isValid());

  /*if  (index.column()!=1 && index.column()!=2)
    return QStyledItemDelegate::paint(painter, option, index);*/

  painter->save();


  QStyleOptionViewItemV4 opt = option;
  initStyleOption(&opt, index);

  //QBrush brush = qvariant_cast<QBrush>(index.data(Qt::BackgroundRole));
  QBrush brush = opt.backgroundBrush;

  /* if (option.showDecorationSelected && (option.state & QStyle::State_Selected))
    brush.setColor(brush.color().darker(140));*/

  //QPointF oldBO = painter->brushOrigin();
  //painter->setBrushOrigin(option.rect.topLeft());
  painter->fillRect(option.rect, brush);
  //painter->setBrushOrigin(oldBO);

  if (option.state & QStyle::State_Selected) {
    const ItAlignmentModel * model = qobject_cast<const ItAlignmentModel*>(index.model());
    //QColor cursor("#33f");
    QColor cursor(model->getColors().cursor);
    cursor.setAlpha(model->getColors().cursorOpac);
    brush.setColor(cursor);
    //painter->translate(-opt.rect.x(), -opt.rect.y());
    //oldBO = painter->brushOrigin();
    //painter->setBrushOrigin(option.rect.topLeft());
    painter->fillRect(opt.rect, brush);
    //painter->setBrushOrigin(oldBO);
  }

  painter->setPen(QPen("#ccc"));
  painter->drawLine(QLine(opt.rect.topRight(), opt.rect.bottomRight()));

  if (!index.parent().isValid() && index.column()!=1 && index.column()!=2) {
    painter->drawImage(opt.rect.x()+4,opt.rect.y()+4,QImage(index.data(Qt::DecorationRole).toString()));
    //painter->drawImage(opt.rect.x()+4,opt.rect.y()+4, QImage(opt.icon.name()));
  } else {
    QTextDocument *doc = new QTextDocument;
    createContents(doc, opt, index);
    QAbstractTextDocumentLayout::PaintContext context;
    doc->setPageSize(opt.rect.size());
    //painter->setClipRect(opt.rect);
    painter->translate( opt.rect.x(), opt.rect.y() );
    doc->documentLayout()->draw(painter, context);
    delete doc;
  }

  painter->restore();
}

QSize ItAbstractDelegate::sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
  //return QStyledItemDelegate::sizeHint(option,index);
  if  (!index.parent().isValid() && index.column()!=1 && index.column()!=2) {
    return QSize(24,24);
  } else {
    //QSize size = QStyledItemDelegate::sizeHint(option, index);
    //return index.data(Qt::SizeHintRole);
    /*QSize size = index.data(Qt::SizeHintRole);
    if (size.width()==option.rect.width() && size.height()!=0) {
      return size;
    } else {*/
      //colWidth[c] = option.rect.width();
      QStyleOptionViewItemV4 opt = option;
      initStyleOption(&opt, index);

      QTextDocument *doc = new QTextDocument;

      createContents(doc, opt, index);
      doc->setPageSize(QSizeF(opt.rect.width(),999));
      QSize size = doc->size().toSize();
      delete doc;
      size+=QSize(0,1);
      return size;
    /*}*/
  }
}

void ItAbstractDelegate::createContents(QTextDocument * doc, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
  const ItAlignmentModel * model = qobject_cast<const ItAlignmentModel*>(index.model());
  QString text, complete;
  text = index.data().toString();
  QList<QVariant> vl = index.data(Qt::UserRole).toList();
  TxtMark m;
  complete = text;
  if (!htmlView)
    complete = complete.replace("&","&amp;").replace(">","&gt;").replace("<","&lt;");
  //QString header = "<body onMouseOver=\"document.getElementById('controls').style.visibility='visible';\" onMouseOut=\"document.getElementById('controls').style.visibility='hidden';\">";
  //header.append("<div id=\"controls\">Controls</div>");
  complete = QString("<body><p>%1</p></body>").arg(complete.replace("\n","</p><p>"));
  complete.replace(QChar(0x25BA),"<img src=\":/images/16/dblarrow.png\"/>");
  complete.replace(QChar(0x25A0),"<img src=\":/images/16/arrow.png\"/>");
  QString css = QString("p{margin:0px;padding:0px;color:%1;} #controls{display:none;} ").arg(option.palette.color(QPalette::Text).name());
  css.append(model->getCSS());
  doc->setDefaultStyleSheet(css);
  // span.match{background-color:#f96;} span.repl{background-color:#6f6;}");
  doc->setHtml(complete);//.replace("&","&amp;").replace(">","&gt;").replace("<","&lt;"));
  doc->adjustSize();
  doc->setDefaultFont(option.font);
  QTextCursor c(doc);
  QVariant i;
  QTextCharFormat replFormat, foundFormat;
  //QString start, block;
  //uint s_strpos, s_len;
  //replFormat.setBackground(QBrush(QColor("#6f6")));
  //foundFormat.setBackground(QBrush(QColor("#f96")));
  replFormat.setForeground(QBrush(model->getColors().fgdefault));
  replFormat.setBackground(QBrush(model->getColors().bgrepl));
  foundFormat.setForeground(QBrush(model->getColors().fgdefault));
  foundFormat.setBackground(QBrush(model->getColors().bgfound));
  foreach (i, vl) {
    m = qvariant_cast<TxtMark>(i);
    /*if (htmlView) {
      start = text.left(m.strpos);
      block = text.mid(m.strpos, m.len);
      s_strpos = start.replace(QRegExp("<[^>]*>"),"").replace(QRegExp("&[a-zA-Z0-9]*;"),"&").length();
      s_len = block.replace(QRegExp("<[^>]*>"),"").replace(QRegExp("&[a-zA-Z0-9]*;"),"&").length();
    } else {*/
      //s_strpos = m.strpos;
      //s_len = m.len;
    //}
    //c.setPosition(s_strpos);
    //c.setPosition(s_strpos+s_len, QTextCursor::KeepAnchor);
    c.setPosition(m.strpos);
    c.setPosition(m.strpos+m.len, QTextCursor::KeepAnchor);
    if (m.mark==ReplMark)
      c.setCharFormat(replFormat);
    else if (m.mark==FoundMark)
      c.setCharFormat(foundFormat);
  }
}

void ItAbstractDelegate::setHtmlView(bool set)
{
  htmlView = set;
}

bool ItAbstractDelegate::getHtmlView() const
{
  return htmlView;
}


/*void ItAbstractDelegate::setAutoSaveElement(AutoState value) {
    autoSaveElement = value;
}

AutoState ItAbstractDelegate::getAutoSaveElement() {
    return autoSaveElement;
}*/

/*void ItAbstractDelegate::onClose ( QWidget * editor, QAbstractItemDelegate::EndEditHint hint )
{
    if (QString(editor->metaObject()->className())=="ItPlainTextEdit") {
        ItPlainTextEdit * texted = static_cast<ItPlainTextEdit*>(editor);
        if (autoSaveElement == AutoAsk && texted->hasBeenChanged()) {
            ItQuestionDialog * qd = new ItQuestionDialog(myparent);
            qd->exec();
            if (qd->result() == QDialog::Accepted) {
                if (qd->getRememberChoice())
                    autoSaveElement = AutoYes;
                emit commitData(editor);
            } else if (qd->getRememberChoice()) {
                autoSaveElement = AutoNo;
                //setEditorData(texted, texted->index);
            }
        } else if (autoSaveElement == AutoYes) {
            emit commitData(editor);
        } else if (autoSaveElement == AutoNo) {
            //setEditorData(texted, texted->index);
        }
    }
    //closeEditor(editor, hint);
}*/

void ItAbstractDelegate::editorCloseRequested(QWidget * editor, QAbstractItemDelegate::EndEditHint hint)
{
    emit closeEditor(editor, hint);
}
