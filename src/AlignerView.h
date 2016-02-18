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

#ifndef ALIGNERVIEW_H
#define ALIGNERVIEW_H

#include <QDialog>
#include <QAbstractButton>
#include <QProcess>

namespace Ui {
    class AlignerView;
}

class AlignerView : public QDialog
{
    Q_OBJECT

public:
  explicit AlignerView(QString cmd, QString params, QString &fname1, QString &fname2, int fromPos, int toPos, bool autoClose, QWidget *parent);
  ~AlignerView();
  void run_aligner();

signals:
  void result(QString res, int fromPos, int toPos);

private:
    Ui::AlignerView *ui;
    QProcess * my_proc;
    QString command, filename1, filename2;
    QString args;
    int startPos, endPos;
    bool userAbort, close_me;

private slots:
  void buttonPressed(QAbstractButton * button);
  void readProc();
  void proc_finished(int ret, QProcess::ExitStatus stat);
};

#endif // ALIGNERVIEW_H
