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

#ifndef AUTOALIGNDIALOG_H
#define AUTOALIGNDIALOG_H

#include <QDialog>
#include <QIntValidator>

namespace Ui {
    class AutoAlignDialog;
}

class AutoAlignDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AutoAlignDialog(QWidget *parent = 0, int max = 0);
    ~AutoAlignDialog();
  void addAligner(QString name, QStringList a_profiles);
  void setStartPos(int pos);
  void setEndPos(int pos);
  int getStartPos();
  int getEndPos();
  int getAligner();
  int getProfile();
  bool getAutoClose();

private:
    Ui::AutoAlignDialog *ui;
    QIntValidator * from_validator;
    QIntValidator * to_validator;
    QList<QStringList> profiles;
    void checkValidity();

private slots:
    void fromChanged();
    void toChanged();
    void alignerChanged(int al);
};

#endif // AUTOALIGNDIALOG_H
