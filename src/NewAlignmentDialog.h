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

#ifndef NEWALIGNMENTDIALOG_H
#define NEWALIGNMENTDIALOG_H

#include <QDialog>
#include "ItAlignment.h"

class ItWindow;

namespace Ui {
    class NewAlignmentDialog;
}

class NewAlignmentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewAlignmentDialog(ItWindow *parent, ItAlignment * a, QStringList texts, QStringList versions);
    ~NewAlignmentDialog();
    void accept();
    void enableOnly(int ver = 0);

private:
    Ui::NewAlignmentDialog *ui;
    ItAlignment * alignment;
    ItWindow *window;
    QStringList m_texts;
    QStringList m_versions;
    QStringList remoteDocs [2];
    QString textname;
    QStringList getRemoteDocs(QString server);
    QStringList filterDocs(int doc, QString filter);
private slots:
    void textNameChanged(QString val);
    void src1Changed(int val);
    void src2Changed(int val);
};

#endif // NEWALIGNMENTDIALOG_H
