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

#include "AlignmentAttrDialog.h"
#include "ui_AlignmentAttrDialog.h"

AlignmentAttrDialog::AlignmentAttrDialog(QWidget *parent, ItAlignment *a, bool del) :
    QDialog(parent),
    ui(new Ui::AlignmentAttrDialog)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    del_on_close = del;
    alignment = a;
    attrs = alignment->info;
    ui->setupUi(this);
    QString src;

    setWindowTitle(attrs.name);
    ui->edit_textName->setText(attrs.docId);
    src = attrs.source;
    if (src.endsWith("api.php"))
        src.chop(7);
    ui->edit_source->setText(src);
    ui->lastUpdate->setText(attrs.changed.toString(Qt::SystemLocaleLongDate));
    ui->lastSynced->setText(attrs.synced.toString(Qt::SystemLocaleLongDate));

    ui->edit_v1Name->setText(attrs.ver[0].name);
    src = attrs.ver[0].source;
    if (src.endsWith("api.php"))
        src.chop(7);
    ui->edit_v1Source->setText(src);
    ui->v1LastUpdated->setText(attrs.ver[0].changed.toString(Qt::SystemLocaleLongDate));
    ui->v1LastSynced->setText(attrs.ver[0].synced.toString(Qt::SystemLocaleLongDate));
    if (attrs.ver[0].perm_chtext)
        ui->permv1Editing->setChecked(true);
    else
        ui->permv1Editing->setChecked(false);
    if (attrs.ver[0].perm_chstruct)
        ui->permv1ChStruct->setChecked(true);
    else
        ui->permv1ChStruct->setChecked(false);
    ui->edit_v1Levels->setValue(attrs.ver[0].numLevels);
    ui->edit_v1prefix->setText(attrs.ver[0].numParentPrefix);
    ui->edit_v1Separ->setText(attrs.ver[0].numPrefix);

    ui->edit_v2Name->setText(attrs.ver[1].name);
    src = attrs.ver[1].source;
    if (src.endsWith("api.php"))
        src.chop(7);
    ui->edit_v2Source->setText(src);
    ui->v2LastUpdated->setText(attrs.ver[1].changed.toString(Qt::SystemLocaleLongDate));
    ui->v2LastSynced->setText(attrs.ver[1].synced.toString(Qt::SystemLocaleLongDate));
    if (attrs.ver[1].perm_chtext)
        ui->permv2Editing->setChecked(true);
    else
        ui->permv2Editing->setChecked(false);
    if (attrs.ver[1].perm_chstruct)
        ui->permv2ChStruct->setChecked(true);
    else
        ui->permv2ChStruct->setChecked(false);
    ui->edit_v2Levels->setValue(attrs.ver[1].numLevels);
    ui->edit_v2prefix->setText(attrs.ver[1].numParentPrefix);
    ui->edit_v2Separ->setText(attrs.ver[1].numPrefix);

    connect(ui->edit_textName, SIGNAL(textChanged(QString)), this, SLOT(setTextName(QString)));
    connect(ui->edit_source, SIGNAL(textChanged(QString)), this, SLOT(setAlSource(QString)));

    connect(ui->edit_v1Name, SIGNAL(textChanged(QString)), this, SLOT(setV1Name(QString)));
    connect(ui->edit_v1Source, SIGNAL(textChanged(QString)), this, SLOT(setV1Source(QString)));
    connect(ui->edit_v1Levels, SIGNAL(valueChanged(int)), this, SLOT(setV1Levels(int)));
    connect(ui->edit_v1prefix, SIGNAL(textChanged(QString)), this, SLOT(setV1Prefix(QString)));
    connect(ui->edit_v1Separ, SIGNAL(textChanged(QString)), this, SLOT(setV1Separ(QString)));
    connect(ui->permv1Editing, SIGNAL(stateChanged(int)), this, SLOT(changeV1Editing(int)));
    connect(ui->permv1ChStruct, SIGNAL(stateChanged(int)), this, SLOT(changeV1ChStruct(int)));

    connect(ui->edit_v2Name, SIGNAL(textChanged(QString)), this, SLOT(setV2Name(QString)));
    connect(ui->edit_v2Source, SIGNAL(textChanged(QString)), this, SLOT(setV2Source(QString)));
    connect(ui->edit_v2Levels, SIGNAL(valueChanged(int)), this, SLOT(setV2Levels(int)));
    connect(ui->edit_v2prefix, SIGNAL(textChanged(QString)), this, SLOT(setV2Prefix(QString)));
    connect(ui->edit_v2Separ, SIGNAL(textChanged(QString)), this, SLOT(setV2Separ(QString)));
    connect(ui->permv2Editing, SIGNAL(stateChanged(int)), this, SLOT(changeV2Editing(int)));
    connect(ui->permv2ChStruct, SIGNAL(stateChanged(int)), this, SLOT(changeV2ChStruct(int)));

    if (attrs.source.startsWith("http") || attrs.ver[0].source.startsWith("http") || attrs.ver[1].source.startsWith("http")) {
        ui->label_warning->setStyleSheet("QLabel { color : red; padding: 5px; }");
        ui->label_warning->setText(tr("Warning: Changing properties of remote alignments and texts will usually disable any further synchronization of the text and alignment with the server. Use only if you know well what you are doing!"));
    }
    adjustSize();
}

AlignmentAttrDialog::~AlignmentAttrDialog()
{
    delete ui;
}

void AlignmentAttrDialog::accept()
{
    if (attrs.docId!=alignment->info.docId || attrs.ver[0].name!=alignment->info.ver[0].name || attrs.ver[1].name!=alignment->info.ver[1].name) {
        ItAlignment::alignmentInfo oldinfo = alignment->info;
        QString oldname = oldinfo.name;
        attrs.name = "";
        attrs.filename = "";
        attrs.ver[0].filename = "";
        attrs.ver[1].filename = "";
        alignment->info = attrs;
        if ((oldinfo.docId!=alignment->info.docId || oldinfo.ver[0].name!=alignment->info.ver[0].name) && alignment->alVerExists(0)) {
            QMessageBox::critical(this, tr("Renaming document"), tr("A document already exists with the same name you entered for the first document. Use another name or delete the original document (alignment)."));
            return;
        }
        if ((oldinfo.docId!=alignment->info.docId || oldinfo.ver[1].name!=alignment->info.ver[1].name) && alignment->alVerExists(1)) {
            QMessageBox::critical(this, tr("Renaming document"), tr("A document already exists with the same name you entered for the second document. Use another name or delete the original document (alignment)."));
            return;
        }
        if (alignment->save()) {
            if (oldname!=alignment->info.name) /* better safe than sorry */
                ItAlignment::deleteAlignment(alignment->storagePath, oldname);
        } else {
            alignment->info = oldinfo;
            QMessageBox::warning(this, tr("Saving alignment"), tr("Changing alignment name failed. Cannot save the alignment under the new name."));
        }
    } else {
        alignment->info = attrs;
        alignment->save();
    }
    if (del_on_close)
        delete alignment;
    QDialog::accept();
}

void AlignmentAttrDialog::reject()
{
    if (del_on_close)
        delete alignment;
    QDialog::reject();
}

void AlignmentAttrDialog::setTextName(QString str)
{
    attrs.docId = str;
}

void AlignmentAttrDialog::setAlSource(QString str)
{
    if (str.startsWith("http") && !str.endsWith(".php")) {
        if (!str.endsWith("/"))
            str.append("/");
        str.append("api.php");
    }
    attrs.source = str;
}

void AlignmentAttrDialog::setV1Name(QString str)
{
    attrs.ver[0].name = str;
}

void AlignmentAttrDialog::setV1Source(QString str)
{
    if (str.startsWith("http") && !str.endsWith(".php")) {
        if (!str.endsWith("/"))
            str.append("/");
        str.append("api.php");
    }
    attrs.ver[0].source = str;
}

void AlignmentAttrDialog::setV1Levels(int val)
{
    attrs.ver[0].numLevels = val;
}

void AlignmentAttrDialog::setV1Prefix(QString str)
{
    attrs.ver[0].numParentPrefix = str;
}

void AlignmentAttrDialog::setV1Separ(QString str)
{
    attrs.ver[0].numPrefix = str;
}

void AlignmentAttrDialog::changeV1Editing(int val)
{
    if (val==Qt::Checked)
        attrs.ver[0].perm_chtext = true;
    else
        attrs.ver[0].perm_chtext = false;
}

void AlignmentAttrDialog::changeV1ChStruct(int val)
{
    if (val==Qt::Checked)
        attrs.ver[0].perm_chstruct = true;
    else
        attrs.ver[0].perm_chstruct = false;
}

void AlignmentAttrDialog::setV2Name(QString str)
{
    attrs.ver[1].name = str;
}

void AlignmentAttrDialog::setV2Source(QString str)
{
    if (str.startsWith("http") && !str.endsWith(".php")) {
        if (!str.endsWith("/"))
            str.append("/");
        str.append("api.php");
    }
    attrs.ver[1].source = str;
}

void AlignmentAttrDialog::setV2Levels(int val)
{
    attrs.ver[1].numLevels = val;
}

void AlignmentAttrDialog::setV2Prefix(QString str)
{
    attrs.ver[1].numParentPrefix = str;
}

void AlignmentAttrDialog::setV2Separ(QString str)
{
    attrs.ver[1].numPrefix = str;
}

void AlignmentAttrDialog::changeV2Editing(int val)
{
    if (val==Qt::Checked)
        attrs.ver[1].perm_chtext = true;
    else
        attrs.ver[1].perm_chtext = false;
}

void AlignmentAttrDialog::changeV2ChStruct(int val)
{
    if (val==Qt::Checked)
        attrs.ver[1].perm_chstruct = true;
    else
        attrs.ver[1].perm_chstruct = false;
}
