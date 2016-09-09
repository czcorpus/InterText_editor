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

#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

SettingsDialog::SettingsDialog(ItWindow *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);
    ui->editProfileButton->setEnabled(false);
    ui->delProfileButton->setEnabled(false);
    ui->editSAbbrevButton->setEnabled(false);
    ui->delSAbbrevButton->setEnabled(false);
    ui->editSRuleButton->setEnabled(false);
    ui->delSRuleButton->setEnabled(false);
    ui->upSRuleButton->setEnabled(false);
    ui->downSRuleButton->setEnabled(false);
    ui->editTransRuleButton->setEnabled(false);
    ui->delTransRuleButton->setEnabled(false);
    ui->upTransRuleButton->setEnabled(false);
    ui->downTransRuleButton->setEnabled(false);
    ui->editUrl->setEnabled(false);
    ui->editUsername->setEnabled(false);
    ui->editPasswd->setEnabled(false);
    ui->autoUpdateCheck->setEnabled(false);
    ui->edit_exec->setEnabled(false);
    ui->edit_tmpdir->setEnabled(false);
    ui->edit_header->setEnabled(false);
    ui->edit_footer->setEnabled(false);
    ui->edit_elSep->setEnabled(false);
    ui->edit_parSep->setEnabled(false);
    ui->sel_impRes->setEnabled(false);
    ui->list_profiles->setEnabled(false);
    ui->expProfReplEditButton->setEnabled(false);
    ui->expProfReplDelButton->setEnabled(false);
    ui->expProfReplUpButton->setEnabled(false);
    ui->expProfReplDownButton->setEnabled(false);
    ui->expProfCustVarEditButton->setEnabled(false);
    ui->expProfCustVarDelButton->setEnabled(false);
    window = parent;
    aligners = window->autoAligners;
    exTextProfiles = window->exTextProfiles;
    textfont = window->view->font();
    sprofiles = window->splitter.getProfiles();
    transformations = window->transformations;
    refreshTransrules();
    cfDefault = window->colors.fgdefault;
    cDefault = window->colors.bgdefault;
    cHighltd = window->colors.bgnon11;
    cMarked = window->colors.bgmarked;
    cAddDark = window->colors.bgAddDark;
    cCursor = window->colors.cursor;
    cursorOpac = window->colors.cursorOpac;
    cFound = window->colors.bgfound;
    cRepl = window->colors.bgrepl;
    servers = window->servers;
    curServer = -1;
    curExpProfile = -1;
    ui->editSkipMargin->setValue(window->view->skipMargin);
    ui->editAutoSaveInterval->setValue(window->getAutoSaveInterval());
    ui->editAutoHide->setValue(window->view->controlsHideTimeOut);
    ui->labelIdNameSpace->hide(); /* not implemented well into ItDocument/ItElement */
    ui->editIdNameSpace->hide(); /* not implemented well into ItDocument/ItElement */
    ui->editIdNameSpace->setText(window->defaultIdNamespaceURI);
    showColors();
    ui->cssEdit->setText(window->getCss());

    showTextFont();
    connect(ui->changeFontButton, SIGNAL(clicked()), this, SLOT(changeTextFont()));

    QStringList autoSaveStates;
    autoSaveStates << tr("Discard");
    autoSaveStates << tr("Save");
    autoSaveStates << tr("Ask");
    ui->changeAutoSaveElement->addItems(autoSaveStates);
    ui->changeAutoSaveElement->setCurrentIndex((int)window->view->getAutoSaveElement());

    QStringList impAlMethods;
    impAlMethods << tr("Plain 1:1 aligner");
    impAlMethods << tr("Hunalign ladder parser");
    ui->sel_impRes->addItems(impAlMethods);
    Aligner al;
    foreach (al, aligners) {
        ui->sel_aligner->addItem(al.name);
    }
    showAligner(0);

    foreach (QByteArray c, QTextCodec::availableCodecs()) {
        encodings << QString::fromLatin1(c);
    }
    encodings.sort();
    ui->expProfEncodingSel->insertItems(0, encodings);
    ui->encSel->insertItems(0, encodings);

    ui->autoCheckForUpdates->setChecked(window->autoCheckUpdates);
    ui->crossorderEnableCheckBox->setChecked(window->enableCrossOrderAlignment);
    updateServerList();
    updateExpProfList();

    ItSentenceSplitter::SplitterProfile p;
    foreach (p, sprofiles) {
        ui->sel_sprofile->addItem(p.name);
    }
    if (window->splitter.selectedProfile<0 || window->splitter.selectedProfile>=sprofiles.size())
        window->splitter.selectedProfile = 0;
    if (sprofiles.size()>0)
        showSProfile(window->splitter.selectedProfile);
    QStringList labels, cvlabels;
    labels << tr("find") << tr("replace");
    ui->srules_view->setHorizontalHeaderLabels(labels);
    ui->srules_view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->srules_view->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->transrules_view->setHorizontalHeaderLabels(labels);
    ui->transrules_view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->transrules_view->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->expProfReplView->setHorizontalHeaderLabels(labels);
    ui->expProfReplView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->expProfReplView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    cvlabels << tr("symbol") << tr("description") << tr("default value");
    ui->expProfCustVarView->setHorizontalHeaderLabels(cvlabels);
    ui->expProfCustVarView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->expProfCustVarView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->expProfCustVarView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    // import defaults
    ui->askOnTextCheckBox->setChecked(window->askOnTxtImport);
    ui->askOnXmlCheckBox->setChecked(window->askOnXmlImport);
    ui->lockIDs_checkBox->setChecked(window->importXmlLock);
    ui->encSel->setCurrentIndex(encodings.indexOf(window->importTxtEncoding));
    importXmlFooter = window->importXmlFooter;
    importXmlHeader = window->importXmlHeader;
    emptyDocTemplate = window->emptyDocTemplate;
    if (window->splitSetTxt)
        ui->sel_autoSep->setChecked(true);
    QStringList items;
    items << tr("line break") << tr("empty line");
    int set;
    QString sep = window->importParSeparator;
    if (sep=="\n")
        set = 0;
    else if (sep=="\n\n")
        set = 1;
    else {
        items << sep;
        set = 2;
    }
    ui->parSepEdit->addItems(items);
    ui->parSepEdit->setCurrentIndex(set);
    if (window->textElements.size())
        ui->parElEdit->setText(window->textElements.at(0));
    else
        ui->parElEdit->setText("");
    items.clear();
    items << tr("line break") << tr("empty line");
    sep = window->importSentenceSeparator;
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
    ui->sentElEdit->setText(window->splitterElName);
    ui->splitterProfileSel->insertItems(0, window->splitter.getProfileNames());
    ui->keepMarkup->setChecked(window->importKeepMarkup);
    if (window->splitSetXml)
        ui->sel_textelements->setChecked(true);
    ui->edit_alelements->setText(window->alignableElements.join(","));
    ui->edit_textelements->setText(window->textElements.join(","));
    if (window->defaultNumberingLevels == 2)
        ui->twolevelButton->setChecked(true);

    connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
    connect(ui->sel_aligner, SIGNAL(currentIndexChanged(int)), this, SLOT(showAligner(int)));
    connect(ui->edit_exec, SIGNAL(textChanged(QString)), this, SLOT(setAlExec(QString)));
    connect(ui->edit_tmpdir, SIGNAL(textChanged(QString)), this, SLOT(setAlTmpdir(QString)));
    connect(ui->edit_header, SIGNAL(textChanged(QString)), this, SLOT(setAlExHead(QString)));
    connect(ui->edit_footer, SIGNAL(textChanged(QString)), this, SLOT(setAlExFoot(QString)));
    connect(ui->edit_elSep, SIGNAL(textChanged(QString)), this, SLOT(setAlExElSep(QString)));
    connect(ui->edit_parSep, SIGNAL(textChanged(QString)), this, SLOT(setAlExParSep(QString)));
    connect(ui->sel_impRes, SIGNAL(currentIndexChanged(int)), this, SLOT(setAlImpRes(int)));
    connect(ui->selExecButton, SIGNAL(clicked()), this, SLOT(selectAlExec()));
    connect(ui->selTmpDirButton, SIGNAL(clicked()), this, SLOT(selectAlTmpDir()));
    connect(ui->newAlButton, SIGNAL(clicked()), this, SLOT(newAligner()));
    connect(ui->delAlButton, SIGNAL(clicked()), this, SLOT(delAligner()));
    connect(ui->renameAlButton, SIGNAL(clicked()), this, SLOT(renameAligner()));
    connect(ui->newProfileButton, SIGNAL(clicked()), this, SLOT(newAlProfile()));
    connect(ui->editProfileButton, SIGNAL(clicked()), this, SLOT(editAlProfile()));
    connect(ui->list_profiles, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(editAlProfile()));
    connect(ui->delProfileButton, SIGNAL(clicked()), this, SLOT(delAlProfile()));
    connect(ui->list_profiles, SIGNAL(currentRowChanged(int)), this, SLOT(curProfileChanged(int)));
    connect(ui->newServerButton, SIGNAL(clicked()), this, SLOT(newServer()));
    connect(ui->delServerButton, SIGNAL(clicked()), this, SLOT(delServer()));
    connect(ui->renameServerButton, SIGNAL(clicked()), this, SLOT(renameServer()));
    connect(ui->serverSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(curServerChanged(int)));
    connect(ui->editUrl, SIGNAL(textChanged(QString)), this, SLOT(setServerUrl(QString)));
    connect(ui->editUsername, SIGNAL(textChanged(QString)), this, SLOT(setServerUsername(QString)));
    connect(ui->editPasswd, SIGNAL(textChanged(QString)), this, SLOT(setServerPasswd(QString)));
    connect(ui->autoUpdateCheck, SIGNAL(stateChanged(int)), this, SLOT(changeAutoUpdateCheck(int)));

    connect(ui->newExpProfButton, SIGNAL(clicked()), this, SLOT(newExpProf()));
    connect(ui->delExpProfButton, SIGNAL(clicked()), this, SLOT(delExpProf()));
    connect(ui->renameExpProfButton, SIGNAL(clicked()), this, SLOT(renameExpProf()));
    connect(ui->expProfSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(curExpProfChanged(int)));
    connect(ui->expProfHeaderEdit, SIGNAL(textChanged(QString)), this, SLOT(setExpProfHeader(QString)));
    connect(ui->expProfFooterEdit, SIGNAL(textChanged(QString)), this, SLOT(setExpProfFooter(QString)));
    connect(ui->expProfElStartEdit, SIGNAL(textChanged(QString)), this, SLOT(setExpProfElStart(QString)));
    connect(ui->expProfElEndEdit, SIGNAL(textChanged(QString)), this, SLOT(setExpProfElEnd(QString)));
    connect(ui->expProfElSepEdit, SIGNAL(textChanged(QString)), this, SLOT(setExpProfElSep(QString)));
    connect(ui->expProfSegStartEdit, SIGNAL(textChanged(QString)), this, SLOT(setExpProfSegStart(QString)));
    connect(ui->expProfSegEndEdit, SIGNAL(textChanged(QString)), this, SLOT(setExpProfSegEnd(QString)));
    connect(ui->expProfEmptySegFillEdit, SIGNAL(textChanged(QString)), this, SLOT(setExpProfEmptySegFiller(QString)));
    connect(ui->expProfSegSepEdit, SIGNAL(textChanged(QString)), this, SLOT(setExpProfSegSep(QString)));
    connect(ui->expProfParSepEdit, SIGNAL(textChanged(QString)), this, SLOT(setExpProfParSep(QString)));
    connect(ui->expProfVerSepEdit, SIGNAL(textChanged(QString)), this, SLOT(setExpProfVerSep(QString)));
    connect(ui->expProfExtEdit, SIGNAL(textChanged(QString)), this, SLOT(setExpProfExt(QString)));
    connect(ui->expProfKeepBoth, SIGNAL(stateChanged(int)), this, SLOT(changeExpProfKeepBoth(int)));
    connect(ui->expProfKeepSeg, SIGNAL(stateChanged(int)), this, SLOT(changeExpProfKeepSeg(int)));
    connect(ui->expProfKeepPar, SIGNAL(stateChanged(int)), this, SLOT(changeExpProfKeepPar(int)));
    connect(ui->expProfSkipEmptySegs, SIGNAL(stateChanged(int)), this, SLOT(changeExpProfSkipEmpty(int)));
    connect(ui->expProfSkipUnconfSegs, SIGNAL(stateChanged(int)), this, SLOT(changeExpProfSkipUnconf(int)));
    connect(ui->expProfEncodingSel, SIGNAL(currentIndexChanged(int)), this, SLOT(changeExpProfEncoding(int)));
    connect(ui->expProfReplView, SIGNAL(cellChanged(int,int)), this, SLOT(updateExpProfRepl(int,int)));
    connect(ui->expProfReplView, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(currentExpProfReplChanged(int,int,int,int)));
    connect(ui->expProfReplNewButton, SIGNAL(clicked()), this, SLOT(newExpProfReplRule()));
    connect(ui->expProfReplDelButton, SIGNAL(clicked()), this, SLOT(delExpProfReplRule()));
    connect(ui->expProfReplUpButton, SIGNAL(clicked()), this, SLOT(moveupExpProfReplRule()));
    connect(ui->expProfReplDownButton, SIGNAL(clicked()), this, SLOT(movedownExpProfReplRule()));
    connect(ui->expProfReplEditButton, SIGNAL(clicked()), this, SLOT(editExpProfReplRule()));
    connect(ui->expProfCustVarView, SIGNAL(cellChanged(int,int)), this, SLOT(updateExpProfCustVar(int,int)));
    connect(ui->expProfCustVarView, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(currentExpProfCustVarChanged(int,int,int,int)));
    connect(ui->expProfCustVarNewButton, SIGNAL(clicked()), this, SLOT(newExpProfCustVar()));
    connect(ui->expProfCustVarDelButton, SIGNAL(clicked()), this, SLOT(delExpProfCustVar()));
    connect(ui->expProfCustVarEditButton, SIGNAL(clicked()), this, SLOT(editExpProfCustVar()));

    connect(ui->sel_sprofile, SIGNAL(currentIndexChanged(int)), this, SLOT(showSProfile(int)));
    connect(ui->editSAbbrevButton, SIGNAL(clicked()), this, SLOT(editSAbbrev()));
    connect(ui->editSRuleButton, SIGNAL(clicked()), this, SLOT(editSRule()));
    connect(ui->srules_view, SIGNAL(cellChanged(int,int)), this, SLOT(updateSRule(int,int)));
    connect(ui->srules_view, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(currentSRuleChanged(int,int,int,int)));
    connect(ui->sabbrevs_view, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(updateSAbbrev(QListWidgetItem*)));
    connect(ui->sabbrevs_view, SIGNAL(currentRowChanged(int)), this, SLOT(curSAbbrevChanged(int)));
    connect(ui->newSProfileButton, SIGNAL(clicked()), this, SLOT(newSProfile()));
    connect(ui->delSProfileButton, SIGNAL(clicked()), this, SLOT(delSProfile()));
    connect(ui->renameSProfileButton, SIGNAL(clicked()), this, SLOT(renameSProfile()));
    connect(ui->newSRuleButton, SIGNAL(clicked()), this, SLOT(newSRule()));
    connect(ui->delSRuleButton, SIGNAL(clicked()), this, SLOT(delSRule()));
    connect(ui->newSAbbrevButton, SIGNAL(clicked()), this, SLOT(newSAbbrev()));
    connect(ui->delSAbbrevButton, SIGNAL(clicked()), this, SLOT(delSAbbrev()));
    connect(ui->upSRuleButton, SIGNAL(clicked()), this, SLOT(moveupSRule()));
    connect(ui->downSRuleButton, SIGNAL(clicked()), this, SLOT(movedownSRule()));
    connect(ui->changeCFDefaultButton, SIGNAL(clicked()), this, SLOT(changeCFDefault()));
    connect(ui->changeCDefaultButton, SIGNAL(clicked()), this, SLOT(changeCDefault()));
    connect(ui->changeCNon11Button, SIGNAL(clicked()), this, SLOT(changeCNon11()));
    connect(ui->changeCMarkedButton, SIGNAL(clicked()), this, SLOT(changeCMarked()));
    connect(ui->changeCCursorButton, SIGNAL(clicked()), this, SLOT(changeCCursor()));
    connect(ui->changeCFoundButton, SIGNAL(clicked()), this, SLOT(changeCFound()));
    connect(ui->changeCReplButton, SIGNAL(clicked()), this, SLOT(changeCRepl()));
    connect(ui->edit_caddDarker, SIGNAL(valueChanged(int)), this, SLOT(cDarkerChanged(int)));
    connect(ui->edit_cursorOpac, SIGNAL(valueChanged(int)), this, SLOT(cursorOpacChanged(int)));
    connect(ui->defaultColorsButton, SIGNAL(clicked()), this, SLOT(setDefaultColors()));

    connect(ui->editTransRuleButton, SIGNAL(clicked()), this, SLOT(editTransRule()));
    connect(ui->transrules_view, SIGNAL(cellChanged(int,int)), this, SLOT(updateTransRule(int,int)));
    connect(ui->transrules_view, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(currentTransRuleChanged(int,int,int,int)));
    connect(ui->newTransRuleButton, SIGNAL(clicked()), this, SLOT(newTransRule()));
    connect(ui->delTransRuleButton, SIGNAL(clicked()), this, SLOT(delTransRule()));
    connect(ui->upTransRuleButton, SIGNAL(clicked()), this, SLOT(moveupTransRule()));
    connect(ui->downTransRuleButton, SIGNAL(clicked()), this, SLOT(movedownTransRule()));

    connect(ui->buttonXmlTemplateEdit, SIGNAL(clicked(bool)), this, SLOT(editEmptyDocTemplate()));
    connect(ui->buttonXmlHeaderEdit, SIGNAL(clicked(bool)), this, SLOT(editXmlHeader()));
    connect(ui->buttonXmlFooterEdit, SIGNAL(clicked(bool)), this, SLOT(editXmlFooter()));

    //connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::changeTextFont()
{
    //textfont.setItalic(false);
    textfont = QFontDialog::getFont(0, textfont);
    textfont.setItalic(false);
    showTextFont();
}

void SettingsDialog::showTextFont()
{
    ui->demo_font->setFont(textfont);
    ui->demo_font->setText(QString("%1, %2pt").arg(textfont.family(), QString::number(textfont.pointSize())));
}

void SettingsDialog::accept()
{
    apply();
    QDialog::accept();
}

void SettingsDialog::apply()
{
    if (textfont!=window->view->font())
        window->setTextFont(textfont);
    window->defaultIdNamespaceURI = ui->editIdNameSpace->text();
    if (window->model)
        window->model->alignment->setIdNamespaceURI(window->defaultIdNamespaceURI); /* :-b */
    window->autoAligners = aligners;
    window->splitter.setProfiles(sprofiles);
    window->view->skipMargin = ui->editSkipMargin->value();
    window->setAutoSaveInterval(ui->editAutoSaveInterval->value());
    window->view->setAutoSaveElement((AutoState)ui->changeAutoSaveElement->currentIndex());
    window->view->controlsHideTimeOut = ui->editAutoHide->value();
    window->autoCheckUpdates = ui->autoCheckForUpdates->isChecked();
    window->enableCrossOrderAlignment = ui->crossorderEnableCheckBox->isChecked();
    Colors c;
    c.fgdefault = cfDefault;
    c.bgdefault = cDefault;
    c.bgnon11 = cHighltd;
    c.bgmarked = cMarked;
    c.bgAddDark = ui->edit_caddDarker->value();
    c.cursor = cCursor;
    c.cursorOpac = ui->edit_cursorOpac->value();
    c.bgfound = cFound;
    c.bgrepl = cRepl;
    window->updateColors(c);
    window->setCss(ui->cssEdit->toPlainText());
    window->setTransformations(transformations);
    window->servers = servers;
    window->exTextProfiles = exTextProfiles;
    window->updateServerMenu();

    // import settings
    window->askOnTxtImport = ui->askOnTextCheckBox->isChecked();
    window->askOnXmlImport = ui->askOnXmlCheckBox->isChecked();
    window->importXmlLock = ui->lockIDs_checkBox->isChecked();
    window->emptyDocTemplate = emptyDocTemplate;
    window->importXmlHeader = importXmlHeader;
    window->importXmlFooter = importXmlFooter;
    window->importTxtEncoding = ui->encSel->itemText(ui->encSel->currentIndex());
    window->splitSetTxt = ui->sel_autoSep->isChecked();
    window->importKeepMarkup = ui->keepMarkup->isChecked();
    window->splitterElName = ui->sentElEdit->text();
    window->splitter.selectProfile(ui->splitterProfileSel->currentIndex());
    QString ret = ui->parSepEdit->currentText();
    QString sep;
    if (ret==tr("line break"))
        sep = "\n";
    else if (ret==tr("empty line"))
        sep = "\n\n";
    else
        sep = ret;
    window->importParSeparator = sep;
    QString elName = ui->parElEdit->text();
    int i = window->textElements.indexOf(elName);
    if (i == -1)
        window->textElements.insert(0,elName);
    else if (i != 0) {
        window->textElements.move(i, 0);
    }
    ret = ui->sentSepEdit->currentText();
    if (ret==tr("line break"))
        sep = "\n";
    else if (ret==tr("empty line"))
        sep = "\n\n";
    else
        sep = ret;
    window->importSentenceSeparator = sep;
    QStringList list;
    foreach (elName, ui->edit_alelements->text().split(",", QString::SkipEmptyParts)) {
        list.append(elName.trimmed());
    }
    window->alignableElements = list;
    list.clear();
    foreach (elName, ui->edit_textelements->text().split(",", QString::SkipEmptyParts)) {
        list.append(elName.trimmed());
    }
    window->textElements = list;
    window->splitSetXml = ui->sel_textelements->isChecked();
    if (ui->singleButton->isChecked())
        window->defaultNumberingLevels = 1;
    else
        window->defaultNumberingLevels = 2;
}

void SettingsDialog::showAligner(int n)
{
    curAligner = n;
    if (n>=0 && n<aligners.size()) {
        Aligner al = aligners.at(n);
        ui->edit_exec->setText(al.exec);
        ui->edit_tmpdir->setText(al.tmpdir);
        ui->edit_header->setText(strescape(al.exp_head));
        ui->edit_footer->setText(strescape(al.exp_foot));
        ui->edit_elSep->setText(strescape(al.exp_el_sep));
        ui->edit_parSep->setText(strescape(al.exp_parent_sep));
        ui->sel_impRes->setCurrentIndex(al.al_imp_method);
        AlignerProfile p;
        ui->list_profiles->clear();
        foreach(p, al.profiles) {
            ui->list_profiles->addItem(p.name);
        }
        ui->edit_exec->setEnabled(true);
        ui->edit_tmpdir->setEnabled(true);
        ui->edit_header->setEnabled(true);
        ui->edit_footer->setEnabled(true);
        ui->edit_elSep->setEnabled(true);
        ui->edit_parSep->setEnabled(true);
        ui->sel_impRes->setEnabled(true);
        ui->list_profiles->setEnabled(true);
        ui->alprofilesBox->setEnabled(true);
    } else {
        ui->edit_exec->setEnabled(false);
        ui->edit_tmpdir->setEnabled(false);
        ui->edit_header->setEnabled(false);
        ui->edit_footer->setEnabled(false);
        ui->edit_elSep->setEnabled(false);
        ui->edit_parSep->setEnabled(false);
        ui->sel_impRes->setEnabled(false);
        ui->list_profiles->setEnabled(false);
        ui->alprofilesBox->setEnabled(false);
        ui->edit_exec->clear();
        ui->edit_tmpdir->clear();
        ui->edit_header->clear();
        ui->edit_footer->clear();
        ui->edit_elSep->clear();
        ui->edit_parSep->clear();
        ui->list_profiles->clear();
    }
}

void SettingsDialog::showSProfile(int n)
{
    curSProfile = n;
    ui->srules_view->clearContents();
    ui->srules_view->setRowCount(0);
    ui->sabbrevs_view->clear();
    if (n<0 || n>=sprofiles.size()) {
        ui->renameSProfileButton->setEnabled(false);
        ui->delSProfileButton->setEnabled(false);
        ui->srulesBox->setEnabled(false);
        ui->sabbrevBox->setEnabled(false);
        return;
    }
    ui->renameSProfileButton->setEnabled(true);
    ui->delSProfileButton->setEnabled(true);
    ui->srulesBox->setEnabled(true);
    ui->sabbrevBox->setEnabled(true);
    ItSentenceSplitter::SplitterProfile p = sprofiles.at(n);
    QTableWidgetItem *newItem;
    ui->srules_view->setRowCount(p.expressions.size());
    for (int i=0; i<p.expressions.size(); i++) {
        ItSentenceSplitter::Replacement r = p.expressions.at(i);
        newItem = new QTableWidgetItem(strescape(r.src));
        ui->srules_view->setItem(i, 0, newItem);
        newItem = new QTableWidgetItem(strescape(r.repl));
        ui->srules_view->setItem(i, 1, newItem);
    }
    ui->sabbrevs_view->addItems(p.abbrevs);
    QListWidgetItem *item;
    for (int i=0; i<p.abbrevs.size(); i++) {
        item = ui->sabbrevs_view->item(i);
        item->setFlags (item->flags() | Qt::ItemIsEditable);
    }
}

void SettingsDialog::refreshTransrules()
{
    QTableWidgetItem * newItem;
    ui->transrules_view->clearContents();
    ui->transrules_view->setRowCount(transformations.size());
    for (int i=0; i<transformations.size(); i++) {
        Replacement r = transformations.at(i);
        newItem = new QTableWidgetItem(strescape(r.src));
        ui->transrules_view->setItem(i, 0, newItem);
        newItem = new QTableWidgetItem(strescape(r.repl));
        ui->transrules_view->setItem(i, 1, newItem);
    }

}

void SettingsDialog::setAlExec(QString str)
{
    if (curAligner<0 || curAligner>aligners.size())
        return;
    aligners[curAligner].exec = str;
}

void SettingsDialog::setAlTmpdir(QString str)
{
    if (curAligner<0 || curAligner>aligners.size())
        return;
    aligners[curAligner].tmpdir = str;
}

void SettingsDialog::setAlExHead(QString str)
{
    if (curAligner<0 || curAligner>aligners.size())
        return;
    aligners[curAligner].exp_head = strunescape(str);
}

void SettingsDialog::setAlExFoot(QString str)
{
    if (curAligner<0 || curAligner>aligners.size())
        return;
    aligners[curAligner].exp_foot = strunescape(str);
}

void SettingsDialog::setAlExParSep(QString str)
{
    if (curAligner<0 || curAligner>aligners.size())
        return;
    aligners[curAligner].exp_parent_sep = strunescape(str);
}

void SettingsDialog::setAlExElSep(QString str)
{
    if (curAligner<0 || curAligner>aligners.size())
        return;
    aligners[curAligner].exp_el_sep = strunescape(str);
}

void SettingsDialog::setAlImpRes(int n)
{
    if (curAligner<0 || curAligner>aligners.size())
        return;
    aligners[curAligner].al_imp_method = (AlignerImportMethod)n;
}

void SettingsDialog::selectAlExec()
{
    QString filename("");
    filename = QFileDialog::getOpenFileName(this, tr("Select automatic aligner"), QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0));
    QFileInfo info(filename);
    if (!info.isExecutable()) {
        QMessageBox::warning(this, tr("Error"), tr("Selected file is not executable!"));
    } else {
        ui->edit_exec->setText(info.canonicalFilePath());
    }
}

void SettingsDialog::selectAlTmpDir()
{
    QString dirname;
    dirname = QFileDialog::getExistingDirectory(this, tr("Select temporary directory"), QStandardPaths::standardLocations( QStandardPaths::TempLocation).at(0));
    QFileInfo dir(dirname);
    if (!dir.isWritable()) {
        QMessageBox::warning(this, tr("Error"), tr("Selected directory is not writable!"));
    } else {
        ui->edit_tmpdir->setText(dir.canonicalFilePath());
    }
}

void SettingsDialog::newAligner()
{
    Aligner al;
    al.name = QInputDialog::getText(this, tr("New aligner"), tr("Name for the new aligner:")).trimmed();
    al.tmpdir = QStandardPaths::standardLocations( QStandardPaths::TempLocation).at(0);
    Aligner oa;
    foreach (oa, aligners) {
        if (oa.name == al.name) {
            QMessageBox::critical(this, tr("Invalid value"), tr("This name is already in use."));
            return;
        }
    }
    if (!al.name.isEmpty()) {
        aligners << al;
        ui->sel_aligner->addItem(al.name);
        ui->sel_aligner->setCurrentIndex(aligners.size()-1);
        curAligner = aligners.size()-1;
    } else {
        QMessageBox::critical(this, tr("Invalid value"), tr("Name cannot be empty."));
    }
}

void SettingsDialog::delAligner()
{
    if (QMessageBox::Ok == QMessageBox::question(this, tr("Delete aligner"),
                                                 tr("Do you really want to delete all settings for the aligner '%1'?").arg(aligners[ui->sel_aligner->currentIndex()].name),
                                                 QMessageBox::Ok | QMessageBox::Cancel)) {
        aligners.removeAt(ui->sel_aligner->currentIndex());
        ui->sel_aligner->removeItem(ui->sel_aligner->currentIndex());
        curAligner = -1;
    }
}

void SettingsDialog::renameAligner()
{
    Aligner * al = &aligners[ui->sel_aligner->currentIndex()];
    QString name = QInputDialog::getText(this, tr("New aligner"), tr("Name of the aligner:"), QLineEdit::Normal, al->name).trimmed();
    if (name.isNull())
        return;
    Aligner oa;
    foreach (oa, aligners) {
        if (oa.name == name) {
            QMessageBox::critical(this, tr("Invalid value"), tr("This name is already in use."));
            return;
        }
    }
    if (!name.isEmpty()) {
        al->name=name;
        ui->sel_aligner->setItemText(ui->sel_aligner->currentIndex(), al->name);
    } else {
        QMessageBox::critical(this, tr("Invalid value"), tr("Name cannot be empty."));
    }
}

void SettingsDialog::newAlProfile()
{
    QString name, params;
    int pn = ui->list_profiles->currentRow();
    int an = ui->sel_aligner->currentIndex();
    if (pn>=0)
        params = aligners[an].profiles[pn].params;
    AlignerProfileDialog * d = new AlignerProfileDialog(this, name, params);
    int res = d->exec();
    if (res==QDialog::Accepted) {
        name = d->getName().trimmed();
        if (name.isEmpty()) {
            QMessageBox::critical(this, tr("Invalid value"), tr("Name cannot be empty."));
            return;
        }
        AlignerProfile tap;
        foreach (tap, aligners[an].profiles) {
            if (tap.name == name) {
                QMessageBox::critical(this, tr("Invalid value"), tr("This name is already in use."));
                return;
            }
        }
        params = d->getParams();
        ui->list_profiles->addItem(name);
        AlignerProfile p;
        p.name = name;
        p.params = params;
        aligners[an].profiles.append(p);
        ui->list_profiles->setCurrentRow(ui->list_profiles->count()-1);
    }
    delete d;
}

void SettingsDialog::editAlProfile()
{
    AlignerProfile p;
    int an = ui->sel_aligner->currentIndex();
    int pn = ui->list_profiles->currentIndex().row();
    p = aligners[an].profiles[pn];
    AlignerProfileDialog * d = new AlignerProfileDialog(this, p.name, p.params);
    int res = d->exec();
    if (res==QDialog::Accepted) {
        QString name = d->getName().trimmed();
        if (name.isEmpty()) {
            QMessageBox::critical(this, tr("Invalid value"), tr("Name cannot be empty."));
            return;
        }
        for (int i=0; i<aligners[an].profiles.size(); i++) {
            if (i==pn)
                continue;
            if (aligners[an].profiles[i].name == name) {
                QMessageBox::critical(this, tr("Invalid value"), tr("This name is already in use."));
                return;
            }
        }
        QString params = d->getParams();
        ui->list_profiles->takeItem(pn);
        ui->list_profiles->insertItem(pn, name);
        p.name = name;
        p.params = params;
        aligners[an].profiles[pn] = p;
        ui->list_profiles->setCurrentRow(pn);
    }
    delete d;
}

void SettingsDialog::delAlProfile()
{
    AlignerProfile p;
    int an = ui->sel_aligner->currentIndex();
    int pn = ui->list_profiles->currentIndex().row();
    p = aligners[an].profiles[pn];
    if (QMessageBox::Ok == QMessageBox::question(this, tr("Delete profile"),
                                                 tr("Do you really want to delete the profile '%1'?").arg(p.name),
                                                 QMessageBox::Ok | QMessageBox::Cancel)) {
        aligners[an].profiles.removeAt(pn);
        ui->list_profiles->takeItem(pn);
    }
}

void SettingsDialog::curProfileChanged(int n)
{
    int an = ui->sel_aligner->currentIndex();
    if (n>=0 && n<aligners[an].profiles.size()) {
        ui->editProfileButton->setEnabled(true);
        ui->delProfileButton->setEnabled(true);
    } else {
        ui->editProfileButton->setEnabled(false);
        ui->delProfileButton->setEnabled(false);
    }
}

void SettingsDialog::newSProfile()
{
    ItSentenceSplitter::SplitterProfile pp;
    if (curSProfile>=0 && curSProfile<sprofiles.size())
        pp = sprofiles.at(curSProfile);
    QString name = QInputDialog::getText(this, tr("New splitter profile"), tr("Name for the new profile:")).trimmed();
    if (name.isNull())
        return;
    for (int i=0; i<sprofiles.size(); i++) {
        if (sprofiles[i].name == name) {
            QMessageBox::critical(this, tr("Invalid value"), tr("This name is already in use."));
            return;
        }
    }
    if (!name.isEmpty()) {
        ItSentenceSplitter::SplitterProfile p;
        p.name = name;
        p.expressions = pp.expressions;
        sprofiles << p;
        ui->sel_sprofile->addItem(name);
        ui->sel_sprofile->setCurrentIndex(sprofiles.size()-1);
    } else {
        QMessageBox::critical(this, tr("Invalid value"), tr("Name cannot be empty."));
    }
}

void SettingsDialog::delSProfile()
{
    if (QMessageBox::Ok == QMessageBox::question(this, tr("Delete splitter profile"),
                                                 tr("Do you really want to delete the profile '%1'?").arg(sprofiles.at(curSProfile).name),
                                                 QMessageBox::Ok | QMessageBox::Cancel)) {
        sprofiles.removeAt(curSProfile);
        ui->sel_sprofile->removeItem(curSProfile);
    }
}

void SettingsDialog::renameSProfile()
{
    ItSentenceSplitter::SplitterProfile * pp = &sprofiles[curSProfile];
    QString name = QInputDialog::getText(this, tr("New splitter profile"), tr("Name for the new profile:"), QLineEdit::Normal, pp->name).trimmed();
    if (name.isNull())
        return;
    for (int i=0; i<sprofiles.size(); i++) {
        if (i==curSProfile)
            continue;
        if (sprofiles[i].name == name) {
            QMessageBox::critical(this, tr("Invalid value"), tr("This name is already in use."));
            return;
        }
    }
    if (!name.isEmpty()) {
        pp->name = name;
        ui->sel_sprofile->setItemText(curSProfile, name);
    } else {
        QMessageBox::critical(this, tr("Invalid value"), tr("Name cannot be empty."));
    }
}

void SettingsDialog::newSRule()
{
    ItSentenceSplitter::Replacement r;
    sprofiles[curSProfile].expressions.append(r);
    ui->srules_view->setRowCount(sprofiles[curSProfile].expressions.size());
    int c = sprofiles[curSProfile].expressions.size()-1;
    //ui->srules_view->insertRow(c);
    QTableWidgetItem * newItem;
    newItem = new QTableWidgetItem("");
    ui->srules_view->setItem(c, 0, newItem);
    newItem = new QTableWidgetItem("");
    ui->srules_view->setItem(c, 1, newItem);
    ui->srules_view->setCurrentItem(ui->srules_view->item(c,0));
    editSRule();
}

void SettingsDialog::delSRule()
{
    if (QMessageBox::Ok == QMessageBox::question(this, tr("Delete rule"),
                                                 tr("Do you really want to delete the selected rule?"),
                                                 QMessageBox::Ok | QMessageBox::Cancel)) {
        int n = ui->srules_view->currentIndex().row();
        sprofiles[curSProfile].expressions.removeAt(n);
        ui->srules_view->removeRow(n);
    }
}

void SettingsDialog::newTransRule()
{
    Replacement r;
    transformations.append(r);
    ui->transrules_view->setRowCount(transformations.size());
    int c = transformations.size()-1;
    QTableWidgetItem * newItem;
    newItem = new QTableWidgetItem("");
    ui->transrules_view->setItem(c, 0, newItem);
    newItem = new QTableWidgetItem("");
    ui->transrules_view->setItem(c, 1, newItem);
    ui->transrules_view->setCurrentItem(ui->transrules_view->item(c,0));
    editTransRule();
}

void SettingsDialog::delTransRule()
{
    if (QMessageBox::Ok == QMessageBox::question(this, tr("Delete rule"),
                                                 tr("Do you really want to delete the selected rule?"),
                                                 QMessageBox::Ok | QMessageBox::Cancel)) {
        int n = ui->transrules_view->currentIndex().row();
        transformations.removeAt(n);
        ui->transrules_view->removeRow(n);
    }
}
void SettingsDialog::newSAbbrev()
{
    QString s;
    sprofiles[curSProfile].abbrevs.append(s);
    ui->sabbrevs_view->addItem("");
    int n = ui->sabbrevs_view->count()-1;
    ui->sabbrevs_view->setCurrentRow(n);
    QListWidgetItem *item;
    item = ui->sabbrevs_view->item(n);
    item->setFlags (item->flags() | Qt::ItemIsEditable);
    editSAbbrev();
}

void SettingsDialog::delSAbbrev()
{
    if (QMessageBox::Ok == QMessageBox::question(this, tr("Delete abbreviation"),
                                                 tr("Do you really want to delete the selected abbreviation?"),
                                                 QMessageBox::Ok | QMessageBox::Cancel)) {
        int n = ui->sabbrevs_view->currentRow();
        sprofiles[curSProfile].abbrevs.removeAt(n);
        QListWidgetItem * item = ui->sabbrevs_view->takeItem(n);
        delete item;
    }
}

void SettingsDialog::editSAbbrev()
{
    ui->sabbrevs_view->editItem(ui->sabbrevs_view->currentItem());
}

void SettingsDialog::editSRule()
{
    ui->srules_view->editItem(ui->srules_view->currentItem());
}

void SettingsDialog::updateSRule(int rule, int prop)
{
    QString newtext = ui->srules_view->item(rule, prop)->text();
    if (prop==0) {
        if (!QRegularExpression(newtext).isValid()) {
            QMessageBox::critical(this, tr("Updating rules"), tr("Invalid regular expression."));
            ItSentenceSplitter::Replacement r = sprofiles.at(curSProfile).expressions.at(rule);
            if (prop==0)
                newtext = strescape(r.src);
            else
                newtext = strescape(r.repl);
            ui->srules_view->item(rule, prop)->setText(newtext);
            return;
        }
    }
    if (prop==0)
        sprofiles[curSProfile].expressions[rule].src = strunescape(newtext);
    else
        sprofiles[curSProfile].expressions[rule].repl = strunescape(newtext);
}

void SettingsDialog::editTransRule()
{
    ui->transrules_view->editItem(ui->transrules_view->currentItem());
}

void SettingsDialog::updateTransRule(int rule, int prop)
{
    QString newtext = ui->transrules_view->item(rule, prop)->text();
    if (prop==0) {
        if (!QRegularExpression(newtext).isValid()) {
            QMessageBox::critical(this, tr("Updating rules"), tr("Invalid regular expression."));
            Replacement r = transformations.at(rule);
            if (prop==0)
                newtext = strescape(r.src);
            else
                newtext = strescape(r.repl);
            ui->transrules_view->item(rule, prop)->setText(newtext);
            return;
        }
    }
    if (prop==0)
        transformations[rule].src = strunescape(newtext);
    else
        transformations[rule].repl = strunescape(newtext);
}
void SettingsDialog::updateSAbbrev(QListWidgetItem * item)
{
    sprofiles[curSProfile].abbrevs[ui->sabbrevs_view->row(item)] = item->text();
}

void SettingsDialog::currentSRuleChanged(int row, int col, int prow, int pcol)
{
    if (row>=0 && col>=0) {
        ui->editSRuleButton->setEnabled(true);
        ui->delSRuleButton->setEnabled(true);
        ui->upSRuleButton->setEnabled(true);
        ui->downSRuleButton->setEnabled(true);
    } else {
        ui->editSRuleButton->setEnabled(false);
        ui->delSRuleButton->setEnabled(false);
        ui->upSRuleButton->setEnabled(false);
        ui->downSRuleButton->setEnabled(false);
    }
}

void SettingsDialog::currentTransRuleChanged(int row, int col, int prow, int pcol)
{
    if (row>=0 && col>=0) {
        ui->editTransRuleButton->setEnabled(true);
        ui->delTransRuleButton->setEnabled(true);
        ui->upTransRuleButton->setEnabled(true);
        ui->downTransRuleButton->setEnabled(true);
    } else {
        ui->editTransRuleButton->setEnabled(false);
        ui->delTransRuleButton->setEnabled(false);
        ui->upTransRuleButton->setEnabled(false);
        ui->downTransRuleButton->setEnabled(false);
    }
}

void SettingsDialog::curSAbbrevChanged(int n)
{
    if (n>=0) {
        ui->editSAbbrevButton->setEnabled(true);
        ui->delSAbbrevButton->setEnabled(true);
    } else {
        ui->editSAbbrevButton->setEnabled(false);
        ui->delSAbbrevButton->setEnabled(false);
    }
}

void SettingsDialog::buttonClicked(QAbstractButton * button)
{
    if (ui->buttonBox->buttonRole(button)==QDialogButtonBox::ApplyRole)
        apply();
}

void SettingsDialog::moveupSRule()
{
    int n = ui->srules_view->currentIndex().row();
    if (n>0) {
        int c = ui->srules_view->currentIndex().column();
        ItSentenceSplitter::Replacement r = sprofiles[curSProfile].expressions.takeAt(n);
        sprofiles[curSProfile].expressions.insert(n-1, r);
        showSProfile(curSProfile);
        ui->srules_view->setCurrentCell(n-1, c);
    }
}

void SettingsDialog::movedownSRule()
{
    int n = ui->srules_view->currentIndex().row();
    if (n<sprofiles[curSProfile].expressions.size()-1) {
        int c = ui->srules_view->currentIndex().column();
        ItSentenceSplitter::Replacement r = sprofiles[curSProfile].expressions.takeAt(n);
        sprofiles[curSProfile].expressions.insert(n+1, r);
        showSProfile(curSProfile);
        ui->srules_view->setCurrentCell(n+1, c);
    }
}

void SettingsDialog::moveupTransRule()
{
    int n = ui->transrules_view->currentIndex().row();
    if (n>0) {
        int c = ui->transrules_view->currentIndex().column();
        Replacement r = transformations.takeAt(n);
        transformations.insert(n-1, r);
        refreshTransrules();
        ui->transrules_view->setCurrentCell(n-1, c);
    }
}

void SettingsDialog::movedownTransRule()
{
    int n = ui->transrules_view->currentIndex().row();
    if (n<transformations.size()-1) {
        int c = ui->transrules_view->currentIndex().column();
        Replacement r = transformations.takeAt(n);
        transformations.insert(n+1, r);
        refreshTransrules();
        ui->transrules_view->setCurrentCell(n+1, c);
    }
}

void SettingsDialog::changeCFDefault()
{
    cfDefault = QColorDialog::getColor(cfDefault, this, tr("Choose default text color"));
    showColors();
}

void SettingsDialog::changeCDefault()
{
    cDefault = QColorDialog::getColor(cDefault, this, tr("Choose default background color"));
    showColors();
    //ui->sample_cdefault->setStyleSheet(QString("background-color:%1;").arg(cDefault.name()));
}

void SettingsDialog::changeCNon11()
{
    cHighltd = QColorDialog::getColor(cHighltd, this, tr("Choose background color for non-1:1 segments"));
    showColors();
    //ui->sample_cnon11->setStyleSheet(QString("background-color:%1;").arg(cHighltd.name()));
}

void SettingsDialog::changeCMarked()
{
    cMarked = QColorDialog::getColor(cMarked, this, tr("Choose background color for marked segments"));
    showColors();
    //ui->sample_cmarked->setStyleSheet(QString("background-color:%1;").arg(cMarked.name()));
}

void SettingsDialog::changeCCursor()
{
    cCursor = QColorDialog::getColor(cCursor, this, tr("Choose cursor background color"));
    showColors();
}

void SettingsDialog::changeCFound()
{
    cFound = QColorDialog::getColor(cFound, this, tr("Choose background color for found text"));
    showColors();
}

void SettingsDialog::changeCRepl()
{
    cRepl = QColorDialog::getColor(cRepl, this, tr("Choose background color for replaced text"));
    showColors();
}

void SettingsDialog::setDefaultColors()
{
    cfDefault = QColor(DEFAULT_FGCOLOR_DEFAULT);
    cDefault = QColor(DEFAULT_BGCOLOR_DEFAULT);
    cHighltd = QColor(DEFAULT_BGCOLOR_NON11);
    cMarked = QColor(DEFAULT_BGCOLOR_MARKED);
    cAddDark = DEFAULT_EVENROW_DARKENING;
    cCursor = DEFAULT_BGCOLOR_CURSOR;
    cursorOpac = DEFAULT_CURSOR_OPACITY;
    cFound = DEFAULT_BGCOLOR_FOUND;
    cRepl = DEFAULT_BGCOLOR_REPL;
    showColors();
}

void SettingsDialog::showColors()
{
    ui->sample_cdefault->setStyleSheet(QString("background-color:%1;color:%2;").arg(cDefault.name(), cfDefault.name()));
    ui->sample_cdefault_even->setStyleSheet(QString("background-color:%1;color:%2;").arg(cDefault.darker(100+cAddDark).name(), cfDefault.name()));
    ui->sample_cnon11->setStyleSheet(QString("background-color:%1;color:%2;").arg(cHighltd.name(), cfDefault.name()));
    ui->sample_cnon11_even->setStyleSheet(QString("background-color:%1;color:%2;").arg(cHighltd.darker(100+cAddDark).name(), cfDefault.name()));
    ui->sample_cmarked->setStyleSheet(QString("background-color:%1;color:%2;").arg(cMarked.name(), cfDefault.name()));
    ui->sample_cmarked_even->setStyleSheet(QString("background-color:%1;color:%2;").arg(cMarked.darker(100+cAddDark).name(), cfDefault.name()));
    ui->sample_ccursor->setStyleSheet(QString("background-color:%1;color:%2;").arg(cCursor.name(), cfDefault.name()));
    ui->sample_cfound->setStyleSheet(QString("background-color:%1;color:%2;").arg(cFound.name(), cfDefault.name()));
    ui->sample_crepl->setStyleSheet(QString("background-color:%1;color:%2;").arg(cRepl.name(), cfDefault.name()));
    ui->edit_caddDarker->setValue(cAddDark);
    ui->edit_cursorOpac->setValue(cursorOpac);
}

void SettingsDialog::cDarkerChanged(int val)
{
    cAddDark = val;
    showColors();
}

void SettingsDialog::cursorOpacChanged(int val)
{
    cursorOpac = val;
    showColors();
}


void SettingsDialog::newServer()
{
    ItServer s;
    s.name = QInputDialog::getText(this, tr("New server"), tr("Name for the new server connection:")).trimmed();
    if (s.name.isNull())
        return;
    if (servers.keys().contains(s.name)) {
        QMessageBox::critical(this, tr("Invalid value"), tr("This name is already in use."));
        return;
    }
    s.autoUpdateCheck = true;
    if (!s.name.isEmpty()) {
        servers.insert(s.name, s);
        updateServerList();
        ui->serverSelect->setCurrentIndex(ui->serverSelect->findText(s.name));
    } else {
        QMessageBox::critical(this, tr("Invalid value"), tr("Name cannot be empty."));
    }
}

void SettingsDialog::delServer()
{
    if (ui->serverSelect->currentIndex()<0)
        return;
    if (QMessageBox::Ok == QMessageBox::question(this, tr("Delete server"),
                                                 tr("Do you really want to delete all settings for the server connection '%1'?").arg(servers[ui->serverSelect->currentText()].name),
                                                 QMessageBox::Ok | QMessageBox::Cancel)) {
        servers.remove(servers[ui->serverSelect->currentText()].name);
        curServer = -1;
        updateServerList();
    }
}

void SettingsDialog::renameServer()
{
    if (ui->serverSelect->currentIndex()<0)
        return;
    ItServer s = servers.value(ui->serverSelect->currentText());
    QString name = s.name;
    name = QInputDialog::getText(this, tr("Rename server"), tr("Name for the server connection:"), QLineEdit::Normal, name).trimmed();
    if (name.isNull())
        return;
    if (servers.keys().contains(s.name)) {
        QMessageBox::critical(this, tr("Invalid value"), tr("This name is already in use."));
        return;
    }
    if (!name.isEmpty()) {
        servers.take(ui->serverSelect->currentText());
        s.name = name;
        servers.insert(s.name, s);
        updateServerList();
        ui->serverSelect->setCurrentIndex(ui->serverSelect->findText(s.name));
    } else {
        QMessageBox::critical(this, tr("Invalid value"), tr("Name cannot be empty."));
    }
}

void SettingsDialog::updateServerList()
{
    ui->serverSelect->clear();
    QMapIterator<QString, ItServer> i(servers);
    while (i.hasNext()) {
        i.next();
        ui->serverSelect->addItem(i.value().name);
    }
    if (curServer<0 || ui->serverSelect->currentIndex()<0) {
        ui->serverSelect->setCurrentIndex(0);
        curServerChanged(0);
    }
}

void SettingsDialog::curServerChanged(int n)
{
    curServer = n;
    QString key = ui->serverSelect->itemText(n);
    if (!key.isEmpty()) {
        QString url = servers[key].url;
        if (url.endsWith("api.php"))
            url.chop(7);
        ui->editUrl->setText(url);
        ui->editUsername->setText(servers[key].username);
        ui->editPasswd->setText(servers[key].passwd);
        if (servers[key].autoUpdateCheck)
            ui->autoUpdateCheck->setChecked(true);
        else
            ui->autoUpdateCheck->setChecked(false);
        ui->editUrl->setEnabled(true);
        ui->editUsername->setEnabled(true);
        ui->editPasswd->setEnabled(true);
        ui->autoUpdateCheck->setEnabled(true);
    } else {
        ui->editUrl->setEnabled(false);
        ui->editUsername->setEnabled(false);
        ui->editPasswd->setEnabled(false);
        ui->autoUpdateCheck->setEnabled(false);
        ui->editUrl->clear();
        ui->editUsername->clear();
        ui->editPasswd->clear();
    }
}

void SettingsDialog::setServerUrl(QString url)
{
    if (curServer<0 || curServer>servers.size())
        return;
    QString key = ui->serverSelect->itemText(curServer);
    if (!url.endsWith(".php")) {
        if (!url.endsWith("/"))
            url.append("/");
        url.append("api.php");
    }
    servers[key].url = url;
}

void SettingsDialog::setServerUsername(QString str)
{
    if (curServer<0 || curServer>servers.size())
        return;
    QString key = ui->serverSelect->itemText(curServer);
    servers[key].username = str;
}

void SettingsDialog::setServerPasswd(QString str)
{
    if (curServer<0 || curServer>servers.size())
        return;
    QString key = ui->serverSelect->itemText(curServer);
    servers[key].passwd = str;
}

void SettingsDialog::changeAutoUpdateCheck(int state)
{
    if (curServer<0 || curServer>servers.size())
        return;
    QString key = ui->serverSelect->itemText(curServer);
    if (state==Qt::Checked)
        servers[key].autoUpdateCheck = true;
    else
        servers[key].autoUpdateCheck = false;
}

void SettingsDialog::setTab(int n)
{
    ui->tabWidget->setCurrentIndex(n);
}

void SettingsDialog::updateExpProfList()
{
    ui->expProfSelect->clear();
    QMapIterator<QString, ExTextProfile> i(exTextProfiles);
    while (i.hasNext()) {
        i.next();
        ui->expProfSelect->addItem(i.value().name);
    }
    if (curExpProfile<0 || ui->expProfSelect->currentIndex()<0) {
        ui->expProfSelect->setCurrentIndex(0);
        curExpProfChanged(0);
    }
}

void SettingsDialog::curExpProfChanged(int n)
{
    curExpProfile = n;

    QString key = ui->expProfSelect->itemText(n);
    if (!key.isEmpty()) {
        ui->expProfHeaderEdit->setText(strescape(exTextProfiles[key].header));
        ui->expProfFooterEdit->setText(strescape(exTextProfiles[key].footer));
        ui->expProfExtEdit->setText(exTextProfiles[key].ext);
        ui->expProfSegStartEdit->setText(strescape(exTextProfiles[key].segStart));
        ui->expProfSegEndEdit->setText(strescape(exTextProfiles[key].segEnd));
        ui->expProfEmptySegFillEdit->setText(strescape(exTextProfiles[key].emptySegFiller));
        ui->expProfSegSepEdit->setText(strescape(exTextProfiles[key].segSep));
        ui->expProfElStartEdit->setText(strescape(exTextProfiles[key].elStart));
        ui->expProfElEndEdit->setText(strescape(exTextProfiles[key].elEnd));
        ui->expProfElSepEdit->setText(strescape(exTextProfiles[key].elSep));
        ui->expProfParSepEdit->setText(strescape(exTextProfiles[key].parSep));
        ui->expProfVerSepEdit->setText(strescape(exTextProfiles[key].verSep));
        if (exTextProfiles[key].keepSegs)
            ui->expProfKeepSeg->setChecked(true);
        else
            ui->expProfKeepSeg->setChecked(false);
        if (exTextProfiles[key].skipEmptySeg)
            ui->expProfSkipEmptySegs->setChecked(true);
        else
            ui->expProfSkipEmptySegs->setChecked(false);
        if (exTextProfiles[key].skipUnconfirmed)
            ui->expProfSkipUnconfSegs->setChecked(true);
        else
            ui->expProfSkipUnconfSegs->setChecked(false);
        if (exTextProfiles[key].keepPars)
            ui->expProfKeepPar->setChecked(true);
        else
            ui->expProfKeepPar->setChecked(false);
        if (exTextProfiles[key].bothTexts)
            ui->expProfKeepBoth->setChecked(true);
        else
            ui->expProfKeepBoth->setChecked(false);
        ui->expProfEncodingSel->setCurrentIndex(encodings.indexOf(exTextProfiles[key].encoding));
        QTableWidgetItem *newItem;
        ui->expProfReplView->clearContents();
        ui->expProfReplView->setRowCount(exTextProfiles[key].replacements.size());
        for (int i=0; i<exTextProfiles[key].replacements.size(); i++) {
            Replacement r = exTextProfiles[key].replacements.at(i);
            newItem = new QTableWidgetItem(strescape(r.src));
            ui->expProfReplView->setItem(i, 0, newItem);
            newItem = new QTableWidgetItem(strescape(r.repl));
            ui->expProfReplView->setItem(i, 1, newItem);
        }
        ui->expProfCustVarView->clearContents();
        ui->expProfCustVarView->setRowCount(exTextProfiles[key].customVars.size());
        for (int i=0; i<exTextProfiles[key].customVars.size(); i++) {
            CustomVar c = exTextProfiles[key].customVars.at(i);
            newItem = new QTableWidgetItem(strescape(c.custSymbol));
            ui->expProfCustVarView->setItem(i, 0, newItem);
            newItem = new QTableWidgetItem(strescape(c.desc));
            ui->expProfCustVarView->setItem(i, 1, newItem);
            newItem = new QTableWidgetItem(strescape(c.defaultVal));
            ui->expProfCustVarView->setItem(i, 2, newItem);
        }

        ui->expProfHeaderEdit->setEnabled(true);
        ui->expProfFooterEdit->setEnabled(true);
        ui->expProfEncodingSel->setEnabled(true);
        ui->expProfExtEdit->setEnabled(true);
        ui->expProfSegStartEdit->setEnabled(true);
        ui->expProfSegEndEdit->setEnabled(true);
        ui->expProfEmptySegFillEdit->setEnabled(true);
        ui->expProfSegSepEdit->setEnabled(true);
        ui->expProfElStartEdit->setEnabled(true);
        ui->expProfElEndEdit->setEnabled(true);
        ui->expProfElSepEdit->setEnabled(true);
        ui->expProfParSepEdit->setEnabled(true);
        ui->expProfVerSepEdit->setEnabled(true);
        ui->expProfKeepSeg->setEnabled(true);
        ui->expProfKeepPar->setEnabled(true);
        ui->expProfKeepBoth->setEnabled(true);
        ui->expProfSkipEmptySegs->setEnabled(true);
        ui->expProfSkipUnconfSegs->setEnabled(true);
        ui->expProfReplBox->setEnabled(true);
        ui->expProfCustVarBox->setEnabled(true);
    } else {
        ui->expProfHeaderEdit->clear();
        ui->expProfFooterEdit->clear();
        ui->expProfExtEdit->clear();
        ui->expProfSegStartEdit->clear();
        ui->expProfSegEndEdit->clear();
        ui->expProfEmptySegFillEdit->clear();
        ui->expProfSegSepEdit->clear();
        ui->expProfElStartEdit->clear();
        ui->expProfElEndEdit->clear();
        ui->expProfElSepEdit->clear();
        ui->expProfParSepEdit->clear();
        ui->expProfVerSepEdit->clear();
        ui->expProfEncodingSel->setCurrentIndex(encodings.indexOf("UTF-8"));
        ui->expProfReplView->clearContents();
        ui->expProfReplView->setRowCount(0);
        ui->expProfCustVarView->clearContents();
        ui->expProfCustVarView->setRowCount(0);

        ui->expProfHeaderEdit->setEnabled(false);
        ui->expProfFooterEdit->setEnabled(false);
        ui->expProfEncodingSel->setEnabled(false);
        ui->expProfExtEdit->setEnabled(false);
        ui->expProfSegStartEdit->setEnabled(false);
        ui->expProfSegEndEdit->setEnabled(false);
        ui->expProfEmptySegFillEdit->setEnabled(false);
        ui->expProfSegSepEdit->setEnabled(false);
        ui->expProfElStartEdit->setEnabled(false);
        ui->expProfElEndEdit->setEnabled(false);
        ui->expProfElSepEdit->setEnabled(false);
        ui->expProfParSepEdit->setEnabled(false);
        ui->expProfVerSepEdit->setEnabled(false);
        ui->expProfKeepSeg->setEnabled(false);
        ui->expProfKeepPar->setEnabled(false);
        ui->expProfKeepBoth->setEnabled(false);
        ui->expProfSkipEmptySegs->setEnabled(false);
        ui->expProfSkipUnconfSegs->setEnabled(false);
        ui->expProfReplBox->setEnabled(false);
        ui->expProfCustVarBox->setEnabled(false);
    }
}

void SettingsDialog::newExpProf()
{
    ExTextProfile p;
    if (curExpProfile>0 && curExpProfile<exTextProfiles.size())
        p = exTextProfiles.value(ui->expProfSelect->currentText());
    else {
        p.encoding = "UTF-8";
        p.ext = "txt";
        p.keepPars = true;
        p.keepSegs = true;
        p.bothTexts = false;
        p.skipEmptySeg = false;
        p.skipUnconfirmed = false;
    }
    p.name = QInputDialog::getText(this, tr("New export profile"), tr("Name for the new profile:")).trimmed();
    if (p.name.isNull())
        return;
    if (exTextProfiles.keys().contains(p.name)) {
        QMessageBox::critical(this, tr("Invalid value"), tr("This name is already in use."));
        return;
    }
    if (!p.name.isEmpty()) {
        exTextProfiles.insert(p.name, p);
        updateExpProfList();
        ui->expProfSelect->setCurrentIndex(ui->expProfSelect->findText(p.name));
    } else {
        QMessageBox::critical(this, tr("Invalid value"), tr("Name cannot be empty."));
    }
}

void SettingsDialog::delExpProf()
{
    if (ui->expProfSelect->currentIndex()<0)
        return;
    if (QMessageBox::Ok == QMessageBox::question(this, tr("Delete server"),
                                                 tr("Do you really want to delete all settings for the profile '%1'?").arg(exTextProfiles[ui->expProfSelect->currentText()].name),
                                                 QMessageBox::Ok | QMessageBox::Cancel)) {
        exTextProfiles.remove(exTextProfiles[ui->expProfSelect->currentText()].name);
        curExpProfile = -1;
        updateExpProfList();
    }
}

void SettingsDialog::renameExpProf()
{
    if (ui->expProfSelect->currentIndex()<0)
        return;
    ExTextProfile p = exTextProfiles.value(ui->expProfSelect->currentText());
    QString name = p.name;
    name = QInputDialog::getText(this, tr("Rename profile"), tr("New name for the profile:"), QLineEdit::Normal, name).trimmed();
    if (name.isNull())
        return;
    if (exTextProfiles.keys().contains(p.name)) {
        QMessageBox::critical(this, tr("Invalid value"), tr("This name is already in use."));
        return;
    }
    if (!name.isEmpty()) {
        exTextProfiles.take(ui->expProfSelect->currentText());
        p.name = name;
        exTextProfiles.insert(p.name, p);
        updateExpProfList();
        ui->expProfSelect->setCurrentIndex(ui->expProfSelect->findText(p.name));
    } else {
        QMessageBox::critical(this, tr("Invalid value"), tr("Name cannot be empty."));
    }
}

void SettingsDialog::setExpProfHeader(QString str)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    exTextProfiles[key].header = strunescape(str);
}

void SettingsDialog::setExpProfFooter(QString str)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    exTextProfiles[key].footer = strunescape(str);
}

void SettingsDialog::setExpProfElStart(QString str)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    exTextProfiles[key].elStart = strunescape(str);
}

void SettingsDialog::setExpProfElEnd(QString str)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    exTextProfiles[key].elEnd = strunescape(str);
}

void SettingsDialog::setExpProfElSep(QString str)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    exTextProfiles[key].elSep = strunescape(str);
}

void SettingsDialog::setExpProfSegStart(QString str)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    exTextProfiles[key].segStart = strunescape(str);
}

void SettingsDialog::setExpProfSegEnd(QString str)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    exTextProfiles[key].segEnd = strunescape(str);
}

void SettingsDialog::setExpProfEmptySegFiller(QString str)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    exTextProfiles[key].emptySegFiller = strunescape(str);
}

void SettingsDialog::setExpProfSegSep(QString str)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    exTextProfiles[key].segSep = strunescape(str);
}

void SettingsDialog::setExpProfParSep(QString str)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    exTextProfiles[key].parSep = strunescape(str);
}

void SettingsDialog::setExpProfVerSep(QString str)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    exTextProfiles[key].verSep = strunescape(str);
}

void SettingsDialog::setExpProfExt(QString str)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    exTextProfiles[key].ext = str;
}

void SettingsDialog::changeExpProfKeepBoth(int state)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    if (state==Qt::Checked)
        exTextProfiles[key].bothTexts = true;
    else
        exTextProfiles[key].bothTexts = false;
}

void SettingsDialog::changeExpProfKeepPar(int state)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    if (state==Qt::Checked)
        exTextProfiles[key].keepPars = true;
    else
        exTextProfiles[key].keepPars = false;
}

void SettingsDialog::changeExpProfKeepSeg(int state)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    if (state==Qt::Checked)
        exTextProfiles[key].keepSegs = true;
    else
        exTextProfiles[key].keepSegs = false;
}

void SettingsDialog::changeExpProfSkipEmpty(int state)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    if (state==Qt::Checked)
        exTextProfiles[key].skipEmptySeg = true;
    else
        exTextProfiles[key].skipEmptySeg = false;
}

void SettingsDialog::changeExpProfSkipUnconf(int state)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    if (state==Qt::Checked)
        exTextProfiles[key].skipUnconfirmed = true;
    else
        exTextProfiles[key].skipUnconfirmed = false;
}

void SettingsDialog::changeExpProfEncoding(int n)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    exTextProfiles[key].encoding = ui->expProfEncodingSel->itemText(n);
}

void SettingsDialog::updateExpProfRepl(int rule, int prop)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    QString newtext = ui->expProfReplView->item(rule, prop)->text();
    if (prop==0) {
        if (!QRegularExpression(newtext).isValid()) {
            QMessageBox::critical(this, tr("Updating rules"), tr("Invalid regular expression."));
            Replacement r = exTextProfiles[key].replacements.at(rule);
            //if (prop==0)
            newtext = strescape(r.src);
            //else
            //  newtext = strescape(r.repl);
            ui->expProfReplView->item(rule, prop)->setText(newtext);
            return;
        }
    }
    if (prop==0)
        exTextProfiles[key].replacements[rule].src = strunescape(newtext);
    else
        exTextProfiles[key].replacements[rule].repl = strunescape(newtext);
}

void SettingsDialog::currentExpProfReplChanged(int row, int col, int prow, int pcol)
{
    if (row>=0 && col>=0) {
        ui->expProfReplEditButton->setEnabled(true);
        ui->expProfReplDelButton->setEnabled(true);
        ui->expProfReplUpButton->setEnabled(true);
        ui->expProfReplDownButton->setEnabled(true);
    } else {
        ui->expProfReplEditButton->setEnabled(false);
        ui->expProfReplDelButton->setEnabled(false);
        ui->expProfReplUpButton->setEnabled(false);
        ui->expProfReplDownButton->setEnabled(false);
    }
}

void SettingsDialog::newExpProfReplRule()
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    Replacement r;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    exTextProfiles[key].replacements.append(r);
    ui->expProfReplView->setRowCount(exTextProfiles[key].replacements.size());
    int c = exTextProfiles[key].replacements.size()-1;
    QTableWidgetItem * newItem;
    newItem = new QTableWidgetItem("");
    ui->expProfReplView->setItem(c, 0, newItem);
    newItem = new QTableWidgetItem("");
    ui->expProfReplView->setItem(c, 1, newItem);
    ui->expProfReplView->setCurrentItem(ui->expProfReplView->item(c,0));
    editExpProfReplRule();
}

void SettingsDialog::editExpProfReplRule()
{
    ui->expProfReplView->editItem(ui->expProfReplView->currentItem());
}

void SettingsDialog::delExpProfReplRule()
{
    if (!ui->expProfReplView->currentIndex().isValid())
        return;
    if (QMessageBox::Ok == QMessageBox::question(this, tr("Delete rule"),
                                                 tr("Do you really want to delete the selected rule?"),
                                                 QMessageBox::Ok | QMessageBox::Cancel)) {
        int n = ui->expProfReplView->currentIndex().row();
        QString key = ui->expProfSelect->itemText(curExpProfile);
        exTextProfiles[key].replacements.removeAt(n);
        ui->expProfReplView->removeRow(n);
    }
}

void SettingsDialog::moveupExpProfReplRule()
{
    if (!ui->expProfReplView->currentIndex().isValid())
        return;
    int n = ui->expProfReplView->currentIndex().row();
    QString key = ui->expProfSelect->itemText(curExpProfile);
    if (n>0) {
        int c = ui->expProfReplView->currentIndex().column();
        Replacement r = exTextProfiles[key].replacements.takeAt(n);
        exTextProfiles[key].replacements.insert(n-1, r);
        curExpProfChanged(curExpProfile);
        ui->expProfReplView->setCurrentCell(n-1, c);
    }
}

void SettingsDialog::movedownExpProfReplRule()
{
    if (!ui->expProfReplView->currentIndex().isValid())
        return;
    int n = ui->expProfReplView->currentIndex().row();
    QString key = ui->expProfSelect->itemText(curExpProfile);
    if (n<exTextProfiles[key].replacements.size()-1) {
        int c = ui->expProfReplView->currentIndex().column();
        Replacement r = exTextProfiles[key].replacements.takeAt(n);
        exTextProfiles[key].replacements.insert(n+1, r);
        curExpProfChanged(curExpProfile);
        ui->expProfReplView->setCurrentCell(n+1, c);
    }
}

void SettingsDialog::updateExpProfCustVar(int var, int prop)
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    QString newtext = ui->expProfCustVarView->item(var, prop)->text();
    if (prop==0)
        exTextProfiles[key].customVars[var].custSymbol = strunescape(newtext);
    else if (prop==1)
        exTextProfiles[key].customVars[var].desc = strunescape(newtext);
    else
        exTextProfiles[key].customVars[var].defaultVal = strunescape(newtext);
}

void SettingsDialog::currentExpProfCustVarChanged(int row, int col, int prow, int pcol)
{
    if (row>=0 && col>=0) {
        ui->expProfCustVarEditButton->setEnabled(true);
        ui->expProfCustVarDelButton->setEnabled(true);
    } else {
        ui->expProfCustVarEditButton->setEnabled(false);
        ui->expProfCustVarDelButton->setEnabled(false);
    }
}

void SettingsDialog::newExpProfCustVar()
{
    if (curExpProfile<0 || curExpProfile>exTextProfiles.size())
        return;
    CustomVar cv;
    QString key = ui->expProfSelect->itemText(curExpProfile);
    exTextProfiles[key].customVars.append(cv);
    ui->expProfReplView->setRowCount(exTextProfiles[key].customVars.size());
    int c = exTextProfiles[key].customVars.size()-1;
    QTableWidgetItem * newItem;
    newItem = new QTableWidgetItem("");
    ui->expProfCustVarView->setItem(c, 0, newItem);
    newItem = new QTableWidgetItem("");
    ui->expProfCustVarView->setItem(c, 1, newItem);
    newItem = new QTableWidgetItem("");
    ui->expProfCustVarView->setItem(c, 2, newItem);
    ui->expProfCustVarView->setCurrentItem(ui->expProfCustVarView->item(c,0));
    editExpProfCustVar();
}

void SettingsDialog::editExpProfCustVar()
{
    ui->expProfCustVarView->editItem(ui->expProfCustVarView->currentItem());
}

void SettingsDialog::delExpProfCustVar()
{
    if (!ui->expProfCustVarView->currentIndex().isValid())
        return;
    if (QMessageBox::Ok == QMessageBox::question(this, tr("Delete custom variable"),
                                                 tr("Do you really want to delete the selected variable?"),
                                                 QMessageBox::Ok | QMessageBox::Cancel)) {
        int n = ui->expProfCustVarView->currentIndex().row();
        QString key = ui->expProfSelect->itemText(curExpProfile);
        exTextProfiles[key].customVars.removeAt(n);
        ui->expProfCustVarView->removeRow(n);
    }
}


QString SettingsDialog::strescape(QString str) {
    return str.replace("\n","\\n").replace("\t", "\\t");
}

QString SettingsDialog::strunescape(QString str) {
    return str.replace("\\n","\n").replace("\\t", "\t");
}

void SettingsDialog::editEmptyDocTemplate()
{
    bool ok;
    QString text = emptyDocTemplate;
    do {
        text = QInputDialog::getMultiLineText(this, tr("New empty document"), tr("Document template"), text, &ok);
        if (!ok || text.isEmpty()) {
            return;
        }
    } while (!checkXml(text));
    emptyDocTemplate = text;
}

void SettingsDialog::editXmlHeader()
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

void SettingsDialog::editXmlFooter()
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

bool SettingsDialog::checkXml(QString xml, int linedec)
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
