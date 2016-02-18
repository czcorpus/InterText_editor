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

#include "AlignmentManager.h"
#include "ui_AlignmentManager.h"
#include "ItWindow.h"
#include <QPushButton>
#include <QDir>

/***********************************************
* AlignmentManager
************************************************/

AlignmentManager::AlignmentManager(QString repository, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AlignmentManager)
{
  repository_path = repository;
  window = static_cast<ItWindow* >(parent);
  model = new AlListModel(repository, this);
  openButton = new QPushButton(tr("&Open"));
  connect(openButton, SIGNAL(clicked()), this, SLOT(alOpen()));
  openButton->setDefault(true);
  newButton = new QPushButton(tr("&New"));
  connect(newButton, SIGNAL(clicked()), this, SLOT(alNew()));
  importButton = new QPushButton(tr("&Import from file"));
  connect(importButton, SIGNAL(clicked()), this, SLOT(alImport()));
  delButton = new QPushButton(tr("&Delete"));
  connect(delButton, SIGNAL(clicked()), this, SLOT(alDelete()));
  propButton = new QPushButton(tr("&Properties"));
  connect(propButton, SIGNAL(clicked()), this, SLOT(alProp()));

  ui->setupUi(this);
  //setWindowTitle(tr("Repository manager"));
  ui->buttonBox->addButton(openButton, QDialogButtonBox::ActionRole);
  ui->buttonBox->addButton(newButton, QDialogButtonBox::ActionRole);
  ui->buttonBox->addButton(importButton, QDialogButtonBox::ActionRole);
  ui->buttonBox->addButton(delButton, QDialogButtonBox::ActionRole);
  ui->buttonBox->addButton(propButton, QDialogButtonBox::ActionRole);
  connect(ui->alView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(alOpen()));
  connect(ui->alView, SIGNAL(clicked(QModelIndex)), this, SLOT(curAlChanged(QModelIndex)));
  ui->alView->setModel(model);
  curAlChanged(ui->alView->currentIndex());
}

AlignmentManager::~AlignmentManager()
{
  delete ui;
  delete openButton;
  delete newButton;
  delete importButton;
  delete delButton;
  delete model;
}


void AlignmentManager::show(bool openonly)
{
    if (openonly && !(model->rowCount()==0)) {
        setWindowTitle(tr("Open alignment"));
        newButton->setVisible(false);
        delButton->setVisible(false);
        propButton->setVisible(false);
        importButton->setVisible(false);
    } else {
        setWindowTitle(tr("Repository manager"));
        newButton->setVisible(true);
        delButton->setVisible(true);
        propButton->setVisible(true);
        importButton->setVisible(true);
    }
    externalChange();
    QDialog::show();
    if (model->rowCount()==0) {
       QMessageBox::warning(this, tr("Empty repository"), tr("There are no alignments in your repository. Please, import or create some first."));
    }
}

void AlignmentManager::alOpen()
{
  if (!ui->alView->currentIndex().isValid())
    return;
  QString al = model->getName(ui->alView->currentIndex().row());
  hide();
  window->open(al);
}

void AlignmentManager::alDelete()
{
  QModelIndex i = ui->alView->currentIndex();
  if (!i.isValid() || !(i.row()<model->rowCount())){//qDebug()<<(i.row()<model->rowCount())<<i.isValid();
    return;}
  QString al = model->getName(ui->alView->currentIndex().row());
  QMessageBox::StandardButton resp = QMessageBox::question(this, tr("Deleting alignment"), tr("Are you sure you want to completely delete the alignment '%1'?").arg(al),
                                                           QMessageBox::Cancel | QMessageBox::Ok, QMessageBox::Cancel);
  if (resp==QMessageBox::Ok) {
    model->deleteAl(al);
    emit alDeletedInRepo(al);
  }
}

void AlignmentManager::alNew()
{
  hide();
  window->createNewAlignment();
}

void AlignmentManager::alImport()
{
  hide();
  window->importFile();
}

void AlignmentManager::alProp()
{
    bool del = true;
    if (!ui->alView->currentIndex().isValid())
      return;
    QString al = model->getName(ui->alView->currentIndex().row());
    QStringList p = al.split(".");
    int reload = window->isDependent(p.at(0), p.at(1), p.at(2));
    if (reload==-1)
      return;
    ItAlignment *a;
    if (window->model!=0 && al==window->model->alignment->info.name) {
        a = window->model->alignment;
        del = false;
    } else
        a = new ItAlignment(window->storagePath, al, window->defaultIdNamespaceURI);
    AlignmentAttrDialog *d = new AlignmentAttrDialog(this, a, del);
    if (!del)
        connect(d, SIGNAL(accepted()), window, SLOT(propertiesChanged()));
    if (reload==1)
        connect(d, SIGNAL(accepted()), window, SLOT(reloadAlignmentSilently()));
    d->exec();
}

QStringList AlignmentManager::listTexts()
{
  return model->listTexts();
}

QStringList AlignmentManager::listVersions()
{
  return model->listVersions();
}

void AlignmentManager::curAlChanged(QModelIndex index)
{
    if (!index.isValid()) {
      openButton->setEnabled(false);
      propButton->setEnabled(false);
      delButton->setEnabled(false);
    } else {
      openButton->setEnabled(true);
      propButton->setEnabled(true);
      delButton->setEnabled(true);
    }

}

void AlignmentManager::externalChange()
{
    model->externalChange();
}

/***********************************************
* AlListModel
************************************************/

AlListModel::AlListModel(QString repository, QObject *parent) : QAbstractListModel(parent)
{
  repository_path = repository;
}

AlListModel::~AlListModel()
{
}

QVariant AlListModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (!hasIndex(index.row(), index.column(), index.parent()))
    return QVariant();

  if (role != Qt::DisplayRole)
    return QVariant();

  QDir dir(repository_path);
  QStringList list = dir.entryList(QStringList("*.conf"), QDir::Files, QDir::Name);
  QString filename = list.at(index.row());
  return filename.replace(QRegExp("([^\\.]*)\\.([^\\.]*)\\.([^\\.]*)\\.conf"), "\\1 (\\2 - \\3)");
  //return filename.split(".").at(index.column());

}

int AlListModel::rowCount(const QModelIndex &parent) const
{
  if (parent.isValid())
    return 0;

  QDir dir(repository_path);
  QStringList list = dir.entryList(QStringList("*.conf"), QDir::Files, QDir::Name);
  return list.size();
}

int AlListModel::columnCount(const QModelIndex &parent) const
{
  return 1;
}

QString AlListModel::getName(int i)
{
  QDir dir(repository_path);
  QStringList list = dir.entryList(QStringList("*.conf"), QDir::Files, QDir::Name);
  QString name = list.at(i);
  return name.remove(-5,5);
}

void AlListModel::deleteAl(QString al)
{
  ItAlignment::deleteAlignment(repository_path, al);
  emit layoutChanged();
}

QStringList AlListModel::listTexts()
{
  QDir dir(repository_path);
  QStringList list = dir.entryList(QStringList("*.conf"), QDir::Files, QDir::Name);
  list.replaceInStrings(QRegExp("\\..*$"),"");
  list.removeDuplicates();
  list.sort();
  return list;
}

QStringList AlListModel::listVersions()
{
  QDir dir(repository_path);
  QStringList list = dir.entryList(QStringList("*.xml"));
  QString v;
  QStringList versions;
  foreach (v, list) {
    versions << v.split(".").at(1);
  }
  versions.removeDuplicates();
  versions.sort();
  return versions;
}

void AlListModel::externalChange()
{
    emit layoutChanged();
}
