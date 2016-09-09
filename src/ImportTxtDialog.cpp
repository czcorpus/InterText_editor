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

#include "ImportTxtDialog.h"
#include "ui_ImportTxtDialog.h"
#include <QInputDialog>
#include <QDomDocument>
#include <QMessageBox>

ImportTxtDialog::ImportTxtDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImportTxtDialog)
{
    ui->setupUi(this);

    foreach (QByteArray c, QTextCodec::availableCodecs()) {
        encodings << QString::fromLatin1(c);
    }
    encodings.sort();
    ui->encSel->insertItems(0, encodings);
    setEncoding("UTF-8");
    connect(ui->editHeaderButton, SIGNAL(clicked(bool)), this, SLOT(editXmlHeader()));
    connect(ui->editFooterButton, SIGNAL(clicked(bool)), this, SLOT(editXmlFooter()));

    //adjustSize(); // makes it tall and narrow :-/
}

ImportTxtDialog::~ImportTxtDialog()
{
    delete ui;
}

void ImportTxtDialog::setHeaderFooterModeOnly(bool set)
{
    if (set) {
        ui->parBox->hide();
        ui->sentBox->hide();
        adjustSize();
    }
}

void ImportTxtDialog::setXmlHeader(QString text)
{
    importXmlHeader = text;
}

QString ImportTxtDialog::getXmlHeader()
{
    return importXmlHeader;
}

void ImportTxtDialog::setParSep(QString sep)
{
    QStringList items;
    items << tr("line break") << tr("empty line");
    int set;
    if (sep=="\n")
        set = 0;
    else if (sep=="\n\n")
        set = 1;
    else {
        items << sep;
        set = 2;
    }
    ui->parSepBox->addItems(items);
    ui->parSepBox->setCurrentIndex(set);
}

QString ImportTxtDialog::getParSep()
{
    QString ret = ui->parSepBox->currentText();
    if (ret==tr("line break"))
        return "\n";
    else if (ret==tr("empty line"))
        return "\n\n";
    else
        return ret;
}

void ImportTxtDialog::setParEl(QString name)
{
    ui->parElEdit->setText(name);
}

QString ImportTxtDialog::getParEl()
{
    return ui->parElEdit->text();
}

void ImportTxtDialog::setSplit(bool set)
{
    ui->sel_autoSep->setChecked(set);
}

bool ImportTxtDialog::getSplit()
{
    return ui->sel_autoSep->isChecked();
}

void ImportTxtDialog::setSentEl(QString name)
{
    ui->sentElEdit->setText(name);
}

QString ImportTxtDialog::getSentEl()
{
    return ui->sentElEdit->text();
}

void ImportTxtDialog::setSentSep(QString sep)
{
    QStringList items;
    items << tr("line break") << tr("empty line");
    int set;
    if (sep=="\n")
        set = 0;
    else if (sep=="\n\n")
        set = 1;
    else {
        items << sep;
        set = 2;
    }
    ui->sentSepEdit->addItems(items);
    ui->sentSepEdit->setCurrentIndex(set);
}

QString ImportTxtDialog::getSentSep()
{
    QString ret = ui->sentSepEdit->currentText();
    if (ret==tr("line break"))
        return "\n";
    else if (ret==tr("empty line"))
        return "\n\n";
    else
        return ret;
}

void ImportTxtDialog::setSplitProfiles(QStringList list)
{
    ui->profileBox->insertItems(0, list);
}

int ImportTxtDialog::getSplitProfile()
{
    return ui->profileBox->currentIndex();
}

void ImportTxtDialog::setXmlFooter(QString text)
{
    importXmlFooter = text;
}

QString ImportTxtDialog::getXmlFooter()
{
    return importXmlFooter;
}

void ImportTxtDialog::setKeepMarkup(bool set)
{
    ui->keepMarkup->setChecked(set);
}

bool ImportTxtDialog::getKeepMarkup()
{
    return ui->keepMarkup->isChecked();
}

void ImportTxtDialog::setEncoding(QString text)
{
    ui->encSel->setCurrentIndex(encodings.indexOf(text));
}

QString ImportTxtDialog::getEncoding()
{
    return ui->encSel->itemText(ui->encSel->currentIndex());
}


bool ImportTxtDialog::dontAsk()
{
    return ui->dontAsk->isChecked();
}
void ImportTxtDialog::editXmlHeader()
{
    bool ok;
    QString text = importXmlHeader;
    do {
        text = QInputDialog::getMultiLineText(this, tr("XML template"), tr("Document header"), text, &ok);
        if (!ok || text.isEmpty()) {
            return;
        }
    } while (!checkXml(text + importXmlFooter));
    importXmlHeader = text;
}

void ImportTxtDialog::editXmlFooter()
{
    bool ok;
    QString text = importXmlFooter;
    do {
        text = QInputDialog::getMultiLineText(this, tr("XML template"), tr("Document footer"), text, &ok);
        if (!ok || text.isEmpty()) {
            return;
        }
    } while (!checkXml(importXmlHeader + text, importXmlHeader.count("\n")));
    importXmlFooter = text;

}

bool ImportTxtDialog::checkXml(QString xml, int linedec)
{
    QDomDocument doc;
    QString errorMsg; int errorLine, errorColumn;
    if (!doc.setContent(xml, true, &errorMsg, &errorLine, &errorColumn)) {
        QString errorMessage = QObject::tr("Error parsing XML at line %1, column %2: %3.").arg(QString::number(errorLine-linedec), QString::number(errorColumn), errorMsg);
        QMessageBox::critical(this, tr("XML validation"), tr("Error: ").append(errorMessage));
        return false;
    }
    return true;
}
