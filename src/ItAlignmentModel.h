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

#ifndef IT_ALIGNMENT_MODEL_H
#define IT_ALIGNMENT_MODEL_H

#include <QBrush>
#include <QUndoStack>
#include "ItCommon.h"
#include "ItAlignment.h"
#include "ItSearchBar.h"

#define COLUMNS 4
#define FIRST_COLUMN 0
#define LAST_COLUMN 3

struct ItSegment
{
	aligned_doc doc;
	uint pos;
};

enum markType { NoMark = 0, ReplMark, FoundMark};

struct TxtMark
{
  int strpos;
  int len;
  markType mark;
};
bool operator==(TxtMark a, TxtMark b);
Q_DECLARE_METATYPE(TxtMark);

struct MarkFilter
{
  QString str;
  ItSearchBar::searchType stype;
  ItSearchBar::searchSide side;
};

struct Colors
{
  QColor bgdefault;
  QColor bgnon11;
  QColor bgmarked;
  QColor fgdefault;
  QColor cursor;
  QColor bgfound;
  QColor bgrepl;
  int cursorOpac;
  int bgAddDark;
};

class ItAlignmentModel : public QAbstractTableModel
{
	Q_OBJECT

public:

  ItAlignmentModel(ItAlignment * a, QObject *parent = 0);
	~ItAlignmentModel();

  QModelIndex lastMatchIndex;
  void status(int stat[]);
  bool save();
	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role);
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex &index) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	void setAlignment(ItAlignment * a);
	
  bool moveUp(const QModelIndex &idx, QList<ItAlignment::statRec> * slist = 0, bool nofocus = false);
  bool moveDown(const QModelIndex &idx, QList<ItAlignment::statRec> * slist = 0, bool nofocus = false);
  bool shift(const QModelIndex &idx, QList<ItAlignment::statRec> * slist = 0);
  bool pop(const QModelIndex &idx, QList<ItAlignment::statRec> * slist = 0, bool nofocus = false);
  bool swapWithPrevPosition(const QModelIndex &idx, QList<ItAlignment::statRec> * slist = 0, bool nofocus = false);
	void toggleMark(const QModelIndex &idx);
	void toggleStat(const QModelIndex &idx);
    void confirmAll(const QModelIndex &idx, QList<ItAlignment::statRec> * slist);
  bool updateContents(const QModelIndex &index, const QString &text);
    void undoStatusChanges(QList<ItAlignment::statRec> &slist);
    bool insert(QModelIndex &index);
    //void closeInsert();
	bool merge(QModelIndex index, int count = 1);
    bool split(QModelIndex &index, QStringList &stringlist, bool clear_history = true);
	bool remove(QModelIndex index, int count = 1);
	bool splitParent(QModelIndex index);
	bool mergeParent(QModelIndex index);
	bool canMerge(QModelIndex index, int count = 1);
  bool canMergeDeps(QModelIndex index, int count = 1);
	bool canSplitParent(QModelIndex index);
	bool canMergeParent(QModelIndex index);
	bool isFirstParEl(QModelIndex index);
    int getPrevNonemptyRow(QModelIndex idx);

	bool updateStat();
	void setUpdateStat(bool update);
	
  QModelIndex find(uint startpos, bool forward, ItSearchBar::searchSide side, ItSearchBar::searchType stype, QString str, bool silent = false);
  bool canReplace();
  bool replace(QString replacement, bool movecursor = true);
  bool replaceAll(uint * count, ItSearchBar::searchSide side, ItSearchBar::searchType stype, QString str, QString replacement);
  void replaceHistoryInsert(QModelIndex idx, int strpos, int len);
  void replaceHistoryRemove(QModelIndex idx, int strpos, int len);
  void replaceHistoryClear();
  void setLastMatch(QModelIndex idx, int strpos, int len, bool replaced = false, bool silent = false, QStringList captures = QStringList());

  bool realign(int fromPos, int toPos, QList<QStringList> alignedIDs [2], ushort newstatus, QList<ItAlignment::statRec> * slist = 0);

  void setColors(Colors c);
  void setTransformations(QList<Replacement> &trans);
  void setCSS(QString css);
  Colors getColors() const;
  QString getCSS() const;
  void setHighlNon11(bool set);
  void setHighlMarked(bool set);

  /*void cacheSize(QModelIndex &idx, QSize size);
	void resetSize(QModelIndex &idx);
  void resetAllSizes();*/
	
//private:
	//void setupModelData(const QStringList &lines, TreeItem *parent);
	ItAlignment *alignment;
  QUndoStack * undoStack;
public slots:
  void resetLastMatch();
  void setFilter(QString str, ItSearchBar::searchType stype, ItSearchBar::searchSide side);
  void resetFilter();
  void stopButtonPress();
  void externalDataChange();
  void refocusOnIndex(QModelIndex idx);
  void setHtmlViewMode(bool set);
  void undo();
  void commitInsert();
  void cancelInsert();
signals:
  void lastMatchChanged(QModelIndex idx);
  void updateFailure(QModelIndex idx);
  void focusOnChange(QModelIndex idx);

private:
  MarkFilter filter;
  QMultiHash<QModelIndex,TxtMark> replaceHistory;
  bool hEvenRows, hNon11, hMarked;
  Colors colors;
  QString cssStyle;
  QList<Replacement> transformations;

  bool autoUpdateStatus;
  bool stopButtonPressed;
  bool htmlViewMode;
  bool insertingElement;
};

#endif
