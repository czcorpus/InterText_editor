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

#ifndef NUMBERINGDIALOG_H
#define NUMBERINGDIALOG_H

#include <QDialog>
#include "ItAlignment.h"

namespace Ui {
    class numberingDialog;
}

class numberingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit numberingDialog(QWidget *parent, ItAlignment * a, int document, bool allowLockOnly = true);
    ~numberingDialog();
    void accept();
    void setDefaultLevels(int level);

private:
    Ui::numberingDialog *ui;
    ItAlignment * alignment;
    int doc;
};

#endif // NUMBERINGDIALOG_H
