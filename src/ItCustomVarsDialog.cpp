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

#include "ItCustomVarsDialog.h"
#include "ui_ItCustomVarsDialog.h"
#include "ItWindow.h"

ItCustomVarsDialog::ItCustomVarsDialog(ItWindow *parent, ExTextProfile &prof) :
    QDialog(parent),
    ui(new Ui::ItCustomVarsDialog)
{
    ui->setupUi(this);
    window = parent;
    QLineEdit * edit;
    QString defaultVal;
    for (int i=0; i<prof.customVars.size(); i++) {
        defaultVal = prof.customVars.at(i).defaultVal;
        defaultVal.replace("%t%", window->model->alignment->info.docId);
        defaultVal.replace("%v1%", window->model->alignment->info.ver[0].name);
        defaultVal.replace("%v2%", window->model->alignment->info.ver[1].name);
        edit = new QLineEdit(defaultVal, this);
        editors.append(edit);
        ui->formLayout->addRow(prof.customVars.at(i).desc, edit);
    }
}

ItCustomVarsDialog::~ItCustomVarsDialog()
{
    delete ui;
    qDeleteAll(editors);
}

QStringList ItCustomVarsDialog::getStringList()
{
    QStringList ret;
    for (int i=0; i<editors.size(); i++) {
        ret.append(editors.at(i)->text());
    }
    return ret;
}
