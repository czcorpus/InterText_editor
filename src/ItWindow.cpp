/*  Copyright (c) 2010-2017 Pavel Vondřička (Pavel.Vondricka@korpus.cz)
 *  Copyright (c) 2010-2017 Charles University in Prague, Faculty of Arts,
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

#include "ItWindow.h"
#include "SettingsDialog.h"
#include "ItCustomVarsDialog.h"
#include "CustomizeDialog.h"
#include "ItServerConn.h"

ItWindow::ItWindow() : QMainWindow()
{
    qRegisterMetaType<AlignerImportMethod>("AlignerImportMethod");
    qsrand(QDateTime::currentDateTime().toTime_t());
    crypto.setKey(Q_UINT64_C(0xfb6cfc42d17bb211));
    restartApp = false;
    autoCheckUpdates = true;
    enableCrossOrderAlignment = false;
    htmlViewAct = 0;
    defaultIdNamespaceURI = "";
    searchBar = new ItSearchBar(this);

    storagePath = QStandardPaths::standardLocations(QStandardPaths::DataLocation).at(0);
    //qDebug()<<QStandardPaths::standardLocations(QStandardPaths::DataLocation);
    workDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
    //qDebug()<<QStandardPaths::standardLocations(QStandardPaths::HomeLocation);

    if (!QDir(storagePath).exists())
        QDir().mkdir(storagePath);

    lockfile.setFileName(storagePath+"/.lock");
    if (lockfile.exists()) {
        QMessageBox::StandardButton resp = QMessageBox::question(this, tr("Repository locked"),
                                                                 tr("Another instance of InterText editor seems to be running (or the last one exited unexpectedly). Running multiple instances of this application simultaneously may damage your data. Are you sure you want to continue?"),
                                                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (resp == QMessageBox::No) {
            exit(0);
        }
    }

    lockfile.open(QIODevice::WriteOnly);
    lockfile.close();
    askOnTxtImport = true;
    askOnXmlImport = true;
    importXmlLock = false;
    infoBar = new QLabel();
    toolBarSize = Small;
    toolBarLocation = Qt::TopToolBarArea;
    serverMapper = 0;
    exTextMapper = 0;
    model = 0;
    segview = 0;
    nsAct = 0;
    alTitleFormat = "%1 (%2 - %3)";
    alignableElements << "head" << "s" << "verse";
    textElements << "p";
    splitterElName = "s";
    splitSetTxt = false;
    splitSetXml = false;
    defaultNumberingLevels = 2;
    importKeepMarkup = false;
    importTxtEncoding = "UTF-8";
    importXmlHeader = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<text>\n";
    importXmlFooter = "</text>\n";
    emptyDocTemplate = importXmlHeader + "<p id=\"1\">\n<s id=\"1:1\">Empty document template.</s>\n</p>\n" + importXmlFooter;
    importParSeparator = "\n\n";
    importSentenceSeparator = "\n";
    importFormat = 0;
    colors.fgdefault = QColor(DEFAULT_FGCOLOR_DEFAULT);
    colors.bgdefault = QColor(DEFAULT_BGCOLOR_DEFAULT);
    colors.bgnon11 = QColor(DEFAULT_BGCOLOR_NON11);
    colors.bgmarked = QColor(DEFAULT_BGCOLOR_MARKED);
    colors.bgAddDark = DEFAULT_EVENROW_DARKENING;
    colors.cursor = QColor(DEFAULT_BGCOLOR_CURSOR);
    colors.cursorOpac = DEFAULT_CURSOR_OPACITY;
    colors.bgfound = QColor(DEFAULT_BGCOLOR_FOUND);
    colors.bgrepl = QColor(DEFAULT_BGCOLOR_REPL);
    editorKeys.saveExit = QKeySequence(DEFAULT_EDITOR_SAVEEXIT);
    editorKeys.discardExit = QKeySequence(DEFAULT_EDITOR_DISCARDEXIT);
    editorKeys.saveNext = QKeySequence(DEFAULT_EDITOR_SAVENEXT);
    editorKeys.savePrev = QKeySequence(DEFAULT_EDITOR_SAVEPREV);
    editorKeys.saveInsertNext = QKeySequence(DEFAULT_EDITOR_SAVEINSERTNEXT);
    syncMarkChanges = true;
    cssStyle = "";
    setUnifiedTitleAndToolBarOnMac(true);

    view = new ItAlignmentView(this);
    view->hide();

    exTextActGroup = new QActionGroup(this);

    createActions();
    enableActions(false);
    settings = new QSettings("InterText");
    createMenus();
    progressBar = new QProgressBar();
    createStatusBar();

    connect(view, SIGNAL(editingStarted()), this, SLOT(updateActions()));
    connect(view, SIGNAL(editingFinished()), this, SLOT(updateActions()));
    connect(view, SIGNAL(cursorChanged()), this, SLOT(updateActions()));
    connect(view, SIGNAL(focusChanged()), this, SLOT(updateActions()));
    connect(view, SIGNAL(segViewChanged(ItSegmentView*)), this, SLOT(setSegView(ItSegmentView*)));
    connect(searchBar, SIGNAL(hiding()), this, SLOT(resetSearchResults()));

    QWidget *centralWidget = new QWidget;
    centralWidget->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(view);
    layout->addWidget(searchBar);
    centralWidget->setLayout(layout);

    setCentralWidget(centralWidget);

    setWindowTitle(QObject::tr("InterText"));
    //view->setFont(QFont("Times New Roman",12));
    alManager = new AlignmentManager(storagePath, this);
    connect(alManager, SIGNAL(alDeletedInRepo(QString)), this, SLOT(alignmentDeletedInRepo(QString)));
    show();
    readSettings();
    view->setEditorKeys(editorKeys);
    if (autoCheckUpdates)
        checkForUpdates(false);
    //statusBar()->showMessage(tr("Ready"), 2000);
}

ItWindow::~ItWindow() {
    delete exTextActGroup;
    delete settings;
    //delete model;
    //delete view;
    delete alManager;
    delete progressBar;
    //delete toolBar;
    lockfile.remove();
}

void ItWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        writeSettings();
        event->accept();
        if (restartApp)
            QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
    } else {
        event->ignore();
    }
}

void ItWindow::open(const QString &name, bool skipsync)
{
    if (!maybeSave())
        return;

    QString selname;
    if (name.isEmpty()) {
        alManager->show(true);
        return;
        /*bool ok;
    QStringList list = scanDataDir(storagePath);
    if (list.empty()) {
      QMessageBox::warning(this, tr("Open alignment"), tr("No stored alignments. Import or create some first."));
      return;
    }
    QString aname;
    QStringList titles, parts;
    foreach (aname, list) {
      parts = aname.split('.');
      titles.append(alTitleFormat.arg(parts[0], parts[1], parts[2]));
    }

    QString seltitle = QInputDialog::getItem(this, tr("Open alignment"), tr("Select alignment:"), titles, 0, false, &ok);
    selname = list.at(titles.indexOf(seltitle));
    if (!ok || selname.isEmpty()) return;*/
    } else selname = name;

#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif

    //if (model->alignment != 0) delete model->alignment;
    statusBar()->showMessage(tr("Opening alignment..."),0);
    ItAlignment * a = new ItAlignment(storagePath, selname, defaultIdNamespaceURI);

#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif

    if (!a->errorMessage.isEmpty()) {
        statusBar()->showMessage(tr("Opening alignment failed."), 5000);
        QMessageBox::warning(this, tr("Failure"), a->errorMessage);
        delete a;
    } else {
        setNewAlignment(a);
        if (a->info.source.startsWith("http"))
            syncAct->setEnabled(true);
        else
            syncAct->setEnabled(false);
        statusBar()->showMessage(tr("Alignment loaded"), 1000);
        if (!skipsync)
            checkForServerUpdates(a);
        updateInfoBar();
    }

}

void ItWindow::setNewAlignment(ItAlignment * a)
{
    ItAlignmentModel *oldmodel = model;
    model = new ItAlignmentModel(a);
    model->setColors(colors);
    model->setCSS(cssStyle);
    model->setTransformations(transformations);
    model->setHtmlViewMode(view->htmlView());
    if (a->info.autoUpdateStatus) {
        model->setUpdateStat(true);
        updateStatAct->setChecked(true);
    } else {
        model->setUpdateStat(false);
        updateStatAct->setChecked(false);
    }
    connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(dataChanged()));
    //connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(resetSearchResults()));
    view->setModel(model);
    if (oldmodel) {
        disconnect(oldmodel->undoStack, SIGNAL(cleanChanged(bool)), this, SLOT(dataChanged()));
        disconnect(oldmodel->undoStack, SIGNAL(canRedoChanged(bool)), this, SLOT(dataChanged()));
        disconnect(oldmodel->undoStack, SIGNAL(canUndoChanged(bool)), this, SLOT(dataChanged()));
        disconnect(&autoSaveTimer, SIGNAL(timeout()), this, SLOT(save()));
        delete oldmodel;
    }
    view->optimizeSize(statusBar());

    /*statusBar()->showMessage("Resizing...");
  view->hide();
  view->resizeRowsToContents();
  view->show();
  statusBar()->clearMessage();*/

    connect(model, SIGNAL(layoutChanged()), this, SLOT(updateActions()));
    //connect(model, SIGNAL(layoutChanged()), this, SLOT(updateInfoBar()));
    connect(model->undoStack, SIGNAL(cleanChanged(bool)), this, SLOT(dataChanged()));
    connect(model->undoStack, SIGNAL(canRedoChanged(bool)), this, SLOT(dataChanged()));
    connect(model->undoStack, SIGNAL(canUndoChanged(bool)), this, SLOT(dataChanged()));
    connect(model, SIGNAL(lastMatchChanged(QModelIndex)), view, SLOT(focusIndex(QModelIndex)));
    connect(model, SIGNAL(updateFailure(QModelIndex)), this, SLOT(receiveUpdateFailure(QModelIndex)));
    undoAct->setEnabled(true);
    redoAct->setEnabled(true);
    dataChanged();
    setWindowTitle(QObject::tr("%1 - InterText").arg(a->info.docId));
    enableActions(true);
    connect(&autoSaveTimer, SIGNAL(timeout()), this, SLOT(save()));
    view->setFocus();
}

void ItWindow::alignmentDeletedInRepo(QString alname) {
    if (model && model->alignment->info.name == alname)
        closeAlignment();
}

void ItWindow::alignmentChangedInRepo()
{

}

void ItWindow::closeAlignment() {
    if (model==0)
        return;

    if (!maybeSave())
        return;

    searchBar->hide();
    view->setModel(0);
    ItAlignmentModel *oldmodel = model;
    model = 0;
    disconnect(oldmodel->undoStack, SIGNAL(cleanChanged(bool)), this, SLOT(dataChanged()));
    disconnect(oldmodel->undoStack, SIGNAL(canRedoChanged(bool)), this, SLOT(dataChanged()));
    disconnect(oldmodel->undoStack, SIGNAL(canUndoChanged(bool)), this, SLOT(dataChanged()));
    disconnect(&autoSaveTimer, SIGNAL(timeout()), this, SLOT(save()));
    delete oldmodel;
    infoBar->setText("");
    enableActions(false);
    setWindowTitle(QObject::tr("InterText"));
}

bool ItWindow::save()
{
    if (model==0)
        return false;
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    statusBar()->showMessage(tr("Storing alignment..."));
    bool ret = model->save();
    statusBar()->showMessage(tr("Alignment stored"), 1000);
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif

    dataChanged();
    return ret;
}

void ItWindow::importFile() {
    if (maybeSave()) {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Import alignment"), workDir);
        QFileInfo fileinfo(fileName);
        workDir = fileinfo.absolutePath();
        if (fileName.isEmpty()) return;
        /*QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
      QMessageBox::warning(this, tr("Application"),
        tr("Cannot read file %1:\n%2.").arg(fileName).arg(file.errorString()));
      return;
    }*/

        ItAlignment * a = new ItAlignment(storagePath);
        a->setIdNamespaceURI(defaultIdNamespaceURI);
        QStringList secs = fileinfo.fileName().split(".");
        if (secs.count()>3) {
            a->info.docId = secs.at(0);
            a->info.ver[0].name = secs.at(1);
            a->info.ver[1].name = secs.at(2);
        } else if (secs.count()>1) {
            a->info.docId = secs.at(0);
            a->info.ver[0].name = QString("ver1");
            a->info.ver[1].name = QString("ver2");
        } else {
            a->info.docId = tr("Unknown document");
            a->info.ver[0].name = QString("ver1");
            a->info.ver[1].name = QString("ver2");
        }
        a->info.source = fileinfo.canonicalPath();

        if (!setAlignmentNames(a)) {
            delete a;
            return;
        }
        /*    NewAlignmentDialog * form = new NewAlignmentDialog(this, a);
    form->exec();
    if (!form->result()) { delete form; delete a; return; }
    while (a->alDocExists() && a->alVerExists(0) && a->alVerExists(1)) {
            QMessageBox::warning(this, tr("Import conflict"),
                tr("Such alignment already exists in the local storage. Change the names or cancel the import."));
      form->exec();
      if (!form->result()) { delete form; delete a; return; }
    }
    while (a->alVerExists(0)) {
            QMessageBox::StandardButton resp = QMessageBox::question(this, tr("Import conflict"),
        tr("Version '%1' of this document is already present in the system. Do you want to use it? "
        "(Otherwise you have to choose a different name.)").arg(a->info.ver[0].name),
                QMessageBox::Ok|QMessageBox::Cancel);
      if (resp != QMessageBox::Ok) {
        form->enableOnly(1);
        form->exec();
        if (!form->result()) { delete form; delete a; return; }
      } else break;
    }
    while (a->alVerExists(1)) {
            QMessageBox::StandardButton resp = QMessageBox::question(this, tr("Import conflict"),
        tr("Version '%1' of this document is already present in the system. Do you want to use it? "
          "(Otherwise you have to choose a different name.)").arg(a->info.ver[1].name),
                QMessageBox::Ok|QMessageBox::Cancel);
      if (resp != QMessageBox::Ok) {
        form->enableOnly(2);
        form->exec();
        if (!form->result()) { delete form; delete a; return; }
      } else break;
    }
    delete form;*/

#ifndef QT_NO_CURSOR
        QApplication::setOverrideCursor(Qt::BusyCursor);
#endif

        statusBar()->showMessage(tr("Loading alignment..."));
        if (!a->loadFile(fileName)) {
#ifndef QT_NO_CURSOR
            QApplication::restoreOverrideCursor();
#endif
            statusBar()->showMessage(tr("Import failed."), 5000);
            QMessageBox::warning(this, tr("Import failure"), a->errorMessage);
            delete a;
            statusBar()->clearMessage();
            return;
        }
        syncAct->setEnabled(false);
        statusBar()->showMessage(tr("Alignment imported"), 1000);


#ifndef QT_NO_CURSOR
        QApplication::restoreOverrideCursor();
#endif

        // restrict permissions for shared document(s) if appropriate
        a->syncDepsPermissions();
        /*a->info.ver[0].perm_chtext = a->canDepsChtext(0);
        a->info.ver[1].perm_chtext = a->canDepsChtext(1);
        a->info.ver[0].perm_chstruct = a->canDepsChstruct(0);
    a->info.ver[1].perm_chstruct = a->canDepsChstruct(1);*/

        if (!checkNumbering(a, 0))
            a->renumber(0);
        if (!checkNumbering(a, 1))
            a->renumber(1);

        /*if (a->info.ver[0].numLevels<1) {
      numberingDialog * form = new numberingDialog(this, a, 0);
      while (a->info.ver[0].numLevels<1) form->exec();
      delete form;
    }
    if (a->info.ver[1].numLevels<1) {
      numberingDialog * form = new numberingDialog(this, a, 1);
      while (a->info.ver[1].numLevels<1) form->exec();
      delete form;
    }*/

        setNewAlignment(a);

        /*ItAlignmentModel *oldmodel = model;
    model = new ItAlignmentModel(a);
    connect(model,SIGNAL(dataChanged(QModelIndex,QModelIndex)), this,SLOT(dataChanged()));
    view->setModel(model);
    view->optimizeSize(statusBar());
    delete oldmodel;
    connect(model, SIGNAL(layoutChanged()), this, SLOT(updateActions()));
    connect(model->undoStack, SIGNAL(cleanChanged(bool)), this, SLOT(dataChanged()));
    connect(model->undoStack, SIGNAL(canRedoChanged(bool)), this, SLOT(dataChanged()));
    connect(model->undoStack, SIGNAL(canUndoChanged(bool)), this, SLOT(dataChanged()));
    connect(model, SIGNAL(lastMatchChanged(QModelIndex)), view, SLOT(focusIndex(QModelIndex)));
    connect(model, SIGNAL(updateFailure(QModelIndex)), this, SLOT(receiveUpdateFailure(QModelIndex)));
        undoAct->setEnabled(true);
        redoAct->setEnabled(true);
    dataChanged();*/

        save();
    }
}

void ItWindow::createNewAlignment() {
    if (!maybeSave())
        return;

    ItAlignment * a = new ItAlignment(storagePath);
    a->setIdNamespaceURI(defaultIdNamespaceURI);
    if (!setAlignmentNames(a)) {
        delete a;
        return;
    }

    QString title [2];
    title[0] = tr("Import left side text");
    title[1] = tr("Import right side text");
    QStringList filters;
    filters << tr("XML document (*.xml)")
            << tr("Plain text file (*.*)")
            << tr("New-line aligned XML fragment (*.*)");

    // check whether such alignment can be generated from existing ones (cross-alignment)
    if (generateCrossAlignment(&a->info)) {
        open(a->info.name);
        delete a;
        return;
    }

    QString selFilter = filters.at(importFormat);
    bool noalign = false;
    for (int d=0; d<=1; d++) {
        if (a->info.ver[d].source.startsWith("http")) { // download text document from server
            ItServer s;
            QMapIterator<QString, ItServer> it(servers);
            while (it.hasNext()) {
                it.next();
                if (a->info.ver[d].source == it.value().url) {
                    s = it.value();
                    break;
                }
            }
            //ItServer s = servers.value(a->info.ver[d].source);
            //a->info.ver[d].source = s.url;
            ServerDialog * sd = new ServerDialog(this, storagePath, s.url, s.username, s.passwd, true);
            connect(sd, SIGNAL(alDeletedInRepo(QString)), this, SLOT(alignmentDeletedInRepo(QString)));
            connect(sd, SIGNAL(alRepoChanged()), alManager, SLOT(externalChange()));
            sd->connectToServer();
            if (!sd->docDownload(a, d)) {
                delete sd;
                delete a;
                return;
            }
            delete sd;
            if (!checkNumbering(a, d, false))
                a->renumber(d);
            a->syncDepsPermissions();
        } else if (a->loadDoc(d)) { // open existing text document from repository
            QStringList alignable = a->getAlignableElementnamesForDoc(d);
            if (alignable.isEmpty()) { // This should actually never happen!
                alignable = alignableElements;
            }
            a->createLinks(d, alignable);
            a->detectIdSystem(d);
            if (!checkNumbering(a, d, false))
                a->renumber(d);
        } else if (a->info.ver[d].source == "0") { // create new empty XML document
            a->info.ver[d].source = "";
            a->setDocXml(d, emptyDocTemplate);
            noalign = true;
            a->createLinks(d, alignableElements);
            a->detectIdSystem(d);
            if (!checkNumbering(a, d, false))
                a->renumber(d);
        } else { // import from file
            QString fileName = QFileDialog::getOpenFileName(this, title[d], workDir, filters.join(";;"), &selFilter);
            if (!fileName.isEmpty()) {
                workDir = QFileInfo(fileName).absolutePath();
                importFormat = filters.indexOf(selFilter);
            }
            if (fileName.isEmpty() || !processImportFile(a, d, fileName, importFormat)) {
                delete a;
                return;
            }
        }
    }

    if (!a->loadDependentAlignments()) {
        QMessageBox::critical(this, tr("Dependent alignment"), tr("Error: ").append(a->errorMessage));
        delete a;
        return;
    }
    a->syncDepsPermissions();
    setNewAlignment(a);
    syncAct->setEnabled(false);
    save();
    if (importFormat!=2 && !noalign)
        autoAlign();
}

bool ItWindow::generateCrossAlignment(ItAlignment::alignmentInfo *newinfo)
{
    QStringList parts, allist1, allist2;
    QString tname = newinfo->docId;
    QString v1name = newinfo->ver[0].name;
    QString v2name = newinfo->ver[1].name;
    newinfo->name = QString("%1.%2.%3").arg(tname, v1name, v2name);
    QDir dir(storagePath);
    QStringList list = dir.entryList(QStringList(QString("%1.*.conf").arg(tname)), QDir::Files, QDir::Name);
    for (int i=0; i<list.size(); i++) {
        parts = list.at(i).split(".");
        if (parts.at(1)==v1name && !allist1.contains(parts.at(2)))
            allist1.append(parts.at(2));
        else if (parts.at(2)==v1name && !allist1.contains(parts.at(1)))
            allist1.append(parts.at(1));
        else if (parts.at(1)==v2name && !allist2.contains(parts.at(2)))
            allist2.append(parts.at(2));
        else if (parts.at(2)==v2name && !allist2.contains(parts.at(1)))
            allist2.append(parts.at(1));
    }
    QString medVersion = "";
    for (int i=0; i<allist1.size(); i++) {
        if (allist2.contains(allist1.at(i))) {
            medVersion = allist1.at(i);
            break;
        }
    }
    if (medVersion.isEmpty()) {
        return false;
    }
    QMessageBox::StandardButton res = QMessageBox::question(this, tr("New alignment"),
                                                            tr("Such alignment can be automatically generated from existing alignments. Do you want to use this possibility?"),
                                                            QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes);
    if (res==QMessageBox::No)
        return false;
    //bool swap = false;
    aligned_doc baseDoc = 0;
    list = dir.entryList(QStringList(QString("%1.%2.%3.conf").arg(tname, v1name, medVersion)), QDir::Files, QDir::Name);
    if (list.size()==0) {
        list = dir.entryList(QStringList(QString("%1.%3.%2.conf").arg(tname, v1name, medVersion)), QDir::Files, QDir::Name);
        //swap = true;
        baseDoc = 1;
    }
    QString medAl = list.at(0);
    ItAlignment *a = new ItAlignment(storagePath, medAl.remove(list.at(0).length()-5, 5), defaultIdNamespaceURI);
    /*int depAl = a->getDepAlignment(medDoc, v2name);
    if (depAl<0) {
        qDebug()<<"Ooops! Something is wrong with generating cross-alignments!";
        return false;
    }*/
    if (!a->createCrossAlignment(baseDoc, v2name)) {
        QMessageBox::warning(this, tr("New alignment"), tr("Generating cross-alignment failed: %1").arg(a->errorMessage));
        delete a;
        return false;
    }
    delete a;
    return true;
}

bool ItWindow::processImportFile(ItAlignment * a, aligned_doc d, QString filename, int format)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Opening file"),
                             tr("Cannot read file %1:\n%2.").arg(filename).arg(file.errorString()));
        return false;
    }

    a->info.ver[d].source = filename;
    QString data;
    if (format==0) {
        /* XML */
        if (askOnXmlImport) {
            ImportXmlDialog * xmld = new ImportXmlDialog(this);
            if (splitSetXml)
                xmld->setSplitter();
            xmld->setAlElements(alignableElements);
            xmld->setTextElements(textElements);
            xmld->setProfiles(splitter.getProfileNames());
            xmld->setNewElName(splitterElName);
            xmld->exec();
            alignableElements = xmld->getAlElements();
            textElements = xmld->getTextElements();
            splitter.selectProfile(xmld->getProfile());
            splitterElName = xmld->getNewElName();
            splitSetXml = xmld->getSplitter();
            if (xmld->dontAsk())
                askOnXmlImport = false;
            delete xmld;
        }
        //data = QString::fromUtf8(file.readAll());
        QByteArray ba = file.readAll();
        if (!a->setDocXml(d, ba)) {
            QMessageBox::critical(this, tr("Import"), tr("Error: ").append(a->errorMessage));
            return false;
        }
        if (splitSetXml) {
            checkNumbering(a, d, false);
            a->applySentenceSplitter(d, &splitter, textElements, splitterElName);
            a->createLinks(d, QStringList(splitterElName));
            a->renumber(d);
        } else {
            a->createLinks(d, alignableElements);
            a->detectIdSystem(d);
            if (!checkNumbering(a, d, true))
                a->renumber(d);
        }
        return true;
    } else if (format==2) {
        /* XML fragment, newline-aligned */
        if (askOnXmlImport) {
            ImportXmlDialog * xmld = new ImportXmlDialog(this);
            xmld->setAlignableOnlyMode();
            xmld->setAlElements(alignableElements);
            xmld->exec();
            alignableElements = xmld->getAlElements();
            if (xmld->dontAsk())
                askOnXmlImport = false;
            delete xmld;
        }
        QString ae, line;
        int c;
        QList<int> groups;
        QTextCodec *codec = QTextCodec::codecForName(importTxtEncoding.toLatin1());
        if (!codec) {
            QMessageBox::critical(this, tr("Import"), tr("Unknown encoding '%1'.").arg(importTxtEncoding));
            return false;
        }
        data = codec->toUnicode(file.readAll());
        foreach (line, data.split("\n")) {
            c = 0;
            foreach (ae, alignableElements) {
                c += line.count(QRegExp(QString("<%1[ >]").arg(ae)));
            }
            groups << c;
        }
        if (askOnTxtImport) {
            ImportTxtDialog * txtd = new ImportTxtDialog(this);
            txtd->setHeaderFooterModeOnly();
            txtd->setXmlHeader(importXmlHeader);
            txtd->setXmlFooter(importXmlFooter);
            txtd->setEncoding(importTxtEncoding);
            txtd->exec();
            importTxtEncoding = txtd->getEncoding();
            importXmlHeader = txtd->getXmlHeader();
            importXmlFooter = txtd->getXmlFooter();
            if (txtd->dontAsk())
                askOnTxtImport = false;
            delete txtd;
        }
        data = QString("%1\n%2%3").arg(importXmlHeader, data, importXmlFooter);
        if (!a->setDocXml(d, data)) {
            QMessageBox::critical(this, tr("Import"), tr("Error: ").append(a->errorMessage));
            return false;
        }
        a->createLinks(d, alignableElements, groups);
        a->detectIdSystem(d);
        if (!checkNumbering(a, d, true))
            a->renumber(d);
        return true;
    } else {
        /* Plain text */
        if (askOnTxtImport) {
            ImportTxtDialog * txtd = new ImportTxtDialog(this);
            if (splitSetTxt)
                txtd->setSplit(true);
            txtd->setXmlHeader(importXmlHeader);
            txtd->setXmlFooter(importXmlFooter);
            txtd->setEncoding(importTxtEncoding);
            txtd->setParSep(importParSeparator);
            if (textElements.size())
                txtd->setParEl(textElements.at(0));
            else
                txtd->setParEl("");
            txtd->setSentSep(importSentenceSeparator);
            txtd->setSentEl(splitterElName);
            txtd->setSplitProfiles(splitter.getProfileNames());
            txtd->setKeepMarkup(importKeepMarkup);
            txtd->exec();
            importTxtEncoding = txtd->getEncoding();
            splitSetTxt = txtd->getSplit();
            importKeepMarkup = txtd->getKeepMarkup();
            splitterElName = txtd->getSentEl();
            splitter.selectProfile(txtd->getSplitProfile());
            importXmlHeader = txtd->getXmlHeader();
            importXmlFooter = txtd->getXmlFooter();
            importParSeparator = txtd->getParSep();
            int i = textElements.indexOf(txtd->getParEl());
            if (i == -1)
                textElements.insert(0,txtd->getParEl());
            else if (i != 0) {
                textElements.move(i, 0);
            }
            importSentenceSeparator = txtd->getSentSep();
            if (txtd->dontAsk())
                askOnTxtImport = false;
            delete txtd;
        }
        QTextCodec *codec = QTextCodec::codecForName(importTxtEncoding.toLatin1());
        if (!codec) {
            QMessageBox::critical(this, tr("Import"), tr("Unknown encoding '%1'.").arg(importTxtEncoding));
            return false;
        }
        data = codec->toUnicode(file.readAll());
        data = data.trimmed();
        if (!importKeepMarkup)
            data.replace("&","&amp;").replace(">","&gt;").replace("<","&lt;");
        if (splitSetTxt) {
            data.replace(importParSeparator, QString("</%1>\n<%1>").arg(textElements.at(0)));
            data.replace(QRegExp(QString("<%1>\\s*</%1>").arg(textElements.at(0))), ""); // delete empty containers
            data = QString("<%1>\n%2</%1>\n").arg(textElements.at(0), data);

            a->fixBrokenTags(data);

            data = QString("%1%2%3\n").arg(importXmlHeader, data, importXmlFooter);
            if (!a->setDocXml(d, data)) {
                QMessageBox::critical(this, tr("Import"), tr("Error: ").append(a->errorMessage));
                return false;
            }
            checkNumbering(a, d, false);
            a->applySentenceSplitter(d, &splitter, QStringList(textElements.at(0)), splitterElName);
            a->createLinks(d, QStringList(splitterElName));
            a->renumber(d);
        } else {
            data.replace(importParSeparator, QString("</%1></%2><%2><%1>").arg(splitterElName, textElements.at(0)));
            data.replace(importSentenceSeparator, QString("</%1><%1>").arg(splitterElName));
            data.replace(QRegExp(QString("<%1>\\s*</%1>").arg(splitterElName)), ""); // delete empty elements
            data.replace(QRegExp(QString("<%1>\\s*</%1>").arg(textElements.at(0))), ""); // delete empty containers
            data.replace(QString("</%1>").arg(splitterElName), QString("</%1>\n").arg(splitterElName));
            data.replace(QString("</%1>").arg(textElements.at(0)), QString("\n</%1>\n").arg(textElements.at(0)));
            data.replace(QString("<%1>").arg(textElements.at(0)), QString("<%1>\n").arg(textElements.at(0)));
            data = QString("%1<%2>\n<%5>%3</%5></%2>\n%4").arg(importXmlHeader, textElements.at(0), data, importXmlFooter, splitterElName);
            if (!a->setDocXml(d, data)) {
                QMessageBox::critical(this, tr("Import"), tr("Error: ").append(a->errorMessage));
                return false;
            }
            checkNumbering(a, d, false);
            a->createLinks(d, QStringList(splitterElName));
            a->renumber(d);
        }
        return true;
    }
}

bool ItWindow::checkNumbering(ItAlignment * a, aligned_doc doc, bool allowLock, int levels)
{
    if (levels<0)
        levels = defaultNumberingLevels;
    if (a->info.ver[doc].numLevels<1) {
        if (allowLock && askOnXmlImport && a->info.ver[doc].numLevels!=NO_IDS_IN_DOCUMENT) {
            // this is only relevant when importing unknown XML files with some unknown IDs
            numberingDialog * form = new numberingDialog(this, a, doc, allowLock);
            int xmllevels = levels;
            if (importXmlLock)
                xmllevels = 0;
            form->setDefaultLevels(xmllevels);
            //if (a->info.ver[doc].numLevels<1) {
            while (a->info.ver[doc].numLevels<1) form->exec();
            //a->renumber(doc);
            //}
            delete form;
        } else { // otherwise just use the defaults
            if (allowLock && (importXmlLock || levels==0)) {
                a->info.ver[doc].numLevels=1;
                a->info.ver[doc].perm_chstruct=false;
            } else if (levels==1) {
                a->info.ver[doc].numLevels=1;
            } else {
                a->info.ver[doc].numLevels=2;
                a->info.ver[doc].numPrefix=":";
            }
        }
        return false;
    }
    return true;
}

bool ItWindow::setAlignmentNames(ItAlignment * a)
{
    NewAlignmentDialog * form = new NewAlignmentDialog(this, a, alManager->listTexts(), alManager->listVersions());
    form->exec();
    if (!form->result()) { delete form; return false; }
    while (a->alExists()) {
        QMessageBox::warning(this, tr("Import conflict"),
                             tr("Such alignment already exists in the local storage. Change the names or cancel the import."));
        form->exec();
        if (!form->result()) { delete form; return false; }
    }
    while (a->alVerExists(0) && a->alVerExists(1)) {
        QMessageBox::StandardButton resp = QMessageBox::question(this, tr("Import conflict"),
                                                                 tr("Such text versions are already present in the repository. Do you want to use them? "
                                                                    "(Otherwise you have to choose different names.)"),
                                                                 QMessageBox::Ok|QMessageBox::Cancel);
        if (resp != QMessageBox::Ok) {
            //form->enableOnly(1);
            form->exec();
            if (!form->result()) { delete form; return false; }
        } else break;
    }
    while (a->alVerExists(0) && !a->alVerExists(1)) {
        QMessageBox::StandardButton resp = QMessageBox::question(this, tr("Import conflict"),
                                                                 tr("Version '%1' of this document is already present in the repository. Do you want to use it? "
                                                                    "(Otherwise you have to choose a different name.)").arg(a->info.ver[0].name),
                QMessageBox::Ok|QMessageBox::Cancel);
        if (resp != QMessageBox::Ok) {
            form->enableOnly(1);
            form->exec();
            if (!form->result()) { delete form; return false; }
        } else break;
    }
    while (a->alVerExists(1) && !a->alVerExists(0)) {
        QMessageBox::StandardButton resp = QMessageBox::question(this, tr("Import conflict"),
                                                                 tr("Version '%1' of this document is already present in the repository. Do you want to use it? "
                                                                    "(Otherwise you have to choose a different name.)").arg(a->info.ver[1].name),
                QMessageBox::Ok|QMessageBox::Cancel);
        if (resp != QMessageBox::Ok) {
            form->enableOnly(2);
            form->exec();
            if (!form->result()) { delete form; return false; }
        } else break;
    }
    delete form;
    return true;
}


bool ItWindow::exportFile() {
    if (model==0)
        return false;
    QString dest = QFileDialog::getExistingDirectory(this, tr("Save files into..."), workDir);
    if (dest.isEmpty())
        return false;
    workDir = QDir(dest).absolutePath();
    QString doc1target = model->alignment->info.ver[0].filename;
    QString doc2target = model->alignment->info.ver[1].filename;
    QString altarget = model->alignment->info.filename;
    QString doc1FileName = QFileInfo(workDir, doc1target).absoluteFilePath();
    QString doc2FileName = QFileInfo(workDir, doc2target).absoluteFilePath();
    QString alFileName = QFileInfo(workDir, altarget).absoluteFilePath();
    if (!model->alignment->saveFile(alFileName, doc1target, doc2target)) {
        QMessageBox::warning(this, tr("Saving alignment"), tr("Cannot save file '%1':\n%2.").arg(alFileName, model->alignment->errorMessage));
        return false;
    }
    if (!model->alignment->saveDoc(0, doc1FileName))
        QMessageBox::warning(this, tr("Saving first document"), tr("Cannot save file '%1':\n%2.").arg(doc1FileName, model->alignment->errorMessage));
    if (!model->alignment->saveDoc(1, doc2FileName))
        QMessageBox::warning(this, tr("Saving second document"), tr("Cannot save file '%1':\n%2.").arg(doc2FileName, model->alignment->errorMessage));
    return true;
}

void ItWindow::about()
{
    QMessageBox m;
    m.setWindowTitle(tr("About InterText"));
    m.setIconPixmap(QPixmap(":/images/32/InterText.ico"));
    m.setText(tr("<h1>InterText editor 1.6</h1>"));
    m.setInformativeText(tr("<p><b>Alignment editor for parallel texts.</b></p>"
                            "<p>Copyright &copy; 2010-2017 Pavel Vondřička,<br/>"
                            "Institute of the Czech National Corpus,<br/>"
                            "Charles University in Prague, Faculty of Arts</p>"
                            "<p>This software is licensed under the GNU General Public License (Version 3).</p>"));
    m.setDetailedText(tr("Homepage: http://wanthalf.saga.cz/intertext/\n"
                         "Github: https://github.com/czcorpus/InterText_editor\n"
                         "License: http://www.gnu.org/licenses/gpl-3.0.html\n"));
    m.exec();
}

void ItWindow::createActions()
{
    newAct = new QAction(tr("New"), this);
    //newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create new alignment from custom text or XML files"));
    //newAct->setIcon();
    connect(newAct, SIGNAL(triggered()), this, SLOT(createNewAlignment()));
    newAct->setData(QVariant("new"));
    allActions.append(newAct);
    toolBarActions.append(newAct);

    openAct = new QAction(tr("Open from repository"), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open alignment from the local repository"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));
    openAct->setData(QVariant("open"));
    allActions.append(openAct);
    toolBarActions.append(openAct);

    saveAct = new QAction(tr("Save to repository"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save alignment to the local repository"));
    saveAct->setIcon(composeIcon("document-save.png"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));
    saveAct->setData(QVariant("save"));
    saveAct->setEnabled(false);
    allActions.append(saveAct);
    toolBarActions.append(saveAct);

    closeAct = new QAction(tr("Close"), this);
    closeAct->setShortcuts(QKeySequence::Close);
    closeAct->setStatusTip(tr("Close current alignment"));
    connect(closeAct, SIGNAL(triggered()), this, SLOT(closeAlignment()));
    closeAct->setData(QVariant("close"));
    allActions.append(closeAct);
    toolBarActions.append(closeAct);

    syncAct = new QAction(tr("Synchronize with server"), this);
    //Act->setShortcuts(QKeySequence::Save);
    syncAct->setStatusTip(tr("Sync with InterText server"));
    syncAct->setIcon(composeIcon("synchronize.png"));
    connect(syncAct, SIGNAL(triggered()), this, SLOT(sync()));
    syncAct->setData(QVariant("sync"));
    syncAct->setEnabled(false);
    allActions.append(syncAct);
    toolBarActions.append(syncAct);

    fimportAct = new QAction(tr("Import"), this);
    //fimportAct->setShortcuts(QKeySequence::New);
    fimportAct->setStatusTip(tr("Import alignment from XML files to the repository"));
    connect(fimportAct, SIGNAL(triggered()), this, SLOT(importFile()));
    fimportAct->setData(QVariant("import"));
    allActions.append(fimportAct);
    toolBarActions.append(fimportAct);

    fexportAct = new QAction(tr("Export"), this);
    //fexportAct->setShortcuts(QKeySequence::SaveAs);
    fexportAct->setStatusTip(tr("Export alignment from repository to XML files"));
    connect(fexportAct, SIGNAL(triggered()), this, SLOT(exportFile()));
    fexportAct->setData(QVariant("export"));
    allActions.append(fexportAct);
    toolBarActions.append(fexportAct);

    alManAct = new QAction(tr("Repository manager"), this);
    alManAct->setShortcut(Qt::CTRL|Qt::Key_M);
    alManAct->setStatusTip(tr("Open alignment manager of the local repository"));
    alManAct->setIcon(composeIcon("manager.png"));
    connect(alManAct, SIGNAL(triggered()), this, SLOT(showAlMan()));
    alManAct->setData(QVariant("alignment_manager"));
    allActions.append(alManAct);
    toolBarActions.append(alManAct);

    alPropAct = new QAction(tr("Properties"), this);
    alPropAct->setStatusTip(tr("Edit alignment properties"));
    connect(alPropAct, SIGNAL(triggered()), this, SLOT(alPropEdit()));
    alPropAct->setData(QVariant("alignment_properties"));
    allActions.append(alPropAct);
    toolBarActions.append(alPropAct);

    undoAct = new QAction(tr("Undo"), this);
    undoAct->setShortcuts(QKeySequence::Undo);
    undoAct->setStatusTip(tr("Undo last change"));
    undoAct->setIcon(composeIcon("edit-undo.png"));
    undoAct->setEnabled(false);
    connect(undoAct,SIGNAL(triggered()), this, SLOT(undo()));
    undoAct->setData(QVariant("undo"));
    allActions.append(undoAct);
    toolBarActions.append(undoAct);
    ctxmenuActions.append(undoAct);

    redoAct = new QAction(tr("Redo"), this);
    redoAct->setShortcuts(QKeySequence::Redo);
    redoAct->setStatusTip(tr("Redo last undone change"));
    redoAct->setIcon(composeIcon("edit-redo.png"));
    redoAct->setEnabled(false);
    connect(redoAct,SIGNAL(triggered()), this, SLOT(redo()));
    redoAct->setData(QVariant("redo"));
    allActions.append(redoAct);
    toolBarActions.append(redoAct);
    ctxmenuActions.append(redoAct);

    exitAct = new QAction(tr("Exit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));
    exitAct->setData(QVariant("exit"));
    allActions.append(exitAct);
    toolBarActions.append(exitAct);

    aboutAct = new QAction(tr("About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    //cutAct->setEnabled(false);
    //copyAct->setEnabled(false);
    //connect(textEdit, SIGNAL(copyAvailable(bool)), cutAct, SLOT(setEnabled(bool)));
    //connect(textEdit, SIGNAL(copyAvailable(bool)), copyAct, SLOT(setEnabled(bool)));

    checkUpdatesAct = new QAction(tr("Check for updates..."), this);
    checkUpdatesAct->setStatusTip(tr("Check for new updates of InterText or the integrated tools."));
    connect(checkUpdatesAct, SIGNAL(triggered()), this, SLOT(checkForUpdates()));

    moveUpAct = new QAction(tr("Move text up"), this);
    moveUpAct->setShortcut(Qt::Key_Backspace);
    moveUpAct->setStatusTip(tr("Move text up"));
    moveUpAct->setIcon(composeIcon("arrow-up-double.png"));
    connect(moveUpAct, SIGNAL(triggered()), view, SLOT(moveUp()));
    moveUpAct->setEnabled(false);
    moveUpAct->setData(QVariant("move_up"));
    allActions.append(moveUpAct);
    toolBarActions.append(moveUpAct);
    ctxmenuActions.append(moveUpAct);

    moveDownAct = new QAction(tr("Move text down"), this);
    moveDownAct->setShortcut(Qt::Key_Return);
    moveDownAct->setStatusTip(tr("Move text down"));
    moveDownAct->setIcon(composeIcon("arrow-down-double.png"));
    connect(moveDownAct, SIGNAL(triggered()), view, SLOT(moveDown()));
    moveDownAct->setData(QVariant("move_down"));
    moveDownAct->setEnabled(false);
    allActions.append(moveDownAct);
    toolBarActions.append(moveDownAct);
    ctxmenuActions.append(moveDownAct);

    moveBUpAct = new QAction(tr("Merge segments (move both up)"), this);
    moveBUpAct->setShortcut(Qt::CTRL|Qt::Key_Backspace);
    moveBUpAct->setStatusTip(tr("Move both up (merge segment with the previous one)"));
    moveBUpAct->setIcon(composeIcon("go-up.png"));
    connect(moveBUpAct, SIGNAL(triggered()), view, SLOT(moveBothUp()));
    moveBUpAct->setEnabled(false);
    moveBUpAct->setData(QVariant("move_both_up"));
    allActions.append(moveBUpAct);
    toolBarActions.append(moveBUpAct);
    ctxmenuActions.append(moveBUpAct);

    moveBDownAct = new QAction(tr("Insert segment (move both down)"), this);
    moveBDownAct->setShortcut(Qt::CTRL|Qt::Key_Return);
    moveBDownAct->setStatusTip(tr("Move both down (insert an empty segment)"));
    moveBDownAct->setIcon(composeIcon("go-down.png"));
    connect(moveBDownAct, SIGNAL(triggered()), view, SLOT(moveBothDown()));
    moveBDownAct->setData(QVariant("move_both_down"));
    allActions.append(moveBDownAct);
    toolBarActions.append(moveBDownAct);
    ctxmenuActions.append(moveBDownAct);

    moveBDownAct->setEnabled(false);
    moveTextAct = new QAction(tr("Move text"), this);
    //moveTextAct->setShortcut(Qt::CTRL|Qt::Key_Return);
    moveTextAct->setStatusTip(tr("Move text version to another position"));
    //moveTextAct->setIcon(composeIcon("go-down.png"));
    connect(moveTextAct, SIGNAL(triggered()), view, SLOT(moveText()));
    moveTextAct->setEnabled(false);
    moveTextAct->setData(QVariant("move_text"));
    allActions.append(moveTextAct);
    toolBarActions.append(moveTextAct);
    ctxmenuActions.append(moveTextAct);

    shiftAct = new QAction(tr("Shift first up"), this);
    shiftAct->setShortcut(Qt::CTRL|Qt::Key_Up);
    shiftAct->setStatusTip(tr("Shift first up"));
    shiftAct->setIcon(composeIcon("arrow-up.png"));
    connect(shiftAct, SIGNAL(triggered()), view, SLOT(shift()));
    shiftAct->setEnabled(false);
    shiftAct->setData(QVariant("shift"));
    allActions.append(shiftAct);
    toolBarActions.append(shiftAct);
    ctxmenuActions.append(shiftAct);

    popAct = new QAction(tr("Pop last down"), this);
    popAct->setShortcut(Qt::CTRL|Qt::Key_Down);
    popAct->setStatusTip(tr("Pop last down"));
    popAct->setIcon(composeIcon("arrow-down.png"));
    connect(popAct, SIGNAL(triggered()), view, SLOT(pop()));
    popAct->setEnabled(false);
    popAct->setData(QVariant("pop"));
    allActions.append(popAct);
    toolBarActions.append(popAct);
    ctxmenuActions.append(popAct);

    swapAct = new QAction(tr("Swap with preceding segment"), this);
    swapAct->setShortcut(Qt::CTRL|Qt::Key_X);
    swapAct->setStatusTip(tr("Swap with preceding segment"));
    swapAct->setIcon(composeIcon("swap.png"));
    connect(swapAct, SIGNAL(triggered()), view, SLOT(swapSegments()));
    swapAct->setEnabled(false);
    swapAct->setData(QVariant("swap"));
    allActions.append(swapAct);
    toolBarActions.append(swapAct);
    ctxmenuActions.append(swapAct);

    toggleMarkAct = new QAction(tr("Toggle bookmark"), this);
    toggleMarkAct->setShortcut(Qt::Key_M);
    toggleMarkAct->setStatusTip(tr("Toggle bookmark"));
    connect(toggleMarkAct, SIGNAL(triggered()), view, SLOT(toggleMark()));
    toggleMarkAct->setEnabled(false);
    toggleMarkAct->setData(QVariant("mark"));
    allActions.append(toggleMarkAct);
    toolBarActions.append(toggleMarkAct);
    ctxmenuActions.append(toggleMarkAct);

    toggleStatusAct = new QAction(tr("Toggle status"), this);
    toggleStatusAct->setShortcut(Qt::Key_S);
    toggleStatusAct->setStatusTip(tr("Toggle status"));
    connect(toggleStatusAct, SIGNAL(triggered()), view, SLOT(toggleStat()));
    toggleStatusAct->setEnabled(false);
    toggleStatusAct->setData(QVariant("status"));
    allActions.append(toggleStatusAct);
    toolBarActions.append(toggleStatusAct);
    ctxmenuActions.append(toggleStatusAct);

    confirmAct = new QAction(tr("Confirm all preceding"), this);
    confirmAct->setShortcut(Qt::Key_C);
    confirmAct->setIcon(composeIcon("confirmed.png"));
    confirmAct->setStatusTip(tr("Set status of this and all the preceding segments to confirmed"));
    connect(confirmAct, SIGNAL(triggered()), view, SLOT(confirmAll()));
    confirmAct->setEnabled(false);
    confirmAct->setData(QVariant("confirm"));
    allActions.append(confirmAct);
    toolBarActions.append(confirmAct);
    ctxmenuActions.append(confirmAct);

    editAct = new QAction(tr("Edit element"), this);
    editAct->setShortcut(Qt::Key_E);
    editAct->setStatusTip(tr("Edit and/or split element"));
    editAct->setIcon(composeIcon("edit-rename.png"));
    connect(editAct, SIGNAL(triggered()), view, SLOT(openEditor()));
    editAct->setEnabled(false);
    editAct->setData(QVariant("edit"));
    allActions.append(editAct);
    toolBarActions.append(editAct);
    ctxmenuActions.append(editAct);

    insertAct = new QAction(tr("Insert element"), this);
    insertAct->setShortcut(Qt::Key_I);
    insertAct->setStatusTip(tr("Insert new element and edit it"));
    insertAct->setIcon(composeIcon("insert.png"));
    connect(insertAct, SIGNAL(triggered()), view, SLOT(insertElement()));
    insertAct->setEnabled(false);
    insertAct->setData(QVariant("insert"));
    allActions.append(insertAct);
    toolBarActions.append(insertAct);
    ctxmenuActions.append(insertAct);

    editXmlAct = new QAction(tr("Edit XML tree"), this);
    //editAct->setShortcut(Qt::Key_E);
    editXmlAct->setStatusTip(tr("Edit document's XML tree"));
    //editXmlAct->setIcon(composeIcon("edit-rename.png"));
    connect(editXmlAct, SIGNAL(triggered()), this, SLOT(openXMLTreeEd()));
    editXmlAct->setEnabled(false);
    editXmlAct->setData(QVariant("edit_xml"));
    //allActions.append(editAct);
    //toolBarActions.append(editAct);
    //ctxmenuActions.append(editAct);

    splitParentAct = new QAction(tr("Insert paragraph break"), this);
    splitParentAct->setShortcut(Qt::CTRL|Qt::Key_B);
    splitParentAct->setStatusTip(tr("Insert new parent (paragraph) break, i.e. split paragraph"));
    connect(splitParentAct, SIGNAL(triggered()), view, SLOT(splitParent()));
    splitParentAct->setEnabled(false);
    splitParentAct->setData(QVariant("split_parent"));
    allActions.append(splitParentAct);
    toolBarActions.append(splitParentAct);
    ctxmenuActions.append(splitParentAct);

    mergeParentAct = new QAction(tr("Delete paragraph break"), this);
    mergeParentAct->setShortcut(Qt::CTRL|Qt::Key_D);
    mergeParentAct->setStatusTip(tr("Delete parent (paragraph) break, i.e. merge with the previous paragraph"));
    connect(mergeParentAct, SIGNAL(triggered()), view, SLOT(mergeParent()));
    mergeParentAct->setEnabled(false);
    mergeParentAct->setData(QVariant("merge_parent"));
    allActions.append(mergeParentAct);
    toolBarActions.append(mergeParentAct);
    ctxmenuActions.append(mergeParentAct);

    mergeAct = new QAction(tr("Merge with preceding element"), this);
    mergeAct->setShortcut(Qt::ALT|Qt::Key_Backspace);
    mergeAct->setStatusTip(tr("Merge element with the preceding one"));
    mergeAct->setIcon(composeIcon("merge.png"));
    connect(mergeAct, SIGNAL(triggered()), this, SLOT(merge()));
    mergeAct->setEnabled(false);
    mergeAct->setData(QVariant("merge"));
    allActions.append(mergeAct);
    toolBarActions.append(mergeAct);
    ctxmenuActions.append(mergeAct);

    autoAlignAct = new QAction(tr("Auto aligner"), this);
    autoAlignAct->setStatusTip(tr("Run automatic aligner"));
    connect(autoAlignAct, SIGNAL(triggered()), this, SLOT(autoAlign()));
    autoAlignAct->setData(QVariant("align"));
    allActions.append(autoAlignAct);
    toolBarActions.append(autoAlignAct);

    updateStatAct = new QAction(tr("Auto update status"), this);
    updateStatAct->setStatusTip(tr("Automatically update status of all segments preceding any change to 'confirmed'"));
    connect(updateStatAct, SIGNAL(triggered()), this, SLOT(toggleAutoUpdateStatus()));
    updateStatAct->setCheckable(true);
    updateStatAct->setChecked(true);
    htmlViewAct = new QAction(tr("HTML view"), this);
    htmlViewAct->setStatusTip(tr("Show text rendered as HTML"));
    connect(htmlViewAct, SIGNAL(triggered()), this, SLOT(toggleHtmlView()));
    htmlViewAct->setCheckable(true);
    htmlViewAct->setChecked(true);
    highlNon11Act = new QAction(tr("Highligt non-1:1"), this);
    highlNon11Act->setStatusTip(tr("Highligt non-1:1 segments"));
    connect(highlNon11Act, SIGNAL(triggered()), this, SLOT(toggleHighlNon11()));
    highlNon11Act->setCheckable(true);
    highlNon11Act->setChecked(true);
    highlMarkedAct = new QAction(tr("Highlight marked"), this);
    highlMarkedAct->setStatusTip(tr("Highlight marked segments"));
    connect(highlMarkedAct, SIGNAL(triggered()), this, SLOT(toggleHighlMarked()));
    highlMarkedAct->setCheckable(true);
    highlMarkedAct->setChecked(true);
    customizeAct = new QAction(tr("Customize"), this);
    customizeAct->setStatusTip(tr("Customize interface"));
    connect(customizeAct, SIGNAL(triggered()), this, SLOT(customize()));
    settingsAct = new QAction(tr("Settings..."), this);
    settingsAct->setStatusTip(tr("Configure application settings"));
    connect(settingsAct, SIGNAL(triggered()), this, SLOT(editSettings()));

    findNextBmAct = new QAction(tr("Next bookmark"), this);
    findNextBmAct->setStatusTip(tr("Find next bookmark"));
    findNextBmAct->setIcon(composeIcon("go-next-view.png"));
    findNextBmAct->setIconVisibleInMenu(false);
    connect(findNextBmAct, SIGNAL(triggered()), this, SLOT(findNextBookmark()));
    findNextBmAct->setShortcut(Qt::Key_B);
    findNextBmAct->setData(QVariant("next_bookmark"));
    allActions.append(findNextBmAct);
    toolBarActions.append(findNextBmAct);

    findPrevBmAct = new QAction(tr("Previous bookmark"), this);
    findPrevBmAct->setStatusTip(tr("Find previous bookmark"));
    findPrevBmAct->setIcon(composeIcon("go-previous-view.png"));
    findPrevBmAct->setIconVisibleInMenu(false);
    connect(findPrevBmAct, SIGNAL(triggered()), this, SLOT(findPrevBookmark()));
    findPrevBmAct->setShortcut(Qt::ALT|Qt::Key_B);
    findPrevBmAct->setData(QVariant("previous_bookmark"));
    allActions.append(findPrevBmAct);
    toolBarActions.append(findPrevBmAct);

    findNextNon11Act = new QAction(tr("Next non-1:1"), this);
    findNextNon11Act->setStatusTip(tr("Find next non-1:1 segment"));
    findNextNon11Act->setIcon(composeIcon("go-next-view.png"));
    findNextNon11Act->setIconVisibleInMenu(false);
    connect(findNextNon11Act, SIGNAL(triggered()), this, SLOT(findNextNon11()));
    findNextNon11Act->setShortcut(Qt::Key_N);
    findNextNon11Act->setData(QVariant("next_non11"));
    allActions.append(findNextNon11Act);
    toolBarActions.append(findNextNon11Act);

    findPrevNon11Act = new QAction(tr("Previous non-1:1"), this);
    findPrevNon11Act->setStatusTip(tr("Find previous non-1:1 segment"));
    findPrevNon11Act->setIcon(composeIcon("go-previous-view.png"));
    findPrevNon11Act->setIconVisibleInMenu(false);
    connect(findPrevNon11Act, SIGNAL(triggered()), this, SLOT(findPrevNon11()));
    findPrevNon11Act->setShortcut(Qt::ALT|Qt::Key_N);
    findPrevNon11Act->setData(QVariant("previous_non11"));
    allActions.append(findPrevNon11Act);
    toolBarActions.append(findPrevNon11Act);

    findNextUnconfAct = new QAction(tr("Next unconfirmed"), this);
    findNextUnconfAct->setStatusTip(tr("Find next unconfirmed segment"));
    findNextUnconfAct->setIcon(composeIcon("go-next-view.png"));
    findNextUnconfAct->setIconVisibleInMenu(false);
    connect(findNextUnconfAct, SIGNAL(triggered()), this, SLOT(findNextUnconfirmed()));
    findNextUnconfAct->setShortcut(Qt::Key_U);
    findNextUnconfAct->setData(QVariant("next_unconfirmed"));
    allActions.append(findNextUnconfAct);
    toolBarActions.append(findNextUnconfAct);

    findPrevUnconfAct = new QAction(tr("Previous unconfirmed"), this);
    findPrevUnconfAct->setStatusTip(tr("Find previous unconfirmed segment"));
    findPrevUnconfAct->setIcon(composeIcon("go-previous-view.png"));
    findPrevUnconfAct->setIconVisibleInMenu(false);
    connect(findPrevUnconfAct, SIGNAL(triggered()), this, SLOT(findPrevUnconfirmed()));
    findPrevUnconfAct->setShortcut(Qt::ALT|Qt::Key_U);
    findPrevUnconfAct->setData(QVariant("previous_unconfirmed"));
    allActions.append(findPrevUnconfAct);
    toolBarActions.append(findPrevUnconfAct);

    findBmAct = new QAction(tr("First bookmark"), this);
    findBmAct->setStatusTip(tr("Find first bookmark"));
    findBmAct->setIcon(composeIcon("mark.png"));
    connect(findBmAct, SIGNAL(triggered()), this, SLOT(findFirstBookmark()));
    //findBmAct->setShortcut(Qt::Key_B);
    findBmAct->setData(QVariant("first_bookmark"));
    allActions.append(findBmAct);
    toolBarActions.append(findBmAct);

    findNon11Act = new QAction(tr("First non-1:1"), this);
    findNon11Act->setStatusTip(tr("Find first non-1:1 segment"));
    findNon11Act->setIcon(composeIcon("non11.png"));
    connect(findNon11Act, SIGNAL(triggered()), this, SLOT(findFirstNon11()));
    //findNon11Act->setShortcut(Qt::Key_B);
    findNon11Act->setData(QVariant("first_non11"));
    allActions.append(findNon11Act);
    toolBarActions.append(findNon11Act);

    findUnconfAct = new QAction(tr("First unconfirmed"), this);
    findUnconfAct->setStatusTip(tr("Find first unconfirmed segment"));
    findUnconfAct->setIcon(composeIcon("unconfirmed.png"));
    connect(findUnconfAct, SIGNAL(triggered()), this, SLOT(findFirstUnconfirmed()));
    //findUnconfAct->setShortcut(Qt::Key_B);
    findUnconfAct->setData(QVariant("first_unconfirmed"));
    allActions.append(findUnconfAct);
    toolBarActions.append(findUnconfAct);

    findAct = new QAction(tr("Find"), this);
    findAct->setStatusTip(tr("Search texts"));
    findAct->setIcon(composeIcon("search.png"));
    connect(findAct, SIGNAL(triggered()), searchBar, SLOT(showSearch()));
    findAct->setShortcuts(QKeySequence::Find);
    findAct->setData(QVariant("find"));
    allActions.append(findAct);
    toolBarActions.append(findAct);

    findNextAct = new QAction(tr("Find next"), this);
    findNextAct->setStatusTip(tr("Find next occurrence"));
    connect(findNextAct, SIGNAL(triggered()), this, SLOT(findNext()));
    findNextAct->setShortcuts(QKeySequence::FindNext);
    findNextAct->setData(QVariant("find_next"));
    allActions.append(findNextAct);

    findPrevAct = new QAction(tr("Find previous"), this);
    findPrevAct->setStatusTip(tr("Find previous occurrence"));
    connect(findPrevAct, SIGNAL(triggered()), this, SLOT(findPrev()));
    findPrevAct->setShortcuts(QKeySequence::FindPrevious);
    findPrevAct->setData(QVariant("find_previous"));
    allActions.append(findPrevAct);

    replaceAct = new QAction(tr("Replace"), this);
    replaceAct->setStatusTip(tr("Search and replace"));
    connect(replaceAct, SIGNAL(triggered()), searchBar, SLOT(showReplace()));
    replaceAct->setShortcuts(QKeySequence::Replace);
    replaceAct->setData(QVariant("replace"));
    allActions.append(replaceAct);

    toolBarSizeGroup = new QActionGroup(this);

    hiddenTB = new QAction(tr("hidden"), toolBarSizeGroup);
    hiddenTB->setCheckable(true);
    connect(hiddenTB, SIGNAL(triggered()), this, SLOT(hiddenTBar()));
    tinyTB = new QAction(tr("tiny"), toolBarSizeGroup);
    tinyTB->setCheckable(true);
    connect(tinyTB, SIGNAL(triggered()), this, SLOT(tinyTBar()));
    smallTB = new QAction(tr("small"), toolBarSizeGroup);
    smallTB->setCheckable(true);
    connect(smallTB, SIGNAL(triggered()), this, SLOT(smallTBar()));
    mediumTB = new QAction(tr("medium"), toolBarSizeGroup);
    mediumTB->setCheckable(true);
    connect(mediumTB, SIGNAL(triggered()), this, SLOT(mediumTBar()));
    largeTB = new QAction(tr("large"), toolBarSizeGroup);
    largeTB->setCheckable(true);
    connect(largeTB, SIGNAL(triggered()), this, SLOT(largeTBar()));
    xlTB = new QAction(tr("XL"), toolBarSizeGroup);
    xlTB->setCheckable(true);
    connect(xlTB, SIGNAL(triggered()), this, SLOT(xlTBar()));
    xxlTB = new QAction(tr("XXL"), toolBarSizeGroup);
    xxlTB->setCheckable(true);
    connect(xxlTB, SIGNAL(triggered()), this, SLOT(xxlTBar()));
    xxxlTB = new QAction(tr("XXXL"), toolBarSizeGroup);
    xxxlTB->setCheckable(true);
    connect(xxxlTB, SIGNAL(triggered()), this, SLOT(xxxlTBar()));
    ulTB = new QAction(tr("UL"), toolBarSizeGroup);
    ulTB->setCheckable(true);
    connect(ulTB, SIGNAL(triggered()), this, SLOT(ulTBar()));


    ctrlGroup = new QActionGroup(this);

    hiddenCtrlAct = new QAction(tr("hidden"), ctrlGroup);
    hiddenCtrlAct->setCheckable(true);
    connect(hiddenCtrlAct, SIGNAL(triggered()), this, SLOT(hiddenCtrl()));
    onmoveCtrlAct = new QAction(tr("on mouse move"), ctrlGroup);
    onmoveCtrlAct->setCheckable(true);
    connect(onmoveCtrlAct, SIGNAL(triggered()), this, SLOT(onmoveCtrl()));
    onclickCtrlAct = new QAction(tr("on mouse click"), ctrlGroup);
    onclickCtrlAct->setCheckable(true);
    connect(onclickCtrlAct, SIGNAL(triggered()), this, SLOT(onclickCtrl()));

    ctrlSizeGroup = new QActionGroup(this);
    tinyCtrlAct = new QAction(tr("tiny"), ctrlSizeGroup);
    tinyCtrlAct->setCheckable(true);
    connect(tinyCtrlAct, SIGNAL(triggered()), this, SLOT(tinyCtrl()));
    smallCtrlAct = new QAction(tr("small"), ctrlSizeGroup);
    smallCtrlAct->setCheckable(true);
    connect(smallCtrlAct, SIGNAL(triggered()), this, SLOT(smallCtrl()));
    mediumCtrlAct = new QAction(tr("medium"), ctrlSizeGroup);
    mediumCtrlAct->setCheckable(true);
    connect(mediumCtrlAct, SIGNAL(triggered()), this, SLOT(mediumCtrl()));
    largeCtrlAct = new QAction(tr("large"), ctrlSizeGroup);
    largeCtrlAct->setCheckable(true);
    connect(largeCtrlAct, SIGNAL(triggered()), this, SLOT(largeCtrl()));
    xlCtrlAct = new QAction(tr("XL"), ctrlSizeGroup);
    xlCtrlAct->setCheckable(true);
    connect(xlCtrlAct, SIGNAL(triggered()), this, SLOT(xlCtrl()));
    xxlCtrlAct = new QAction(tr("XXL"), ctrlSizeGroup);
    xxlCtrlAct->setCheckable(true);
    connect(xxlCtrlAct, SIGNAL(triggered()), this, SLOT(xxlCtrl()));
    xxxlCtrlAct = new QAction(tr("XXXL"), ctrlSizeGroup);
    xxxlCtrlAct->setCheckable(true);
    connect(xxxlCtrlAct, SIGNAL(triggered()), this, SLOT(xxxlCtrl()));
    ulCtrlAct = new QAction(tr("UL"), ctrlSizeGroup);
    ulCtrlAct->setCheckable(true);
    connect(ulCtrlAct, SIGNAL(triggered()), this, SLOT(ulCtrl()));
}

void ItWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&Alignment"));
    fileMenu->addAction(alManAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(closeAct);
    fileMenu->addSeparator();
    serverMenu = fileMenu->addMenu(tr("Remote server"));
    fileMenu->addSeparator();
    fileMenu->addAction(newAct);
    fileMenu->addAction(fimportAct);
    fileMenu->addAction(fexportAct);
    exTextMenu = fileMenu->addMenu(tr("Export texts as"));
    fileMenu->addSeparator();
    fileMenu->addAction(alPropAct);
    fileMenu->addAction(syncAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(undoAct);
    editMenu->addAction(redoAct);
    editMenu->addSeparator();
    editMenu->addAction(shiftAct);
    editMenu->addAction(moveUpAct);
    editMenu->addAction(moveBUpAct);
    editMenu->addAction(moveTextAct);
    editMenu->addAction(moveBDownAct);
    editMenu->addAction(moveDownAct);
    editMenu->addAction(popAct);
    editMenu->addSeparator();
    editMenu->addAction(swapAct);
    editMenu->addSeparator();
    editMenu->addAction(toggleMarkAct);
    editMenu->addAction(toggleStatusAct);
    editMenu->addAction(confirmAct);
    editMenu->addSeparator();
    editMenu->addAction(editAct);
    editMenu->addAction(insertAct);
    //editMenu->addAction(editXmlAct);
    editMenu->addSeparator();
    editMenu->addAction(mergeAct);
    editMenu->addAction(splitParentAct);
    editMenu->addAction(mergeParentAct);
    editMenu->addSeparator();
    editMenu->addAction(autoAlignAct);

    searchMenu = menuBar()->addMenu(tr("&Search"));
    searchMenu->addAction(findNextBmAct);
    searchMenu->addAction(findNextUnconfAct);
    searchMenu->addAction(findNextNon11Act);
    editMenu->addSeparator();
    searchMenu->addAction(findPrevBmAct);
    searchMenu->addAction(findPrevUnconfAct);
    searchMenu->addAction(findPrevNon11Act);
    editMenu->addSeparator();
    searchMenu->addAction(findBmAct);
    searchMenu->addAction(findUnconfAct);
    searchMenu->addAction(findNon11Act);
    searchMenu->addSeparator();
    searchMenu->addAction(findAct);
    searchMenu->addAction(findNextAct);
    searchMenu->addAction(findPrevAct);
    searchMenu->addAction(replaceAct);

    setMenu = menuBar()->addMenu(tr("&Options"));
    setMenu->addAction(updateStatAct);
    setMenu->addSeparator();
    setMenu->addAction(htmlViewAct);
    setMenu->addAction(highlNon11Act);
    setMenu->addAction(highlMarkedAct);
    setMenu->addSeparator();
    toolBarMenu = setMenu->addMenu(tr("Toolbar"));
    toolBarMenu->addActions(toolBarSizeGroup->actions());
    ctrlMenu = setMenu->addMenu(tr("Controls"));
    ctrlMenu->addActions(ctrlGroup->actions());
    ctrlMenu->addSeparator();
    ctrlMenu->addActions(ctrlSizeGroup->actions());
    setMenu->addSeparator();
    setMenu->addAction(customizeAct);
    setMenu->addAction(settingsAct);

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(checkUpdatesAct);
    helpMenu->addSeparator();
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}

void ItWindow::updateServerMenu()
{
    serverMenu->clear();
    if (serverMapper!=0) {
        delete serverMapper;
        serverMapper = 0;
        qDeleteAll(serverActions);
        serverActions.clear();
    }

    if (servers.size()==0) {
        nsAct = new QAction(tr("New connection"), this);
        nsAct->setStatusTip(tr("Configure new server connection"));
        connect(nsAct, SIGNAL(triggered()), this, SLOT(newServerConnection()));
        serverMenu->addAction(nsAct);
        return;
    }

    if (nsAct!=0) {
        delete nsAct;
        nsAct = 0;
    }

    QAction * sAct;
    serverMapper = new QSignalMapper(this);
    QMapIterator<QString, ItServer> i(servers);
    while (i.hasNext()) {
        i.next();
        sAct = new QAction(i.value().name, this);
        serverActions.append(sAct);
        serverMenu->addAction(sAct);
        serverMapper->setMapping(sAct, i.value().name);
        connect(sAct, SIGNAL(triggered()), serverMapper, SLOT(map()));
    }
    connect(serverMapper, SIGNAL(mapped(QString)), this, SLOT(serverDialog(QString)));
}

void ItWindow::updateExTextMenu()
{
    exTextMenu->clear();
    if (exTextMapper!=0) {
        delete exTextMapper;
        exTextMapper = 0;
        qDeleteAll(exTextActions);
        exTextActions.clear();
    }
    foreach (QAction * a, exTextActGroup->actions())
    {
        exTextActGroup->removeAction(a);
    }

    QAction * teAct;
    exTextMapper = new QSignalMapper(this);
    QMapIterator<QString, ExTextProfile> i(exTextProfiles);
    while (i.hasNext()) {
        i.next();
        teAct = new QAction(i.value().name, this);
        exTextActions.append(teAct);
        exTextMenu->addAction(teAct);
        exTextActGroup->addAction(teAct);
        exTextMapper->setMapping(teAct, i.value().name);
        connect(teAct, SIGNAL(triggered()), exTextMapper, SLOT(map()));
    }
    connect(exTextMapper, SIGNAL(mapped(QString)), this, SLOT(extractTextAndSave(QString)));
}

void ItWindow::createStatusBar()
{
    statusBar()->addWidget(infoBar);
}

bool ItWindow::saveFile(const QString &fileName)
{
    return model->alignment->saveFile(fileName);
}

void ItWindow::readSettings()
{

    QHash<QString, QAction*> aclist;
    QAction *a;
    foreach (a, allActions) {
        aclist.insert(a->data().toString(), a);
    }

    int version = settings->value("config_ver", 0).toInt();
    QPoint pos = settings->value("pos", QPoint(0, 0)).toPoint();
    QSize size = settings->value("size", QSize(640, 480)).toSize();
    setAutoSaveInterval(settings->value("autosave", 5).toInt());
    autoCheckUpdates = settings->value("check_updates_on_startup", "true").toBool();
    enableCrossOrderAlignment = settings->value("enable_cross_order_alignment", "false").toBool();
    /*defaultIdNamespaceURI = settings->value("default_id_namespace_uri", defaultIdNamespaceURI).toString();*/ /* not implemented well into ItDocument/ItElement */
    view->setShowControls((ShowControlsType)settings->value("show_controls", view->getShowControls()).toInt());
    view->setSizeControls(settings->value("size_controls", view->getSizeControls()).toInt());
    view->controlsHideTimeOut = settings->value("autohide_controls", view->controlsHideTimeOut).toInt();
    if (view->getShowControls() == HiddenCtrl)
        hiddenCtrlAct->setChecked(true);
    else if (view->getShowControls() == OnMove)
        onmoveCtrlAct->setChecked(true);
    else if (view->getShowControls() == OnClick)
        onclickCtrlAct->setChecked(true);
    switch (view->getSizeControls()) {
    case TINY:
        tinyCtrlAct->setChecked(true);
        break;
    case SMALL:
        smallCtrlAct->setChecked(true);
        break;
    case MEDIUM:
        mediumCtrlAct->setChecked(true);
        break;
    case LARGE:
        largeCtrlAct->setChecked(true);
        break;
    case XLARGE:
        xlCtrlAct->setChecked(true);
        break;
    case XXLARGE:
        xxlCtrlAct->setChecked(true);
        break;
    case XXXLARGE:
        xxxlCtrlAct->setChecked(true);
        break;
    case ULARGE:
        ulCtrlAct->setChecked(true);
        break;
    }

    toolBarSize = (TBSize)settings->value("toolbar_size", toolBarSize).toInt();
    toolBarLocation = (Qt::ToolBarArea)settings->value("toolbar_area", toolBarLocation).toInt();
    createToolBar();
    QString aname;
    if (!settings->value("toolbar_actions", "").toString().isEmpty()) {
        QStringList toolbarConf = settings->value("toolbar_actions", "").toString().split(" ");
        toolBar->clear();
        foreach (aname, toolbarConf) {
            if (aname == "|")
                toolBar->addSeparator();
            else if (aclist.contains(aname)) {
                a = aclist.value(aname);
                toolBar->addAction(a);
            }
        }
    }
    QString defaultCtxMenuConf("shift move_up move_both_up move_both_down move_down pop | edit insert | merge split_parent merge_parent | confirm | undo redo");
    QStringList ctxmenuConf = settings->value("context_menu_actions", defaultCtxMenuConf).toString().split(" ");
    foreach (aname, ctxmenuConf) {
        if (aname == "|")
            contextMenuCurActions.append(0);
        else if (aclist.contains(aname)) {
            a = aclist.value(aname);
            contextMenuCurActions.append(a);
        }
    }
    view->createContextMenu();

    view->setAutoSaveElement((AutoState)settings->value("auto_save_element", AutoAsk).toInt());
    view->skipMargin = settings->value("skip_margin", view->skipMargin).toInt();
    QString font_fam = settings->value("text_font_family", "Times New Roman").toString();
    int font_size = settings->value("text_font_size",12).toInt();
    QFont font(font_fam, font_size, -1, false);
    view->setFont(font);
    bool htmlView = settings->value("html_view","true").toBool();
    cssStyle = settings->value("css_stylesheet", "").toString();
    if (!htmlView)
        htmlViewAct->setChecked(false);
    if (!settings->value("highlight_non11","true").toBool())
        toggleHighlNon11();
    if (!settings->value("highlight_marked","true").toBool())
        toggleHighlMarked();
    QColor c;
    c.setNamedColor(settings->value("fgcolor_default", colors.fgdefault.name()).toString());
    colors.fgdefault = c;
    c.setNamedColor(settings->value("bgcolor_default", colors.bgdefault.name()).toString());
    colors.bgdefault = c;
    c.setNamedColor(settings->value("bgcolor_non11", colors.bgnon11.name()).toString());
    colors.bgnon11 = c;
    c.setNamedColor(settings->value("bgcolor_marked", colors.bgmarked.name()).toString());
    colors.bgmarked = c;
    colors.bgAddDark = settings->value("even_rows_darkness_change", colors.bgAddDark).toInt();
    c.setNamedColor(settings->value("bgcolor_cursor", colors.cursor.name()).toString());
    colors.cursor = c;
    colors.cursorOpac = settings->value("cursor_opacity", colors.cursorOpac).toInt();
    c.setNamedColor(settings->value("bgcolor_found", colors.bgfound.name()).toString());
    colors.bgfound = c;
    c.setNamedColor(settings->value("bgcolor_replaced", colors.bgrepl.name()).toString());
    colors.bgrepl = c;

    view->setHtmlView(htmlView);
    resize(size);
    move(pos);

    settings->beginGroup("shortcuts");
    foreach (a, allActions) {
        defaultShortcuts.insert(a->data().toString(), a->shortcut().toString());
        QKeySequence keys = QKeySequence::fromString(settings->value(a->data().toString(), a->shortcut().toString()).toString());
        //if (!keys.isEmpty())
        a->setShortcut(keys);
    }
    settings->endGroup();

    settings->beginGroup("editor_shortcuts");
    editorKeys.saveExit = QKeySequence::fromString(settings->value("save_exit", editorKeys.saveExit.toString()).toString());
    editorKeys.discardExit = QKeySequence::fromString(settings->value("discard_exit", editorKeys.discardExit.toString()).toString());
    editorKeys.saveNext = QKeySequence::fromString(settings->value("save_next", editorKeys.saveNext.toString()).toString());
    editorKeys.savePrev = QKeySequence::fromString(settings->value("save_prev", editorKeys.savePrev.toString()).toString());
    editorKeys.saveInsertNext = QKeySequence::fromString(settings->value("save_insert_next", editorKeys.saveInsertNext.toString()).toString());
    settings->endGroup();

    int alsize = settings->beginReadArray("aligners");
    for (int i=0; i<alsize; i++) {
        settings->setArrayIndex(i);
        Aligner aligner;
        aligner.name = settings->value("name").toString();
        aligner.exec = settings->value("exec").toString();
        aligner.tmpdir = settings->value("temp_dir").toString();
        aligner.exp_head = strunescape(settings->value("exp_head").toString());
        aligner.exp_el_sep = strunescape(settings->value("exp_element_separator").toString());
        aligner.exp_parent_sep = strunescape(settings->value("exp_parent_separator").toString());
        aligner.exp_foot = strunescape(settings->value("exp_foot").toString());
        aligner.al_imp_method = (AlignerImportMethod)settings->value("imp_method").toInt();
        int psize = settings->beginReadArray("profiles");
        for (int j=0; j<psize; j++) {
            settings->setArrayIndex(j);
            AlignerProfile profile;
            profile.name = settings->value("name").toString();
            profile.params = settings->value("params").toString();
            aligner.profiles.append(profile);
        }
        settings->endArray();
        autoAligners.append(aligner);
    }
    settings->endArray();

    if (autoAligners.isEmpty()) {
        Aligner aligner;
        aligner.name = "Hunalign";
        aligner.exec = "hunalign";
        aligner.tmpdir = QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0);
        aligner.exp_el_sep = "\n";
        aligner.exp_parent_sep = "<p>\n";
        aligner.al_imp_method = HunalignLadder;
        AlignerProfile profile;
        profile.name = "default";
        profile.params = "-utf -realign {EXEPATH}/empty.dic {TXT1} {TXT2}";
        aligner.profiles.append(profile);
        autoAligners.append(aligner);
    }

    // fix old configuration bugs...
    if (version == 0) {
        for (int i=0; i<autoAligners.size(); i++) {
            autoAligners[i].al_imp_method = HunalignLadder;
        }
        Aligner aligner;
        aligner.name = "Plain 1:1 aligner";
        aligner.exec = "";
        aligner.tmpdir = "";
        aligner.exp_el_sep = "";
        aligner.exp_parent_sep = "";
        aligner.al_imp_method = PlainAligner;
        /*AlignerProfile profile;
      profile.name = "plain";
      profile.params = "";
      aligner.profiles.append(profile);*/
        autoAligners.append(aligner);
    }
    if (version<2) {
        for (int i=0; i<autoAligners.size(); i++) {
            if (autoAligners[i].name=="Hunalign")
                autoAligners[i].al_imp_method = HunalignLadder;
        }
    }

    settings->beginGroup("import");
    askOnTxtImport = settings->value("ask_on_txtimport", askOnTxtImport).toBool();
    askOnXmlImport = settings->value("ask_on_xmlimport", askOnXmlImport).toBool();
    importXmlLock = settings->value("xmlimport_lock", importXmlLock).toBool();
    workDir = settings->value("working_dir", workDir).toString();
    splitSetTxt = settings->value("splitter_on", splitSetTxt).toBool();
    splitSetXml = settings->value("splitter_on_xml", splitSetTxt).toBool();
    alignableElements = settings->value("alignable_elements", alignableElements).toStringList();
    textElements = settings->value("splitter_text_containers", textElements).toStringList();
    splitterElName = settings->value("splitter_new_element", splitterElName).toString();
    splitter.selectedProfile = settings->value("splitter_profile", splitter.selectedProfile).toInt();
    importTxtEncoding = settings->value("txt_import_encoding", importTxtEncoding).toString();
    importXmlHeader = strunescape(settings->value("xml_header", importXmlHeader).toString());
    importXmlFooter = strunescape(settings->value("xml_footer", importXmlFooter).toString());
    emptyDocTemplate = strunescape(settings->value("empty_doc_template", emptyDocTemplate).toString());
    importParSeparator = strunescape(settings->value("par_separator", importParSeparator).toString());
    importSentenceSeparator = strunescape(settings->value("sentence_separator", importSentenceSeparator).toString());
    importKeepMarkup = settings->value("keep_markup", importKeepMarkup).toBool();
    importFormat = settings->value("import_format", importFormat).toInt();
    defaultNumberingLevels = settings->value("default_num_levels",defaultNumberingLevels).toInt();
    settings->endGroup();

    QList<ItSentenceSplitter::SplitterProfile> sprofiles;

    int spsize = settings->beginReadArray("splitter_profiles");
    for (int i=0; i<spsize; i++) {
        settings->setArrayIndex(i);
        ItSentenceSplitter::SplitterProfile p;
        p.name = settings->value("name", "").toString();
        p.abbrevs = settings->value("abbrevs", QStringList()).toStringList();
        int psize = settings->beginReadArray("expr");
        for (int j=0; j<psize; j++) {
            settings->setArrayIndex(j);
            ItSentenceSplitter::Replacement r;
            r.src = strunescape(settings->value("src", QString()).toString());
            r.repl = strunescape(settings->value("repl", QString()).toString());
            p.expressions << r;
        }
        settings->endArray();
        sprofiles.append(p);
    }
    settings->endArray();

    int ssize = settings->beginReadArray("servers");
    for (int i=0; i<ssize; i++) {
        settings->setArrayIndex(i);
        ItServer server;
        server.name = settings->value("name").toString();
        server.url = settings->value("url").toString();
        server.username = settings->value("user").toString();
        server.passwd = crypto.decryptToString(settings->value("passwd").toString());
        server.autoUpdateCheck = settings->value("check_updates", "true").toBool();
        servers.insert(server.name, server);
    }
    settings->endArray();

    settings->beginGroup("sync");
    syncMarkChanges = settings->value("mark_changes", syncMarkChanges).toBool();
    settings->endGroup();

    if (sprofiles.isEmpty()) {
        ItSentenceSplitter::SplitterProfile p;
        p.name = "default";
        ItSentenceSplitter::Replacement r;

        r.src = QString::fromUtf8("(\\.|\\?|\\!|…\\s?|:)\\s*((?:<[^>]*>|”|“|‛|’|‘|‟|′|″|‵|‶|»|\\\"|\\'|\\]|\\)|\\s)*)(\\s+)"
                                  "((?:<[^\\/][^>]*>|«|\\\"|„|”|“|‛|‚|’|‘|‟|′|″|‵|‶|\\'|\\(|\\[|\\&mdash;|-|—|\\s)*)(\\p{Lu})");
        r.repl = "\\1\\2#!#\\4\\5";
        p.expressions << r;
        r.src = QString::fromUtf8(";((?:\\s*<\\/[^>]*>)*)(\\s+)");
        r.repl = ";\\1#!#";
        p.expressions << r;
        r.src = QString::fromUtf8("(&[a-zA-Z#0-9]+;)#!#");
        r.repl = "\\1 ";
        p.expressions << r;
        //r.src = QString::fromUtf8("([0-9])\\.#!#([a-zěščřžýáíéúóďťňůøåæþðöäüß])");
        //r.repl = "\\1. \\2";
        //p.expressions << r;

        p.abbrevs << QString::fromUtf8("\\p{Lu}.");
        p.abbrevs << "Ch.";
        p.abbrevs << "[Ss]t.";
        p.abbrevs << "[Ss]v.";
        p.abbrevs << "[Mm]r.";
        p.abbrevs << "[Mm]rs.";
        p.abbrevs << "[Mm]s.";
        p.abbrevs << "[Hh]r.";
        p.abbrevs << "[Dd]r.";
        p.abbrevs << "[Pp]rof.";
        p.abbrevs << "[Ii]ng.";

        sprofiles << p;
    }
    splitter.setProfiles(sprofiles);

    int transsize = settings->beginReadArray("transformations");
    for (int i=0; i<transsize; i++) {
        settings->setArrayIndex(i);
        Replacement r;
        r.src = strunescape(settings->value("src", QString()).toString());
        r.repl = strunescape(settings->value("repl", QString()).toString());
        transformations << r;
    }
    settings->endArray();

    int tesize = settings->beginReadArray("text_export_profiles");
    for (int i=0; i<tesize; i++) {
        settings->setArrayIndex(i);
        ExTextProfile p;
        p.name = settings->value("name", "").toString();
        p.header = strunescape(settings->value("header", "").toString());
        p.footer = strunescape(settings->value("footer", "").toString());
        p.encoding = settings->value("encoding", "utf-8").toString();
        p.ext = settings->value("extension", "txt").toString();
        p.elSep = strunescape(settings->value("element_separator", "").toString());
        p.elStart = strunescape(settings->value("element_start", "").toString());
        p.elEnd = strunescape(settings->value("element_end", "").toString());
        p.parSep = strunescape(settings->value("parent_separator", "").toString());
        p.segSep = strunescape(settings->value("segment_separator", "").toString());
        p.segStart = strunescape(settings->value("segment_start", "").toString());
        p.segEnd = strunescape(settings->value("segment_end", "").toString());
        p.emptySegFiller = strunescape(settings->value("empty_segment_filler", "").toString());
        p.verSep = strunescape(settings->value("version_separator", "").toString());
        p.keepPars = settings->value("keep_parbreaks", "false").toBool();
        p.keepSegs = settings->value("keep_segments", "true").toBool();
        p.skipEmptySeg = settings->value("skip_empty_segs", "false").toBool();
        p.skipUnconfirmed = settings->value("skip_unconfirmed", "false").toBool();
        p.bothTexts = settings->value("both_texts", "false").toBool();
        int psize = settings->beginReadArray("replacements");
        for (int j=0; j<psize; j++) {
            settings->setArrayIndex(j);
            Replacement r;
            r.src = strunescape(settings->value("src", "").toString());
            r.repl = strunescape(settings->value("repl", "").toString());
            p.replacements << r;
        }
        settings->endArray();
        psize = settings->beginReadArray("custom_variables");
        for (int j=0; j<psize; j++) {
            settings->setArrayIndex(j);
            CustomVar c;
            c.custSymbol = strunescape(settings->value("symbol", "").toString());
            c.desc = strunescape(settings->value("desc", "").toString());
            c.defaultVal = strunescape(settings->value("default", "").toString());
            p.customVars << c;
        }
        settings->endArray();
        exTextProfiles.insert(p.name, p);
    }
    settings->endArray();

    if (exTextProfiles.isEmpty()) {
        Replacement r;

        ExTextProfile p;
        p.name = "ParaConc text (UTF-8)";
        p.header = "<p id=\"%p%\">";
        p.footer = "\n</p>\n";
        p.encoding = "UTF-8";
        p.elSep = "";
        p.elStart = "\n<s id=\"%p%:%pe%\">";
        p.elEnd = "</s>";
        p.parSep = "\n</p>\n<p id=\"%p%\">";
        p.segSep = "";
        p.segStart = "\n<seg id=\"%s%\">";
        p.segEnd = "\n</seg>";
        p.emptySegFiller = "";
        p.verSep = "";
        p.ext = "txt";
        p.keepPars = true;
        p.keepSegs = true;
        p.skipEmptySeg = false;
        p.skipUnconfirmed = false;
        p.bothTexts = false;
        p.replacements.clear();
        p.customVars.clear();
        exTextProfiles.insert(p.name, p);

        p.name = "Newline aligned (separate files)";
        p.header = "<p id=\"%p%\">";
        p.footer = "</p>";
        p.encoding = "UTF-8";
        p.elSep = " ";
        p.elStart = "<s id=\"%p%:%pe%\">";
        p.elEnd = "</s>";
        p.parSep = "</p><p id=\"%p%\">";
        p.segSep = "\n";
        p.segStart = "";
        p.segEnd = "";
        p.emptySegFiller = "";
        p.verSep = "";
        p.ext = "txt";
        p.keepPars = true;
        p.keepSegs = true;
        p.skipEmptySeg = false;
        p.skipUnconfirmed = false;
        p.bothTexts = false;
        p.replacements.clear();
        r.src="\n"; r.repl=" ";
        p.replacements.append(r);
        p.customVars.clear();
        exTextProfiles.insert(p.name, p);

        p.name = "Newline aligned (one file)";
        p.header = "<p id=\"%p%\">";
        p.footer = "</p>";
        p.encoding = "UTF-8";
        p.elSep = " ";
        p.elStart = "<s id=\"%p%:%pe%\">";
        p.elEnd = "</s>";
        p.parSep = "</p><p id=\"%p%\">";
        p.segSep = "\n";
        p.segStart = "";
        p.segEnd = "";
        p.emptySegFiller = "";
        p.verSep = "\t";
        p.ext = "txt";
        p.keepPars = true;
        p.keepSegs = true;
        p.skipEmptySeg = false;
        p.skipUnconfirmed = false;
        p.bothTexts = true;
        p.replacements.clear();
        r.src="\n"; r.repl=" ";
        p.replacements.append(r);
        r.src="\t"; r.repl=" ";
        p.replacements.append(r);
        p.customVars.clear();
        exTextProfiles.insert(p.name, p);
    }

    if (exTextProfiles.isEmpty() || version<2) {
        if (version<2) {
            exTextProfiles.remove("TMX (stripped markup)");
        }
        Replacement r;
        CustomVar c;

        ExTextProfile p;

        p.name = "TMX (stripped markup)";
        p.header = "<tmx version=\"1.4b\">\n"
                   "\t<header creationtool=\"InterText\" creationtoolversion=\"1.0\" datatype=\"PlainText\" segtype=\"block\" adminlang=\"en-us\" "
                   "srclang=\"%srclang%\" o-tmf=\"XML aligned text\"></header>\n<body>";
        p.footer = "\n</body>\n</tmx>\n";
        p.encoding = "UTF-8";
        p.elSep = " |#| ";
        p.elStart = "";
        p.elEnd = "";
        p.parSep = "";
        p.segSep = "";
        p.segStart = "\n<tu><prop type=\"x-sentbreak\">|#|</prop>\n\t<tuv xml:lang=\"%lang1%\"><seg>";
        p.segEnd = "</seg></tuv>\n</tu>";
        p.emptySegFiller = "";
        p.verSep = "</seg></tuv>\n\t<tuv xml:lang=\"%lang2%\"><seg>";
        p.ext = "tmx";
        p.keepPars = false;
        p.keepSegs = true;
        p.skipEmptySeg = true;
        p.skipUnconfirmed = false;
        p.bothTexts = true;
        p.replacements.clear();
        r.src="<[^>]+>"; r.repl="";
        p.replacements.append(r);
        p.customVars.clear();
        c.custSymbol = "%srclang%"; c.desc = "Header srclang"; c.defaultVal = "%v1%";
        p.customVars.append(c);
        c.custSymbol = "%lang1%"; c.desc = "Language code for version 1"; c.defaultVal = "%v1%";
        p.customVars.append(c);
        c.custSymbol = "%lang2%"; c.desc = "Language code for version 2"; c.defaultVal = "%v2%";
        p.customVars.append(c);
        exTextProfiles.insert(p.name, p);
    }

    if (!settings->value("alignment",QString()).toString().isEmpty()) {
        statusBar()->showMessage(tr("Loading last session..."));
        open(settings->value("alignment").toString());
    }
    if (model!=0 && settings->value("currow",-1)!=-1 && settings->value("curcol",-1)!=-1) {
        QModelIndex cur = model->index(settings->value("currow").toInt(), settings->value("curcol").toInt(), QModelIndex());
        view->setCurrentIndex(cur);
        //view->scrollTo(cur, QAbstractItemView::PositionAtCenter);
    }

    updateServerMenu();
    updateExTextMenu();
}

void ItWindow::writeSettings()
{
    settings->setValue("config_ver", 2);
    settings->setValue("pos", pos());
    settings->setValue("size", size());
    settings->setValue("autosave", autoSaveInterval);
    /*settings->setValue("default_id_namespace_uri", defaultIdNamespaceURI);*/
    settings->setValue("check_updates_on_startup", QVariant(autoCheckUpdates).toString());
    settings->setValue("enable_cross_order_alignment", QVariant(enableCrossOrderAlignment).toString());
    settings->setValue("show_controls", view->getShowControls());
    settings->setValue("size_controls", view->getSizeControls());
    settings->setValue("autohide_controls", view->controlsHideTimeOut);
    settings->setValue("toolbar_size", toolBarSize);
    settings->setValue("toolbar_area", toolBarArea(toolBar));
    QStringList toolbarConf;
    QAction *a;
    foreach (a, toolBar->actions()) {
        if (a->data().toString().isEmpty())
            toolbarConf.append("|");
        else
            toolbarConf.append(a->data().toString());
    }
    settings->setValue("toolbar_actions", toolbarConf.join(" "));
    QStringList ctxmenuConf;
    foreach (a, contextMenuCurActions) {
        if (!a)
            ctxmenuConf.append("|");
        else
            ctxmenuConf.append(a->data().toString());
    }
    settings->setValue("context_menu_actions", ctxmenuConf.join(" "));
    settings->setValue("auto_save_element", view->getAutoSaveElement());
    settings->setValue("skip_margin", view->skipMargin);
    if (model!=0) settings->setValue("alignment", model->alignment->info.name);
    else settings->setValue("alignment", QString());
    settings->setValue("currow", view->currentIndex().row());
    settings->setValue("curcol", view->currentIndex().column());
    settings->setValue("text_font_family", view->font().family());
    settings->setValue("text_font_size", view->font().pointSize());
    settings->setValue("css_stylesheet", cssStyle);
    settings->setValue("html_view", QVariant(view->htmlView()).toString());
    settings->setValue("highlight_non11", QVariant(view->highlNon11()).toString());
    settings->setValue("highlight_marked", QVariant(view->highlMarked()).toString());
    settings->setValue("fgcolor_default", colors.fgdefault.name());
    settings->setValue("bgcolor_default", colors.bgdefault.name());
    settings->setValue("bgcolor_non11", colors.bgnon11.name());
    settings->setValue("bgcolor_marked", colors.bgmarked.name());
    settings->setValue("even_rows_darkness_change", colors.bgAddDark);
    settings->setValue("bgcolor_cursor", colors.cursor.name());
    settings->setValue("cursor_opacity", colors.cursorOpac);
    settings->setValue("bgcolor_found", colors.bgfound.name());
    settings->setValue("bgcolor_replaced", colors.bgrepl.name());

    settings->beginGroup("shortcuts");
    foreach (a, allActions) {
        if (a->shortcut().toString() != defaultShortcuts.value(a->data().toString()))
            settings->setValue(a->data().toString(), a->shortcut().toString());
        else
            settings->remove(a->data().toString());
    }
    settings->endGroup();

    settings->beginGroup("editor_shortcuts");
    settings->setValue("save_exit", editorKeys.saveExit.toString());
    settings->setValue("discard_exit", editorKeys.discardExit.toString());
    settings->setValue("save_next", editorKeys.saveNext.toString());
    settings->setValue("save_prev", editorKeys.savePrev.toString());
    settings->setValue("save_insert_next", editorKeys.saveInsertNext.toString());
    settings->endGroup();


    settings->beginWriteArray("transformations");
    for (int i=0; i<transformations.size(); i++) {
        settings->setArrayIndex(i);
        Replacement r = transformations.at(i);
        settings->setValue("src", strescape(r.src));
        settings->setValue("repl", strescape(r.repl));
    }
    settings->endArray();

    settings->beginWriteArray("aligners");
    for (int i=0; i<autoAligners.size(); i++) {
        settings->setArrayIndex(i);
        Aligner aligner = autoAligners.at(i);
        settings->setValue("name", aligner.name);
        settings->setValue("exec", aligner.exec);
        settings->setValue("temp_dir", aligner.tmpdir);
        settings->setValue("exp_head", strescape(aligner.exp_head));
        //settings->setValue("exp_segment_separator", aligner.exp_seg_sep);
        settings->setValue("exp_element_separator", strescape(aligner.exp_el_sep));
        settings->setValue("exp_parent_separator", strescape(aligner.exp_parent_sep));
        settings->setValue("exp_foot", strescape(aligner.exp_foot));
        settings->setValue("imp_method", aligner.al_imp_method);
        settings->beginWriteArray("profiles");
        for (int j=0; j<aligner.profiles.size(); j++) {
            settings->setArrayIndex(j);
            AlignerProfile profile = aligner.profiles.at(j);
            settings->setValue("name", profile.name);
            settings->setValue("params", profile.params);
        }
        settings->endArray();
    }
    settings->endArray();

    settings->beginGroup("import");
    settings->setValue("ask_on_txtimport", QVariant(askOnTxtImport).toString());
    settings->setValue("ask_on_xmlimport", QVariant(askOnXmlImport).toString());
    settings->setValue("xmlimport_lock", QVariant(importXmlLock).toString());
    settings->setValue("working_dir", workDir);
    settings->setValue("splitter_on", QVariant(splitSetTxt).toString());
    settings->setValue("splitter_on_xml", QVariant(splitSetXml).toString());
    settings->setValue("alignable_elements", alignableElements);
    settings->setValue("splitter_text_containers", textElements);
    settings->setValue("splitter_new_element", splitterElName);
    settings->setValue("splitter_profile", splitter.selectedProfile);
    settings->setValue("txt_import_encoding", importTxtEncoding);
    settings->setValue("xml_header", strescape(importXmlHeader));
    settings->setValue("xml_footer", strescape(importXmlFooter));
    settings->setValue("empty_doc_template", strescape(emptyDocTemplate));
    settings->setValue("par_separator", strescape(importParSeparator));
    settings->setValue("sentence_separator", strescape(importSentenceSeparator));
    settings->setValue("keep_markup", QVariant(importKeepMarkup).toString());
    settings->setValue("import_format", importFormat);
    settings->setValue("default_num_levels", defaultNumberingLevels);
    settings->endGroup();

    settings->beginWriteArray("splitter_profiles");
    QList<ItSentenceSplitter::SplitterProfile> sprofiles = splitter.getProfiles();
    for (int i=0; i<sprofiles.size(); i++) {
        settings->setArrayIndex(i);
        ItSentenceSplitter::SplitterProfile p = sprofiles.at(i);
        settings->setValue("name", p.name);
        settings->beginWriteArray("expr");
        for (int j=0; j<p.expressions.size(); j++) {
            settings->setArrayIndex(j);
            ItSentenceSplitter::Replacement r = p.expressions.at(j);
            settings->setValue("src", strescape(r.src));
            settings->setValue("repl", strescape(r.repl));
        }
        settings->endArray();
        settings->setValue("abbrevs", p.abbrevs);
    }
    settings->endArray();

    QList<ItServer> serverlist = servers.values();
    settings->beginWriteArray("servers");
    for (int i=0; i<serverlist.size(); i++) {
        settings->setArrayIndex(i);
        ItServer server = serverlist.at(i);
        settings->setValue("name", server.name);
        settings->setValue("url", server.url);
        settings->setValue("user", server.username);
        settings->setValue("passwd", crypto.encryptToString(server.passwd));
        settings->setValue("check_updates", QVariant(server.autoUpdateCheck).toString());
    }
    settings->endArray();

    settings->beginGroup("sync");
    settings->setValue("mark_changes", QVariant(syncMarkChanges).toString());
    settings->endGroup();

    settings->beginWriteArray("text_export_profiles");
    QStringList keys = exTextProfiles.keys();
    for (int i=0; i<exTextProfiles.size(); i++) {
        settings->setArrayIndex(i);
        ExTextProfile p = exTextProfiles.value(keys[i]);
        settings->setValue("name", p.name);
        settings->setValue("header", strescape(p.header));
        settings->setValue("footer", strescape(p.footer));
        settings->setValue("extension", p.ext);
        settings->setValue("encoding", p.encoding);
        settings->setValue("element_start", strescape(p.elStart));
        settings->setValue("element_end", strescape(p.elEnd));
        settings->setValue("element_separator", strescape(p.elSep));
        settings->setValue("segment_start", strescape(p.segStart));
        settings->setValue("segment_end", strescape(p.segEnd));
        settings->setValue("empty_segment_filler", strescape(p.emptySegFiller));
        settings->setValue("segment_separator", strescape(p.segSep));
        settings->setValue("parent_separator", strescape(p.parSep));
        settings->setValue("version_separator", strescape(p.verSep));
        settings->setValue("both_texts", QVariant(p.bothTexts).toString());
        settings->setValue("keep_parbreaks", QVariant(p.keepPars).toString());
        settings->setValue("keep_segments", QVariant(p.keepSegs).toString());
        settings->setValue("skip_empty_segs", QVariant(p.skipEmptySeg).toString());
        settings->setValue("skip_unconfirmed", QVariant(p.skipUnconfirmed).toString());
        settings->beginWriteArray("replacements");
        for (int j=0; j<p.replacements.size(); j++) {
            settings->setArrayIndex(j);
            Replacement r = p.replacements.at(j);
            settings->setValue("src", strescape(r.src));
            settings->setValue("repl", strescape(r.repl));
        }
        settings->endArray();
        settings->beginWriteArray("custom_variables");
        for (int j=0; j<p.customVars.size(); j++) {
            settings->setArrayIndex(j);
            CustomVar c = p.customVars.at(j);
            settings->setValue("symbol", strescape(c.custSymbol));
            settings->setValue("desc", strescape(c.desc));
            settings->setValue("default", strescape(c.defaultVal));
        }
        settings->endArray();
    }
    settings->endArray();
}

QStringList ItWindow::scanDataDir(const QString &dir) {
    QStringList list;
    QDir datadir(dir);
    if (!datadir.exists()) {
        if (!datadir.mkpath(dir)) {
            QMessageBox::critical(this, tr("Error"), tr("Directory %1 does not exist and cannot be created.").arg(dir));
            return list;
        }
    }
    datadir.setNameFilters(QStringList("*.conf"));
    list = datadir.entryList();
    list.replaceInStrings(QRegExp(".conf$"),"");
    return list;
}

bool ItWindow::maybeSave()
{
    if (model==0) return true;
    if (segview!=0) view->closeEditor(segview, QAbstractItemDelegate::NoHint);
    if (!model->undoStack->isClean()) {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("Quitting InterText"),
                                   tr("The document has been modified.\n"
                                      "Do you want to save the changes?"),
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save)
            return save();
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    return true;
}

void ItWindow::dataChanged() {
    if (!model->undoStack->isClean()) setWindowTitle(QObject::tr("%1 - InterText *").arg(model->alignment->info.docId));
    else setWindowTitle(QObject::tr("%1 - InterText").arg(model->alignment->info.docId));
    if (model->undoStack->canUndo()) {
        undoAct->setText(tr("&Undo: %1").arg(model->undoStack->undoText()));
        undoAct->setEnabled(true);
    } else {
        undoAct->setText(tr("&Undo"));
        undoAct->setEnabled(false);
    }
    if (model->undoStack->canRedo()) {
        redoAct->setText(tr("&Redo: %1").arg(model->undoStack->redoText()));
        redoAct->setEnabled(true);
    } else {
        redoAct->setText(tr("&Redo"));
        redoAct->setEnabled(false);
    }
    if (model->undoStack->isClean())
        saveAct->setEnabled(false);
    else
        saveAct->setEnabled(true);
    updateInfoBar();
}

void ItWindow::enableEditAct(bool en) {
    editAct->setEnabled(en);
    editXmlAct->setEnabled(en);
}

void ItWindow::updateActions() {
    QModelIndex cur;
    //if (!view->hasFocus() && (segview==0 || !segview->hasFocus())) {
    if (!view->currentIndex().isValid() && (segview==0 || !segview->hasFocus())) {
        moveUpAct->setEnabled(false);
        moveDownAct->setEnabled(false);
        shiftAct->setEnabled(false);
        popAct->setEnabled(false);
        swapAct->setEnabled(false);
        mergeAct->setEnabled(false);
        splitParentAct->setEnabled(false);
        mergeParentAct->setEnabled(false);
        moveBUpAct->setEnabled(false);
        moveBDownAct->setEnabled(false);
        moveTextAct->setEnabled(false);
        toggleMarkAct->setEnabled(false);
        toggleStatusAct->setEnabled(false);
        confirmAct->setEnabled(false);
        enableEditAct(false);
        insertAct->setEnabled(false);
        return;
    }
    if (segview==0) {
        /* TableView */
        if (!view->isEditing())
            cur = view->currentIndex();
        if (cur.isValid() && model->rowCount(cur)==2 && model->canMerge(model->index(0,0,cur))) {
            mergeAct->setText(tr("Merge both elements"));
            mergeAct->setToolTip(tr("Merge both elements within the segment"));
            mergeAct->setEnabled(true);
        } else if (cur.isValid() && model->rowCount(cur)>2) {
            mergeAct->setText(tr("Merge elements"));
            mergeAct->setToolTip(tr("Select an element within the segment to be merged with the preceding one"));
            mergeAct->setEnabled(true);
        } else {
            mergeAct->setText(tr("Merge is not possible"));
            mergeAct->setToolTip(tr("Merge is not possible"));
            mergeAct->setEnabled(false);
        }
        if (cur.isValid() && (cur.column()==1 || cur.column()==2)) {
            insertAct->setEnabled(true);
        } else {
            insertAct->setEnabled(false);
        }
        if (cur.isValid() && (cur.column()==1 || cur.column()==2) && !model->data(cur, Qt::DisplayRole).toString().isEmpty()) {
            if (cur.row()!=0) {
                moveUpAct->setEnabled(true);
                if (crossOrderAlignmentAllowed())
                    swapAct->setEnabled(true);
                else
                    swapAct->setEnabled(false);
            } else {
                moveUpAct->setEnabled(false);
                swapAct->setEnabled(false);
            }
            moveDownAct->setEnabled(true);
            if (cur.row()!=0)
                shiftAct->setEnabled(true);
            else
                shiftAct->setEnabled(false);
            popAct->setEnabled(true);
            if (model->canSplitParent(cur))
                splitParentAct->setEnabled(true);
            else
                splitParentAct->setEnabled(false);
            if (model->canMergeParent(cur))
                mergeParentAct->setEnabled(true);
            else
                mergeParentAct->setEnabled(false);
            moveTextAct->setEnabled(true);
        } else {
            moveUpAct->setEnabled(false);
            moveDownAct->setEnabled(false);
            moveTextAct->setEnabled(false);
            shiftAct->setEnabled(false);
            popAct->setEnabled(false);
            swapAct->setEnabled(false);
            splitParentAct->setEnabled(false);
            mergeParentAct->setEnabled(false);
        }
        if (cur.isValid()) {
            if (cur.row()!=0)
                moveBUpAct->setEnabled(true);
            else
                moveBUpAct->setEnabled(false);
            moveBDownAct->setEnabled(true);
            toggleMarkAct->setEnabled(true);
            toggleStatusAct->setEnabled(true);
            confirmAct->setEnabled(true);
            if (cur.flags() & Qt::ItemIsEditable)
                enableEditAct(true);
            else
                enableEditAct(false);
        } else {
            moveBUpAct->setEnabled(false);
            moveBDownAct->setEnabled(false);
            toggleMarkAct->setEnabled(false);
            toggleStatusAct->setEnabled(false);
            confirmAct->setEnabled(false);
            enableEditAct(false);
        }
    } else {
        /* SegView */
        cur = segview->currentIndex();
        ItAlignmentModel * almodel = static_cast<ItAlignmentModel*>(segview->model());
        moveBUpAct->setEnabled(false);
        moveBDownAct->setEnabled(false);
        moveTextAct->setEnabled(false);
        moveUpAct->setEnabled(false);
        swapAct->setEnabled(false);
        insertAct->setEnabled(true);
        moveDownAct->setEnabled(false);
        if (cur.row()==0 && cur.parent().row()!=0)
            shiftAct->setEnabled(true);
        else
            shiftAct->setEnabled(false);
        if (cur.row()==model->rowCount(cur.parent())-1)
            popAct->setEnabled(true);
        else
            popAct->setEnabled(false);
        if (almodel->canSplitParent(cur))
            splitParentAct->setEnabled(true);
        else
            splitParentAct->setEnabled(false);
        if (almodel->canMergeParent(cur))
            mergeParentAct->setEnabled(true);
        else
            mergeParentAct->setEnabled(false);
        QModelIndex previous = almodel->index(cur.row()-1, cur.column(), cur.parent());
        if (previous.isValid() && almodel->canMerge(previous)) {
            mergeAct->setText(tr("Merge with preceding element"));
            mergeAct->setToolTip(tr("Merge element with its preceding sibling"));
            mergeAct->setEnabled(true);
        } else {
            mergeAct->setText(tr("Merge is not possible"));
            mergeAct->setToolTip(tr("Merge is not possible"));
            mergeAct->setEnabled(false);
        }
        if (cur.isValid() &&
                ((cur.parent().column()==1 && model->alignment->info.ver[0].perm_chtext) ||
                 (cur.parent().column()==2 && model->alignment->info.ver[1].perm_chtext)))
            enableEditAct(true);
        else
            enableEditAct(false);
    }
}

void ItWindow::updateInfoBar(QString msg)
{
    if (model==0)
        return;
    statusBar()->clearMessage();
    if (!msg.isEmpty()) {
        infoBar->setText(msg);
        return;
    }

    int s [6] = {0,0,0,0,-1,0};
    model->status(s);
    double proc [4];
    proc[0] = 100;
    proc[1] = (s[1]*1.0/s[0])*100;
    proc[2] = (s[2]*1.0/s[0])*100;
    proc[3] = (s[3]*1.0/s[0])*100;
    QString firstunconf;
    if (s[4]>=0)
        firstunconf = tr("; first unconfirmed segment: %1").arg(QString::number(s[4]+1));
    infoBar->setText(tr("Segments: %1; confirmed: %2 (%3\%); auto: %4 (%5\%); unconfirmed: %6 (%7\%)%8; bookmarks: %9")
                     .arg(QString::number(s[0]),
                     QString::number(s[1]), QString::number(proc[1],'f',0),
            QString::number(s[2]), QString::number(proc[2],'f',0),
            QString::number(s[3]), QString::number(proc[3],'f',0),
            firstunconf, QString::number(s[5])));
}

void ItWindow::setSegView(ItSegmentView * cursegview) {
    segview = cursegview;
    if (cursegview!=0) {
        connect(segview, SIGNAL(cursorChanged()), this, SLOT(updateActions()));
        connect(segview, SIGNAL(focusChanged()), this, SLOT(updateActions()));
    }
}

void ItWindow::toggleAutoUpdateStatus() {
    if (model==0) return;
    if (model->updateStat()) {
        model->setUpdateStat(false);
        updateStatAct->setChecked(false);
    } else {
        model->setUpdateStat(true);
        updateStatAct->setChecked(true);
    }
}

void ItWindow::toggleHtmlView()
{
    bool set = true;
    if (view->htmlView()) {
        set = false;
        htmlViewAct->setChecked(false);
    } else {
        htmlViewAct->setChecked(true);
    }
    view->setHtmlView(set);
    if (model)
        model->setHtmlViewMode(set);
    resetSearchResults();
}

void ItWindow::findNext()
{
    search_wrapper(true);
}

void ItWindow::findPrev()
{
    search_wrapper(false);
}

void ItWindow::search_wrapper(bool forward, bool fromzero, bool silent)
{
    if (searchBar->emptySearch())
        return;
    if ((searchBar->getSearchType()==ItSearchBar::RegExp || searchBar->getSearchType()==ItSearchBar::RegExpCS) &&
            !QRegularExpression(searchBar->getSearchString()).isValid()) {
        QMessageBox::warning(this, tr("Search"), tr("Invalid regular expression."));
        return;
    }
    uint pos = 0;
    searchBar->addCurrentQuery();
    /*if (view->currentIndex().row()!=model->alignment->lastMatch.pos)*/
    if (!fromzero) {
        if (view->currentIndex().isValid())
            pos = view->currentIndex().row();
        if (!newSearchQuery(pos) && model->alignment->lastMatch.set)
            pos = INVALID_POSITION;
    }
    //;
    find(pos, forward, searchBar->getSearchType(), searchBar->getSearchSide(), searchBar->getSearchString(), silent);
}

void ItWindow::findFirstBookmark()
{
    find(0, true, ItSearchBar::Bookmark);
}

void ItWindow::findNextBookmark()
{
    if (view->currentIndex().isValid() && view->currentIndex().row()!=model->rowCount()-1)
        find(view->currentIndex().row()+1, true, ItSearchBar::Bookmark);
    else
        find(0, true, ItSearchBar::Bookmark);
}

void ItWindow::findPrevBookmark()
{
    if (view->currentIndex().isValid() && view->currentIndex().row()!=0)
        find(view->currentIndex().row()-1, false, ItSearchBar::Bookmark);
    else
        find(model->rowCount()-1, false, ItSearchBar::Bookmark);
}

void ItWindow::findFirstNon11()
{
    find(0, true, ItSearchBar::Non1Seg);
}

void ItWindow::findNextNon11()
{
    if (view->currentIndex().isValid() && view->currentIndex().row()!=model->rowCount()-1)
        find(view->currentIndex().row()+1, true, ItSearchBar::Non1Seg);
    else
        find(0, true, ItSearchBar::Non1Seg);
}

void ItWindow::findPrevNon11()
{
    if (view->currentIndex().isValid() && view->currentIndex().row()!=0)
        find(view->currentIndex().row()-1, false, ItSearchBar::Non1Seg);
    else
        find(model->rowCount()-1, false, ItSearchBar::Non1Seg);
}

void ItWindow::findFirstUnconfirmed()
{
    find(0, true, ItSearchBar::UnConfirmed);
}

void ItWindow::findNextUnconfirmed()
{
    if (view->currentIndex().isValid() && view->currentIndex().row()!=model->rowCount()-1)
        find(view->currentIndex().row()+1, true, ItSearchBar::UnConfirmed);
    else
        find(0, true, ItSearchBar::UnConfirmed);
}

void ItWindow::findPrevUnconfirmed()
{
    if (view->currentIndex().isValid() && view->currentIndex().row()!=0)
        find(view->currentIndex().row()-1, false, ItSearchBar::UnConfirmed);
    else
        find(model->rowCount()-1, false, ItSearchBar::UnConfirmed);
}

void ItWindow::find(uint startpos, bool forward, ItSearchBar::searchType stype, ItSearchBar::searchSide side, QString str, bool silent)
{
    if (!model)
        return;
    QModelIndex index = model->find(startpos, forward, side, stype, str);
    //qDebug()<<"Got index"<<index.row()<<index.column()<<index.parent().row()<<index.parent().column();
    if (model->canReplace() && !view->htmlView())
        searchBar->enableReplace(true);
    else
        searchBar->enableReplace(false);
    if (index.isValid()) {
    } else {
        if (forward && lastSearch.startpos!=0) {
            QMessageBox::StandardButton resp = QMessageBox::question(this, tr("Search"),
                                                                     tr("No more matches until the end of the text. Do you want to start search from the beginning again?"),
                                                                     QMessageBox::Ok|QMessageBox::Cancel);
            if (resp == QMessageBox::Ok) {
                lastSearch.startpos = 0;
                find(0, forward, stype, side, str);
            }
        } else if (!forward && lastSearch.startpos!=model->rowCount()-1) {
            QMessageBox::StandardButton resp = QMessageBox::question(this, tr("Search"),
                                                                     tr("No more matches until the beginning of the text. Do you want to start search from the end again?"),
                                                                     QMessageBox::Ok|QMessageBox::Cancel);
            if (resp == QMessageBox::Ok) {
                lastSearch.startpos = model->rowCount()-1;
                find(model->rowCount()-1, forward, stype, side, str);
            }
        } else if (!silent) {
            QMessageBox::information(this, tr("Search"),
                                     tr("Not found."), QMessageBox::Ok);
        }
    }
}

void ItWindow::replace()
{
    if (!model)
        return;
    if (model->replace(searchBar->getReplacementString()))
        searchBar->enableReplace(false);
}

void ItWindow::replFind()
{
    if (!model)
        return;
    if (model->canReplace() && !newSearchQuery()) {
        replace();
        findNext();
    } else {
        findNext();
    }
}

void ItWindow::replaceAll()
{
    if (searchBar->emptySearch())
        return;
    if (!model)
        return;

#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    uint count = 0;
    statusBar()->showMessage(tr("Replace all in progress..."),0);

    //QPushButton * stop = new QPushButton(tr("Stop"),this);
    //connect(stop, SIGNAL(clicked()), model, SLOT(stopButtonPress()));
    //statusBar()->addPermanentWidget(stop);

    //ReplaceAllThread * thread = new ReplaceAllThread(this);
    //thread->start();

    /* // replaceAll at this level
  search_wrapper(true, true);
  if (model->canReplace()) {
    double prog = 0;
    uint max = model->rowCount();
    uint cur = 0;
    stopButtonPressed = false;
    model->undoStack->beginMacro("Replace all");
    while (model->canReplace() && !stopButtonPressed) {
      cur = model->lastMatchIndex.parent().row();
      prog = ((cur*1.0) / max)*100;
      statusBar()->showMessage(tr("Replace all in progress... %1% (segment: %2 of %3; occurrence: %4)").arg(QString::number(prog,'f',1),
                                                                                                            QString::number(cur),
                                                                                                            QString::number(max),
                                                                                                            QString::number(count)),0);
      model->replace(searchBar->getReplacementString()); //replace();
      count++;
      //search_wrapper(true, false, true); //findNext();
      //find(INVALID_POSITION, true, searchBar->getSearchType(), searchBar->getSearchSide(), searchBar->getSearchString(), true);
      model->find(INVALID_POSITION, true, searchBar->getSearchSide(), searchBar->getSearchType(), searchBar->getSearchString());
    }
    model->undoStack->endMacro();
    //delete stop;
  }
*/

    // call the model to replace all
    if ((searchBar->getSearchType()==ItSearchBar::RegExp || searchBar->getSearchType()==ItSearchBar::RegExpCS) &&
            !QRegularExpression(searchBar->getSearchString()).isValid()) {
#ifndef QT_NO_CURSOR
        QApplication::restoreOverrideCursor();
#endif
        QMessageBox::warning(this, tr("Search"), tr("Invalid regular expression."));
        return;
    }
    newSearchQuery(0, true);
    searchBar->addCurrentQuery();
    if (!model->replaceAll(&count, searchBar->getSearchSide(), searchBar->getSearchType(), searchBar->getSearchString(), searchBar->getReplacementString()))
        QMessageBox::information(this, tr("Search"),
                                 tr("Not found."), QMessageBox::Ok);

#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    //statusBar()->removeWidget(stop);
    //delete stop;
    statusBar()->showMessage(tr("Ready."),500);
    if (count>0)
        QMessageBox::information(this, tr("Replace"),
                                 tr("Replaced %1 occurrences.").arg(QString::number(count)), QMessageBox::Ok);
}

void ItWindow::resizeEvent ( QResizeEvent * event )
{
    QMainWindow::resizeEvent(event);
    if (model!=0)
        view->updateRowSize();
}

void ItWindow::resetSearchResults()
{
    if (!model)
        return;
    model->resetLastMatch();
    model->replaceHistoryClear();
    model->resetFilter();
}


bool ItWindow::newSearchQuery(uint startpos, bool force)
{
    if (!model)
        return true;
    if (model->alignment->lastMatch.set && lastSearch.str==searchBar->getSearchString() && lastSearch.stype==searchBar->getSearchType() && (model->alignment->lastMatch.pos == startpos || startpos == INVALID_POSITION)) {
        if (force)
            model->replaceHistoryClear();
        return false;
    } else {
        resetSearchResults();
        lastSearch.str=searchBar->getSearchString();
        lastSearch.stype=searchBar->getSearchType();
        lastSearch.startpos = startpos;
        return true;
    }
}

void ItWindow::setFilter()
{
    if (!model)
        return;
    if (!QRegularExpression(searchBar->getSearchString()).isValid()) {
        QMessageBox::warning(this, tr("Search"), tr("Invalid regular expression."));
        return;
    }
    model->setFilter(searchBar->getSearchString(), searchBar->getSearchType(), searchBar->getSearchSide());
}

void ItWindow::receiveUpdateFailure(QModelIndex idx)
{
    //model->undoStack->undo();
    QString side;
    if (idx.parent().column()==1)
        side = tr("left side document");
    else
        side = tr("right side document");
    QMessageBox::critical(this,
                          tr("Update"),
                          tr("Update failed for %1 on position %2, element %3. Invalid XML structure.").arg(side,
                                                                                                            QString::number(idx.parent().row()+1),
                                                                                                            QString::number(idx.row()+1)));
}

void ItWindow::undo()
{
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    statusBar()->showMessage(tr("Undo in progress..."),0);
    model->undoStack->undo();
    statusBar()->showMessage(tr("Ready."),500);
    //updateInfoBar();
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
}

void ItWindow::redo()
{
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    statusBar()->showMessage(tr("Redo in progress..."),0);
    model->undoStack->redo();
    statusBar()->showMessage(tr("Ready."),500);
    //updateInfoBar();
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
}

void ItWindow::setTextFont(QFont &font)
{
    view->setFont(font);
    view->optimizeSize(statusBar());
}

void ItWindow::autoAlign()
{
    AutoAlignDialog * form = new AutoAlignDialog(this, model->rowCount());
    Aligner aligner;
    foreach(aligner, autoAligners) {
        QStringList myprofiles;
        AlignerProfile profile;
        foreach (profile, aligner.profiles)
            myprofiles.append(profile.name);
        form->addAligner(aligner.name, myprofiles);
    }
    if (!view->currentIndex().isValid())
        form->setStartPos(1);
    else
        form->setStartPos(view->currentIndex().row()+1);
    form->setEndPos(model->rowCount());
    form->exec();
    if (!form->result()) {
        delete form;
        return;
    }
    aligner = autoAligners.at(form->getAligner());
    AlignerProfile profile;
    if (aligner.profiles.size())
        profile = aligner.profiles.at(form->getProfile());
    int startPos = form->getStartPos()-1;
    if (startPos<0)
        startPos = 0;
    if (startPos>model->rowCount())
        startPos = model->rowCount();
    int endPos = form->getEndPos()-1;
    if (endPos>=model->rowCount())
        endPos = model->rowCount()-1;
    if (endPos<startPos)
        endPos = startPos;
    bool autoClose = form->getAutoClose();
    delete form;

    if (aligner.al_imp_method == PlainAligner) {
        applyPlainAlign(startPos, endPos);
        return;
    }

    QString mykey = QString::number(qrand(),16);
    QString filename1 = QString("%1/InterText_%2_txt1.txt").arg(aligner.tmpdir, mykey);
    QString filename2 = QString("%1/InterText_%2_txt2.txt").arg(aligner.tmpdir, mykey);
    //QString outfilename = QString("%1/InterText_%2_out.txt").arg(aligner.tmpdir, mykey);
    if (!model->alignment->export_text(filename1, 0, startPos, endPos,
                                       aligner.exp_head, aligner.exp_el_sep, aligner.exp_parent_sep, aligner.exp_foot)) {
        QMessageBox::critical(this, tr("Automatic alignment"), model->alignment->errorMessage);
        return;
    }
    if (!model->alignment->export_text(filename2, 1, startPos, endPos,
                                       aligner.exp_head, aligner.exp_el_sep, aligner.exp_parent_sep, aligner.exp_foot)) {
        QMessageBox::critical(this, tr("Automatic alignment"), model->alignment->errorMessage);
        QFile::remove(filename1);
        return;
    }
    QString exec(aligner.exec);
    if (!exec.startsWith(QDir::separator())) // relative path is relative to the executable
        exec.prepend(QCoreApplication::applicationDirPath().append(QDir::separator()));
    QFileInfo einfo(exec);
    QString exepath = einfo.path();
    QString args = profile.params;
    filename1.prepend("\"").append("\"");
    filename2.prepend("\"").append("\"");
    exepath.prepend("\"").append("\"");
    args.replace("{EXEPATH}", exepath).replace("{TXT1}", filename1).replace("{TXT2}", filename2); //.replace("{OUT}", outfilename);

    AlignerView * alview = new AlignerView(exec, args, filename1, filename2, startPos, endPos, autoClose, this);
    if (aligner.al_imp_method == HunalignLadder)
        connect(alview, SIGNAL(result(QString,int,int)), this, SLOT(applyHunalignLadder(QString,int,int)));
    alview->run_aligner();
    return;
}

void ItWindow::applyHunalignLadder(QString output, int fromPos, int toPos)
{
    if (!output.contains("\n")) {
        QMessageBox::critical(this, tr("Automatic alignment"), tr("Error: No results."));
        return;
    }

    // get IDs of aligned elements
    QStringList list1, list2, ids1, ids2;
    for (int pos=fromPos; pos<=toPos; pos++) {
        list1 = model->alignment->getIDs(0, pos);
        list2 = model->alignment->getIDs(1, pos);
        for (int el=0; el<list1.size(); el++ ) {
            if (model->alignment->isFirst(0, pos, el))
                ids1 << "";
            ids1 << list1.at(el);
        }
        for (int el=0; el<list2.size(); el++ ) {
            if (model->alignment->isFirst(1, pos, el))
                ids2 << "";
            ids2 << list2.at(el);
        }
    }

    // create alignment of IDs from the hunalign ladder
    QList<QStringList> alignedIDs [2];
    QString line;
    QStringList items;
    int p1 = 0, p2 = 0, v1p = 0, v2p = 0;
    foreach(line, output.split('\n', QString::SkipEmptyParts)) {
        items = line.split('\t');
        if (items.size()!=3) {
            qDebug()<<"Wrong line:"<<line;
            continue;
        }
        v1p = items.at(0).toInt();
        v2p = items.at(1).toInt();
        QStringList v1ids, v2ids;
        while (p1<v1p) {
            if (!ids1.at(p1).isEmpty())
                v1ids.append(ids1.at(p1));
            p1++;
        }
        while (p2<v2p) {
            if (!ids2.at(p2).isEmpty())
                v2ids.append(ids2.at(p2));
            p2++;
        }
        if (v1ids.size()>0 || v2ids.size()>0) {
            alignedIDs[0].append(v1ids);
            alignedIDs[1].append(v2ids);
        }
    }

    // apply new alignment
    if (!view->realign(fromPos, toPos, alignedIDs)) {
        QMessageBox::critical(this, tr("Automatic alignment"), tr("Error: %1").arg(model->alignment->errorMessage));
    }

    return;
}

void ItWindow::applyPlainAlign(int fromPos, int toPos)
{
    // get IDs of aligned elements
    QStringList list1, list2;
    QList<QStringList> alignedIDs [2];
    for (int pos=fromPos; pos<=toPos; pos++) {
        list1 = model->alignment->getIDs(0, pos);
        list2 = model->alignment->getIDs(1, pos);
        for (int el=0; el<list1.size(); el++ ) {
            alignedIDs[0].append(QStringList(list1.at(el)));
        }
        for (int el=0; el<list2.size(); el++ ) {
            alignedIDs[1].append(QStringList(list2.at(el)));
        }
    }

    while (alignedIDs[0].size()<alignedIDs[1].size()) {
        alignedIDs[0].append(QStringList());
    }
    while (alignedIDs[0].size()>alignedIDs[1].size()) {
        alignedIDs[1].append(QStringList());
    }

    // apply new alignment
    if (!view->realign(fromPos, toPos, alignedIDs)) {
        QMessageBox::critical(this, tr("Automatic alignment"), tr("Error: %1").arg(model->alignment->errorMessage));
    }

    return;
}

void ItWindow::enableActions(bool en)
{
    findNextBmAct->setEnabled(en);
    findPrevBmAct->setEnabled(en);
    findBmAct->setEnabled(en);
    findNextNon11Act->setEnabled(en);
    findPrevNon11Act->setEnabled(en);
    findNon11Act->setEnabled(en);
    findNextUnconfAct->setEnabled(en);
    findPrevUnconfAct->setEnabled(en);
    findUnconfAct->setEnabled(en);
    findAct->setEnabled(en);
    findNextAct->setEnabled(en);
    findPrevAct->setEnabled(en);
    replaceAct->setEnabled(en);
    autoAlignAct->setEnabled(en);
    //saveAct->setEnabled(en);
    closeAct->setEnabled(en);
    fexportAct->setEnabled(en);
    exTextActGroup->setEnabled(en);
    alPropAct->setEnabled(en);
    syncAct->setEnabled(en);
}

void ItWindow::showAlMan()
{
    alManager->show();
}

void ItWindow::editSettings()
{
    SettingsDialog * d = new SettingsDialog(this);
    d->show();
    connect(d, SIGNAL(accepted()), this, SLOT(updateServerMenu()));
}

void ItWindow::customize()
{
    CustomizeDialog * d = new CustomizeDialog(this);
    d->show();
}


void ItWindow::toggleHighlNon11()
{
    if (view->highlNon11()) {
        view->setHighlNon11(false);
        highlNon11Act->setChecked(false);
    } else {
        view->setHighlNon11(true);
        highlNon11Act->setChecked(true);
    }
}

void ItWindow::toggleHighlMarked()
{
    if (view->highlMarked()) {
        view->setHighlMarked(false);
        highlMarkedAct->setChecked(false);
    } else {
        view->setHighlMarked(true);
        highlMarkedAct->setChecked(true);
    }
}

void ItWindow::updateColors(Colors c)
{
    colors = c;
    if (model)
        model->setColors(c);
}

void ItWindow::serverDialog(QString name)
{
    ItServer s = servers.value(name);
    ServerDialog * d = new ServerDialog(this, storagePath, s.url, s.username, s.passwd);
    connect(d, SIGNAL(alDeletedInRepo(QString)), this, SLOT(alignmentDeletedInRepo(QString)));
    connect(d, SIGNAL(alRepoChanged()), alManager, SLOT(externalChange()));
    d->connectToServer();
}

void ItWindow::sync()
{
    if (model==0)
        return;
    save();
    ItServer s;
    QMapIterator<QString, ItServer> i(servers);
    while (s.url!=model->alignment->info.source && i.hasNext()) {
        i.next();
        s = i.value();
    }
    if (s.url!=model->alignment->info.source) {
        QMessageBox::critical(this, tr("Error"), tr("No server configuration for URL '%1'. Check your settings.").arg(model->alignment->info.source));
        return;
    }
    statusBar()->insertWidget(1,progressBar,5);
    progressBar->setMaximumHeight(infoBar->height());
    progressBar->show();
    ServerDialog * d = new ServerDialog(this, storagePath, s.url, s.username, s.passwd, true);
    connect(d, SIGNAL(alDeletedInRepo(QString)), this, SLOT(alignmentDeletedInRepo(QString)));
    connect(d, SIGNAL(alRepoChanged()), alManager, SLOT(externalChange()));
    connect(d, SIGNAL(reloadNeeded()), this, SLOT(reloadAlignment()));
    connect(d, SIGNAL(statusChanged(QString)), this, SLOT(updateInfoBar(QString)));
    connect(d, SIGNAL(syncFinished()), this, SLOT(syncFinished()));
    connect(d, SIGNAL(settingProgressBarRange(int,int)), this, SLOT(setProgressBarRange(int,int)));
    connect(d, SIGNAL(settingProgressBarValue(int)), this, SLOT(setProgressBarValue(int)));
    d->connectToServer();
    if (!d->isConnected) {
        QMessageBox::critical(this, tr("Error"), tr("Cannot connect to the server."));
        delete d;
        return;
    }
    d->syncCurrentAlignment();
}

void ItWindow::reloadAlignmentSilently()
{
    reloadAlignment(false);
}

void ItWindow::reloadAlignment(bool warn)
{
    if (warn)
        QMessageBox::critical(this, tr("Error"), tr("Synchronization aborted. Returning to the last safe state."));
    QString name(model->alignment->info.name);
    open(name, true);
}

void ItWindow::checkForServerUpdates(ItAlignment * a, int d)
{
    ItServer s;
    //bool disassoc = false;
    /*if (a->info.source.startsWith("http") && getServerConfig(a->info.source, &s)) {
      updateInfoBar(tr("Checking for text changes on the server..."));
      statusBar()->insertWidget(1,progressBar,5);
      setProgressBarRange(0,0);
      progressBar->setMaximumHeight(infoBar->height());
      progressBar->show();
      ServerDialog * d = new ServerDialog(this, storagePath, s.url, s.username, s.passwd, true, true);
      connect(d, SIGNAL(statusChanged(QString)), statusBar(), SLOT(showMessage(QString)));
      connect(d, SIGNAL(alDeletedInRepo(QString)), this, SLOT(alignmentDeletedInRepo(QString)));
      connect(d, SIGNAL(alRepoChanged()), alManager, SLOT(externalChange()));
      statusBar()->clearMessage();
      statusBar()->removeWidget(progressBar);
      updateInfoBar();
      if (d->disconnected()) {
        delete d;
        return;
      }
      ServerDialog::RemoteAlignment ra = d->getRemAlignmentInfo(a);
      delete d;
      if (ra.text.isEmpty() && d->lastErrCode==ERR_NOT_FOUND) {
          if (QMessageBox::question(this, tr("Update"),
                                    tr("Alignment has been removed from the server and cannot be synchronized anymore. "
                                       "Do you want to remove all associations?"), QMessageBox::Ok|QMessageBox::No)==QMessageBox::Ok) {
              a->info.source = QString("removed_from_").append(a->info.source);
              disassoc = true;
              a->save();
          }
      }
      if (ra.v1Stat==ServerDialog::Obsolete || ra.v2Stat==ServerDialog::Obsolete) {
        int res = QMessageBox::question(this, tr("Update"),
                                        tr("Text has been changed on the server. Do you want to synchronize now?"), QMessageBox::Ok|QMessageBox::No);
        if (res==QMessageBox::Ok) {
          sync();
          return;
        }
      }
  }*/
    /*if (!a->info.source.startsWith("http"))
  {
      for (int d=0; d<2; d++) {
      if (a->info.ver[d].source.startsWith("http") && getServerConfig(a->info.ver[d].source, &s)) {
          updateInfoBar(tr("Checking for text changes on the server..."));
          statusBar()->insertWidget(1,progressBar,5);
          setProgressBarRange(0,0);
          progressBar->setMaximumHeight(infoBar->height());
          progressBar->show();
          ServerDialog * sd = new ServerDialog(this, storagePath, s.url, s.username, s.passwd, true, true);
          connect(sd, SIGNAL(statusChanged(QString)), statusBar(), SLOT(showMessage(QString)));
          connect(sd, SIGNAL(alDeletedInRepo(QString)), this, SLOT(alignmentDeletedInRepo(QString)));
          connect(sd, SIGNAL(alRepoChanged()), alManager, SLOT(externalChange()));
          statusBar()->clearMessage();
          statusBar()->removeWidget(progressBar);
          updateInfoBar();
          if (sd->disconnected()) {
            delete sd;
            return;
          }
          if (sd->docGetLastChange(a->info.docId, a->info.ver[d].name) > a->info.ver[d].synced) {
              int res = QMessageBox::question(this, tr("Update"),
                                              tr("Document version '%1' has been changed on the server. Do you want to update now?").arg(a->info.ver[d].name),
                                              QMessageBox::Ok|QMessageBox::No);
              if (res==QMessageBox::Ok) {
                if (!syncDoc(a, sd, d))
                  reloadAlignment();
                  return;
              }
          } else if (sd->lastErrCode==ERR_NOT_FOUND) {
              if (disassoc || QMessageBox::question(this, tr("Update"),
                                                    tr("Document version '%1' has been removed from the server and cannot be synchronized anymore. "
                                                       "Do you want to remove all associations?").arg(a->info.ver[d].name),
                                                    QMessageBox::Ok|QMessageBox::No)==QMessageBox::Ok) {
                  a->setDocDepSource(d, QString("removed_from_").append(a->info.ver[d].source));
                  a->save();
                  disassoc = true;
              }
          }
          delete sd;
      }
    }
  }*/
    //for (int d=0; d<2; d++) {
    if (a->info.ver[d].source.startsWith("http") && getServerConfig(a->info.ver[d].source, &s)) {
        updateInfoBar(tr("Checking for text changes on the server..."));
        /*statusBar()->insertWidget(1,progressBar,5);
          setProgressBarRange(0,0);
          progressBar->setMaximumHeight(infoBar->height());
          progressBar->show();*/
        ItServerConn * sc = new ItServerConn(s.url, s.username, s.passwd);
        connect(sc, SIGNAL(statusChanged(QString)), statusBar(), SLOT(showMessage(QString)));
        connect(sc, SIGNAL(docGetLastChangeMessage(QString,QString,QDateTime)),
                this, SLOT(serverDocLastChangeMessage(QString,QString,QDateTime)));
        /*statusBar()->clearMessage();
          statusBar()->removeWidget(progressBar);*/
        updateInfoBar();
        if (!(sc->netAvailable() && sc->login())) {
            delete sc;
            return;
        }
        sc->docGetLastChange(a->info.docId, a->info.ver[d].name);
        /*if (sc->docGetLastChange(a->info.docId, a->info.ver[d].name) > a->info.ver[d].synced) {
              int res = QMessageBox::question(this, tr("Update"),
                                              tr("Document version '%1' has been changed on the server. Do you want to update now?").arg(a->info.ver[d].name),
                                              QMessageBox::Ok|QMessageBox::No);
              if (res==QMessageBox::Ok) {
                if (!syncDoc(a, sd, d))
                  reloadAlignment();
                  return;
              }
          } else if (sc->lastErrCode==ERR_NOT_FOUND) {
              if (disassoc || QMessageBox::question(this, tr("Update"),
                                                    tr("Document version '%1' has been removed from the server and cannot be synchronized anymore. "
                                                       "Do you want to remove all associations?").arg(a->info.ver[d].name),
                                                    QMessageBox::Ok|QMessageBox::No)==QMessageBox::Ok) {
                  a->setDocDepSource(d, QString("removed_from_").append(a->info.ver[d].source));
                  a->save();
                  disassoc = true;
              }
          }
          delete sc;*/
    }
    //}
}

void ItWindow::serverDocLastChangeMessage(QString tname, QString vname, QDateTime lastchange)
{
    if (!model)
        return;
    ItAlignment * a = model->alignment;
    if (tname != a->info.docId)
        return;
    int d = 0;
    if (vname != a->info.ver[0].name) {
        d = 1;
        if (vname != a->info.ver[1].name)
            return;
    }
    ItServerConn * sc = static_cast<ItServerConn*>(sender());
    if (sc->getLastErrCode()==ERR_NOT_FOUND) {
        if (/*disassoc ||*/ QMessageBox::question(this, tr("Update"),
                                                  tr("Document version '%1' has been removed from the server and cannot be synchronized anymore. "
                                                     "Do you want to remove all associations?").arg(a->info.ver[d].name),
                                                  QMessageBox::Ok|QMessageBox::No)==QMessageBox::Ok) {
            a->setDocDepSource(d, QString("removed_from_").append(a->info.ver[d].source));
            a->save();
            //disassoc = true;
        }
    }
    delete sc;
    if (lastchange > a->info.ver[d].synced) {
        int res = QMessageBox::question(this, tr("Update"),
                                        tr("Document version '%1' has been changed on the server. Do you want to update now?").arg(a->info.ver[d].name),
                                        QMessageBox::Ok|QMessageBox::No);
        if (res==QMessageBox::Ok) {
            ItServer s;
            if (a->info.ver[d].source.startsWith("http") && getServerConfig(a->info.ver[d].source, &s)) {
                ServerDialog * sd = new ServerDialog(this, storagePath, s.url, s.username, s.passwd, true, true);
                connect(sd, SIGNAL(statusChanged(QString)), statusBar(), SLOT(showMessage(QString)));
                connect(sd, SIGNAL(alDeletedInRepo(QString)), this, SLOT(alignmentDeletedInRepo(QString)));
                connect(sd, SIGNAL(alRepoChanged()), alManager, SLOT(externalChange()));
                sd->connectToServer();
                if (sd->disconnected()) {
                    delete sd;
                    //return;
                } else if (!syncDoc(a, sd, d))
                    reloadAlignment();
                delete sd;
            }
            return;
        }
    }
    if (d==0) {
        checkForServerUpdates(a, 1);
    }
}

bool ItWindow::syncDoc(ItAlignment * a, ServerDialog *sd, aligned_doc d)
{
    if (sd->docDownloadChanges(a, d)) {
        save();
        model->undoStack->clear();
        return true;
    } else {
        return false;
        //reloadAlignment();
    }
}

bool ItWindow::getServerConfig(QString url, ItServer * s)
{
    QMapIterator<QString, ItServer> i(servers);
    while (s->url!=url && i.hasNext()) {
        i.next();
        *s = i.value();
    }
    if (s->url==url)
        return true;
    else
        return false;

}

void ItWindow::merge() {
    if (model==0) return;
    QModelIndex index;
    QModelIndex previndex;
    if (segview==0) {
        int items = model->rowCount(view->currentIndex());
        if (items==2) {
            index = model->index(1, 0, view->currentIndex());
            previndex = model->index(0, 0, view->currentIndex());
        } else if (items>2) {
            view->openEditor();
            statusBar()->showMessage(tr("Select element to merge with a preceding one and trigger 'merge' again..."), 5000);
            return;
        } else
            return;
    } else {
        index = model->index(segview->currentIndex().row(), segview->currentIndex().column(), segview->currentIndex().parent());
        previndex = model->index(segview->currentIndex().row()-1, segview->currentIndex().column(), segview->currentIndex().parent());
    }
    if (!previndex.isValid()) return;
    if (!model->canMerge(previndex)) return;
    if (!model->canMergeDeps(previndex)) {
        QMessageBox::warning(this, tr("Merging elements"), model->alignment->errorMessage);
        return;
    }
    int canmerge = 1;
    QString message;
    if (model->alignment->info.ver[previndex.parent().column()-1].source.startsWith("http")
            && model->alignment->getRepl(index.parent().column()-1, index.parent().row(), index.row())!=0 ) {
        int n = model->alignment->getOriginalNumOfElement(previndex.parent().column()-1, previndex.parent().row(), previndex.row());
        ItServer s;
        /*QMapIterator<QString, ItServer> i(servers);
    while (s.url!=model->alignment->info.source && i.hasNext()) {
      i.next();
      s = i.value();
    }*/
        if (!getServerConfig(model->alignment->info.ver[previndex.parent().column()-1].source, &s)) {
            canmerge = -1;
        } else {
            //ServerDialog * d = new ServerDialog(this, storagePath, s.url, s.username, s.passwd, true, true);
            ItServerConn * d = new ItServerConn(s.url, s.username, s.passwd);
            connect(d, SIGNAL(statusChanged(QString)), statusBar(), SLOT(showMessage(QString)));

#ifndef QT_NO_CURSOR
            QApplication::setOverrideCursor(Qt::BusyCursor);
#endif

            int v1 = previndex.parent().column()-1;
            int v2 = 1;
            if (v1==1)
                v2 = 0;
            if (!(d->netAvailable() && d->login())) {
                canmerge = -1;
#ifndef QT_NO_CURSOR
                QApplication::restoreOverrideCursor();
#endif
            } else {
                canmerge = d->canMerge(0, model->alignment->info.docId, model->alignment->info.ver[v1].name,
                                       model->alignment->info.ver[v2].name, n, 1, model->alignment->info.ver[v1].synced, &message);
#ifndef QT_NO_CURSOR
                QApplication::restoreOverrideCursor();
#endif
                if (canmerge<0) {
                    if (QMessageBox::warning(this, tr("Out of sync"), tr("The text on the server has been changed. You are highly advised to sync in order to prevent conflicting changes to the text. Do you want to sync?"), QMessageBox::Ok|QMessageBox::No)==QMessageBox::Ok) {
                        sync();
                        canmerge = -2;
                    }
                }
            }

            //connect(d, SIGNAL(alDeletedInRepo(QString)), this, SLOT(alignmentDeletedInRepo(QString)));
            //connect(d, SIGNAL(alRepoChanged()), alManager, SLOT(externalChange()));
            /*if (d->disconnected()) {
        canmerge = -1;
      } else {
        canmerge = d->canMerge(model->alignment, previndex.parent().column()-1, n, 1, &message);
      }
      disconnect(d, SIGNAL(statusChanged(QString)), statusBar(), SLOT(showMessage(QString)));*/
            delete d;
        }
    }
    if (canmerge==-2) { // sync started, forget about merging for now...
        return;
    } else if (canmerge==0) {
        QMessageBox::critical(this, tr("Merging elements"),
                              tr("Cannot merge elements - they are linked to different segments in other alignments on the server: %1").arg(message));
        return;
    } else {
        QMessageBox::StandardButton button;
        if (canmerge==-1) {
            message = tr("Cannot verify acceptability of the merge for the server. You risk unability to commit the text back to the server repository. "
                         "Are you sure you still want to merge the element with the preceding one?");
            button = QMessageBox::warning(this, tr("Merging elements"), message, QMessageBox::Ok|QMessageBox::Abort, QMessageBox::Abort);
        } else {
            message = tr("Are you sure you want to merge the element with the preceding one?");
            button = QMessageBox::question(this, tr("Merging elements"), message, QMessageBox::Ok|QMessageBox::Abort);
        }
        if (button==QMessageBox::Ok) {
            if (segview!=0)
                segview->setCurrentIndex(previndex);
            MergeCommand * merge = new MergeCommand(model, previndex);
            model->undoStack->push(merge);
        }
    }
    statusBar()->clearMessage();
}

void ItWindow::alPropEdit()
{
    if (model==0)
        return;
    AlignmentAttrDialog *pred = new AlignmentAttrDialog(this, model->alignment);
    connect(pred, SIGNAL(accepted()), this, SLOT(propertiesChanged()));
    pred->exec();
}

void ItWindow::propertiesChanged()
{
    //model->undoStack->setClean();
    dataChanged();
}
// check whether the given RemoteAlignment is not dependent on the currently open one
// (do not worry if it IS the currently open one, since this is treated separately everywhere)
// 1=yes (call reload when action is finished!), 0=no (or it IS the current al.), -1=abort the action!
int ItWindow::isDependent(QString textName, QString v1name, QString v2name)
{
    if (model==0) return 0;

    ItAlignment::alignmentInfo curinfo = model->alignment->info;

    // a completely different text? => no problem
    if (textName!=curinfo.docId)
        return 0;

    // or the current alignment? => do not worry, this must always be treated separately...
    if ((curinfo.ver[0].name==v1name && curinfo.ver[1].name==v2name) || (curinfo.ver[1].name==v1name && curinfo.ver[0].name==v2name))
        return 0;

    // partial dependence? => save current alignment and reload it when everything is finished
    if ((curinfo.ver[0].name==v1name || curinfo.ver[0].name==v2name) || (curinfo.ver[1].name==v1name || curinfo.ver[1].name==v2name)) {

        if (segview!=0) view->closeEditor(segview, QAbstractItemDelegate::NoHint);
        if (!model->undoStack->isClean()) {
            QMessageBox::StandardButton ret;
            ret = QMessageBox::warning(this, tr("Conflict"),
                                       tr("This action affects the currently open alignment.\n"
                                          "Do you want to save your changes first?\n"
                                          "(The current alignment will be reloaded when finished.)"),
                                       QMessageBox::Save | QMessageBox::Abort);
            if (ret == QMessageBox::Save && save())
                return 1; // saved, reload it later!
            else
                return -1; // ABORT!
        } else {
            return 1; // does not need saving, but reload it later!
        }
    }

    return 0; // completely independent versions of the same text, no problem...

}

void ItWindow::newServerConnection()
{
    if (QMessageBox::question(this, tr("Set-up server connection"),
                              tr("In order to connect to a remote InterText server, you have to set the connection in the settings. "
                                 "You may create several connections to different servers. Do you want to continue?"),
                              QMessageBox::Ok|QMessageBox::Cancel) == QMessageBox::Ok)
    {
        SettingsDialog * d = new SettingsDialog(this);
        d->setTab(5);
        d->show();
        connect(d, SIGNAL(accepted()), this, SLOT(updateServerMenu()));
    }
}

void ItWindow::createToolBar()
{
    toolBar = new QToolBar(this);
    if (toolBarSize==Hidden) {
        hiddenTB->setChecked(true);
        hiddenTBar();
    } else if (toolBarSize==Tiny) {
        tinyTB->setChecked(true);
        tinyTBar();
    } else if (toolBarSize==Small) {
        smallTB->setChecked(true);
        smallTBar();
    } else if (toolBarSize==Medium) {
        mediumTB->setChecked(true);
        mediumTBar();
    } else if (toolBarSize==Large) {
        largeTB->setChecked(true);
        largeTBar();
    } else if (toolBarSize==XL) {
        xlTB->setChecked(true);
        xlTBar();
    } else if (toolBarSize==XXL) {
        xxlTB->setChecked(true);
        xxlTBar();
    } else if (toolBarSize==XXXL) {
        xxxlTB->setChecked(true);
        xxxlTBar();
    } else if (toolBarSize==UL) {
        ulTB->setChecked(true);
        ulTBar();
    }
    toolBar->addAction(alManAct);
    toolBar->addAction(saveAct);
    toolBar->addAction(syncAct);
    toolBar->addSeparator();
    toolBar->addAction(shiftAct);
    toolBar->addAction(moveUpAct);
    toolBar->addAction(moveBUpAct);
    toolBar->addAction(moveBDownAct);
    toolBar->addAction(moveDownAct);
    toolBar->addAction(popAct);
    toolBar->addSeparator();
    toolBar->addAction(editAct);
    toolBar->addAction(mergeAct);
    toolBar->addSeparator();
    toolBar->addAction(undoAct);
    toolBar->addAction(redoAct);
    toolBar->addSeparator();
    toolBar->addAction(findPrevBmAct);
    toolBar->addAction(findBmAct);
    toolBar->addAction(findNextBmAct);
    toolBar->addSeparator();
    toolBar->addAction(findPrevNon11Act);
    toolBar->addAction(findNon11Act);
    toolBar->addAction(findNextNon11Act);
    toolBar->addSeparator();
    toolBar->addAction(findPrevUnconfAct);
    toolBar->addAction(findUnconfAct);
    toolBar->addAction(findNextUnconfAct);
    toolBar->addSeparator();
    toolBar->addAction(findAct);
    toolBar->addAction(confirmAct);
    addToolBar(toolBarLocation, toolBar);
}

QIcon ItWindow::composeIcon(QString fname)
{
    QIcon i = QIcon();
    i.addFile(QString(":/images/").append(fname));
    i.addFile(QString(":/images/16/").append(fname));
    i.addFile(QString(":/images/22/").append(fname));
    i.addFile(QString(":/images/32/").append(fname));
    i.addFile(QString(":/images/48/").append(fname));
    i.addFile(QString(":/images/svg/").append(fname.replace(".png",".svgz")));
    return i;
}

void ItWindow::hiddenTBar()
{
    toolBarSize = Hidden;
    toolBar->hide();
}

void ItWindow::tinyTBar()
{
    toolBarSize = Tiny;
    toolBar->show();
    toolBar->setIconSize(QSize(TINY,TINY));
}

void ItWindow::smallTBar()
{
    toolBarSize = Small;
    toolBar->show();
    toolBar->setIconSize(QSize(SMALL,SMALL));
}

void ItWindow::mediumTBar()
{
    toolBarSize = Medium;
    toolBar->show();
    toolBar->setIconSize(QSize(MEDIUM,MEDIUM));
}

void ItWindow::largeTBar()
{
    toolBarSize = Large;
    toolBar->show();
    toolBar->setIconSize(QSize(LARGE,LARGE));
}

void ItWindow::xlTBar()
{
    toolBarSize = XL;
    toolBar->show();
    toolBar->setIconSize(QSize(XLARGE,XLARGE));
}

void ItWindow::xxlTBar()
{
    toolBarSize = XXL;
    toolBar->show();
    toolBar->setIconSize(QSize(XXLARGE,XXLARGE));
}

void ItWindow::xxxlTBar()
{
    toolBarSize = XXXL;
    toolBar->show();
    toolBar->setIconSize(QSize(XXXLARGE,XXXLARGE));
}

void ItWindow::ulTBar()
{
    toolBarSize = UL;
    toolBar->show();
    toolBar->setIconSize(QSize(ULARGE,ULARGE));
}

void ItWindow::hiddenCtrl()
{
    view->setShowControls(HiddenCtrl);
}

void ItWindow::onmoveCtrl()
{
    view->setShowControls(OnMove);
}

void ItWindow::onclickCtrl()
{
    view->setShowControls(OnClick);
}

void ItWindow::tinyCtrl()
{
    view->setSizeControls(TINY);
}

void ItWindow::smallCtrl()
{
    view->setSizeControls(SMALL);
}

void ItWindow::mediumCtrl()
{
    view->setSizeControls(MEDIUM);
}

void ItWindow::largeCtrl()
{
    view->setSizeControls(LARGE);
}

void ItWindow::xlCtrl()
{
    view->setSizeControls(XLARGE);
}

void ItWindow::xxlCtrl()
{
    view->setSizeControls(XXLARGE);
}

void ItWindow::xxxlCtrl()
{
    view->setSizeControls(XXXLARGE);
}

void ItWindow::ulCtrl()
{
    view->setSizeControls(ULARGE);
}

void ItWindow::syncFinished()
{
    ServerDialog * d = static_cast<ServerDialog*>(QObject::sender());
    disconnect(d, SIGNAL(alDeletedInRepo(QString)), this, SLOT(alignmentDeletedInRepo(QString)));
    disconnect(d, SIGNAL(alRepoChanged()), alManager, SLOT(externalChange()));
    disconnect(d, SIGNAL(reloadNeeded()), this, SLOT(reloadAlignment()));
    disconnect(d, SIGNAL(statusChanged(QString)), this, SLOT(updateInfoBar(QString)));
    disconnect(d, SIGNAL(syncFinished()), this, SLOT(syncFinished()));
    disconnect(d, SIGNAL(settingProgressBarRange(int,int)), this, SLOT(setProgressBarRange(int,int)));
    disconnect(d, SIGNAL(settingProgressBarValue(int)), this, SLOT(setProgressBarValue(int)));
    d->close();
    statusBar()->clearMessage();
    statusBar()->removeWidget(progressBar);
    updateInfoBar();
    //statusBar()->showMessage(tr("Sync finished."), 4000);
}

void ItWindow::setProgressBarRange(int min, int max)
{
    progressBar->setRange(min, max);
}

void ItWindow::setProgressBarValue(int value)
{
    progressBar->setValue(value);
}

void ItWindow::extractTextAndSave(QString profileName)
{
    if (!model)
        return;
    ExTextProfile prof = exTextProfiles.value(profileName);
    QStringList customValues;
    if (prof.customVars.size()) {
        customValues = customVarsDialog(prof);
        if (customValues.isEmpty())
            return;
    }
    QString title;
    QString defFile;
    if (prof.bothTexts) {
        title = tr("Save aligned texts as...");
        defFile = QString("%1.%2.%3.%4").arg(model->alignment->info.docId, model->alignment->info.ver[0].name, model->alignment->info.ver[1].name, prof.ext);
    } else {
        title = tr("Save text version '%1' as...").arg(model->alignment->info.ver[0].name);
        defFile = QString("%1.%2.%3").arg(model->alignment->info.docId, model->alignment->info.ver[0].name, prof.ext);
    }
    QDir dir = QDir(workDir);
    QString fileName = QFileDialog::getSaveFileName(this, title, QFileInfo(dir, defFile).absoluteFilePath());
    if (fileName.isEmpty())
        return;
    workDir = QFileInfo(fileName).absolutePath();
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Saving file"), tr("Error saving file '%1':\n%2").arg(fileName, file.errorString()));
        return;
    }
    QTextCodec *codec = QTextCodec::codecForName(prof.encoding.toLatin1());
    if (!codec) {
        QMessageBox::critical(this, tr("Saving file"), tr("Unknown encoding '%1' set in profile.").arg(prof.encoding));
        return;
    }
    QString res = extractText(prof, 0, customValues);
    QTextStream out(&file);
    out.setCodec(codec);
    out << res;
    file.close();
    if (!prof.bothTexts) {
        res = extractText(prof, 1, customValues);
        title = tr("Save text version '%1' as...").arg(model->alignment->info.ver[1].name);
        defFile = QString("%1.%2.%3").arg(model->alignment->info.docId, model->alignment->info.ver[1].name, prof.ext);
        dir = QDir(workDir);
        fileName = QFileDialog::getSaveFileName(this, title, QFileInfo(dir, defFile).absoluteFilePath());
        if (fileName.isEmpty())
            return;
        workDir = QFileInfo(fileName).absolutePath();
        file.setFileName(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, tr("Saving file"), tr("Error saving file '%1':\n%2").arg(fileName, file.errorString()));
            return;
        }
        QTextStream out(&file);
        out.setCodec(codec);
        out << res;
        file.close();
    }
}

QString ItWindow::extractText(ExTextProfile &prof, aligned_doc version, QStringList &customValues)
{
    if (!model)
        return QString();
    bool skipFirstParBreak = true;
    QString segSepTemp, verSepTemp, segStartTemp, segEndTemp;
    int segCnt = 1;
    int elCnt = 1;
    int pElCnt = 1;
    int parCnt = 1;
    int elCnt2 = 1;
    int pElCnt2 = 1;
    int parCnt2 = 1;
    QString header = prof.header;
    replaceCustomVars(&header, prof, customValues);
    header.replace("%t%", model->alignment->info.docId);
    header.replace("%v%", model->alignment->info.ver[version].name);
    header.replace("%v1%", model->alignment->info.ver[0].name);
    header.replace("%v2%", model->alignment->info.ver[1].name);
    header.replace("%s%", QString::number(segCnt));
    header.replace("%p%", QString::number(parCnt));
    header.replace("%e%", QString::number(elCnt));
    header.replace("%pe%", QString::number(pElCnt));
    QString out = header;
    int segCount = model->alignment->maxPos()+1;
    for (int seg=0; seg<segCount; seg++) {
        if (prof.skipUnconfirmed && model->alignment->getMark(seg)!=LINK_MANUAL)
            continue;
        if (prof.skipEmptySeg && (model->alignment->getSize(0, seg)<1 || model->alignment->getSize(1, seg)<1))
            continue;
        if (seg != 0 && prof.keepSegs) {
            segSepTemp = prof.segSep;
            replaceCustomVars(&segSepTemp, prof, customValues);
            segSepTemp.replace("%t%", model->alignment->info.docId);
            segSepTemp.replace("%v%", model->alignment->info.ver[version].name);
            segSepTemp.replace("%v1%", model->alignment->info.ver[0].name);
            segSepTemp.replace("%v2%", model->alignment->info.ver[1].name);
            segSepTemp.replace("%s%", QString::number(segCnt));
            segSepTemp.replace("%p%", QString::number(parCnt));
            segSepTemp.replace("%e%", QString::number(elCnt));
            segSepTemp.replace("%pe%", QString::number(pElCnt));
            out.append(segSepTemp);
        }
        segStartTemp = prof.segStart;
        replaceCustomVars(&segStartTemp, prof, customValues);
        segStartTemp.replace("%t%", model->alignment->info.docId);
        segStartTemp.replace("%v%", model->alignment->info.ver[version].name);
        segStartTemp.replace("%v1%", model->alignment->info.ver[0].name);
        segStartTemp.replace("%v2%", model->alignment->info.ver[1].name);
        segStartTemp.replace("%s%", QString::number(segCnt));
        segStartTemp.replace("%p%", QString::number(parCnt));
        segStartTemp.replace("%e%", QString::number(elCnt));
        segStartTemp.replace("%pe%", QString::number(pElCnt));
        out.append(segStartTemp);
        if (prof.bothTexts) {
            verSepTemp = prof.verSep;
            replaceCustomVars(&verSepTemp, prof, customValues);
            verSepTemp.replace("%t%", model->alignment->info.docId);
            verSepTemp.replace("%v%", model->alignment->info.ver[version].name);
            verSepTemp.replace("%v1%", model->alignment->info.ver[0].name);
            verSepTemp.replace("%v2%", model->alignment->info.ver[1].name);
            verSepTemp.replace("%s%", QString::number(segCnt));
            verSepTemp.replace("%p%", QString::number(parCnt));
            verSepTemp.replace("%e%", QString::number(elCnt));
            verSepTemp.replace("%pe%", QString::number(pElCnt));
            out.append(extractTextSegment(prof, 0, seg, skipFirstParBreak, &elCnt, &pElCnt, &parCnt, segCnt, customValues));
            out.append(verSepTemp);
            out.append(extractTextSegment(prof, 1, seg, skipFirstParBreak, &elCnt2, &pElCnt2, &parCnt2, segCnt, customValues));
        } else {
            out.append(extractTextSegment(prof, version, seg, skipFirstParBreak, &elCnt, &pElCnt, &parCnt, segCnt, customValues));
        }
        segEndTemp = prof.segEnd;
        replaceCustomVars(&segEndTemp, prof, customValues);
        segEndTemp.replace("%t%", model->alignment->info.docId);
        segEndTemp.replace("%v%", model->alignment->info.ver[version].name);
        segEndTemp.replace("%v1%", model->alignment->info.ver[0].name);
        segEndTemp.replace("%v2%", model->alignment->info.ver[1].name);
        segEndTemp.replace("%s%", QString::number(segCnt));
        segEndTemp.replace("%p%", QString::number(parCnt));
        segEndTemp.replace("%e%", QString::number(elCnt));
        segEndTemp.replace("%pe%", QString::number(pElCnt));
        out.append(segEndTemp);
        skipFirstParBreak = false;
        segCnt++;
    }
    QString footer = prof.footer;
    replaceCustomVars(&footer, prof, customValues);
    footer.replace("%t%", model->alignment->info.docId);
    footer.replace("%v%", model->alignment->info.ver[version].name);
    footer.replace("%v1%", model->alignment->info.ver[0].name);
    footer.replace("%v2%", model->alignment->info.ver[1].name);
    footer.replace("%s%", QString::number(segCnt));
    footer.replace("%p%", QString::number(parCnt));
    footer.replace("%e%", QString::number(elCnt));
    footer.replace("%pe%", QString::number(pElCnt));
    out.append(footer);
    return out;
}

QString ItWindow::extractTextSegment(ExTextProfile &prof, aligned_doc version, int seg, bool skipFirstParBreak, int *elCnt,
                                     int *pElCnt, int *parCnt, int segCnt, QStringList &customValues)
{
    QString out, parSep, elSep, elStart, elEnd;
    QString elStartTemp = prof.elStart;
    replaceCustomVars(&elStartTemp, prof, customValues);
    elStartTemp.replace("%t%", model->alignment->info.docId);
    elStartTemp.replace("%v%", model->alignment->info.ver[version].name);
    elStartTemp.replace("%v1%", model->alignment->info.ver[0].name);
    elStartTemp.replace("%v2%", model->alignment->info.ver[1].name);
    elStartTemp.replace("%s%", QString::number(segCnt));
    QString elEndTemp = prof.elEnd;
    replaceCustomVars(&elEndTemp, prof, customValues);
    elEndTemp.replace("%t%", model->alignment->info.docId);
    elEndTemp.replace("%v%", model->alignment->info.ver[version].name);
    elEndTemp.replace("%v1%", model->alignment->info.ver[0].name);
    elEndTemp.replace("%v2%", model->alignment->info.ver[1].name);
    elEndTemp.replace("%s%", QString::number(segCnt));
    QString parSepTemp = prof.parSep;
    replaceCustomVars(&parSepTemp, prof, customValues);
    parSepTemp.replace("%t%", model->alignment->info.docId);
    parSepTemp.replace("%v%", model->alignment->info.ver[version].name);
    parSepTemp.replace("%v1%", model->alignment->info.ver[0].name);
    parSepTemp.replace("%v2%", model->alignment->info.ver[1].name);
    parSepTemp.replace("%s%", QString::number(segCnt));
    QString elSepTemp = prof.elSep;
    replaceCustomVars(&elSepTemp, prof, customValues);
    elSepTemp.replace("%t%", model->alignment->info.docId);
    elSepTemp.replace("%v%", model->alignment->info.ver[version].name);
    elSepTemp.replace("%v1%", model->alignment->info.ver[0].name);
    elSepTemp.replace("%v2%", model->alignment->info.ver[1].name);
    elSepTemp.replace("%s%", QString::number(segCnt));
    bool isFirst;
    QStringList elements = model->alignment->getContents(version, seg, false).toStringList();
    if (elements.size()==0) {
        QString filler = prof.emptySegFiller;
        replaceCustomVars(&filler, prof, customValues);
        filler.replace("%t%", model->alignment->info.docId);
        filler.replace("%v%", model->alignment->info.ver[version].name);
        filler.replace("%v1%", model->alignment->info.ver[0].name);
        filler.replace("%v2%", model->alignment->info.ver[1].name);
        filler.replace("%s%", QString::number(segCnt));
        filler.replace("%p%", QString::number(*parCnt));
        filler.replace("%e%", QString::number(*elCnt));
        filler.replace("%pe%", QString::number(*pElCnt));
        out.append(filler);
    }
    for (int i=0; i<elements.size(); i++) {
        // apply all rules (regular expressions)
        foreach (Replacement r, prof.replacements) {
            elements[i].replace(QRegularExpression(r.src, QRegularExpression::UseUnicodePropertiesOption), r.repl);
        }
        isFirst = model->alignment->isFirst(version, seg, i);
        if (!(i==0 && skipFirstParBreak)) {
            (*elCnt)++;
            (*pElCnt)++;
        }
        if (!skipFirstParBreak) {
            if (isFirst) {
                (*parCnt)++;
                *pElCnt = 1;
            }
        }
        if (prof.keepPars && isFirst) {
            if (skipFirstParBreak)
                skipFirstParBreak = false;
            else {
                parSep = parSepTemp;
                parSep.replace("%p%", QString::number(*parCnt));
                parSep.replace("%e%", QString::number(*elCnt));
                parSep.replace("%pe%", QString::number(*pElCnt));
                out.append(parSep);
            }
        }
        elStart = elStartTemp;
        elStart.replace("%p%", QString::number(*parCnt));
        elStart.replace("%e%", QString::number(*elCnt));
        elStart.replace("%pe%", QString::number(*pElCnt));
        elEnd = elEndTemp;
        elEnd.replace("%p%", QString::number(*parCnt));
        elEnd.replace("%e%", QString::number(*elCnt));
        elEnd.replace("%pe%", QString::number(*pElCnt));
        out.append(elStart);
        out.append(elements[i]);
        out.append(elEnd);
        if (i<elements.size()-1) {
            elSep = elSepTemp;
            elSep.replace("%p%", QString::number(*parCnt));
            elSep.replace("%e%", QString::number(*elCnt));
            elSep.replace("%pe%", QString::number(*pElCnt));
            out.append(elSep);
        }
    }
    return out;
}

void ItWindow::setAutoSaveInterval(int minutes)
{
    autoSaveInterval = minutes;
    if (minutes) {
        autoSaveTimer.start(minutes * 60000);
    } else {
        autoSaveTimer.stop();
    }
}

int ItWindow::getAutoSaveInterval()
{
    return autoSaveInterval;
}

QStringList ItWindow::customVarsDialog(ExTextProfile &prof)
{
    QStringList ret;
    ItCustomVarsDialog * cvd = new ItCustomVarsDialog(this, prof);
    cvd->exec();
    if (cvd->result()==QDialog::Accepted)
        ret = cvd->getStringList();
    delete cvd;
    return ret;
}

void ItWindow::replaceCustomVars(QString * str, ExTextProfile &prof, QStringList &values)
{
    for (int i=0; i<prof.customVars.size(); i++) {
        str->replace(prof.customVars.at(i).custSymbol, values.at(i));
    }
}

QString ItWindow::strescape(QString str) {
    return str.replace("\n","\\n").replace("\t", "\\t");
}

QString ItWindow::strunescape(QString str) {
    return str.replace("\\n","\n").replace("\\t", "\t");
}

void ItWindow::checkForUpdates(bool forced)
{
    QProcess * proc = new QProcess(this);
    if (forced)
        connect(proc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(forcedUpdateResultsReceived(int,QProcess::ExitStatus)));
    else
        connect(proc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(updateResultsReceived(int,QProcess::ExitStatus)));
    proc->start(QApplication::applicationDirPath().prepend("\"").append(QDir::separator()).append(UPDATER_BINARY).append("\" --checkupdates"));
    if (forced && !proc->waitForStarted()) {
        QMessageBox::critical(this, tr("Update"), tr("Error: Cannot execute the PackageManager."));
        delete proc;
        return;
    }

}

void ItWindow::updateResultsReceived(int ret, QProcess::ExitStatus stat)
{
    QProcess * proc = static_cast< QProcess* >(this->sender());
    QString output = QString::fromUtf8(proc->readAllStandardOutput());
    bool found = output.contains("<update ");
    delete proc;
    if (found)
        offerUpdates(true);
}

void ItWindow::forcedUpdateResultsReceived(int ret, QProcess::ExitStatus stat)
{
    QProcess * proc = static_cast< QProcess* >(this->sender());
    QString output = QString::fromUtf8(proc->readAllStandardOutput());
    bool found = output.contains("<update ");
    delete proc;
    if (found)
        offerUpdates();
    else
        QMessageBox::information(this, tr("Update"), tr("There are no updates available."));

}

void ItWindow::offerUpdates(bool autoCheck)
{
    QString msg(tr("New update is available. Do you want to update InterText now?<br/>More details: <a href=\"http://wanthalf.saga.cz/doc_iteditor_chlog\">What is new?</a>"));
    if (autoCheck)
        msg.append(tr("<br/>(This check may be disabled in settings.)"));
    if (QMessageBox::question(this, tr("Update"), msg) == QMessageBox::Yes) {
        QProcess *proc = new QProcess();
        connect(proc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(updateFinished(int,QProcess::ExitStatus)));
        proc->start(QApplication::applicationDirPath().prepend("\"").append(QDir::separator()).append(UPDATER_BINARY).append("\" --updater"));
    }
}

void ItWindow::updateFinished(int ret, QProcess::ExitStatus stat)
{
    QProcess * proc = static_cast< QProcess* >(this->sender());
    delete proc;
    if (ret==0) {
        if (QMessageBox::question(this, tr("Update"), tr("InterText has been updated. Do you want to restart now?")) == QMessageBox::Yes) {
            restartApp = true;
            close();
        }

    }
}

void ItWindow::setTransformations(QList<Replacement> &trans)
{
    transformations = trans;
    if (model)
        model->setTransformations(transformations);
}

QString ItWindow::getCss()
{
    return cssStyle;
}

void ItWindow::setCss(QString css)
{
    cssStyle = css;
    if (model)
        model->setCSS(css);
}

void ItWindow::openXMLTreeEd()
{
    /*    QModelIndex cur = view->currentIndex();
    int col =cur.column();
    col--;
    if (col<0 || col>1)
        return;
    XMLTreeDialog * ed = new XMLTreeDialog(model->alignment, col, this);
    int doc, pos, el;
    int expandNext = 0;
    if (cur.parent().isValid()) {
        doc = cur.parent().column()-1;
        pos = cur.parent().row();
        el = cur.row();
    } else {
        doc = cur.column()-1;
        pos = cur.row();
        el = 0;
        expandNext = model->rowCount(cur) - 1;
    }
    ItElement *itel = model->alignment->getElement(doc, pos, el);
    ed->openPath(itel->getDomElPath(), expandNext);
    ed->show();*/
}

void ItWindow::enableHtmlView(bool en) {
    if (htmlViewAct) {
        htmlViewAct->setEnabled(en);
    }
}

bool ItWindow::crossOrderAlignmentAllowed()
{
    if (!enableCrossOrderAlignment)
        return false;
    if (!model)
        return false;
    if (!model->alignment->crossOrderAlignmentAllowed())
        return false;
    return true;
}
