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

#include "AutoAlignDialog.h"
#include "ui_AutoAlignDialog.h"
#include <QPushButton>
#include <QDebug>

AutoAlignDialog::AutoAlignDialog(QWidget *parent, int max) :
    QDialog(parent),
    ui(new Ui::AutoAlignDialog)
{
  from_validator = new QIntValidator(this);
  to_validator = new QIntValidator(this);
  ui->setupUi(this);
  from_validator->setBottom(1);
  from_validator->setTop(max);
  to_validator->setBottom(1);
  to_validator->setTop(max);
  ui->edit_from->setValidator(from_validator);
  ui->edit_to->setValidator(to_validator);
  connect(ui->edit_from, SIGNAL(textChanged(QString)), this, SLOT(fromChanged()));
  connect(ui->edit_to, SIGNAL(textChanged(QString)), this, SLOT(toChanged()));
  connect(ui->selector_aligner, SIGNAL(currentIndexChanged(int)), this, SLOT(alignerChanged(int)));
  adjustSize();
}

AutoAlignDialog::~AutoAlignDialog()
{
  delete ui;
  delete from_validator;
  delete to_validator;
}

void AutoAlignDialog::addAligner(QString name, QStringList a_profiles)
{
  ui->selector_aligner->addItem(name);
  profiles.append(a_profiles);
  alignerChanged(ui->selector_aligner->currentIndex());
}

void AutoAlignDialog::setStartPos(int pos)
{
  ui->edit_from->setText(QString::number(pos));
  fromChanged();
}

void AutoAlignDialog::setEndPos(int pos)
{
  ui->edit_to->setText(QString::number(pos));
}

int AutoAlignDialog::getStartPos()
{
  return ui->edit_from->text().toInt();
}

int AutoAlignDialog::getEndPos()
{
  return ui->edit_to->text().toInt();
}

int AutoAlignDialog::getAligner()
{
  return ui->selector_aligner->currentIndex();
}

int AutoAlignDialog::getProfile()
{
  return ui->selector_profile->currentIndex();
}

void AutoAlignDialog::fromChanged()
{
  to_validator->setBottom(getStartPos());
  checkValidity();
}

void AutoAlignDialog::toChanged()
{
  from_validator->setTop(getEndPos());
  checkValidity();
}

void AutoAlignDialog::checkValidity()
{
  if (getStartPos()>getEndPos())
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
  else
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void AutoAlignDialog::alignerChanged(int al)
{
  ui->selector_profile->clear();
  ui->selector_profile->addItems(profiles.value(al,QStringList()));
}

bool AutoAlignDialog::getAutoClose()
{
  return (ui->autoCloseSel->checkState()!=Qt::Checked);
}
