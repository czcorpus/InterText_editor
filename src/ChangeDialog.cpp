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

#include "ChangeDialog.h"
#include "ui_ChangeDialog.h"
#include "ServerDialog.h"
#include "ItWindow.h"

ChangeDialog::ChangeDialog(ServerDialog *parent, ItWindow * win, ItAlignment * a, aligned_doc d, QDomNodeList &changes, bool * mark, bool acceptonly) :
    QDialog(parent),
    ui(new Ui::ChangeDialog)
{
    ui->setupUi(this);
    server = parent;
    window = win;
    alignment = a;
    doc = d;
    changeList = changes;
    markState = mark;
    ui->markCheckBox->setChecked(*mark);
    setTextFont(window->view->font());
    setAttribute(Qt::WA_DeleteOnClose, true);

    appendButton = new QPushButton(QIcon(":/images/16/list-add.png"), QString());
    connect(appendButton, SIGNAL(clicked()), this, SLOT(appendChange()));
    appendButton->setEnabled(false);
    ui->buttonBox->addButton(appendButton, QDialogButtonBox::ActionRole);
    detachButton = new QPushButton(QIcon(":/images/16/list-remove.png"), QString());
    connect(detachButton, SIGNAL(clicked()), this, SLOT(detachChange()));
    detachButton->setEnabled(false);
    ui->buttonBox->addButton(detachButton, QDialogButtonBox::ActionRole);

    if (acceptonly) {
        ui->buttonBox->button(QDialogButtonBox::No)->setVisible(false);
    }

    connect(ui->markCheckBox, SIGNAL(stateChanged(int)), this, SLOT(changeMark(int)));
    connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(action(QAbstractButton*)));
    ui->buttonBox->button(QDialogButtonBox::Yes)->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::YesToAll)->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::No)->setEnabled(false);
    //ui->buttonBox->button(QDialogButtonBox::NoToAll)->setEnabled(false);
    changenum = 0;
    nextChangeNum = 0;
    autoAccept = false;
    joinConsChanges = -1;
    nextChange();
}

ChangeDialog::~ChangeDialog()
{
    delete ui;
    delete appendButton;
}

void ChangeDialog::action(QAbstractButton * button)
{
    if (ui->buttonBox->buttonRole(button)==QDialogButtonBox::ActionRole)
        return;
    joinConsChanges = 0;
    ui->buttonBox->button(QDialogButtonBox::Yes)->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::YesToAll)->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::No)->setEnabled(false);
    //ui->buttonBox->button(QDialogButtonBox::NoToAll)->setEnabled(false);
    QDialogButtonBox::StandardButton stdbutton = ui->buttonBox->standardButton(button);
    if (stdbutton==QDialogButtonBox::Abort) {
        reject();
        done(result());
        return;
    }
    setResult(QDialog::Accepted);
    if (stdbutton==QDialogButtonBox::YesToAll) {
        autoAccept = true;
        commitChange();
        return;
    } else if (stdbutton==QDialogButtonBox::Yes) {
        commitChange();
    } else if (stdbutton==QDialogButtonBox::No) {
        rejectChange();
    }
    if (nextChangeNum<changeList.size())
        nextChange();
    else
        done(result());
    /*else {
    accept();
    return;
  }*/
}

void ChangeDialog::nextChange()
{
    QDomElement change;
    int repl, n;
    int llen=0, rlen=0;
    QString llaststr, rlaststr;
    QString parbr;
    QString contents;
    ItElement * e;
    changenum = nextChangeNum;

    change = changeList.item(changenum).toElement();
    n = change.attribute("n").toInt();
    repl = change.attribute("repl","1").toInt();

    QList<ItElement* > elist;
    int pre;
    // now the original n-th element may be nn-th; it may even be a part of a merged element (pre=preceding elements in the merger)
    lstartnum = alignment->getNumOfOriginalNth(doc, n, &pre);
    e = alignment->getLinkedElement(doc, lstartnum);
    if (e==0) {
        syncFailure(tr("Element number '%1' not found in document '%2' version '%3'.").arg(QString::number(n), alignment->info.docId, alignment->info.ver[doc].name));
        return;
    }
    elist.append(e);
    // balance changes to be equivalents of each other (the sum of repl values on both sides)
    int rlast = n; // remote item counter
    int llast = lstartnum; // local item counter
    int rcnt = repl+pre; // counter of remotely replaced items
    int lcnt = e->repl(); // counter of locally replaced items
    int chlistptr = changenum; // pointer to number of the last change, that will be processed together with the i-th one
    int add = 0; // missing remote elements to be added
    int appendedChanges = 0;
    int consecCnt = joinConsChanges;
    while (rcnt!=lcnt // balance of replaced elements!
           || (changeList.item(chlistptr+1).toElement().attribute("n").toInt()==rlast+1 && // append all changes following each other immediately
               ((changeList.item(chlistptr+1).toElement().attribute("repl","1")=="0") || //... but only if they are results of a split
                consecCnt>0)) // or requested explicitly
           || (alignment->getLinkedElement(doc, llast+1) && alignment->getLinkedElement(doc, llast+1)->repl()==0)) { // append all local splitted elements/rests as well
        if (consecCnt!=-1 && rcnt==lcnt && changeList.item(chlistptr+1).toElement().attribute("n").toInt()==rlast+1 && changeList.item(chlistptr+1).toElement().attribute("repl","1")!="0")
            consecCnt--;
        if (rcnt>lcnt || (alignment->getLinkedElement(doc, llast+1) && alignment->getLinkedElement(doc, llast+1)->repl()==0)) {
            llast++;
            e = alignment->getLinkedElement(doc, llast);
            if (e==0) {
                llast--;
                break;
            }
            lcnt += e->repl();
            elist.append(e);
            llaststr = e->getContents(false).replace(QRegExp("<[^>]+>"),"");
            llen += llaststr.length();
        } else { // rcnt<lcnt or just the immediately following change
            rlast++;
            chlistptr++;
            QDomElement el = changeList.item(chlistptr).toElement();
            if (el.attribute("n").toInt()==rlast) {
                add=0;
                rcnt += el.attribute("repl","1").toInt();
                rlaststr = el.text();
            } else {
                add++;
                rcnt += 1;
                chlistptr--;
                rlaststr = "";
            }
            rlen += rlaststr.length();
        }
        // should we append further changes to the batch?
        if (joinConsChanges==-1 && rcnt==lcnt && changeList.item(chlistptr+1).toElement().attribute("n").toInt()==rlast+1 && changeList.item(chlistptr+1).toElement().attribute("repl","1")!="0" &&
                (!rlaststr.endsWith(llaststr.right(8)) || abs(rlen-llen)<8)) { // check whether the strings end in the same way or the sum of lengths is similar
            if (consecCnt==-1)
                consecCnt = 0;
            appendedChanges++;
            consecCnt++;
        }
    }
    if (joinConsChanges==-1 && appendedChanges>0)
        joinConsChanges = appendedChanges;
    // is there a consecutive change which may possibly be a continuation of this one?
    if (changeList.item(chlistptr+1).toElement().attribute("n").toInt()==rlast+1)
        appendButton->setEnabled(true);
    else
        appendButton->setEnabled(false);
    if (joinConsChanges>0)
        detachButton->setEnabled(true);
    else
        detachButton->setEnabled(false);

    QStringList lstrings;
    rstringlist.clear();
    QList<bool> lparbr;
    rparbreaks.clear();
    foreach (e, elist) {
        lstrings.append(e->getContents(false));
        lparbr.append(e->first());
    }
    int cnt=n;
    // prepend virtual elements/strings where the change starts in the middle of some current element-merger
    for (int j=0; j<pre; j++) {
        rstringlist.append(QString());
        rparbreaks.append(false);
    }
    for (int j=changenum; j<=chlistptr; j++) {
        while (changeList.item(j).toElement().attribute("n").toInt()>cnt) {
            //qDebug()<<"appending empty/virtual element/string for el."<<cnt<<"because the change "<<j<<"has already n="<<changeList.item(j).toElement().attribute("n").toInt() <<"but cnt is still only at"<<cnt;
            rstringlist.append(QString());
            rparbreaks.append(false);
            cnt++;
        }
        contents = "";
        QTextStream* str = new QTextStream(&contents);
        changeList.item(j).toElement().save(*str,-1);
        delete str;
        contents.remove(QRegExp("^<[^>]+>"));
        contents.remove(QRegExp("</[^>]+>$"));
        rstringlist.append(contents);
        parbr = changeList.item(j).toElement().attribute("parbr");
        if (parbr=="o" || parbr=="n")
            rparbreaks.append(true);
        else
            rparbreaks.append(false);
        cnt++;
    }
    // append virtual elements/strings where the change end in the middle of some current element-merger
    for (int j=0; j<add; j++) {
        rstringlist.append(QString());
        rparbreaks.append(false);
    }

    // display visual comparison
    QString numstr;
    if (chlistptr==changenum)
        numstr = QString::number(changenum+1);
    else
        numstr = QString("%1 - %2").arg(QString::number(changenum+1), QString::number(chlistptr+1));
    setCounter(numstr, QString::number(changeList.size()));
    nextChangeNum = chlistptr+1;
    lcount = lstrings.size();
    startnum = n-pre;
    showChange(lstrings, lparbr);
}

void ChangeDialog::showChange(QStringList &lstrings, QList<bool> &lparbr)
{
    if (!autoAccept) {
        QString icon;
        QString text;
        for (int i=0; i<lstrings.size(); i++) {
            if (lparbr.at(i))
                icon = ":/images/16/dblarrow.png";
            else
                icon = ":/images/16/arrow.png";
            text.append(QString("<p><img src=\"%1\"/> ").arg(icon));
            text.append(lstrings.at(i)).append("</p>");
        }
        ui->view_left->setHtml(text);
        text = "";
    }

    // any item missing? download them from the server!
    QList<int> missing;
    for (int i=0; i<rstringlist.size(); i++) {
        if (rstringlist.at(i).isEmpty())
            missing.append(startnum+i);
    }
    if (missing.size()>0) {
        connect(server, SIGNAL(receivedItems(QDomNodeList)), this, SLOT(updateRStrings(QDomNodeList)));
        connect(server, SIGNAL(failure()), this, SLOT(close()));
        server->requestItems(alignment->info.docId, alignment->info.ver[doc].name, missing);
    } else {
        if (!autoAccept)
            renderRStrings();
        else
            commitChange();
    }
}

void ChangeDialog::changeMark(int state)
{
    if (state==Qt::Checked)
        *markState = true;
    else
        *markState = false;
}

void ChangeDialog::setCounter(QString num, QString total)
{
    ui->label_count->setText(tr("Change %1 of %2").arg(num, total));
}

void ChangeDialog::setTextFont(const QFont &font)
{
    ui->view_left->setFont(font);
    ui->view_right->setFont(font);
}

void ChangeDialog::updateRStrings(QDomNodeList nodelist)
{
    disconnect(server, SIGNAL(failure()), this, SLOT(close()));
    disconnect(server, SIGNAL(receivedItems(QDomNodeList)), this, SLOT(updateRStrings(QDomNodeList)));
    QDomElement el;
    QString contents, parbr;
    int n;
    for (int i=0; i<nodelist.size(); i++) {
        el = nodelist.at(i).toElement();
        n = el.attribute("n").toInt();
        parbr = el.attribute("parbr");
        contents = "";
        QTextStream* str = new QTextStream(&contents);
        el.save(*str,-1);
        delete str;
        contents.remove(QRegExp("^<[^>]+>"));
        contents.remove(QRegExp("</[^>]+>$"));
        rstringlist[n-startnum] = contents;
        if (parbr=="o" || parbr=="n")
            rparbreaks[n-startnum] = true;
    }
    if (!autoAccept)
        renderRStrings();
    else
        commitChange();
}

void ChangeDialog::renderRStrings()
{
    QString icon;
    QString text;
    for (int i=0; i<rstringlist.size(); i++) {
        if (rparbreaks.at(i))
            icon = ":/images/16/dblarrow.png";
        else
            icon = ":/images/16/arrow.png";
        text.append(QString("<p><img src=\"%1\"/> ").arg(icon));
        text.append(rstringlist.at(i)).append("</p>");
    }
    ui->view_right->setHtml(QString("<body>%1</body>").arg(text));
    ui->buttonBox->button(QDialogButtonBox::Yes)->setEnabled(true);
    ui->buttonBox->button(QDialogButtonBox::YesToAll)->setEnabled(true);
    ui->buttonBox->button(QDialogButtonBox::No)->setEnabled(true);
    //ui->buttonBox->button(QDialogButtonBox::NoToAll)->setEnabled(true);
}



void ChangeDialog::syncFailure(QString text)
{
    QMessageBox::critical(this, tr("Sync failure"), text);
    //emit statusChanged(tr("Sync failed."));
    reject();
    return;
}


void ChangeDialog::commitChange()
{
    int first = lstartnum;
    int count = lcount;
    QStringList newstrings = rstringlist;
    ItAlignment * a = alignment;
    int pos, el, newsize;
    ItElement * e;
    a->trackChanges[doc]=false;
    a->ignorePermissions = true;
    e = a->getLinkedElement(doc, first, &pos, &el);
    //firstel = el;
    newsize = newstrings.size();
    // merge into one segment
    int lastpos;
    a->getLinkedElement(doc, first+count-1, &lastpos);
    while (lastpos>pos) {
        a->moveUp(0, lastpos);
        a->moveUp(1, lastpos);
        lastpos--;
    }
    // and what about other possible local dependent alignments?
    if (lcount>1 && !a->canMergeDeps(doc, pos, el, lcount-1)) {
        a->trackChanges[doc]=true;
        syncFailure(a->errorMessage+" (Or should such conflicts be fixed automatically in a future release?)");
        return;
    }
    // now we have everything within one single segment at "pos", starting with "el"

    int diff = newsize - count;
    // less elements? do merge on the first one
    while (diff<0) {
        if (!a->canMergeDeps(doc, pos, el)) {
            a->trackChanges[doc]=true;
            syncFailure(a->errorMessage+" (Or should such conflicts be fixed automatically in a future release?)");
            return;
        }
        a->merge(doc, pos, el);
        diff++;
        count --;
    }
    // update elements with current strings
    while (count>1) {//qDebug()<<doc<<pos<<el<<newstrings.first();
        if (!a->updateContents(doc, pos, el, newstrings.takeFirst())) {
            a->trackChanges[doc]=true;
            syncFailure("Cannot update contents.");
            return;
        }
        el++;
        count --;
    }
    // use the rest of newstrings to automatically split, if necessary
    if (newstrings.size()>1) {
        if (!a->split(doc, pos, el, newstrings)) {
            a->trackChanges[doc]=true;
            syncFailure("Cannot split contents.");
            return;
        }
    } else {
        if (!a->updateContents(doc, pos, el, newstrings.takeFirst())) {
            a->trackChanges[doc]=true;
            syncFailure("Cannot update contents.");
            return;
        }
    }

    // paragraph breaks and change tracking!
    for (int i=0; i<newsize; i++) {
        e = a->getLinkedElement(doc, first+i, &pos, &el);
        if (rparbreaks.at(i) && !(e->first())) {
            if (!a->splitParent(doc, pos, el)) {
                a->trackChanges[doc]=true;
                syncFailure("Cannot split parent element.");
                return;
            }
        } else if (!(rparbreaks.at(i)) && e->first()) {
            if (!a->mergeParent(doc, pos, el)) {
                a->trackChanges[doc]=true;
                syncFailure("Cannot merge parent elements.");
                return;
            }
        }
        e->delRepl();
        e->setParbr("");
    }
    if (*markState)
        a->setMark(pos, 1);
    a->trackChanges[doc]=true;
    a->ignorePermissions = false;
    window->model->externalDataChange();
    window->view->resizeRowToContents(pos);
    if (autoAccept) {
        if (nextChangeNum<changeList.size())
            nextChange();
        else
            accept();
    }
}

void ChangeDialog::rejectChange()
{
    int first = lstartnum;
    int count = lcount;
    int rsize = rstringlist.size();
    ItAlignment * a = alignment;
    int pos, el;
    ItElement * e;
    //int diff = rsize - count;
    a->trackChanges[doc]=false;
    for (int i=0; i<count; i++) {
        e = a->getLinkedElement(doc, first+i, &pos, &el);
        if (i==0) {
            e->setRepl(rsize); // mark as merger of everything
            if (rparbreaks.at(i) && !(e->first()))
                e->setParbr("d");
            else if (!(rparbreaks.at(i)) && e->first())
                e->setParbr("n");
            else
                e->setParbr("");
        } else {
            e->setRepl(0); // just mark as results of a split
            if (e->first())
                e->setParbr("n");
            else
                e->setParbr("");
        }
    }
    // rejecting update from server is equal to a new local change - change time must be > sync time!
    QDateTime now = QDateTime::currentDateTime();
    a->setDocDepCTime(doc, now);
    a->trackChanges[doc]=true;
}

void ChangeDialog::appendChange()
{
    if (joinConsChanges==-1)
        joinConsChanges = 0;
    joinConsChanges++;
    nextChangeNum = changenum;
    nextChange();
}

void ChangeDialog::detachChange()
{
    joinConsChanges--;
    nextChangeNum = changenum;
    nextChange();
}
