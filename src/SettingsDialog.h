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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include "ItWindow.h"
#include "AlignerProfileDialog.h"

#define DEFAULT_FGCOLOR_DEFAULT "#000"
#define DEFAULT_BGCOLOR_DEFAULT "#fff"
#define DEFAULT_BGCOLOR_NON11 "#ff8"
#define DEFAULT_BGCOLOR_MARKED "#f88"
#define DEFAULT_EVENROW_DARKENING 8
#define DEFAULT_BGCOLOR_CURSOR "#33f"
#define DEFAULT_CURSOR_OPACITY 80
#define DEFAULT_BGCOLOR_FOUND "#f96"
#define DEFAULT_BGCOLOR_REPL "#6f6"

#define DEFAULT_EDITOR_SAVEEXIT Qt::Key_F2
#define DEFAULT_EDITOR_DISCARDEXIT Qt::Key_Escape
#define DEFAULT_EDITOR_SAVENEXT Qt::Key_Tab
#define DEFAULT_EDITOR_SAVEPREV Qt::SHIFT+Qt::Key_Backtab
#define DEFAULT_EDITOR_SAVEINSERTNEXT Qt::CTRL+Qt::Key_Tab

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(ItWindow *parent);
    ~SettingsDialog();
    QString strescape(QString str);
    QString strunescape(QString str);
public slots:
    void accept();
    void setTab(int n);

private:
    Ui::SettingsDialog *ui;
    ItWindow * window;
    QList<Aligner> aligners;
    QFont textfont;
    int curAligner, curSProfile, curServer, curExpProfile;
    QList<ItSentenceSplitter::SplitterProfile> sprofiles;
    QColor cDefault, cHighltd, cMarked, cfDefault, cCursor, cFound, cRepl;
    int cAddDark, cursorOpac;
    QMap<QString, ItServer> servers;
    QMap<QString, ExTextProfile> exTextProfiles;
    QStringList encodings;
    QList<Replacement> transformations;

    QString importXmlHeader, importXmlFooter;
    QString emptyDocTemplate;
    bool checkXml(QString xml, int linedec = 0);

private slots:
    void changeTextFont();
    void showTextFont();
    void apply();
    void showAligner(int n);
    void showSProfile(int n);
    void setAlExec(QString str);
    void setAlTmpdir(QString str);
    void setAlExHead(QString str);
    void setAlExFoot(QString str);
    void setAlExParSep(QString str);
    void setAlExElSep(QString str);
    void setAlImpRes(int n);
    void selectAlExec();
    void selectAlTmpDir();
    void newAligner();
    void delAligner();
    void renameAligner();
    void newAlProfile();
    void editAlProfile();
    void delAlProfile();
    void curProfileChanged(int n);
    void editSAbbrev();
    void editSRule();
    void updateSRule(int rule, int prop);
    void updateSAbbrev(QListWidgetItem * item);
    void newSProfile();
    void delSProfile();
    void renameSProfile();
    void newSRule();
    void delSRule();
    void newSAbbrev();
    void delSAbbrev();
    void curSAbbrevChanged(int n);
    void currentSRuleChanged(int row, int col, int prow, int pcol);
    void buttonClicked(QAbstractButton * button);
    void moveupSRule();
    void movedownSRule();
    void changeCFDefault();
    void changeCDefault();
    void changeCNon11();
    void changeCMarked();
    void changeCCursor();
    void changeCFound();
    void changeCRepl();
    void setDefaultColors();
    void showColors();
    void cDarkerChanged(int val);
    void cursorOpacChanged(int val);
    void newServer();
    void delServer();
    void renameServer();
    void updateServerList();
    void curServerChanged(int n);
    void setServerUrl(QString url);
    void setServerUsername(QString str);
    void setServerPasswd(QString str);
    void changeAutoUpdateCheck(int state);
    void newExpProf();
    void delExpProf();
    void renameExpProf();
    void setExpProfHeader(QString str);
    void setExpProfFooter(QString str);
    void setExpProfElStart(QString str);
    void setExpProfElEnd(QString str);
    void setExpProfElSep(QString str);
    void setExpProfSegStart(QString str);
    void setExpProfSegEnd(QString str);
    void setExpProfEmptySegFiller(QString str);
    void setExpProfSegSep(QString str);
    void setExpProfParSep(QString str);
    void setExpProfVerSep(QString str);
    void setExpProfExt(QString str);
    void changeExpProfKeepBoth(int state);
    void changeExpProfKeepPar(int state);
    void changeExpProfKeepSeg(int state);
    void changeExpProfSkipEmpty(int state);
    void changeExpProfSkipUnconf(int state);
    void changeExpProfEncoding(int n);
    void currentExpProfReplChanged(int row, int col, int prow, int pcol);
    void updateExpProfRepl(int rule, int prop);
    void newExpProfReplRule();
    void editExpProfReplRule();
    void delExpProfReplRule();
    void moveupExpProfReplRule();
    void movedownExpProfReplRule();
    void currentExpProfCustVarChanged(int row, int col, int prow, int pcol);
    void updateExpProfCustVar(int rule, int prop);
    void newExpProfCustVar();
    void editExpProfCustVar();
    void delExpProfCustVar();

    void updateExpProfList();
    void curExpProfChanged(int n);

    void refreshTransrules();
    void editTransRule();
    void updateTransRule(int rule, int prop);
    void newTransRule();
    void delTransRule();
    void currentTransRuleChanged(int row, int col, int prow, int pcol);
    void moveupTransRule();
    void movedownTransRule();

    void editEmptyDocTemplate();
    void editXmlHeader();
    void editXmlFooter();
};

#endif // SETTINGSDIALOG_H
