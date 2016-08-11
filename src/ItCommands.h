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

#ifndef ITCOMMANDS_H
#define ITCOMMANDS_H

#include <QUndoCommand>
#include "ItAlignmentModel.h"
#include "itdommodel.h"

class MoveUpCommand : public QUndoCommand
{
public:
  explicit MoveUpCommand(ItAlignmentModel * m, const QModelIndex &index, bool donotfocus = false);
	virtual void redo();
	virtual void undo();
private:
  ItAlignmentModel * m_model;
  QModelIndex m_index;
  int m_items;
	QList<ItAlignment::statRec> m_sr;
  QDateTime m_lastctime;
  bool m_nofocus;
};

class MoveDownCommand : public QUndoCommand
{
public:
  explicit MoveDownCommand(ItAlignmentModel * m, const QModelIndex &index, bool donotfocus = false);
	virtual void redo();
	virtual void undo();
private:
  ItAlignmentModel * m_model;
  QModelIndex m_index;
	QList<ItAlignment::statRec> m_sr;
  QDateTime m_lastctime;
  bool m_nofocus;
};

class ShiftCommand : public QUndoCommand
{
public:
  explicit ShiftCommand(ItAlignmentModel * m, const QModelIndex &index);
	virtual void redo();
	virtual void undo();
private:
  ItAlignmentModel * m_model;
  QModelIndex m_index;
	QList<ItAlignment::statRec> m_sr;
  QDateTime m_lastctime;
};

class PopCommand : public QUndoCommand
{
public:
  explicit PopCommand(ItAlignmentModel * m, const QModelIndex &index);
	virtual void redo();
	virtual void undo();
private:
  ItAlignmentModel * m_model;
  QModelIndex m_index;
  QList<ItAlignment::statRec> m_sr;
  QDateTime m_lastctime;
};

class SwapCommand : public QUndoCommand
{
public:
  explicit SwapCommand(ItAlignmentModel * m, const QModelIndex &index);
    virtual void redo();
    virtual void undo();
private:
  ItAlignmentModel * m_model;
  QModelIndex m_index;
  QList<ItAlignment::statRec> m_sr;
  QDateTime m_lastctime;
};

class ToggleMarkCommand : public QUndoCommand
{
public:
  explicit ToggleMarkCommand(ItAlignmentModel * m, const QModelIndex &index);
	virtual void redo();
	virtual void undo();
private:
  ItAlignmentModel * m_model;
  QModelIndex m_index;
  QDateTime m_lastctime;
};

class ToggleStatCommand : public QUndoCommand
{
public:
  explicit ToggleStatCommand(ItAlignmentModel * m, const QModelIndex &index);
	virtual void redo();
	virtual void undo();
private:
  ItAlignmentModel * m_model;
  QModelIndex m_index;
  QDateTime m_lastctime;
};

class ConfirmCommand : public QUndoCommand
{
public:
  explicit ConfirmCommand(ItAlignmentModel * m, const QModelIndex &index);
    virtual void redo();
    virtual void undo();
private:
  ItAlignmentModel * m_model;
  QModelIndex m_index;
  QList<ItAlignment::statRec> m_sr;
  QDateTime m_lastctime;
};

class UpdateContentsCommand : public QUndoCommand
{
public:
  explicit UpdateContentsCommand(ItAlignmentModel * m, const QModelIndex &index, QString &newtext);
  virtual void redo();
  virtual void undo();
  bool success;
private:
  ItAlignmentModel * m_model;
  QModelIndex m_index;
  QString m_oldtext, m_newtext;
  QDateTime m_lastctime;
  bool m_wasVirgin;
};

class ReplaceCommand : public QUndoCommand
{
public:
  explicit ReplaceCommand(ItAlignmentModel * m, const QModelIndex &index, QString &replacement, int strpos, int len, bool movecursor = true);
  virtual void redo();
  virtual void undo();
private:
  bool m_split;
  ItAlignmentModel * m_model;
  QModelIndex m_index;
  QString m_oldtext, m_replacement;
  QStringList m_stringlist;
  int m_strpos, m_oldlen;
  bool m_success, m_movecursor;
  QDateTime m_lastctime, m_lastalctime;
  QList<QDateTime> m_lastalctimes;
  bool m_wasVirgin;
};

/*class ReplaceAllCommand : public QUndoCommand
{
public:
  explicit ReplaceAllCommand(ItAlignmentModel * m, QStatusBar * statusBar, ItSearchBar::searchSide side, ItSearchBar::searchType stype, QString str, QString &replacement);
  virtual void redo();
  virtual void undo();
private:
  ItAlignmentModel * m_model;
  ItSearchBar::searchSide m_side;
  ItSearchBar::searchType m_stype;
  QString m_str, m_replacement;
  QStatusBar * m_statusBar;
};*/

class SplitCommand : public QUndoCommand
{
public:
	explicit SplitCommand(ItAlignmentModel * m, const QModelIndex &index, QStringList &stringlist);
	virtual void redo();
	virtual void undo();
    bool success;
private:
	ItAlignmentModel * m_model;
	QModelIndex m_index;
	QString m_oldtext;
	QStringList m_stringlist;
  QDateTime m_lastctime, m_lastalctime;
  QList<QDateTime> m_lastalctimes;
  bool m_wasVirgin;
};

class MergeCommand : public QUndoCommand
{
public:
	explicit MergeCommand(ItAlignmentModel * m, const QModelIndex &index, int count = 1);
	virtual void redo();
	virtual void undo();
private:
	ItAlignmentModel * m_model;
	QModelIndex m_index;
	int m_count;
	QStringList m_stringlist;
	QList<int> m_parentbreaks;
  QDateTime m_lastctime, m_lastalctime;
  QList<QDateTime> m_lastalctimes;
  QList<int> m_repllist;
  QStringList m_parbrclist;
};

class SplitParentCommand : public QUndoCommand
{
public:
	explicit SplitParentCommand(ItAlignmentModel * m, const QModelIndex &index);
	virtual void redo();
	virtual void undo();
private:
	ItAlignmentModel * m_model;
	QModelIndex m_index;
  QDateTime m_lastctime, m_lastalctime;
  QList<QDateTime> m_lastalctimes;
};

class MergeParentCommand : public QUndoCommand
{
public:
	explicit MergeParentCommand(ItAlignmentModel * m, const QModelIndex &index);
	virtual void redo();
	virtual void undo();
private:
	ItAlignmentModel * m_model;
	QModelIndex m_index;
  QDateTime m_lastctime, m_lastalctime;
  QList<QDateTime> m_lastalctimes;
};

class RealignCommand : public QUndoCommand
{
public:
  explicit RealignCommand(ItAlignmentModel * m, int fromPos, int toPos, QList<QStringList> alignedIDs [2], ushort status);
  virtual void redo();
  virtual void undo();
  bool m_success;
private:
  ItAlignmentModel * m_model;
  int m_fromPos, m_toPos;
  QList<QStringList> m_newalignedIDs [2];
  QList<QStringList> m_oldalignedIDs [2];
  QList<ItAlignment::statRec> m_sr;
  ushort m_newstatus;
  QDateTime m_lastctime;
};

/************************************************
 * XML Commands
 ************************************************/

class XMLSetValueCommand : public QUndoCommand
{
public:
  explicit XMLSetValueCommand(ItDomModel * m, const QModelIndex &index, const QString &value);
  virtual void redo();
  virtual void undo();
  bool m_success;
private:
  ItDomModel * m_model;
  QModelIndex m_index;
  QString m_oldValue, m_newValue;
  QDateTime m_lastctime;
};

class XMLCutNodeCommand : public QUndoCommand
{
public:
  explicit XMLCutNodeCommand(ItDomModel * m, const QModelIndex &index);
  virtual void redo();
  virtual void undo();
  bool m_success;
private:
  ItDomModel * m_model;
  QModelIndex m_index;
  QDomNode m_node;
  QDateTime m_lastctime;
};

class XMLPasteNodeCommand : public QUndoCommand
{
public:
  explicit XMLPasteNodeCommand(ItDomModel * m, const QModelIndex &parent, int beforePos);
  virtual void redo();
  virtual void undo();
  bool m_success;
private:
  ItDomModel * m_model;
  QModelIndex m_parent;
  int m_beforePos;
  QDomNode m_node;
  QDateTime m_lastctime;
};

class XMLAddChildCommand : public QUndoCommand
{
public:
  explicit XMLAddChildCommand(ItDomModel * m, const QModelIndex &parent, int beforePos, const QString &name);
  virtual void redo();
  virtual void undo();
  bool m_success;
private:
  ItDomModel * m_model;
  QModelIndex m_parent;
  int m_beforePos;
  QString m_name;
  QDateTime m_lastctime;
};

class XMLAddAttrCommand : public QUndoCommand
{
public:
  explicit XMLAddAttrCommand(ItDomModel * m, const QModelIndex &index, const QString &name);
  virtual void redo();
  virtual void undo();
  bool m_success;
private:
  ItDomModel * m_model;
  QModelIndex m_index;
  QString m_name;
  QDateTime m_lastctime;
};

#endif // ITCOMMANDS_H
