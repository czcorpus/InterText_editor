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

#include "RemoteAttrDialog.h"
#include "ui_RemoteAttrDialog.h"

RemoteAttrDialog::RemoteAttrDialog(ServerDialog *parent, QString key) :
    QDialog(parent),
    ui(new Ui::RemoteAttrDialog)
{
    sd = parent;
    al = sd->alignments[key];
    setAttribute(Qt::WA_DeleteOnClose, true);
    ui->setupUi(this);
    QMapIterator<int, ServerDialog::RemoteUser> i(sd->userList);
    ui->selEditor->addItem("---", 0);
    ui->selResp->addItem("---", 0);
    int j=0; int k=0;
    while (i.hasNext()) {
        i.next(); j++;
        ui->selEditor->addItem(i.value().name, i.key());
        ui->selRemoteUser->addItem(i.value().name, i.key());
        /*if (i.key()==al.edUser)
            ui->selEditor->setCurrentIndex(j);
        if (i.key()==al.remoteUser)
            ui->selRemoteUser->setCurrentIndex(j-1);*/
        if (i.value().type==ServerDialog::Admin || i.value().type==ServerDialog::Resp) {
            ui->selResp->addItem(i.value().name, i.key());
            k++;
            /*if (i.key()==al.respUser)
                ui->selResp->setCurrentIndex(k);*/
        }
    }

    selEditorProxyModel = new QSortFilterProxyModel(this);
    selEditorProxyModel->setSortLocaleAware(true);
    oldSelEditorModel = ui->selEditor->model();
    oldSelEditorModel->setParent(this);
    selEditorProxyModel->setSourceModel(oldSelEditorModel);
    selEditorProxyModel->sort(0);
    ui->selEditor->setModel(selEditorProxyModel);
    ui->selEditor->setCurrentText(sd->userList.value(al.edUser).name);
    selREditorProxyModel = new QSortFilterProxyModel(this);
    selREditorProxyModel->setSortLocaleAware(true);
    oldSelREditorModel = ui->selRemoteUser->model();
    oldSelREditorModel->setParent(this);
    selREditorProxyModel->setSourceModel(oldSelREditorModel);
    selREditorProxyModel->sort(0);
    ui->selRemoteUser->setModel(selREditorProxyModel);
    ui->selRemoteUser->setCurrentText(sd->userList.value(al.remoteUser).name);
    selRespProxyModel = new QSortFilterProxyModel(this);
    selRespProxyModel->setSortLocaleAware(true);
    oldSelRespModel = ui->selResp->model();
    oldSelRespModel->setParent(this);
    selRespProxyModel->setSourceModel(oldSelRespModel);
    selRespProxyModel->sort(0);
    ui->selResp->setModel(selRespProxyModel);
    ui->selResp->setCurrentText(sd->userList.value(al.respUser).name);

    if (al.perm_chtext)
        ui->checkBox_edit->setChecked(true);
    else
        ui->checkBox_edit->setChecked(false);
    if (al.perm_cchstruct)
        ui->checkBox_cstruct->setChecked(true);
    else
        ui->checkBox_cstruct->setChecked(false);
    ui->selStatus->addItem(tr("open"), ServerDialog::Open);
    ui->selStatus->addItem(tr("finished"), ServerDialog::Finished);
    if (!(sd->curUser().type==ServerDialog::Resp && (al.status==ServerDialog::Open || al.status==ServerDialog::Finished))) {
        ui->selStatus->addItem(tr("closed"), ServerDialog::Closed);
        ui->selStatus->addItem(tr("blocked"), ServerDialog::Blocked);
        ui->selStatus->addItem(tr("remote editor"), ServerDialog::Remote);
    }
    ui->selStatus->setCurrentIndex(int(al.status));
    if (al.status!=ServerDialog::Remote)
        ui->selRemoteUser->setVisible(false);
    setWindowTitle(sd->alTitleFormat.arg(al.text, al.v1, al.v2));

    setAccess();

    connect(ui->selEditor, SIGNAL(currentIndexChanged(int)), this, SLOT(changeEditor(int)));
    connect(ui->selResp, SIGNAL(currentIndexChanged(int)), this, SLOT(changeResp(int)));
    connect(ui->selRemoteUser, SIGNAL(currentIndexChanged(int)), this, SLOT(changeRemoteUser(int)));
    connect(ui->selStatus, SIGNAL(currentIndexChanged(int)), this, SLOT(changeStatus(int)));
    connect(ui->checkBox_edit, SIGNAL(stateChanged(int)), this, SLOT(changeEdit(int)));
    connect(ui->checkBox_cstruct, SIGNAL(stateChanged(int)), this, SLOT(changeCChstruct(int)));
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(apply()));
    adjustSize();
}

RemoteAttrDialog::~RemoteAttrDialog()
{
    delete ui;
    delete oldSelEditorModel;
    delete oldSelRespModel;
    delete oldSelREditorModel;
}

void RemoteAttrDialog::changeEditor(int n)
{
    al.edUser = ui->selEditor->itemData(n).toInt();
}

void RemoteAttrDialog::changeResp(int n)
{
    al.respUser = ui->selResp->itemData(n).toInt();
}

void RemoteAttrDialog::changeRemoteUser(int n)
{
    al.remoteUser = ui->selRemoteUser->itemData(n).toInt();
}

void RemoteAttrDialog::changeStatus(int n)
{
    al.status = ServerDialog::RemoteAlStatus(ui->selStatus->itemData(n).toInt());
    if (al.status==ServerDialog::Remote) {
        if (!(al.remoteUser>0))
            setRemoteUser(al.edUser);
        if (!(al.remoteUser>0))
            setRemoteUser(sd->curUser().id);
        ui->selRemoteUser->setVisible(true);
    } else
        ui->selRemoteUser->setVisible(false);
    setAccess();
}

void RemoteAttrDialog::setRemoteUser(int userid) {
    al.remoteUser = userid;
    ui->selRemoteUser->setCurrentIndex(ui->selRemoteUser->findData(userid));
}

void RemoteAttrDialog::changeEdit(int state)
{
    if (state == Qt::Checked)
        al.perm_chtext = true;
    else
        al.perm_chtext = false;
}

void RemoteAttrDialog::changeCChstruct(int state)
{
    if (state == Qt::Checked)
        al.perm_cchstruct = true;
    else
        al.perm_cchstruct = false;
}

void RemoteAttrDialog::setAccess()
{
    ui->selRemoteUser->setEnabled(false);
    ui->selEditor->setEnabled(false);
    ui->selResp->setEnabled(false);
    ui->checkBox_cstruct->setEnabled(false);
    ui->checkBox_edit->setEnabled(false);
    ui->selStatus->setEnabled(false);
    ui->selRemoteUser->setEnabled(false);

    if (sd->curUser().type==ServerDialog::Admin) {
        if (al.status==ServerDialog::Open) {
            ui->selRemoteUser->setEnabled(true);
            ui->selEditor->setEnabled(true);
            ui->selResp->setEnabled(true);
            ui->checkBox_cstruct->setEnabled(true);
            ui->checkBox_edit->setEnabled(true);
        }
        ui->selStatus->setEnabled(true);
        ui->selRemoteUser->setEnabled(true);
    } else if (sd->curUser().type==ServerDialog::Resp) {
        if (al.status==ServerDialog::Open) {
            ui->selEditor->setEnabled(true);
            ui->selResp->setEnabled(true);
            ui->checkBox_edit->setEnabled(true);
        }
        ui->selStatus->setEnabled(true);
    }
}

void RemoteAttrDialog::apply()
{
    QString key = sd->alTitleFormat.arg(al.text, al.v1, al.v2);
    if (sd->alignments[key].status==ServerDialog::Remote && (al.status!=ServerDialog::Remote || al.remoteUser!=sd->alignments[key].remoteUser)) {
        if (QMessageBox::Ok != QMessageBox::question(this, tr("Changing remote alignment attributes"),
                                                     tr("This change will disable the current remote editor to upload further updates to this alignment. Do you really want to continue?"),
                                                     QMessageBox::Ok | QMessageBox::Cancel))
            return;
    }
    if (sd->updateAlAttributes(al)) {
        accept();
    } else
        reject();
    //QMessageBox::critical(this, tr("Changing remote alignment attributes"), tr("Update failed...."));
}

