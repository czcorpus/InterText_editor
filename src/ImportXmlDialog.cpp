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

#include "ImportXmlDialog.h"
#include "ui_ImportXmlDialog.h"

ImportXmlDialog::ImportXmlDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImportXmlDialog)
{
    ui->setupUi(this);
    adjustSize();
}

ImportXmlDialog::~ImportXmlDialog()
{
    delete ui;
}

void ImportXmlDialog::setAlignableOnlyMode(bool set)
{
  if (set) {
    ui->label_alelements->setText(tr("Give a comma-separated list of names of all elements containing alignable text (e.g. \"head,s,verse\")."));
    ui->label_textelements->hide();
    ui->label_neweledit->hide();
    ui->edit_newelname->hide();
    ui->sel_textelements->hide();
    ui->edit_textelements->hide();
    ui->label_profilesel->hide();
    ui->sel_profile->hide();
    ui->label_txteledit->hide();
    adjustSize();
  }
}

bool ImportXmlDialog::isSegmented()
{
  if (ui->sel_alelements->isChecked())
    return true;
  else
    return false;
}

void ImportXmlDialog::setSplitter()
{
  ui->sel_textelements->setChecked(true);
}

bool ImportXmlDialog::getSplitter()
{
  return ui->sel_textelements->isChecked();
}

void ImportXmlDialog::setAlElements(QStringList &list)
{
  ui->edit_alelements->setText(list.join(","));
}

void ImportXmlDialog::setTextElements(QStringList &list)
{
  ui->edit_textelements->setText(list.join(","));
}

void ImportXmlDialog::setProfiles(QStringList list)
{
  ui->sel_profile->insertItems(0, list);
}

void ImportXmlDialog::setNewElName(QString name)
{
  ui->edit_newelname->setText(name);
}

QStringList ImportXmlDialog::getAlElements()
{
  QStringList list;
  QString elname;
  foreach (elname, ui->edit_alelements->text().split(",", QString::SkipEmptyParts)) {
    list.append(elname.trimmed());
  }
  return list;
}

QStringList ImportXmlDialog::getTextElements()
{
  QStringList list;
  QString elname;
  foreach (elname, ui->edit_textelements->text().split(",", QString::SkipEmptyParts)) {
    list.append(elname.trimmed());
  }
  return list;
}

int ImportXmlDialog::getProfile()
{
  return ui->sel_profile->currentIndex();
}

QString ImportXmlDialog::getNewElName()
{
  return ui->edit_newelname->text();
}
