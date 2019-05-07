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

#ifndef IT_WINDOW_H
#define IT_WINDOW_H

#include <QtWidgets>
#include "ItCommon.h"
#include "ItSearchBar.h"
#include "ItAlignmentModel.h"
#include "ItAlignmentView.h"
#include "ItAlignmentDelegate.h"
#include "NewAlignmentDialog.h"
#include "numberingdialog.h"
#include "AutoAlignDialog.h"
#include "AlignerView.h"
#include "ImportXmlDialog.h"
#include "ImportTxtDialog.h"
#include "ItSentenceSplitter.h"
#include "AlignmentManager.h"
#include "ServerDialog.h"
#include "simplecrypt.h"
#include "AlignmentAttrDialog.h"
//#include "xmltreedialog.h"
// #include "ItThreads.h"

#ifdef Q_OS_MAC
#define UPDATER_BINARY "../../../PackageManager.app/Contents/MacOS/PackageManager"
#else
#define UPDATER_BINARY "PackageManager"
#endif
enum AlignerImportMethod { PlainAligner = 0, HunalignLadder };
enum TBSize { Hidden = 0, Tiny, Small, Medium, Large, XL, XXL, XXXL, UL };
Q_DECLARE_METATYPE(AlignerImportMethod)
Q_DECLARE_METATYPE(AutoState)

struct AlignerProfile
{
    QString name;
    QString params;
};

struct Aligner
{
    QString name;
    QString exec;
    QString tmpdir;
    QString exp_head;
    //QString exp_seg_sep;
    QString exp_el_sep;
    QString exp_parent_sep;
    QString exp_foot;
    AlignerImportMethod al_imp_method;
    QList<AlignerProfile> profiles;
};

struct ItServer
{
    QString name;
    QString url;
    QString username;
    QString passwd;
    bool autoUpdateCheck;
};

struct CustomVar
{
    QString custSymbol;
    QString desc;
    QString defaultVal;
};

struct ExTextProfile
{
    QString name;
    QString header;
    QString footer;
    QString elSep;
    QString elStart;
    QString elEnd;
    QString parSep;
    QString segSep;
    QString segStart;
    QString segEnd;
    QString verSep;
    QString ext;
    QString encoding;
    QString emptySegFiller;
    QList<Replacement> replacements;
    QList<CustomVar> customVars;
    bool keepPars;
    bool bothTexts;
    bool keepSegs;
    bool skipEmptySeg;
    bool skipUnconfirmed;
};

class ItWindow : public QMainWindow
{
    Q_OBJECT
    Q_ENUMS(AlignerImportMethod)

public:
    ItWindow();
    ~ItWindow();
    QString storagePath;
    QString defaultIdNamespaceURI;
    QList<Aligner> autoAligners;
    ItSentenceSplitter splitter;
    ItAlignmentView *view;
    ItAlignmentModel *model;
    Colors colors;
    QList<Replacement> transformations;
    EditorKeys editorKeys;
    QTimer autoSaveTimer;
    QMap<QString, ItServer> servers;
    QMap<QString, ExTextProfile> exTextProfiles;
    QString alTitleFormat;
    bool syncMarkChanges;
    bool autoCheckUpdates;
    bool enableCrossOrderAlignment;
    TBSize toolBarSize;
    QToolBar *toolBar;
    QList<QAction*> allActions;
    QList<QAction*> toolBarActions;
    QList<QAction*> ctxmenuActions;
    QList<QAction*> contextMenuCurActions;
    QHash<QString, QString> defaultShortcuts;

    bool askOnTxtImport, askOnXmlImport;
    bool importXmlLock;
    QStringList alignableElements;
    QStringList textElements;
    QString splitterElName;
    QString workDir;
    bool splitSetTxt, splitSetXml, importKeepMarkup;
    int defaultNumberingLevels;
    QString importXmlHeader, importXmlFooter;
    QString emptyDocTemplate;
    QString importTxtEncoding;
    QString importParSeparator;
    QString importSentenceSeparator;

    QAction *mergeAct;
    QAction *moveUpAct;
    QAction *moveDownAct;
    QAction *moveBUpAct;
    QAction *moveBDownAct;
    QAction *moveTextAct;
    QAction *shiftAct;
    QAction *popAct;
    QAction *toggleMarkAct;
    QAction *toggleStatusAct;
    QAction *editAct;
    QAction *insertAct;
    QAction *splitParentAct;
    QAction *mergeParentAct;
    QAction *undoAct, *redoAct;
    QAction *confirmAct;
    QAction *swapAct;
    QAction *editXmlAct;


    int isDependent(QString textName, QString v1name, QString v2name); // 1=yes (call reload when finished!), 0=no (or it IS the current al.), -1=abort!
    bool maybeSave();
    QString strescape(QString str);
    QString strunescape(QString str);
    void setAutoSaveInterval(int minutes);
    int getAutoSaveInterval();
    QString getCss();

signals:
    void closing();

public slots:
    void open(const QString &name = QString(), bool skipsync = false);
    QStringList scanDataDir(const QString &dir);
    void createNewAlignment();
    void importFile();
    void dataChanged();
    void propertiesChanged();
    void updateActions();
    void setSegView(ItSegmentView * cursegview);
    void findNext();
    void findPrev();
    void replace();
    void replaceAll();
    void replFind();
    void setFilter();
    void receiveUpdateFailure(QModelIndex idx);
    void undo();
    void redo();
    void applyHunalignLadder(QString output, int fromPos, int toPos);
    void applyPlainAlign(int fromPos, int toPos);
    void setTextFont(QFont &font);
    void toggleHighlNon11();
    void toggleHighlMarked();
    void updateColors(Colors c);
    void setCss(QString css);
    void serverDialog(QString name);
    void updateServerMenu();
    void updateExTextMenu();
    void reloadAlignment(bool warn = true);
    void reloadAlignmentSilently();
    void updateInfoBar(QString msg = QString());
    void alignmentDeletedInRepo(QString alname);
    void alignmentChangedInRepo();
    void closeAlignment();
    void syncFinished();
    void checkForUpdates(bool forced = true);
    void updateResultsReceived(int ret, QProcess::ExitStatus stat);
    void forcedUpdateResultsReceived(int ret, QProcess::ExitStatus stat);
    void setTransformations(QList<Replacement> &trans);
    void openXMLTreeEd();
    void toggleHtmlView();
    void enableHtmlView(bool en);

protected:
    QSettings * settings;
    void closeEvent(QCloseEvent *event);
    void resizeEvent ( QResizeEvent * event );
    void readSettings();
    void writeSettings();
    /*void resizeEvent(QResizeEvent * event);*/

private:
    QFile lockfile;
    AlignmentManager * alManager;
    ItSegmentView * segview;
    SimpleCrypt crypto;
    struct searchQuery
    {
        QString str;
        ItSearchBar::searchType stype;
        int startpos;
    };
    searchQuery lastSearch;
    ItSearchBar * searchBar;
    int importFormat;
    QSignalMapper * serverMapper;
    QSignalMapper * exTextMapper;
    QList< QAction* > serverActions;
    QList< QAction* > exTextActions;
    QActionGroup * exTextActGroup;
    QLabel * infoBar;
    QProgressBar * progressBar;
    int autoSaveInterval;
    bool restartApp;
    //AutoState autoSaveElement;
    QString cssStyle;

    void setNewAlignment(ItAlignment * a);
    bool processImportFile(ItAlignment * a, aligned_doc d, QString filename, int format);
    bool checkNumbering(ItAlignment * a, aligned_doc doc, bool allowLock = true, int levels = -1);
    bool newSearchQuery(uint startpos = INVALID_POSITION, bool force = false);
    bool setAlignmentNames(ItAlignment * a);

    void createActions();
    void createMenus();
    void createToolBar();
    void enableActions(bool en = true);
    //void createToolBars();
    void createStatusBar();
    //void readSettings();
    //void writeSettings();
    bool saveFile(const QString &fileName);
    //void setCurrentFile(const QString &fileName);
    //QString strippedName(const QString &fullFileName);
    void search_wrapper(bool forward, bool fromzero = false, bool silent = false);
    void find(uint startpos, bool forward, ItSearchBar::searchType stype, ItSearchBar::searchSide = ItSearchBar::Both, QString str = QString(), bool silent = false);
    //QPlainTextEdit *textEdit;
    //QString curFile;
    bool getServerConfig(QString url, ItServer * s);
    void checkForServerUpdates(ItAlignment * a, int d = 0);
    bool syncDoc(ItAlignment * a, ServerDialog *sd, aligned_doc d);

    bool generateCrossAlignment(ItAlignment::alignmentInfo *newinfo);

    bool crossOrderAlignmentAllowed();

    QIcon composeIcon(QString fname);

    QString extractText(ExTextProfile &prof, aligned_doc version, QStringList &customValues);
    QString extractTextSegment(ExTextProfile &prof, aligned_doc version, int seg, bool skipFirstParBreak, int *elCnt, int *pElCnt, int *parCnt, int segCnt, QStringList &customValues);
    QStringList customVarsDialog(ExTextProfile &prof);
    void replaceCustomVars(QString * str, ExTextProfile &prof, QStringList &values);

    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *searchMenu;
    QMenu *setMenu;
    QMenu *helpMenu;
    QMenu *serverMenu;
    QMenu *exTextMenu;
    QMenu *toolBarMenu;
    QMenu *ctrlMenu;

    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *closeAct;
    QAction *fimportAct;
    QAction *fexportAct;
    QAction *alPropAct;
    QAction *syncAct;
    QAction *exitAct;
    QAction *alManAct;

    QAction *aboutAct;
    QAction *aboutQtAct;
    QAction *checkUpdatesAct;

    QAction *autoAlignAct;

    QAction *updateStatAct;
    QAction *htmlViewAct;
    QAction *highlNon11Act;
    QAction *highlMarkedAct;
    QAction *customizeAct;
    QAction *settingsAct;

    QAction * findAct;
    QAction * findNextAct;
    QAction * findPrevAct;
    QAction * replaceAct;
    QAction * findNextBmAct;
    QAction * findPrevBmAct;
    QAction * findNextNon11Act;
    QAction * findPrevNon11Act;
    QAction * findNextUnconfAct;
    QAction * findPrevUnconfAct;

    QAction * findBmAct;
    QAction * findNon11Act;
    QAction * findUnconfAct;

    QAction * nsAct;

    QActionGroup * toolBarSizeGroup;
    QAction * hiddenTB;
    QAction * tinyTB;
    QAction * smallTB;
    QAction * mediumTB;
    QAction * largeTB;
    QAction * xlTB;
    QAction * xxlTB;
    QAction * xxxlTB;
    QAction * ulTB;

    QActionGroup * ctrlGroup;
    QAction * hiddenCtrlAct;
    QAction * onmoveCtrlAct;
    QAction * onclickCtrlAct;
    QActionGroup * ctrlSizeGroup;
    QAction * tinyCtrlAct;
    QAction * smallCtrlAct;
    QAction * mediumCtrlAct;
    QAction * largeCtrlAct;
    QAction * xlCtrlAct;
    QAction * xxlCtrlAct;
    QAction * xxxlCtrlAct;
    QAction * ulCtrlAct;

    Qt::ToolBarArea toolBarLocation;

private slots:
    //void newFile();
    bool save();
    void sync();
    bool exportFile();
    void about();
    void enableEditAct(bool en = true);
    void toggleAutoUpdateStatus();
    void findFirstBookmark();
    void findNextBookmark();
    void findPrevBookmark();
    void findFirstNon11();
    void findNextNon11();
    void findPrevNon11();
    void findFirstUnconfirmed();
    void findNextUnconfirmed();
    void findPrevUnconfirmed();
    void resetSearchResults();
    void autoAlign();
    void showAlMan();
    void customize();
    void editSettings();
    void merge();
    void alPropEdit();
    void newServerConnection();
    void hiddenTBar();
    void tinyTBar();
    void smallTBar();
    void mediumTBar();
    void largeTBar();
    void xlTBar();
    void xxlTBar();
    void xxxlTBar();
    void ulTBar();
    void hiddenCtrl();
    void onmoveCtrl();
    void onclickCtrl();
    void tinyCtrl();
    void smallCtrl();
    void mediumCtrl();
    void largeCtrl();
    void xlCtrl();
    void xxlCtrl();
    void xxxlCtrl();
    void ulCtrl();
    void setProgressBarRange(int min, int max);
    void setProgressBarValue(int value);
    void extractTextAndSave(QString profileName);
    void serverDocLastChangeMessage(QString tname, QString vname, QDateTime lastchange);
    void offerUpdates(bool autoCheck = false);
    void updateFinished(int ret, QProcess::ExitStatus stat);

    //void openEditor();
    //void documentWasModified();
};

#endif
