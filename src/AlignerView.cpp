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

#include "AlignerView.h"
#include "ui_AlignerView.h"
#include <QMessageBox>
#include <QFile>
#include <QScrollBar>

AlignerView::AlignerView(QString cmd, QString params, QString &fname1, QString &fname2, int fromPos, int toPos, bool autoClose, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AlignerView)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    close_me = autoClose;
    userAbort = false;
    filename1 = fname1;
    filename2 = fname2;
    startPos = fromPos;
    endPos = toPos;
    command = cmd;
    args = params;//.split(" ");
    my_proc = new QProcess(this);
    connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonPressed(QAbstractButton*)));
    my_proc->setReadChannel(QProcess::StandardError);
    connect(my_proc, SIGNAL(readyRead()), this, SLOT(readProc()));
    connect(my_proc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(proc_finished(int,QProcess::ExitStatus)));
}

void AlignerView::run_aligner()
{
    show();
    my_proc->start(QString("\"%1\" %2").arg(command, args));
    if (!my_proc->waitForStarted()) {
      QMessageBox::critical(this, tr("Automatic alignment"), tr("Error: Cannot execute external aligner (%1). Please, check your settings.").arg(command.split(" ").first()));
      QFile::remove(filename1);
      QFile::remove(filename2);
      close();
    }
}

AlignerView::~AlignerView()
{
  delete my_proc;
  delete ui;
}

void AlignerView::readProc()
{
  QTextCursor c = ui->textView->textCursor();
  c.insertText(my_proc->readAll());
  QScrollBar * sb = ui->textView->verticalScrollBar();
  sb->setValue(sb->maximum());
}

void AlignerView::buttonPressed(QAbstractButton * button)
{
  if (ui->buttonBox->standardButton(button)==QDialogButtonBox::Abort) {
    bool tempClose = close_me;
    close_me = false;
    QMessageBox::StandardButton resp = QMessageBox::question(this, tr("Automatic alignment"), tr("Do you really want to cancel the automatic alignment?"),
                                                             QMessageBox::Ok | QMessageBox::Cancel);
    if (resp==QMessageBox::Ok) {
      userAbort = true;
      my_proc->terminate();
      if (!my_proc->waitForFinished(10000))
        my_proc->kill();
    }
    close_me = tempClose;
  }
}

void AlignerView::proc_finished(int ret, QProcess::ExitStatus stat)
{
  QFile::remove(filename1);
  QFile::remove(filename2);

  if (!userAbort) {
    if (stat!=QProcess::NormalExit) {
      close_me = false;
      QMessageBox::critical(this, tr("Automatic alignment"), tr("Error: External aligner failed."));
    } else {
      emit result(my_proc->readAllStandardOutput(), startPos, endPos);
    }
  }

  if (close_me)
    close();
  else {
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Close);
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(close()));
  }
}
