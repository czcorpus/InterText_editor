/*  Copyright (c) 2010-2017 Pavel Vondřička (Pavel.Vondricka@korpus.cz)
 *  Copyright (c) 2010-2017 Charles University in Prague, Faculty of Arts,
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

#include <QIcon>
#include "ItAlignmentModel.h"
#include "ItCommands.h"

bool operator==(TxtMark a, TxtMark b)
{
    if (a.len==b.len && a.strpos==b.strpos && a.mark==b.mark)
        return true;
    else
        return false;
}

ItAlignmentModel::ItAlignmentModel(ItAlignment *a, QObject *parent) : QAbstractTableModel(parent) {
    setAlignment(a);
    autoUpdateStatus = true;
    undoStack = new QUndoStack(this);
    lastMatchIndex = QModelIndex();
    hEvenRows = true;
    hNon11 = true;
    hMarked = true;
    htmlViewMode = true;
    insertingElement = false;
}

ItAlignmentModel::~ItAlignmentModel() {
    undoStack->clear(); // avoid crash! disconnect signals to window->dataChanged() first!
    resetFilter();
    delete alignment;
    delete undoStack;
}

bool ItAlignmentModel::save() {
    if (alignment->save()) {
        undoStack->setClean();
        return true;
    } else
        return false;
}

void ItAlignmentModel::setHtmlViewMode(bool set)
{
    htmlViewMode = set;
}

QVariant ItAlignmentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (!hasIndex(index.row(), index.column(), index.parent()))
        return QVariant();

    if (index.row() > alignment->maxPos())
        return QVariant();

    QModelIndex segment = index;
    bool prepend = true;
    if (role == Qt::EditRole) prepend = false;

    // *** Inside of segment (single elements) ***
    if (index.parent().isValid()) {

        // Element text

        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            QString text = alignment->getContents(index.parent().column()-1, index.parent().row(), prepend).toStringList().at(index.row());
            if (role == Qt::DisplayRole && htmlViewMode) {
                Replacement r;
                // apply all rules (regular expressions)
                foreach (r, transformations) {
                    text.replace(QRegularExpression(r.src, QRegularExpression::UseUnicodePropertiesOption), r.repl);
                }
            }
            return text;
        }

        // Tool-tip (element ID)

        else if (role == Qt::ToolTipRole)
            return alignment->getIDs(index.parent().column()-1, index.parent().row()).at(index.row());

        else if (role == Qt::BackgroundRole)
            segment = index.parent();

        else if (role == Qt::UserRole) {
            QList<TxtMark> ml = replaceHistory.values(index);
            TxtMark m;
            QList<QVariant> vl;
            foreach(m,ml) {
                m.strpos += 2;
                vl.append(qVariantFromValue(m));
            }
            if (alignment->lastMatch.set && !alignment->lastMatch.replaced && alignment->lastMatch.len>0 && alignment->lastMatch.pos==index.parent().row() && alignment->lastMatch.doc==index.parent().column()-1 && alignment->lastMatch.el==index.row()) {
                m.strpos = alignment->lastMatch.strpos+2;
                m.len = alignment->lastMatch.len;
                m.mark = FoundMark;
                vl.append(qVariantFromValue(m));
            }
            return vl;
        }

        else return QVariant();
    }

    // *** Whole segments (main table) ***

    // Background

    if (role == Qt::BackgroundRole) {
        QColor color;
        if (hMarked && (alignment->getMark(segment.row())>0))
            color = colors.bgmarked;
        else if (hNon11 && (!alignment->is1to1(segment.row())))
            color = colors.bgnon11;
        else
            color = colors.bgdefault;
        if (hEvenRows && (segment.row() % 2))
            color = color.darker(100+colors.bgAddDark);
        return color;
    }

    // Foreground

    if (role == Qt::ForegroundRole) {
        QColor color;
        color = colors.fgdefault;
        return QBrush(color);
    }

    if (index.column()!=FIRST_COLUMN && index.column()!=LAST_COLUMN) {

        // Text of the whole segment

        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            QStringList els = alignment->getContents(index.column()-1, index.row(), prepend).toStringList();
            QString text = els.join("\n");
            if (role == Qt::DisplayRole && htmlViewMode) {
                Replacement r;
                // apply all rules (regular expressions)
                foreach (r, transformations) {
                    text.replace(QRegularExpression(r.src, QRegularExpression::UseUnicodePropertiesOption), r.repl);
                }
            }
            return text;

        } else if (role == Qt::UserRole) {
            TxtMark m;
            QList<QVariant> vl;
            QList<TxtMark> ml;
            QString text;
            QStringList strlist = alignment->getContents(index.column()-1, index.row(), prepend, htmlViewMode, &transformations).toStringList();
            uint prevlen = 0;
            for(int el=0; el<strlist.size(); el++) {
                ml = replaceHistory.values(this->index(el, 0, index));
                foreach(m,ml) {
                    m.strpos += prevlen+2;
                    vl.append(qVariantFromValue(m));
                }
                if (!filter.str.isEmpty() && (filter.side==index.column() || filter.side==ItSearchBar::Both)) {
                    int i=0;
                    QRegularExpression re;
                    if (filter.stype==ItSearchBar::SubStr || filter.stype==ItSearchBar::SubStrCS)
                        re.setPattern(QRegularExpression::escape(filter.str));
                    else
                        re.setPattern(filter.str);
                    if (filter.stype==ItSearchBar::RegExpCS || filter.stype==ItSearchBar::SubStrCS) {
                        re.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
                        //rx.setCaseSensitivity(Qt::CaseSensitive);
                    } else
                        re.setPatternOptions(QRegularExpression::CaseInsensitiveOption|QRegularExpression::UseUnicodePropertiesOption);
                    //rx.setCaseSensitivity(Qt::CaseInsensitive);
                    text = strlist.at(el);
                    QRegularExpressionMatch match;
                    while ((i=text.indexOf(re,i,&match)) != -1) {
                        m.strpos = i+prevlen;
                        m.len = match.capturedLength(0);
                        m.mark = FoundMark;
                        vl.append(qVariantFromValue(m));
                        i++;
                    }
                }
                if (alignment->lastMatch.set && !alignment->lastMatch.replaced && alignment->lastMatch.len>0 && alignment->lastMatch.pos==index.row() && alignment->lastMatch.doc==index.column()-1 && alignment->lastMatch.el==el) {
                    m.strpos = alignment->lastMatch.strpos+prevlen+2;
                    m.len = alignment->lastMatch.len;
                    m.mark = FoundMark;
                    vl.append(qVariantFromValue(m));
                }
                prevlen += strlist.at(el).length()+1;
            }
            return vl;

            // ToolTip (=IDs)

        } else if (role == Qt::ToolTipRole) {
            return alignment->getIDs(index.column()-1, index.row()).join(", ");

            // SizeHint

            /*} else if (role == Qt::SizeHintRole) {
            QSize size = alignment->getSize(index.column()-1, index.row());
            if (size.isValid()) return size;
            else return QVariant();*/
        } else return QVariant();

    } else if (index.column()==FIRST_COLUMN) {

        // Bookmarks

        if (role == Qt::DecorationRole) {
            if (alignment->getMark(index.row())>0)
                return QString(":/images/16/mark.png");
            else
                return QString(":/images/16/nomark.png");
        } else return QVariant();

    } else if (index.column()==LAST_COLUMN) {

        // Status

        if (role == Qt::DecorationRole) {
            ushort s = alignment->getStat(index.row());
            if (s==LINK_MANUAL) return QString(":/images/16/confirmed.png");
            else if (s==LINK_AUTO) return QString(":/images/16/automatic.png");
            else return QString(":/images/16/plain.png");
        } else if (role == Qt::ToolTipRole) {
            ushort s = alignment->getStat(index.row());
            if (s==LINK_MANUAL) return QString(tr("confirmed"));
            else if (s==LINK_AUTO) return QString(tr("automatically aligned"));
            else return QString(tr("unknown status"));
        } else return QVariant();

    } else return QVariant();

}

Qt::ItemFlags ItAlignmentModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    if (index.parent().isValid() && !data(index, Qt::DisplayRole).toString().isEmpty() &&
            ((index.parent().column()==1 && alignment->info.ver[0].perm_chtext) ||
             (index.parent().column()==2 && alignment->info.ver[1].perm_chtext)))
        return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;

    if (((index.column()==1 && alignment->info.ver[0].perm_chtext) || (index.column()==2 && alignment->info.ver[1].perm_chtext)) &&
            !data(index, Qt::DisplayRole).toString().isEmpty())
        return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
    else
        return QAbstractItemModel::flags(index);
}

QModelIndex ItAlignmentModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    if (!parent.isValid()) {
        return createIndex(row, column);
    } else {
        if (parent.column()==FIRST_COLUMN || parent.column()==LAST_COLUMN)
            return QModelIndex();

        //ItSegment * seg = new ItSegment;
        //seg->doc = parent.column()-1;
        //seg->pos = parent.row();
        //QModelIndex * par = const_cast<QModelIndex*>(&parent);
        qint32 seg = (parent.row()*10)+parent.column();
        return createIndex(row,column,seg);
    }
}

QModelIndex ItAlignmentModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    if (index.internalId()==0)
        return QModelIndex();

    //ItSegment *seg = static_cast<ItSegment*>(index.internalPointer());
    //return createIndex(int(seg->pos), int(seg->doc+1), 0);
    //QModelIndex *par = static_cast<QModelIndex*>(index.internalPointer());
    qint32 row = floor(index.internalId()/10);
    return createIndex(row, index.internalId()-(row*10));
}

int ItAlignmentModel::rowCount(const QModelIndex &parent) const 
{
    if (parent.parent().isValid())
        return 0;

    if (!parent.isValid())
        return alignment->maxPos()+1;

    if (parent.column()!=1 && parent.column()!=2)
        return 0;

    return alignment->getContents(parent.column()-1, parent.row()).toStringList().size();
}

int ItAlignmentModel::columnCount(const QModelIndex &parent) const
{
    if (parent.parent().isValid())
        return 0;

    if (!parent.isValid())
        return COLUMNS;
    else
        return 1;
}

QVariant ItAlignmentModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (section==0)
            if (role == Qt::DisplayRole)
                return QString(tr("M"));
            else if (role == Qt::ToolTipRole)
                return QString(tr("mark"));
            else
                return QVariant();
        else if (section==1)
            if (role == Qt::DisplayRole)
                return QString(alignment->info.ver[0].name);
            else
                return QVariant();
        else if (section==2)
            if (role == Qt::DisplayRole)
                return QString(alignment->info.ver[1].name);
            else
                return QVariant();
        else
            if (role == Qt::DisplayRole)
                return QString(tr("S"));
            else if (role == Qt::ToolTipRole)
                return QString(tr("status"));
            else
                return QVariant();
    } else
        if (role == Qt::DisplayRole)
            return QString("%1").arg(section+1);
        else
            return QVariant();
}

void ItAlignmentModel::setAlignment(ItAlignment * a)
{
    replaceHistoryClear();
    resetFilter();
    alignment = a;
}


bool ItAlignmentModel::moveUp(const QModelIndex &idx, QList<ItAlignment::statRec> * slist, bool nofocus)
{
    bool res;
    QModelIndex first, last;

    if (!idx.isValid())
        return false;

    //emit layoutAboutToBeChanged();
    if (autoUpdateStatus) alignment->scanStat(idx.row(), slist);
    res = alignment->moveUp(idx.column()-1, idx.row());
    if (autoUpdateStatus) alignment->updateStat(idx.row()-1);
    first = index(idx.row()-1, FIRST_COLUMN, idx.parent());
    last = index(alignment->maxPos(), LAST_COLUMN, idx.parent());
    //last = index(idx.row(), LAST_COLUMN, idx.parent());
    replaceHistoryClear();
    emit dataChanged(first, last);
    emit layoutChanged();
    if (!nofocus)
        emit focusOnChange(index(idx.row()-1, idx.column(), idx.parent()));
    return res;
}

bool ItAlignmentModel::moveDown(const QModelIndex &idx, QList<ItAlignment::statRec> * slist, bool nofocus)
{
    bool res;
    QModelIndex first, last;

    if (!idx.isValid())
        return false;

    //emit layoutAboutToBeChanged();
    if (autoUpdateStatus) alignment->scanStat(idx.row(), slist);
    res = alignment->moveDown(idx.column()-1, idx.row());
    if (autoUpdateStatus) alignment->updateStat(idx.row());
    first = index(idx.row(), FIRST_COLUMN, idx.parent());
    last = index(alignment->maxPos(), LAST_COLUMN, idx.parent());
    //last = index(idx.row(), LAST_COLUMN, idx.parent());
    replaceHistoryClear();
    emit dataChanged(first, last);
    emit layoutChanged();
    if (!nofocus)
        emit focusOnChange(index(idx.row()+1, idx.column(), idx.parent()));
    return res;
}

bool ItAlignmentModel::shift(const QModelIndex &idx, QList<ItAlignment::statRec> * slist)
{
    bool res;
    QModelIndex prev = index(idx.row()-1, FIRST_COLUMN, idx.parent());
    emit layoutAboutToBeChanged();
    if (autoUpdateStatus) alignment->scanStat(idx.row()-1, slist);
    res = alignment->shift(idx.column()-1, idx.row());
    if (autoUpdateStatus) alignment->updateStat(idx.row()-1);
    QModelIndex cur = index(idx.row(), LAST_COLUMN, idx.parent());
    if (res) {
        replaceHistoryClear();
        emit dataChanged(prev, cur);
    }
    emit layoutChanged();
    emit focusOnChange(idx);
    return res;
}

bool ItAlignmentModel::pop(const QModelIndex &idx, QList<ItAlignment::statRec> * slist, bool nofocus)
{
    bool res;
    QModelIndex next = index(idx.row()+1, LAST_COLUMN, idx.parent());
    emit layoutAboutToBeChanged();
    if (autoUpdateStatus) alignment->scanStat(idx.row(), slist);
    res = alignment->pop(idx.column()-1, idx.row());
    if (autoUpdateStatus) alignment->updateStat(idx.row());
    QModelIndex cur = index(idx.row(), FIRST_COLUMN, idx.parent());
    if (res) {
        replaceHistoryClear();
        emit dataChanged(cur, next);
    }
    emit layoutChanged();
    if (!nofocus)
        emit focusOnChange(idx);
    return res;
}

void ItAlignmentModel::toggleMark(const QModelIndex &idx)
{
    alignment->toggleMark(idx.row());
    QModelIndex curl = index(idx.row(), FIRST_COLUMN, idx.parent());
    QModelIndex curr = index(idx.row(), LAST_COLUMN, idx.parent());
    emit dataChanged(curl, curr);
    emit layoutChanged();
    emit focusOnChange(idx);
}

void ItAlignmentModel::toggleStat(const QModelIndex &idx)
{
    alignment->toggleStat(idx.row());
    QModelIndex curl = index(idx.row(), FIRST_COLUMN, idx.parent());
    QModelIndex curr = index(idx.row(), LAST_COLUMN, idx.parent());
    emit dataChanged(curl, curr);
    emit layoutChanged();
    emit focusOnChange(idx);
}

void ItAlignmentModel::confirmAll(const QModelIndex &idx, QList<ItAlignment::statRec> * slist)
{
    if (!idx.isValid())
        return;

    QModelIndex first, last;
    alignment->scanStat(idx.row(), slist);
    alignment->updateStat(idx.row());
    first = index(0, FIRST_COLUMN, idx.parent());
    last = index(idx.row(), LAST_COLUMN, idx.parent());
    replaceHistoryClear();
    emit dataChanged(first, last);
    emit layoutChanged();
    emit focusOnChange(idx);
}

void ItAlignmentModel::undoStatusChanges(QList<ItAlignment::statRec> &slist) {
    alignment->undoStatusChanges(slist);
    emit layoutChanged();
}

bool ItAlignmentModel::updateContents(const QModelIndex &index, const QString &text)
{
    if (index.isValid() && index.parent().isValid()) {
        if (!alignment->updateContents(index.parent().column()-1,index.parent().row(),index.row(), text)) {
            emit updateFailure(index);
            return false;
        }
        emit dataChanged(index.parent(), index.parent()); // will be done by ItAlignmentView::closeEditor
        emit layoutChanged();
        emit focusOnChange(index.parent());
        return true;
    }
    return false;
}

bool ItAlignmentModel::merge(QModelIndex index, int count) {
    if (!index.isValid())
        return false;
    if (!alignment->merge(index.parent().column()-1, index.parent().row(), index.row(), count))
        return false;
    replaceHistoryClear();
    emit dataChanged(index, this->index(index.row()+count, index.column(), index.parent())); // probably not a "data-change" for an index that was deleted???
    emit layoutChanged();
    emit focusOnChange(index.parent());
    return true;
}

bool ItAlignmentModel::insert(QModelIndex &index) {
    if (!index.isValid())
        return false;
    if (index.parent().isValid()) // only main table indexes (alignment positions) accepted
        return false;
    int back = 0;
    QModelIndex last = index;
    while(last.isValid() && !this->rowCount(last)) {
        back++;
        last = this->index(last.row()-1, last.column());
    }
    if (!last.isValid())
        return false;
    QModelIndex lastchild = this->index(this->rowCount(last)-1, 0, last);
    QStringList templist;
    templist.append(this->data(lastchild, Qt::EditRole).toString());
    templist.append("");
    undoStack->beginMacro("Insert element");
    insertingElement = true;
    SplitCommand * split = new SplitCommand(this, lastchild, templist);
    undoStack->push(split);
    while (back) {
        PopCommand * pop = new PopCommand(this, last);
        undoStack->push(pop);
        last = this->index(last.row()+1, last.column());
        back--;
    }
    //undoStack->endMacro();
    emit layoutChanged();
    return true;
}

void ItAlignmentModel::commitInsert()
{
    if (insertingElement) {
        undoStack->endMacro();
        insertingElement = false;
    }
}

void ItAlignmentModel::cancelInsert()
{
    if (insertingElement) {
        commitInsert();
        undoStack->undo();
    }
}

bool ItAlignmentModel::split(QModelIndex &index, QStringList &stringlist, bool clear_history) {
    if (!index.isValid())
        return false;
    if (!alignment->split(index.parent().column()-1, index.parent().row(), index.row(), stringlist)) {
        emit updateFailure(index);
        return false;
    }
    if (clear_history)
        replaceHistoryClear();
    emit dataChanged(index, this->index(index.row()+stringlist.size(), index.column(), index.parent()));
    emit layoutChanged();
    emit focusOnChange(index.parent());
    return true;
}

bool ItAlignmentModel::splitParent(QModelIndex index) {
    if (!index.parent().isValid())
        index = this->index(0,0,index);
    if (!alignment->splitParent(index.parent().column()-1, index.parent().row(), index.row()))
        return false;
    emit dataChanged(index, index);
    emit layoutChanged();
    emit focusOnChange(index.parent());
    return true;
}

bool ItAlignmentModel::mergeParent(QModelIndex index) {
    if (!index.parent().isValid())
        index = this->index(0,0,index);
    if (!alignment->mergeParent(index.parent().column()-1, index.parent().row(), index.row()))
        return false;
    emit dataChanged(index, index);
    emit layoutChanged();
    emit focusOnChange(index.parent());
    return true;
}

bool ItAlignmentModel::canMerge(QModelIndex index, int count) {
    if (!alignment->canMerge(index.parent().column()-1, index.parent().row(), index.row(), count))
        return false;
    return true;
}

bool ItAlignmentModel::canMergeDeps(QModelIndex index, int count) {
    if (!alignment->canMergeDeps(index.parent().column()-1, index.parent().row(), index.row(), count))
        return false;
    return true;
}

bool ItAlignmentModel::canSplitParent(QModelIndex index) {
    if (!index.parent().isValid())
        index = this->index(0,0,index);
    if (!index.isValid() || !index.parent().isValid())
        return false;
    if (!alignment->canSplitParent(index.parent().column()-1, index.parent().row(), index.row()))
        return false;
    return true;
}

bool ItAlignmentModel::canMergeParent(QModelIndex index) {
    if (!index.parent().isValid())
        index = this->index(0,0,index);
    if (!index.isValid() || !index.parent().isValid())
        return false;
    if (!alignment->canMergeParent(index.parent().column()-1, index.parent().row(), index.row()))
        return false;
    return true;
}

bool ItAlignmentModel::isFirstParEl(QModelIndex index) {
    if (alignment->isFirst(index.parent().column()-1, index.parent().row(), index.row()))
        return true;
    else
        return false;
}

bool ItAlignmentModel::remove(QModelIndex index, int count) {
    if (!index.isValid() || !index.parent().isValid())
        return false;
    if (!alignment->removeAfter(index.parent().column()-1, index.parent().row(), index.row()-1, count))
        return false;
    emit dataChanged(index.parent(), index.parent());
    emit layoutChanged();
    return true;
}

bool ItAlignmentModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && index.parent().isValid() && role == Qt::EditRole && value.toString()!=data(index,Qt::EditRole)) {
        //qDebug() << "SetData" << index.parent().row() << index.parent().column() << ": " << index.row() << index.column() << value << role;
        QString text = value.toString();
        if (alignment->info.ver[index.parent().column()-1].perm_chstruct) {
            QStringList list = text.split("\n\n", QString::SkipEmptyParts);
            if (list.size()>1) {
                SplitCommand * split = new SplitCommand(this, index, list);
                undoStack->push(split);
                if (!split->success) {
                    undoStack->undo();
                    return false;
                } else
                    return true;
            }
        }
        text.replace(QRegExp("\n\n+"), "\n");
        UpdateContentsCommand * update = new UpdateContentsCommand(this, index, text);
        undoStack->push(update);
        if (!update->success) {
            undoStack->undo();
            return false;
        } else {
            replaceHistoryClear();
            return true;
        }
    }
    return false;
}

/*void ItAlignmentModel::cacheSize(QModelIndex &idx, QSize size)
{
    alignment->cacheSize(idx.column()-1, idx.row(),size);
}

void ItAlignmentModel::resetSize(QModelIndex &idx)
{
    alignment->resetSize(idx.column()-1, idx.row());
}

void ItAlignmentModel::resetAllSizes()
{
    alignment->resetAllSizes();
}*/


bool ItAlignmentModel::updateStat() {
    return autoUpdateStatus;
}

void ItAlignmentModel::setUpdateStat(bool update) {
    autoUpdateStatus = update;
    alignment->info.autoUpdateStatus = update;
}

QModelIndex ItAlignmentModel::find(uint startpos, bool forward, ItSearchBar::searchSide side, ItSearchBar::searchType stype, QString str, bool silent)
{
    uint pos = alignment->find(startpos, forward, side, stype, str, htmlViewMode, transformations);
    if (pos!=INVALID_POSITION) {
        /*if (lastMatchIndex.isValid()) {
      emit dataChanged(lastMatchIndex, lastMatchIndex);
    }*/
        if (alignment->lastMatch.el<0)
            lastMatchIndex = index(pos, alignment->lastMatch.doc+1);
        else
            lastMatchIndex = index(alignment->lastMatch.el, 0, index(pos, alignment->lastMatch.doc+1));
        //qDebug()<<"Generated index"<<lastMatchIndex.row()<<lastMatchIndex.column()<<lastMatchIndex.parent().row()<<lastMatchIndex.parent().column();
        if (!silent)
            emit lastMatchChanged(lastMatchIndex);
        emit layoutChanged();
    } else lastMatchIndex = QModelIndex();
    return lastMatchIndex;
}

bool ItAlignmentModel::replace(QString replacement, bool movecursor)
{
    if (!lastMatchIndex.isValid())
        return false;
    ItAlignment::searchMatch m = alignment->lastMatch;
    if (m.replaced)
        return false;
    for(int i=1; i<m.capturedTexts.size(); i++) {
        replacement.replace(QString("\\%1").arg(i), m.capturedTexts.at(i));
    }
    replacement.replace("\\n","\n");
    ReplaceCommand * update = new ReplaceCommand(this, lastMatchIndex, replacement, m.strpos, m.len, movecursor);
    undoStack->push(update);
    if (!alignment->lastMatch.replaced) {
        return false;
    }
    //emit dataChanged(lastMatchIndex.parent(), lastMatchIndex.parent());
    emit layoutChanged();
    return true;
}

bool ItAlignmentModel::replaceAll(uint * count, ItSearchBar::searchSide side, ItSearchBar::searchType stype, QString str, QString replacement)
{
    stopButtonPressed = false;
    resetLastMatch();
    *count = 0;
    find(0, true, side, stype, str, true);
    if (!canReplace())
        return false;
    //double prog = 0;
    //uint max = rowCount();
    //uint cur = 0;
    undoStack->beginMacro("Replace all");
    while (canReplace()) {
        //cur = alignment->lastMatch.pos;
        //prog = ((cur*1.0) / max)*100;
        /*statusBar->showMessage(tr("Replace all in progress... %1% (segment: %2 of %3; occurrence: %4)").arg(QString::number(prog,'f',1),
                                                                                                          QString::number(cur),
                                                                                                          QString::number(max),
                                                                                                          QString::number(*count)),0);*/
        //qDebug()<<prog<<"%; segment"<<cur<<"ze"<<max<<"; count:"<<*count;
        if (replace(replacement, false))
            (*count)++;
        else {
            emit lastMatchChanged(lastMatchIndex);
            break;
        }
        find(INVALID_POSITION, true, side, stype, str, true);
    }
    undoStack->endMacro();
    emit lastMatchChanged(lastMatchIndex);
    emit layoutChanged();
    return true;
}

void ItAlignmentModel::replaceHistoryInsert(QModelIndex idx, int strpos, int len)
{
    TxtMark rm;
    rm.strpos = strpos;
    rm.len = len;
    rm.mark = ReplMark;
    replaceHistory.insert(idx, rm);
}

void ItAlignmentModel::replaceHistoryRemove(QModelIndex idx, int strpos, int len)
{
    TxtMark rm;
    rm.strpos = strpos;
    rm.len = len;
    rm.mark = ReplMark;
    replaceHistory.remove(idx, rm);
}

void ItAlignmentModel::replaceHistoryClear()
{
    replaceHistory.clear();
}

void ItAlignmentModel::resetLastMatch()
{
    alignment->resetLastMatch();
    //emit dataChanged(lastMatchIndex.parent(), lastMatchIndex.parent());
    //emit layoutChanged();
    lastMatchIndex = QModelIndex();
}

void ItAlignmentModel::setLastMatch(QModelIndex idx, int strpos, int len, bool replaced, bool silent, QStringList captures)
{
    alignment->setLastMatch(replaced, idx.parent().column()-1, idx.parent().row(), idx.row(), strpos, len, captures);
    lastMatchIndex = idx;
    if (!silent)
        emit lastMatchChanged(idx);
}

bool ItAlignmentModel::canReplace()
{
    return (lastMatchIndex.isValid() && !alignment->lastMatch.replaced);
}

void ItAlignmentModel::setFilter(QString str, ItSearchBar::searchType stype, ItSearchBar::searchSide side)
{
    filter.str=str;
    filter.stype=stype;
    filter.side=side;
    emit layoutChanged();
}

void ItAlignmentModel::resetFilter()
{
    filter.str="";
}

void ItAlignmentModel::stopButtonPress()
{
    stopButtonPressed = true;
}

bool ItAlignmentModel::realign(int fromPos, int toPos, QList<QStringList> alignedIDs [2], ushort newstatus, QList<ItAlignment::statRec> * slist)
{
    if (alignment->realign(fromPos, toPos, alignedIDs, newstatus, slist)) {
        emit layoutChanged();
        return true;
    } else
        return false;
}

void ItAlignmentModel::setColors(Colors c)
{
    colors = c;
    emit layoutChanged();
}

Colors ItAlignmentModel::getColors() const
{
    return colors;
}

void ItAlignmentModel::setTransformations(QList<Replacement> &trans)
{
    transformations = trans;
    emit layoutChanged();
}

void ItAlignmentModel::setCSS(QString css)
{
    cssStyle = css;
    emit layoutChanged();
}

QString ItAlignmentModel::getCSS() const
{
    return cssStyle;
}

void ItAlignmentModel::setHighlNon11(bool set)
{
    hNon11 = set;
    emit layoutChanged();
}

void ItAlignmentModel::setHighlMarked(bool set)
{
    hMarked = set;
    emit layoutChanged();
}

void ItAlignmentModel::externalDataChange()
{
    emit layoutChanged();
}

void ItAlignmentModel::refocusOnIndex(QModelIndex idx)
{
    emit focusOnChange(idx);
}

void ItAlignmentModel::status(int stat [6])
{
    int count = alignment->maxPos()+1;
    stat[0] = count; stat[4] = -1; stat[5] = 0;
    stat[LINK_PLAIN] = 0; stat[LINK_AUTO] = 0; stat[LINK_MANUAL] = 0;
    ushort s, m;
    for (int i=0; i<count; i++) {
        s = alignment->getStat(i);
        m = alignment->getMark(i);
        stat[s]++;
        if (stat[4]==-1 && s!=LINK_MANUAL)
            stat[4]=i;
        if (m>0)
            stat[5]++;
    }
}

int ItAlignmentModel::getPrevNonemptyRow(QModelIndex idx)
{
    do {
        idx = index(idx.row()-1, idx.column(), idx.parent());
    } while (rowCount(idx)==0);
    if (idx.row()>=0)
        return idx.row();
    else
        return 0;
}

void ItAlignmentModel::undo()
{
    undoStack->undo();
}

bool ItAlignmentModel::swapWithPrevPosition(const QModelIndex &idx, QList<ItAlignment::statRec> * slist, bool nofocus)
{
    bool res;
    QModelIndex prev = index(idx.row()-1, LAST_COLUMN, idx.parent());
    emit layoutAboutToBeChanged();
    if (autoUpdateStatus) alignment->scanStat(idx.row(), slist);
    res = alignment->swapWithPrevPosition(idx.column()-1, idx.row());
    if (autoUpdateStatus) alignment->updateStat(idx.row());
    QModelIndex cur = index(idx.row(), FIRST_COLUMN, idx.parent());
    if (res) {
        replaceHistoryClear();
        emit dataChanged(prev, cur);
    }
    emit layoutChanged();
    if (!nofocus)
        emit focusOnChange(idx.sibling(idx.row()-1, idx.column()));
    return res;
}
