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

#ifndef ITCUSTOMVARSDIALOG_H
#define ITCUSTOMVARSDIALOG_H

#include <QDialog>
#include <QLineEdit>

class ItWindow;
struct ExTextProfile;

namespace Ui {
class ItCustomVarsDialog;
}

class ItCustomVarsDialog : public QDialog
{
  Q_OBJECT
  
public:
  explicit ItCustomVarsDialog(ItWindow *parent, ExTextProfile &prof);
  ~ItCustomVarsDialog();
  QStringList getStringList();
  
private:
  Ui::ItCustomVarsDialog *ui;
  ItWindow * window;
  QList< QLineEdit* > editors;
};

#endif // ITCUSTOMVARSDIALOG_H
