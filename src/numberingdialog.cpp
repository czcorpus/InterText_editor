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

#include "numberingdialog.h"
#include "ui_numberingdialog.h"

numberingDialog::numberingDialog(QWidget *parent, ItAlignment * a, int document, bool allowLock) :
    QDialog(parent),
    ui(new Ui::numberingDialog)
{
    ui->setupUi(this);
    alignment = a;
    doc = document;
    ui->mainLabel->setText(tr("The document version '%1' uses uknown element numbering scheme. Choose how to treat it:").arg(a->info.ver[doc].name));
    if (!allowLock)
        ui->lockButton->hide();
    adjustSize();
}

numberingDialog::~numberingDialog()
{
    delete ui;
}

void numberingDialog::accept() {
    if (ui->lockButton->isChecked()) {
        alignment->info.ver[doc].numLevels=1;
        alignment->info.ver[doc].perm_chstruct=false;
    } else if (ui->singleButton->isChecked()) {
        alignment->info.ver[doc].numLevels=1;
    } else {
        alignment->info.ver[doc].numLevels=2;
        alignment->info.ver[doc].numPrefix=":";
    }
    QDialog::accept();
}

void numberingDialog::setDefaultLevels(int level)
{
    if (level==0)
        ui->lockButton->setChecked(true);
    else if (level==1)
        ui->singleButton->setChecked(true);
    else if (level==2)
        ui->twolevelButton->setChecked(true);
}
