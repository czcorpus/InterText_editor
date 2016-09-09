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

#include "ItCommands.h"

/***************************************************
** MoveUpCommand
*/

MoveUpCommand::MoveUpCommand(ItAlignmentModel * m, const QModelIndex &index, bool donotfocus)
{
    setText(QObject::tr("Move text up"));
    m_model = m;
    m_index = index;
    m_items = m_model->rowCount(index);
    m_lastctime = m_model->alignment->info.changed;
    m_nofocus = donotfocus;
}

void MoveUpCommand::redo()
{
    m_model->moveUp(m_index, &m_sr, m_nofocus);
}

void MoveUpCommand::undo()
{
    m_model->moveDown(m_index, 0, m_nofocus);
    for (int i=0; i < m_items; i++) {
        m_model->pop(m_model->index(m_index.row()-1, m_index.column()), 0, true);
    }
    if (!m_nofocus)
        m_model->refocusOnIndex(m_index);
    m_model->undoStatusChanges(m_sr);
    m_model->alignment->info.changed = m_lastctime;
}

/***************************************************
** MoveDownCommand
*/

MoveDownCommand::MoveDownCommand(ItAlignmentModel * m, const QModelIndex &index, bool donotfocus)
{
    setText(QObject::tr("Move text down"));
    m_model = m;
    m_index = index;
    m_lastctime = m_model->alignment->info.changed;
    m_nofocus = donotfocus;
}

void MoveDownCommand::redo()
{
    m_model->moveDown(m_index, &m_sr, m_nofocus);
}

void MoveDownCommand::undo()
{
    m_model->moveUp(m_model->index(m_index.row()+1, m_index.column()), 0, m_nofocus);
    m_model->undoStatusChanges(m_sr);
    m_model->alignment->info.changed = m_lastctime;
}

/***************************************************
** ShiftCommand
*/

ShiftCommand::ShiftCommand(ItAlignmentModel * m, const QModelIndex &index)
{
    setText(QObject::tr("Shift element up"));
    m_model = m;
    m_index = index;
    m_lastctime = m_model->alignment->info.changed;
}

void ShiftCommand::redo()
{
    m_model->shift(m_index, &m_sr);
}

void ShiftCommand::undo()
{
    m_model->pop(m_model->index(m_index.row()-1, m_index.column()), 0, true);
    m_model->undoStatusChanges(m_sr);
    m_model->refocusOnIndex(m_index);
    m_model->alignment->info.changed = m_lastctime;
}


/***************************************************
** PopCommand
*/

PopCommand::PopCommand(ItAlignmentModel * m, const QModelIndex &index)
{
    setText(QObject::tr("Pop element down"));
    m_model = m;
    m_index = index;
    m_lastctime = m_model->alignment->info.changed;
}

void PopCommand::redo()
{
    m_model->pop(m_index, &m_sr);
}

void PopCommand::undo()
{
    m_model->shift(m_model->index(m_index.row()+1, m_index.column()));
    m_model->undoStatusChanges(m_sr);
    m_model->refocusOnIndex(m_index);
    m_model->alignment->info.changed = m_lastctime;
}

/***************************************************
** SwapCommand
*/

SwapCommand::SwapCommand(ItAlignmentModel * m, const QModelIndex &index)
{
    setText(QObject::tr("Swap with preceding segment"));
    m_model = m;
    m_index = index;
    m_lastctime = m_model->alignment->info.changed;
}

void SwapCommand::redo()
{
    m_model->swapWithPrevPosition(m_index, &m_sr);
}

void SwapCommand::undo()
{
    m_model->swapWithPrevPosition(m_index, 0, false);
    m_model->undoStatusChanges(m_sr);
    m_model->alignment->info.changed = m_lastctime;
    //m_model->refocusOnIndex(m_index);
}

/***************************************************
** ToggleMarkCommand
*/

ToggleMarkCommand::ToggleMarkCommand(ItAlignmentModel * m, const QModelIndex &index)
{
    setText(QObject::tr("Toggle bookmark"));
    m_model = m;
    m_index = index;
    m_lastctime = m_model->alignment->info.changed;
}

void ToggleMarkCommand::redo()
{
    m_model->toggleMark(m_index);
}

void ToggleMarkCommand::undo()
{
    m_model->toggleMark(m_index);
    m_model->alignment->info.changed = m_lastctime;
}

/***************************************************
** ToggleStatCommand
*/

ToggleStatCommand::ToggleStatCommand(ItAlignmentModel * m, const QModelIndex &index)
{
    setText(QObject::tr("Toggle status"));
    m_model = m;
    m_index = index;
    m_lastctime = m_model->alignment->info.changed;
}

void ToggleStatCommand::redo()
{
    m_model->toggleStat(m_index);
}

void ToggleStatCommand::undo()
{
    m_model->toggleStat(m_index);
    m_model->alignment->info.changed = m_lastctime;
}

/***************************************************
** ConfirmCommand
*/

ConfirmCommand::ConfirmCommand(ItAlignmentModel * m, const QModelIndex &index)
{
    setText(QObject::tr("Confirm all preceding"));
    m_model = m;
    m_index = index;
    m_lastctime = m_model->alignment->info.changed;
}

void ConfirmCommand::redo()
{
    m_model->confirmAll(m_index, &m_sr);
}

void ConfirmCommand::undo()
{
    m_model->undoStatusChanges(m_sr);
    m_model->alignment->info.changed = m_lastctime;
    m_model->refocusOnIndex(m_index);
}

/***************************************************
** UpdateContentsCommand
*/

UpdateContentsCommand::UpdateContentsCommand(ItAlignmentModel * m, const QModelIndex &index, QString &newtext)
{
    setText(QObject::tr("Change text"));
    m_model = m;
    m_index = index;
    m_newtext = newtext;
    m_oldtext = m_model->data(m_index, Qt::EditRole).toString();
    m_lastctime = m_model->alignment->info.ver[index.parent().column()-1].changed;
    m_wasVirgin = m_model->alignment->isVirgin(index.parent().column()-1, index.parent().row(), index.row());
    success = false;
}

void UpdateContentsCommand::redo()
{
    success = m_model->updateContents(m_index, m_newtext);
    m_model->replaceHistoryClear();
}

void UpdateContentsCommand::undo()
{
    m_model->updateContents(m_index, m_oldtext);
    m_model->replaceHistoryClear();
    m_model->alignment->setDocDepCTime(m_index.parent().column()-1, m_lastctime);
    if (m_wasVirgin)
        m_model->alignment->setVirgin(m_index.parent().column()-1, m_index.parent().row(), m_index.row());
}

/***************************************************
** ReplaceCommand
*/

ReplaceCommand::ReplaceCommand(ItAlignmentModel * m, const QModelIndex &index, QString &replacement, int strpos, int len, bool movecursor)
{
    setText(QObject::tr("Replace"));
    m_split = false;
    m_model = m;
    m_index = index;
    m_replacement = replacement;
    m_strpos = strpos;
    m_oldlen = len;
    m_oldtext = m_model->data(m_index, Qt::EditRole).toString();
    m_success = false;
    m_movecursor = movecursor;
    m_lastctime = m_model->alignment->info.ver[index.parent().column()-1].changed;
    m_wasVirgin = m_model->alignment->isVirgin(index.parent().column()-1, index.parent().row(), index.row());

    // split?
    if (m->alignment->info.ver[index.parent().column()-1].perm_chstruct) {
        QString newtext = m_oldtext;
        newtext.replace(m_strpos, m_oldlen, m_replacement);
        m_stringlist = newtext.split("\n\n", QString::SkipEmptyParts);
        for (int i=0; i<m_stringlist.length(); i++) {
            m_stringlist[i] = m_stringlist[i].trimmed();
        }
        if (m_stringlist.size()>1) {
            m_split = true;
            m_lastalctime = m_model->alignment->info.changed;
            m_model->alignment->getAlDepCTimes(index.parent().column()-1, &m_lastalctimes);
        }
    }
}

void ReplaceCommand::redo()
{
    QString newtext = m_oldtext;
    newtext.replace(m_strpos, m_oldlen, m_replacement);
    QStringList replfrags(m_replacement.split("\n\n", QString::SkipEmptyParts));
    if (m_split) { // ***************** split *********************
        if (m_model->split(m_index, m_stringlist, false)) {
            m_success = true;
            int istrpos, ilen;
            QModelIndex lidx;
            int i=0;
            for (i=0; i<m_stringlist.length(); i++) {
                lidx = m_model->index(m_index.row()+i, m_index.column(), m_index.parent());
                if (i==0) {
                    istrpos = m_strpos;
                    replfrags[0] = replfrags[0].replace(QRegExp("\\s*$"), "");
                    if (istrpos==0)
                        ilen = replfrags[0].replace(QRegExp("^\\s*"), "").length();
                    else
                        ilen = replfrags.at(0).length();
                } else if (i==m_stringlist.length()-1) {
                    istrpos = 0; ilen = replfrags[i].replace(QRegExp("^\\s*"), "").length();
                } else {
                    istrpos = 0; ilen = m_stringlist[i].length();
                }
                m_model->replaceHistoryInsert(lidx, istrpos, ilen);
            }
            i--;
            //if (m_movecursor)
            m_model->setLastMatch(lidx, replfrags.at(i).length(), 0, true, true);
            /*else {
              m_model->alignment->lastMatch.el = lidx.row();
              m_model->alignment->lastMatch.strpos = replfrags.at(i).length();
              m_model->alignment->lastMatch.len = 0;
              m_model->alignment->lastMatch.replaced = true;
          }*/
        } else {
            if (m_movecursor)
                m_model->setLastMatch(m_index, m_strpos, m_oldlen, false, true);
        }
    } else { // ***************** update *********************
        if (m_model->updateContents(m_index, newtext)) {
            m_success = true;
            if (m_strpos==0) {
                m_replacement.replace(QRegExp("^\\s*"), "");
            }
            m_model->replaceHistoryInsert(m_index, m_strpos, m_replacement.length());
            // oh, I can't find out what was the meaning of this stuff here... it does not seem to matter
            // m_movecursor is true only when "replacing all"...
            //if (m_movecursor)
            m_model->setLastMatch(m_index, m_strpos+m_replacement.length(), 0, true, true);
            /*else {
              m_model->alignment->lastMatch.strpos = m_strpos+m_replacement.length();
              m_model->alignment->lastMatch.len = 0;
              m_model->alignment->lastMatch.replaced = true;
          }*/
        } else {
            if (m_movecursor)
                m_model->setLastMatch(m_index, m_strpos, m_oldlen, false, true);
        }
    }
}

void ReplaceCommand::undo()
{
    if (m_success) {
        m_model->updateContents(m_index, m_oldtext);
        if (m_split) {
            m_model->remove(m_model->index(m_index.row()+1, m_index.column(), m_index.parent()), m_stringlist.size()-1);
            m_model->alignment->info.changed = m_lastalctime;
            m_model->alignment->setAlDepCTimes(m_index.parent().column()-1, &m_lastalctimes);
            int istrpos, ilen;
            QModelIndex lidx;
            QStringList replfrags(m_replacement.split("\n\n", QString::SkipEmptyParts));
            for (int i=0; i<m_stringlist.length(); i++) {
                lidx = m_model->index(m_index.row()+i, m_index.column(), m_index.parent());
                if (i==0) {
                    istrpos = m_strpos;
                    replfrags[0] = replfrags[0].replace(QRegExp("\\s*$"), "");
                    if (istrpos==0)
                        ilen = replfrags[0].replace(QRegExp("^\\s*"), "").length();
                    else
                        ilen = replfrags.at(0).length();
                } else if (i==m_stringlist.length()-1) {
                    istrpos = 0; ilen = replfrags[i].replace(QRegExp("^\\s*"), "").length();
                } else {
                    istrpos = 0; ilen = m_stringlist[i].length();
                }
                m_model->replaceHistoryRemove(lidx, istrpos, ilen);
            }
        } else {
            m_model->replaceHistoryRemove(m_index, m_strpos, m_replacement.length());
        }
        /*if (m_movecursor)
      m_model->setLastMatch(m_index, m_strpos, m_oldlen, false, true);*/
        m_model->resetLastMatch();
        m_model->alignment->setDocDepCTime(m_index.parent().column()-1, m_lastctime);
        if (m_wasVirgin)
            m_model->alignment->setVirgin(m_index.parent().column()-1, m_index.parent().row(), m_index.row());
    }
}

/***************************************************
** ReplaceAllCommand
*/

/*ReplaceAllCommand::ReplaceAllCommand(ItAlignmentModel * m,, QString &replacement)
{
  setText(QObject::tr("Replace all"));
  m_model = m;
  m_index = index;
  m_replacement = replacement;
  m_strpos = strpos;
  m_oldlen = len;
  m_oldtext = m_model->data(m_index, Qt::EditRole).toString();
}

void ReplaceAllCommand::redo()
{
#ifndef QT_NO_CURSOR
  QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
  QString newtext = m_oldtext;
  newtext.replace(m_strpos, m_oldlen, m_replacement);
  m_model->updateContents(m_index, newtext);
  m_model->replaceHistoryInsert(m_index, m_strpos, m_replacement.length());
  m_model->setLastMatch(m_index, m_strpos, m_oldlen, true);
#ifndef QT_NO_CURSOR
  QApplication::restoreOverrideCursor();
#endif
}

void ReplaceAllCommand::undo()
{
#ifndef QT_NO_CURSOR
  QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
  m_model->updateContents(m_index, m_oldtext);
  m_model->replaceHistoryRemove(m_index, m_strpos, m_replacement.length());
  m_model->setLastMatch(m_index, m_strpos, m_oldlen, false);
#ifndef QT_NO_CURSOR
  QApplication::restoreOverrideCursor();
#endif
}*/

/***************************************************
** SplitCommand
*/

SplitCommand::SplitCommand(ItAlignmentModel * m, const QModelIndex &index, QStringList &stringlist)
{
    setText(QObject::tr("Split element"));
    m_model = m;
    m_index = index;
    m_stringlist = stringlist;
    m_oldtext = m_model->data(m_index, Qt::EditRole).toString();
    m_lastalctime = m_model->alignment->info.changed;
    m_model->alignment->getAlDepCTimes(index.parent().column()-1, &m_lastalctimes);
    m_lastctime = m_model->alignment->info.ver[index.parent().column()-1].changed;
    m_wasVirgin = m_model->alignment->isVirgin(index.parent().column()-1, index.parent().row(), index.row());
    success = false;
}

void SplitCommand::redo()
{
    success = m_model->split(m_index, m_stringlist);
}

void SplitCommand::undo()
{
    // cannot just merge, the text may have been modified by the split as well
    m_model->updateContents(m_index, m_oldtext);
    m_model->remove(m_model->index(m_index.row()+1, m_index.column(), m_index.parent()), m_stringlist.size()-1);
    m_model->alignment->info.changed = m_lastalctime;
    m_model->alignment->setAlDepCTimes(m_index.parent().column()-1, &m_lastalctimes);
    m_model->alignment->setDocDepCTime(m_index.parent().column()-1, m_lastctime);
    if (m_wasVirgin)
        m_model->alignment->setVirgin(m_index.parent().column()-1, m_index.parent().row(), m_index.row());
}

/***************************************************
** MergeCommand
*/

MergeCommand::MergeCommand(ItAlignmentModel * m, const QModelIndex &index, int count)
{
    setText(QObject::tr("Merge elements"));
    m_model = m;
    m_index = index;
    m_count = count;
    m_stringlist.append(m_model->data(m_index, Qt::EditRole).toString());
    if (m_model->alignment->isVirgin(index.parent().column()-1, index.parent().row(), index.row()))
        m_repllist.append(-1);
    else
        m_repllist.append(m_model->alignment->getRepl(index.parent().column()-1, index.parent().row(), index.row()));
    m_parbrclist.append(m_model->alignment->getParbr(index.parent().column()-1, index.parent().row(), index.row()));
    for (int i=1; i <= count; i++) {
        QModelIndex idx = m_model->index(m_index.row()+i, m_index.column(), m_index.parent());
        m_stringlist.append(m_model->data(idx, Qt::EditRole).toString());
        if (m_model->isFirstParEl(idx) && m_model->canMergeParent(idx))
            m_parentbreaks.append(i);
        if (m_model->alignment->isVirgin(idx.parent().column()-1, idx.parent().row(), idx.row()))
            m_repllist.append(-1);
        else
            m_repllist.append(m_model->alignment->getRepl(idx.parent().column()-1, idx.parent().row(), idx.row()));
    }
    m_lastalctime = m_model->alignment->info.changed;
    m_model->alignment->getAlDepCTimes(index.parent().column()-1, &m_lastalctimes);
    m_lastctime = m_model->alignment->info.ver[index.parent().column()-1].changed;
}

void MergeCommand::redo()
{
    m_model->merge(m_index, m_count);
}

void MergeCommand::undo()
{
    m_model->split(m_index, m_stringlist);
    for (int i=0; i < m_parentbreaks.size(); i++)
        m_model->splitParent(m_model->index(m_index.row()+m_parentbreaks.at(i), m_index.column(), m_index.parent()));
    for (int i=0; i < m_repllist.size(); i++) {
        if (m_repllist.at(i)==-1)
            m_model->alignment->setVirgin(m_index.parent().column()-1, m_index.parent().row(), m_index.row()+i);
        else
            m_model->alignment->setRepl(m_index.parent().column()-1, m_index.parent().row(), m_index.row()+i, m_repllist.at(i));
    }
    for (int i=0; i < m_parbrclist.size(); i++)
        m_model->alignment->setParbr(m_index.parent().column()-1, m_index.parent().row(), m_index.row()+i, m_parbrclist.at(i));
    m_model->alignment->info.changed = m_lastalctime;
    m_model->alignment->setAlDepCTimes(m_index.parent().column()-1, &m_lastalctimes);
    m_model->alignment->setDocDepCTime(m_index.parent().column()-1, m_lastctime);
}

/***************************************************
** SplitParentCommand
*/

SplitParentCommand::SplitParentCommand(ItAlignmentModel * m, const QModelIndex &index)
{
    setText(QObject::tr("Split parents"));
    m_model = m;
    QModelIndex idx;
    if (!index.parent().isValid())
        idx = m_model->index(0,0,index);
    else
        idx = index;
    m_index = idx;
    m_lastalctime = m_model->alignment->info.changed;
    m_model->alignment->getAlDepCTimes(idx.parent().column()-1, &m_lastalctimes);
    m_lastctime = m_model->alignment->info.ver[idx.parent().column()-1].changed;
}

void SplitParentCommand::redo()
{
    m_model->splitParent(m_index);
}

void SplitParentCommand::undo()
{
    m_model->mergeParent(m_index);
    m_model->alignment->info.changed = m_lastalctime;
    m_model->alignment->setAlDepCTimes(m_index.parent().column()-1, &m_lastalctimes);
    m_model->alignment->setDocDepCTime(m_index.parent().column()-1, m_lastctime);
}


/***************************************************
** MergeParentCommand
*/

MergeParentCommand::MergeParentCommand(ItAlignmentModel * m, const QModelIndex &index)
{
    setText(QObject::tr("Merge parents"));
    m_model = m;
    QModelIndex idx;
    if (!index.parent().isValid())
        idx = m_model->index(0,0,index);
    else
        idx = index;
    m_index = idx;
    m_lastalctime = m_model->alignment->info.changed;
    m_model->alignment->getAlDepCTimes(idx.parent().column()-1, &m_lastalctimes);
    m_lastctime = m_model->alignment->info.ver[idx.parent().column()-1].changed;
}

void MergeParentCommand::redo()
{
    m_model->mergeParent(m_index);
}

void MergeParentCommand::undo()
{
    m_model->splitParent(m_index);
    m_model->alignment->info.changed = m_lastalctime;
    m_model->alignment->setAlDepCTimes(m_index.parent().column()-1, &m_lastalctimes);
    m_model->alignment->setDocDepCTime(m_index.parent().column()-1, m_lastctime);
}


/***************************************************
** RealignCommand
*/

RealignCommand::RealignCommand(ItAlignmentModel * m, int fromPos, int toPos, QList<QStringList> alignedIDs [2], ushort status)
{
    setText(QObject::tr("Automatic alignment"));
    m_model = m;
    m_newstatus = status;
    m_newalignedIDs[0] = alignedIDs[0];
    m_newalignedIDs[1] = alignedIDs[1];
    m_fromPos = fromPos;
    m_toPos = toPos;
    for (int pos=fromPos; pos<=toPos; pos++) {
        m_oldalignedIDs[0] << m_model->alignment->getIDs(0, pos);
        m_oldalignedIDs[1] << m_model->alignment->getIDs(1, pos);
    }
    m_success = false;
    m_lastctime = m_model->alignment->info.changed;
}

void RealignCommand::redo()
{
    m_success = m_model->realign(m_fromPos, m_toPos, m_newalignedIDs, m_newstatus, &m_sr);
}

void RealignCommand::undo()
{
    if (m_success) {
        m_model->realign(m_fromPos, m_fromPos+m_newalignedIDs[0].size()-1, m_oldalignedIDs, 0, 0);
        m_model->undoStatusChanges(m_sr);
        m_model->alignment->info.changed = m_lastctime;
    }
}

/***************************************************
** XMLSetValue
*/

XMLSetValueCommand::XMLSetValueCommand(ItDomModel * m, const QModelIndex &index, const QString &value)
{
    setText(QObject::tr("Change XML node value"));
    m_model = m;
    m_index = index;
    m_newValue = value;
    m_oldValue = m_model->data(m_index, Qt::EditRole).toString();
    m_lastctime = m_model->al->info.ver[m_model->getDocNum()].changed;
    m_success = false;
}

void XMLSetValueCommand::redo()
{
    m_success = m_model->setValue(m_index, m_newValue);
}

void XMLSetValueCommand::undo()
{
    m_model->setValue(m_index, m_oldValue);
    m_model->al->setDocDepCTime(m_model->getDocNum(), m_lastctime);
}

/***************************************************
** XMLCutNode
*/

XMLCutNodeCommand::XMLCutNodeCommand(ItDomModel * m, const QModelIndex &index)
{
    setText(QObject::tr("Cut XML node"));
    m_model = m;
    m_index = index;
    ItDomItem *item = static_cast<ItDomItem*>(index.internalPointer());
    m_node = item->node();
    m_lastctime = m_model->al->info.ver[m_model->getDocNum()].changed;
    m_success = false;
}

void XMLCutNodeCommand::redo()
{
    m_success = m_model->cutItem(m_index);
}

void XMLCutNodeCommand::undo()
{
    m_model->setPasteNode(m_node);
    m_model->pasteItem(m_index.parent(), m_index.row());
    m_model->al->setDocDepCTime(m_model->getDocNum(), m_lastctime);
}

/***************************************************
** XMLCutNode
*/

XMLPasteNodeCommand::XMLPasteNodeCommand(ItDomModel * m, const QModelIndex &parent, int beforePos)
{
    setText(QObject::tr("Paste XML node"));
    m_model = m;
    m_parent = parent;
    m_beforePos = beforePos;
    m_node = m_model->getPasteNode();
    m_lastctime = m_model->al->info.ver[m_model->getDocNum()].changed;
    m_success = false;
}

void XMLPasteNodeCommand::redo()
{
    m_success = m_model->pasteItem(m_parent, m_beforePos);
}

void XMLPasteNodeCommand::undo()
{
    m_model->cutItem(m_model->index(m_beforePos, 0, m_parent));
    m_model->setPasteNode(m_node);
    m_model->al->setDocDepCTime(m_model->getDocNum(), m_lastctime);
}

/***************************************************
** XMLAddChild
*/

XMLAddChildCommand::XMLAddChildCommand(ItDomModel * m, const QModelIndex &parent, int beforePos, const QString &name)
{
    setText(QObject::tr("Add XML child node"));
    m_model = m;
    m_parent = parent;
    m_beforePos = beforePos;
    m_name = name;
    m_lastctime = m_model->al->info.ver[m_model->getDocNum()].changed;
    m_success = false;
}

void XMLAddChildCommand::redo()
{
    m_success = m_model->addChildEl(m_parent, m_beforePos, m_name);
}

void XMLAddChildCommand::undo()
{
    m_model->cutItem(m_model->index(m_beforePos, 0, m_parent));
    m_model->al->setDocDepCTime(m_model->getDocNum(), m_lastctime);
}

/***************************************************
** XMLAddAttr
*/

XMLAddAttrCommand::XMLAddAttrCommand(ItDomModel * m, const QModelIndex &index, const QString &name)
{
    setText(QObject::tr("Add XML attribute"));
    m_model = m;
    m_index = index;
    m_name = name;
    m_lastctime = m_model->al->info.ver[m_model->getDocNum()].changed;
    m_success = false;
}

void XMLAddAttrCommand::redo()
{
    m_success = m_model->addAttribute(m_index, m_name);
}

void XMLAddAttrCommand::undo()
{
    m_model->delAttribute(m_index, m_name);
    m_model->al->setDocDepCTime(m_model->getDocNum(), m_lastctime);
}
