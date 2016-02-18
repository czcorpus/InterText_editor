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

#ifndef ALIGNMENTATTRDIALOG_H
#define ALIGNMENTATTRDIALOG_H

#include <QDialog>
#include "ItAlignment.h"

namespace Ui {
class AlignmentAttrDialog;
}

class AlignmentAttrDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit AlignmentAttrDialog(QWidget *parent, ItAlignment *a, bool del=false);
    ~AlignmentAttrDialog();
public slots:
    void accept();
    void reject();
    
private:
    Ui::AlignmentAttrDialog *ui;
    ItAlignment *alignment;
    ItAlignment::alignmentInfo attrs;
    bool del_on_close;
private slots:
    void setTextName(QString str);
    void setAlSource(QString str);
    void setV1Name(QString str);
    void setV1Source(QString str);
    void setV1Levels(int val);
    void setV1Prefix(QString str);
    void setV1Separ(QString str);
    void changeV1Editing(int val);
    void changeV1ChStruct(int val);
    void setV2Name(QString str);
    void setV2Source(QString str);
    void setV2Levels(int val);
    void setV2Prefix(QString str);
    void setV2Separ(QString str);
    void changeV2Editing(int val);
    void changeV2ChStruct(int val);
};

#endif // ALIGNMENTATTRDIALOG_H
