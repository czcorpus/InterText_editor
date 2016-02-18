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

#include "NewAlignmentDialog.h"
#include "ui_NewAlignmentDialog.h"
#include "ItWindow.h"
#include "ServerDialog.h"

NewAlignmentDialog::NewAlignmentDialog(ItWindow *parent, ItAlignment * a, QStringList texts, QStringList versions) :
    QDialog(parent),
    ui(new Ui::NewAlignmentDialog)
{
    window = parent;
    m_texts = texts;
    m_versions = versions;
    remoteDocs[0] = QStringList();
    remoteDocs[1] = QStringList();
    ui->setupUi(this);
    alignment = a;
    ui->docNameEdit->addItem(alignment->info.docId);
    ui->ver1nameEdit->addItem(alignment->info.ver[0].name);
    ui->ver2nameEdit->addItem(alignment->info.ver[1].name);
    ui->docNameEdit->addItems(m_texts);
    ui->ver1nameEdit->addItems(m_versions);
    ui->ver2nameEdit->addItems(m_versions);
    ui->selSource1->addItem(tr("local file"), "");
    ui->selSource2->addItem(tr("local file"), "");
    connect(ui->selSource1, SIGNAL(currentIndexChanged(int)), this, SLOT(src1Changed(int)));
    connect(ui->selSource2, SIGNAL(currentIndexChanged(int)), this, SLOT(src2Changed(int)));

    QMapIterator<QString, ItServer> it(window->servers);
    QString name;
    int n = 0;
    while (it.hasNext()) {
        it.next();
        name = it.value().name;
        ui->selSource1->addItem(name, name);
        ui->selSource2->addItem(name, name);
        if (a->info.ver[0].source==it.value().name) {
            ui->selSource1->setCurrentIndex(n);
            ui->docNameEdit->setCurrentIndex(ui->docNameEdit->findText(a->info.docId));
            ui->ver1nameEdit->setCurrentIndex(ui->ver1nameEdit->findText(a->info.ver[0].name));
        }
        if (a->info.ver[1].source==it.value().name) {
            ui->selSource2->setCurrentIndex(n);
            ui->docNameEdit->setCurrentIndex(ui->docNameEdit->findText(a->info.docId));
            ui->ver1nameEdit->setCurrentIndex(ui->ver1nameEdit->findText(a->info.ver[1].name));
        }
        n++;
    }
    //adjustSize(); // too narrow...
}

NewAlignmentDialog::~NewAlignmentDialog()
{
    delete ui;
}

void NewAlignmentDialog::accept() {
  if (ui->docNameEdit->currentText().isEmpty()) {
      QMessageBox::critical(this, tr("New alignment"), tr("Document name cannot be empty."));
      return;
  }
  if (ui->ver1nameEdit->currentText().isEmpty() || ui->ver2nameEdit->currentText().isEmpty()) {
      QMessageBox::critical(this, tr("New alignment"), tr("Version name cannot be empty."));
      return;
  }
  if (ui->ver1nameEdit->currentText()==ui->ver2nameEdit->currentText()) {
      QMessageBox::critical(this, tr("New alignment"), tr("Version names cannot be identical."));
      return;
  }
  alignment->info.docId = ui->docNameEdit->currentText();
  alignment->info.ver[0].name = ui->ver1nameEdit->currentText();
  alignment->info.ver[1].name = ui->ver2nameEdit->currentText();
  if (ui->selSource1->currentIndex()>0)
      alignment->info.ver[0].source = ui->selSource1->currentText();
  if (ui->selSource2->currentIndex()>0)
    alignment->info.ver[1].source = ui->selSource2->currentText();
  if (alignment->info.ver[0].source.startsWith("http") && alignment->info.ver[1].source==alignment->info.ver[0].source) {
      ItServer s = window->servers.value(ui->selSource2->currentText());
      ServerDialog * sd = new ServerDialog(window, window->storagePath, s.url, s.username, s.passwd, true);
      sd->connectToServer();
      if (!sd->canUploadAlignment(alignment->info.docId, alignment->info.ver[0].name, alignment->info.ver[1].name)
          && sd->lastErrCode!=ERR_PERM_DENIED
          && (QMessageBox::Abort == QMessageBox::question(this,
                                                          tr("New alignment"),
                                                          tr("Such alignment already exists on the server. Do you want to continue?")
                                                          ,QMessageBox::Ok|QMessageBox::Abort))) {
          delete sd;
          return;
      }
      delete sd;
  }
  //QString t1 = sd->alTitleFormat.arg(alignment->info.docId, alignment->info.ver[0].name, alignment->info.ver[1].name);
  //QString t2 = sd->alTitleFormat.arg(alignment->info.docId, alignment->info.ver[1].name, alignment->info.ver[0].name);
  if (alignment->info.ver[0].source=="" && alignment->info.ver[1].source!="") {
    ItServer s = window->servers.value(alignment->info.ver[1].source);
    ServerDialog * sd = new ServerDialog(window, window->storagePath, s.url, s.username, s.passwd, true);
    sd->connectToServer();
    if (!sd->canUploadDoc(alignment->info.docId, alignment->info.ver[0].name)
       && sd->lastErrCode!=ERR_PERM_DENIED
       && (QMessageBox::Abort == QMessageBox::question(this,
                                                       tr("New alignment"),
                                                       tr("Version '%1' of this text already exists on the same server as version '%2'. Do you want to continue?")
                                                       .arg(alignment->info.ver[0].name, alignment->info.ver[1].name),QMessageBox::Ok|QMessageBox::Abort))) {
      delete sd;
      return;
    }
    delete sd;
  }
  if (alignment->info.ver[1].source=="" && alignment->info.ver[0].source!="") {
    ItServer s = window->servers.value(alignment->info.ver[0].source);
    ServerDialog * sd = new ServerDialog(window, window->storagePath, s.url, s.username, s.passwd, true);
    sd->connectToServer();
    if (!sd->canUploadDoc(alignment->info.docId, alignment->info.ver[1].name)
       && sd->lastErrCode!=ERR_PERM_DENIED
       && (QMessageBox::Abort == QMessageBox::question(this,
                                                       tr("New alignment"),
                                                       tr("Version '%1' of this text already exists on the same server as version '%2'. Do you want to continue?")
                                                       .arg(alignment->info.ver[1].name, alignment->info.ver[0].name),QMessageBox::Ok|QMessageBox::Abort))) {
      delete sd;
      return;
    }
    delete sd;
  }
  QDialog::accept();
}

void NewAlignmentDialog::enableOnly(int ver) {
  if (ver==0) {
    ui->docNameEdit->setEnabled(true);
    ui->ver1nameEdit->setEnabled(true);
    ui->ver2nameEdit->setEnabled(true);
  } else if (ver==1) {
    ui->docNameEdit->setEnabled(false);
    ui->ver1nameEdit->setEnabled(true);
    ui->ver2nameEdit->setEnabled(false);
  } else if (ver==2) {
    ui->docNameEdit->setEnabled(false);
    ui->ver1nameEdit->setEnabled(false);
    ui->ver2nameEdit->setEnabled(true);
  }
}

void NewAlignmentDialog::textNameChanged(QString val)
{
    if (ui->selSource1->currentIndex()!=0) {
       ui->ver1nameEdit->clear();
       ui->ver1nameEdit->addItems(filterDocs(0, val));
    }
    if (ui->selSource2->currentIndex()!=0) {
       ui->ver2nameEdit->clear();
       ui->ver2nameEdit->addItems(filterDocs(1, val));
    }
}

void NewAlignmentDialog::src1Changed(int val)
{
    ui->ver1nameEdit->clear();
    if (val==0) {
        ui->ver1nameEdit->addItem(alignment->info.ver[0].name);
        ui->ver1nameEdit->addItems(m_versions);
        ui->ver1nameEdit->setEditable(true);
        if (ui->selSource2->currentIndex()==0) {
            disconnect(ui->docNameEdit, SIGNAL(currentIndexChanged(QString)), this, SLOT(textNameChanged(QString)));
            ui->docNameEdit->clear();
            ui->docNameEdit->addItems(m_texts);
            ui->docNameEdit->setEditable(true);
        } else {
            QString ctxt = ui->docNameEdit->currentText();
            QString cver = ui->ver2nameEdit->currentText();
            src2Changed(ui->selSource2->currentIndex());
            ui->docNameEdit->setCurrentIndex(ui->docNameEdit->findText(ctxt));
            ui->ver2nameEdit->setCurrentIndex(ui->ver2nameEdit->findText(cver));
        }
    } else {
        ui->ver1nameEdit->setEditable(false);
        ui->docNameEdit->setEditable(false);
        remoteDocs[0] = getRemoteDocs(ui->selSource1->currentText());
        if (ui->selSource2->currentIndex()==0) {
            ui->docNameEdit->clear();
            connect(ui->docNameEdit, SIGNAL(currentIndexChanged(QString)), this, SLOT(textNameChanged(QString)));
            for (int i=0; i<remoteDocs[0].size(); i++) {
                QString txt = remoteDocs[0].at(i);
                txt.remove(QRegExp(":.*$"));
                if (ui->docNameEdit->findText(txt)==-1)
                   ui->docNameEdit->addItem(txt);
            }
        } else {
            ui->ver1nameEdit->clear();
            ui->ver1nameEdit->addItems(filterDocs(0, ui->docNameEdit->currentText()));
        }
    }
}

void NewAlignmentDialog::src2Changed(int val)
{
    ui->ver2nameEdit->clear();
    if (val==0) {
        ui->ver2nameEdit->addItem(alignment->info.ver[1].name);
        ui->ver2nameEdit->addItems(m_versions);
        ui->ver2nameEdit->setEditable(true);
        if (ui->selSource1->currentIndex()==0) {
            disconnect(ui->docNameEdit, SIGNAL(currentIndexChanged(QString)), this, SLOT(textNameChanged(QString)));
            ui->docNameEdit->clear();
            ui->docNameEdit->addItems(m_texts);
            ui->docNameEdit->setEditable(true);
        } else {
            QString ctxt = ui->docNameEdit->currentText();
            QString cver = ui->ver1nameEdit->currentText();
            src1Changed(ui->selSource1->currentIndex());
            ui->docNameEdit->setCurrentIndex(ui->docNameEdit->findText(ctxt));
            ui->ver1nameEdit->setCurrentIndex(ui->ver1nameEdit->findText(cver));
        }
    } else {
        ui->ver2nameEdit->setEditable(false);
        ui->docNameEdit->setEditable(false);
        remoteDocs[1] = getRemoteDocs(ui->selSource2->currentText());
        if (ui->selSource1->currentIndex()==0) {
            ui->docNameEdit->clear();
            connect(ui->docNameEdit, SIGNAL(currentIndexChanged(QString)), this, SLOT(textNameChanged(QString)));
            for (int i=0; i<remoteDocs[1].size(); i++) {
                QString txt = remoteDocs[1].at(i);
                txt.remove(QRegExp(":.*$"));
                if (ui->docNameEdit->findText(txt)==-1)
                   ui->docNameEdit->addItem(txt);
            }
        } else {
            ui->ver2nameEdit->clear();
            ui->ver2nameEdit->addItems(filterDocs(1, ui->docNameEdit->currentText()));
        }
    }
}

QStringList NewAlignmentDialog::getRemoteDocs(QString server)
{
    ItServer s = window->servers.value(server);
    ServerDialog * sd = new ServerDialog(window, window->storagePath, s.url, s.username, s.passwd, true);
    sd->connectToServer();
    QStringList list = sd->getRemoteDocList();
    delete sd;
    return list;
}

QStringList NewAlignmentDialog::filterDocs(int doc, QString filter)
{
    QStringList filtered;
    for (int i=0; i<remoteDocs[doc].size(); i++) {
        if (remoteDocs[doc].at(i).startsWith(filter)) {
            QString ver = remoteDocs[doc].at(i);
            ver.remove(QRegExp("^[^:]*: "));
            filtered << ver;
        }
    }
    return filtered;
}
