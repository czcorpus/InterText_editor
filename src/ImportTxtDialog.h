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

#ifndef IMPORTTXTDIALOG_H
#define IMPORTTXTDIALOG_H

#include <QDialog>
#include <QTextCodec>

namespace Ui {
class ImportTxtDialog;
}

class ImportTxtDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportTxtDialog(QWidget *parent = 0);
    ~ImportTxtDialog();
    void setHeaderFooterModeOnly(bool set = true);
    void setEncoding(QString text);
    QString getEncoding();
    void setXmlHeader(QString text);
    QString getXmlHeader();
    void setParSep(QString sep);
    QString getParSep();
    void setParEl(QString name);
    QString getParEl();
    void setSplit(bool set);
    bool getSplit();
    void setSentEl(QString name);
    QString getSentEl();
    void setSentSep(QString sep);
    QString getSentSep();
    void setSplitProfiles(QStringList list);
    int getSplitProfile();
    void setXmlFooter(QString text);
    QString getXmlFooter();
    void setKeepMarkup(bool set);
    bool getKeepMarkup();
    bool dontAsk();

private:
    Ui::ImportTxtDialog *ui;
    QStringList encodings;
    QString importXmlHeader, importXmlFooter;
    bool checkXml(QString xml, int linedec = 0);

private slots:
    void editXmlHeader();
    void editXmlFooter();
};

#endif // IMPORTTXTDIALOG_H
