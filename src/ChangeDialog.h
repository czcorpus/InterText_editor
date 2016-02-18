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

#ifndef CHANGEDIALOG_H
#define CHANGEDIALOG_H

#include <QDialog>
#include <QDomNodeList>
#include "ItAlignment.h"

class ServerDialog;
class ItWindow;

namespace Ui {
  class ChangeDialog;
}

class ChangeDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ChangeDialog(ServerDialog *parent, ItWindow * win, ItAlignment * a, aligned_doc d, QDomNodeList &changes, bool * mark, bool acceptonly=false);
  ~ChangeDialog();
  void setCounter(QString num, QString total);
  void setTextFont(const QFont &font);
public slots:
  void updateRStrings(QDomNodeList nodelist);

private:
  ServerDialog * server;
  ItWindow * window;
  ItAlignment * alignment;
  QPushButton * appendButton;
  QPushButton * detachButton;
  aligned_doc doc;
  QDomNodeList changeList;
  int changenum, nextChangeNum, startnum, lstartnum, lcount;
  QStringList rstringlist;
  QList<bool> rparbreaks;
  bool autoAccept;
  int joinConsChanges;
  Ui::ChangeDialog * ui;
  //QStringList * m_rstrings;
  //QList<bool> * m_rparbr;
  //int m_rstartnum;
  bool * markState;
  void nextChange();
  void commitChange();
  void rejectChange();
  void syncFailure(QString text);
  void showChange(QStringList &lstrings, QList<bool> &lparbr);
private slots:
  void action(QAbstractButton * button);
  void changeMark(int state);
  void renderRStrings();
  void appendChange();
  void detachChange();
};

#endif // CHANGEDIALOG_H
