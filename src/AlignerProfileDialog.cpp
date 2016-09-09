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

#include "AlignerProfileDialog.h"
#include "ui_AlignerProfileDialog.h"
#include <QPushButton>

AlignerProfileDialog::AlignerProfileDialog(QWidget *parent, QString name, QString params) :
    QDialog(parent),
    ui(new Ui::AlignerProfileDialog)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(ui->edit_name, SIGNAL(textChanged(QString)), this, SLOT(validate(QString)));
    ui->edit_name->setText(name);
    ui->edit_params->setText(params);
    adjustSize();
}

AlignerProfileDialog::~AlignerProfileDialog()
{
    delete ui;
}

QString AlignerProfileDialog::getName()
{
    return ui->edit_name->text();
}

QString AlignerProfileDialog::getParams()
{
    return ui->edit_params->text();
}

void AlignerProfileDialog::validate(QString text)
{
    if (text.isEmpty())
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    else
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}
