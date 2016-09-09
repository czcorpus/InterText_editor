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

#include "ServerDialog.h"
#include "ui_ServerDialog.h"
#include <QMessageBox>
#include "ItWindow.h"
#include "ChangeDialog.h"
#include "RemoteAttrDialog.h"

/* TODO: Separate server communication from ServerDialog object! (>ItServerConn object) This is really dirty! */

ServerDialog::ServerDialog(ItWindow *parent, QString path, QString url, QString username, QString passwd, bool stayhidden, bool besilent) :
    QDialog(parent),
    ui(new Ui::ServerDialog)
{
    lastErrCode = 0;
    hidden = stayhidden;
    silent = besilent;
    isConnected = false; connectionFailed = true;
    window = parent;
    markChanges = &(window->syncMarkChanges);
    alTitleFormat = window->alTitleFormat;
    user = username;
    pwd = passwd;
    userid = -1;
    setAttribute(Qt::WA_DeleteOnClose, true);
    alSyncButton = new QPushButton(tr("&Sync"));
    connect(alSyncButton, SIGNAL(clicked()), this, SLOT(alSync()));
    //alSyncButton->setEnabled(false);
    alReleaseButton = new QPushButton(tr("&Release"));
    connect(alReleaseButton, SIGNAL(clicked()), this, SLOT(alRelease()));
    alReleaseButton->setEnabled(false);
    alEditButton = new QPushButton(tr("&Remote properties"));
    connect(alEditButton, SIGNAL(clicked()), this, SLOT(alEdit()));
    alEditButton->setEnabled(false);
    propButton = new QPushButton(tr("&Properties"));
    connect(propButton, SIGNAL(clicked()), this, SLOT(alProp()));
    propButton->setEnabled(false);

    ui->setupUi(this);
    ui->buttonBox->addButton(alSyncButton, QDialogButtonBox::ActionRole);
    ui->buttonBox->addButton(alReleaseButton, QDialogButtonBox::ActionRole);
    ui->buttonBox->addButton(propButton, QDialogButtonBox::ActionRole);
    ui->buttonBox->addButton(alEditButton, QDialogButtonBox::ActionRole);
    net = new QNetworkAccessManager(this);
    //QNetworkConfigurationManager manager;
    //net->setConfiguration(manager.defaultConfiguration());
    net->setCookieJar(new QNetworkCookieJar(this));
    connect(net, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(handleSSLErrors(QNetworkReply*,QList<QSslError>)));
    serverUrl = url;
    storagePath = path;
    connect(ui->alignmentList, SIGNAL(currentTextChanged(QString)), this, SLOT(selAlChange(QString)));
    ui->progressBar->setVisible(false);
    if (!hidden) {
        connect(this, SIGNAL(statusChanged(QString)), this, SLOT(showStatus(QString)));
        show();
    }
}

void ServerDialog::connectToServer()
{
    // does not work in static builds!? why? ... (anyway, we actually do not need it at all)
    /*if (net->networkAccessible()==QNetworkAccessManager::NotAccessible) {
      QMessageBox::critical(this, tr("Network access"), tr("Network is not accessible. Cannot connect to any server."));
      return;
  }*/

    emit settingProgressBarRange(0, 0);

    //QUrl query(serverUrl);
    QUrlQuery postData;
    postData.addQueryItem("req", "version");
    //query.setQuery(postData);
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    emit statusChanged(tr("Connecting to the server..."));
    QEventLoop * loop = new QEventLoop();
    QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
    loop->exec(QEventLoop::ExcludeUserInputEvents);

    QDomElement body;
    if (!extractReply(reply, &body, false, true))
        return;
    serverVersion = body.firstChildElement("version").text().toFloat();
    if (serverVersion<1) {
        invalidResponse(tr("Unsupported InterText server API version. Please, upgrade your InterText server installation."));
        return;
    }
    //query.clear();
    //query.setUrl(serverUrl);
    postData.clear();
    postData.addQueryItem("req", "login");
    postData.addQueryItem("login", user);
    postData.addQueryItem("passwd", pwd);
    //query.setQuery(postData);

    emit statusChanged(tr("Logging in to the server..."));
    reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
    loop->exec(QEventLoop::ExcludeUserInputEvents);

    if (!extractReply(reply, &body))
        return;
    userid = body.firstChildElement("userid").text().toInt();

    //query.clear();
    //query.setUrl(serverUrl);
    postData.clear();
    postData.addQueryItem("req", "users_list");
    //query.setQuery(postData);
    emit statusChanged(tr("Requesting user list..."));
    reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
    loop->exec(QEventLoop::ExcludeUserInputEvents);
    if (!extractReply(reply, &body))
        return;
    QDomElement ulistEl = body.firstChildElement("users");
    if (ulistEl.isNull())
        return;
    QDomNode n = ulistEl.firstChild();
    RemoteUser u;
    QString mytype;
    while(!n.isNull()) {
        QDomElement e = n.toElement();
        if(!e.isNull() && e.tagName()=="user") {
            if (e.attribute("id","")=="")
                continue;
            u.id = e.attribute("id").toInt();
            u.name = e.text();
            mytype = e.attribute("type","");
            if (mytype==USER_ADMIN)
                u.type = Admin;
            else if (mytype==USER_RESP)
                u.type = Resp;
            else if (mytype==USER_EDITOR)
                u.type = Editor;
            else
                continue;
            userList.insert(u.id, u);
        }
        n = n.nextSibling();
    }

    if (!loadAlList())
        return;

    connectionFailed = false;
    isConnected = true;
    emit statusChanged(tr("Connected."));
    emit connected();
}

bool ServerDialog::loadAlList()
{
    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    ui->alignmentList->clear();
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("req", "al_list");
    //query.setQuery(postData);
    emit statusChanged(tr("Requesting alignment list..."));
    QNetworkReply *reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());
    QEventLoop * loop = new QEventLoop();
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
    openProgressBar();
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(setProgressBar(qint64,qint64)));
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    loop->exec(QEventLoop::ExcludeUserInputEvents);
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    closeProgressBar();
    delete loop;
    QDomElement body;
    if (!extractReply(reply, &body))
        return false;

    ui->alignmentList->clear();
    alignments.clear();
    localAlignments.clear();
    QDomElement allistEl = body.firstChildElement("alignments");
    if (allistEl.isNull())
        return false;
    QDomNode n = allistEl.firstChild();
    while(!n.isNull()) {
        QDomElement e = n.toElement();
        if(!e.isNull() && e.tagName()=="al") {
            insertAlignment(e);
        }
        n = n.nextSibling();
    }
    QStringList locals = window->scanDataDir(storagePath);
    QString item;
    QStringList parts;
    foreach (item, locals) {
        parts = item.split('.');
        if (parts.size()>=3)
            insertLocalAlignment(parts[0], parts[1], parts[2]);
    }
    emit statusChanged(tr("List updated."));
    return true;
}

ServerDialog::~ServerDialog()
{
    delete ui;
    delete net;
    delete alSyncButton;
}

void ServerDialog::showStatus(QString status)
{
    ui->statusLabel->setText(status);
}

bool ServerDialog::extractReply(QNetworkReply * reply, QDomElement * body, bool silentOnReqErr, bool firsttouch)
{
    lastErrCode = ERR_OTHER;
    if (reply->error() != QNetworkReply::NoError) {
        handleNetworkError(reply);
        return false;
    }
    QDomDocument doc;
    QString errorMsg; int errorLine, errorColumn;
    QString contents = QString::fromUtf8(reply->readAll().data());
    reply->deleteLater();
    if (!doc.setContent(contents, true, &errorMsg, &errorLine, &errorColumn)) {
        QString errorMessage = QObject::tr("Error parsing XML response at line %1, column %2: %3.").arg( QString().setNum(errorLine), QString().setNum(errorColumn), errorMsg);
        qDebug() << "Invalid server response:" << errorMessage << "\n" << contents;
        if (firsttouch)
            invalidResponse(tr("URL does not refer to a valid InterText server API. Please, correct your server settings."));
        else if (contents.isEmpty())
            invalidResponse(tr("There was no response from the server. Please, check your server's PHP configuration for sufficient maximal execution time limit and maximal upload and/or POST request size limits if uploading large texts or alignments."));
        else
            invalidResponse(tr("Something seems to be broken in your InterText server installation."));
        return false;
    }
    QDomElement docElem = doc.documentElement();
    QDomElement resElem = docElem.firstChildElement("result");
    QDomElement numElem = resElem.firstChildElement("number");
    int retVal = numElem.text().toInt();
    if (retVal!=0) {
        if (silent)
            lastErrCode = retVal;
        if (!silentOnReqErr)
            handleRequestError(retVal, resElem.firstChildElement("text").text());
        return false;
    } else
        lastErrCode = ERR_NOERR;
    QDomElement bodyElem = docElem.firstChildElement("body");
    if (bodyElem.isNull()) {
        qDebug() << "No contents in server response.";
        invalidResponse(tr("Something seems to be broken in your InterText server installation."));
        return false;
    }
    if (body) {
        *body = bodyElem.cloneNode().toElement();
    }
    return true;
}

void ServerDialog::insertAlignment(QDomElement &e)
{
    RemoteAlignment a;
    a.text = e.firstChildElement("text").text();
    a.v1 = e.firstChildElement("v1").text();
    a.v2 = e.firstChildElement("v2").text();
    a.aid = e.attribute("id").toUInt();
    a.v1LastChng = e.firstChildElement("v1").attribute("changed", "");
    a.v2LastChng = e.firstChildElement("v2").attribute("changed", "");
    QString uid = e.firstChildElement("responsible").text();
    if (uid!="")
        a.respUser = uid.toInt();
    uid = e.firstChildElement("editor").text();
    if (uid!="")
        a.edUser = uid.toInt();
    uid = e.firstChildElement("remote").text();
    if (uid!="")
        a.remoteUser = uid.toInt();
    QString stat = e.firstChildElement("status").text();
    if (stat==ALSTAT_OPEN)
        a.status = Open;
    else if (stat==ALSTAT_FINISHED)
        a.status = Finished;
    else if (stat==ALSTAT_CLOSED)
        a.status = Closed;
    else if (stat==ALSTAT_REMOTE)
        a.status = Remote;
    else
        a.status = Blocked;
    if (e.firstChildElement("perm_central_chstruct").text()=="1")
        a.perm_cchstruct = true;
    else
        a.perm_cchstruct = false;
    if (e.firstChildElement("perm_chtext").text()=="1")
        a.perm_chtext = true;
    else
        a.perm_chtext = false;

    a.title = alTitleFormat.arg(a.text, a.v1, a.v2);
    QListWidgetItem * item = new QListWidgetItem(a.title, ui->alignmentList);
    ui->alignmentList->addItem(item);
    alignments.insert(a.title, a);
    updateRAStatus(a.title);
}

void ServerDialog::updateRAStatus(QString key)
{
    //QString key = alKeyByAid(a->aid);
    //qDebug()<<key;
    //qDebug()<<alignments.keys();
    ItAlignment::alignmentInfo alinfo;
    QIcon icon;
    RemoteAlignment a, ra;
    if (alignments.contains(key)) {
        a = alignments.value(key);
        // check whether some of the documents is not in conflict
        if (QFile::exists(QString("%1/%2.%3.xml").arg(storagePath, a.text, a.v1))) {
            alignments[key].v1Stat = getDocStat(a.text, a.v1, serverUrl, QDateTime::fromString(a.v1LastChng, Qt::ISODate));
        } else {
            alignments[key].v1Stat = NonLocal;
        }
        if (QFile::exists(QString("%1/%2.%3.xml").arg(storagePath, a.text, a.v2))) {
            alignments[key].v2Stat = getDocStat(a.text, a.v2, serverUrl, QDateTime::fromString( a.v2LastChng, Qt::ISODate));
        } else {
            alignments[key].v2Stat = NonLocal;
        }
        a = alignments.value(key);
        // check status of the whole alignment
        QString fname_alconf1 = ItAlignment::createInfoFileName(storagePath, a.text, a.v1, a.v2);
        QString fname_alconf2 = ItAlignment::createInfoFileName(storagePath, a.text, a.v2, a.v1);
        if (a.v1Stat==Conflict || a.v2Stat==Conflict) {
            alignments[key].stat = Conflict;
            icon = QIcon(ICON_CONFLICT);
        } else if (ItAlignment::loadAlignmentConf(fname_alconf1, &alinfo) || ItAlignment::loadAlignmentConf(fname_alconf2, &alinfo)) {
            // is not currently open and unsaved...?
            ItAlignment::alignmentInfo curinfo;
            if (window->model!=0)
                curinfo = window->model->alignment->info;
            if ((curinfo.docId==a.text && curinfo.ver[0].name==a.v1 && curinfo.ver[1].name==a.v2) || (curinfo.docId==a.text && curinfo.ver[1].name==a.v1 && curinfo.ver[0].name==a.v2))
                alinfo = curinfo;
            if (alinfo.source != serverUrl) {
                alignments[key].stat = Conflict;
                icon = QIcon(ICON_CONFLICT);
            } else if (a.v1Stat==Obsolete || a.v2Stat==Obsolete) {
                alignments[key].stat = Obsolete;
                icon = QIcon(ICON_OBSOLETE);
            } else if (alinfo.synced>=alinfo.changed && a.v1Stat==Synced && a.v2Stat==Synced) {
                alignments[key].stat = Synced;
                icon = QIcon(ICON_SYNCED);
            } else {
                alignments[key].stat = NonSynced;
                icon = QIcon(ICON_NONSYNCED);
            }
        } else {
            alignments[key].stat = NonLocal;
            icon = QIcon(ICON_NONLOCAL);
        }
        ra = alignments.value(key);
    } else {
        a = localAlignments.value(key);
        icon = QIcon(ICON_LOCAL);
        if (QFile::exists(QString("%1/%2.%3.xml").arg(storagePath, a.text, a.v1))) {
            localAlignments[key].v1Stat = getDocStat(a.text, a.v1, serverUrl, QDateTime(), true);
        } else {
            localAlignments[key].v1Stat = NonLocal;
        }
        if (QFile::exists(QString("%1/%2.%3.xml").arg(storagePath, a.text, a.v2))) {
            localAlignments[key].v2Stat = getDocStat(a.text, a.v2, serverUrl, QDateTime(), true);
        } else {
            localAlignments[key].v2Stat = NonLocal;
        }
        a = localAlignments.value(key);
        if (a.v1Stat==Conflict || a.v2Stat==Conflict) {
            alignments[key].stat = Conflict;
            icon = QIcon(ICON_CONFLICT);
        }
        ra = localAlignments.value(key);
    }
    QColor bgcolor;
    if (a.status==Finished)
        bgcolor = QColor(STATCOLOR_FINISHED);
    else if (a.status==Closed)
        bgcolor = QColor(STATCOLOR_CLOSED);
    else if (a.status==Blocked)
        bgcolor = QColor(STATCOLOR_BLOCKED);
    else if (a.status==Remote)
        if (a.remoteUser==userid) {
            bgcolor = QColor(STATCOLOR_REMOTE);
        } else {
            bgcolor = QColor(STATCOLOR_REMOTE_NA);
        }
    else
        bgcolor = QColor(STATCOLOR_OPEN);

    int n = ui->alignmentList->row(ui->alignmentList->findItems(key, Qt::MatchExactly).first());
    QString status = createStatusTip(ra);
    ui->alignmentList->item(n)->setStatusTip(status);
    ui->alignmentList->item(n)->setToolTip(status);
    ui->alignmentList->item(n)->setBackground(QBrush(bgcolor));
    ui->alignmentList->item(n)->setForeground(QBrush("#000"));
    ui->alignmentList->item(n)->setIcon(icon);
}

void ServerDialog::insertLocalAlignment(QString text, QString v1, QString v2)
{
    QString title1 = alTitleFormat.arg(text, v1, v2);
    QString title2 = alTitleFormat.arg(text, v2, v1);
    if (!alignments.contains(title1) && !alignments.contains(title2)) {
        RemoteAlignment a;
        a.text = text;
        a.v1 = v1;
        a.v2 = v2;
        a.stat = LocalOnly;
        QString fname_alconf = ItAlignment::createInfoFileName(storagePath, a.text, a.v1, a.v2);
        ItAlignment::alignmentInfo alinfo;
        ItAlignment::loadAlignmentConf(fname_alconf, &alinfo);
        /*if (alinfo.ver[0].source != serverUrl)
      a.v1Stat = LocalOnly;
    else
      a.v1Stat = getDocSyncStat(text, alinfo.ver[0]);
    if (alinfo.ver[1].source != serverUrl)
      a.v2Stat = LocalOnly;
    else
      a.v1Stat = getDocSyncStat(text, alinfo.ver[1]);*/
        QListWidgetItem * item = new QListWidgetItem(QIcon(ICON_LOCAL), title1, ui->alignmentList);
        /*QString status = createStatusTip(a);
    item->setStatusTip(status);
    item->setToolTip(status);*/
        ui->alignmentList->addItem(item);
        a.title = alTitleFormat.arg(a.text, a.v1, a.v2);
        localAlignments.insert(a.title, a);
        updateRAStatus(a.title);
    }
}

QString ServerDialog::statText(SyncStat stat)
{
    if (stat==NonLocal)
        return tr("remote only");
    else if (stat==LocalOnly)
        return tr("local only");
    else if (stat==Synced)
        return tr("synced");
    else if (stat==NonSynced)
        return tr("not synced");
    else if (stat==Obsolete)
        return tr("needs update");
    else if (stat==Conflict)
        return tr("conflicting");
    else if (stat==Unknown)
        return tr("unknown");
    qDebug()<<"Undefined status of synchronization:"<<stat;
    return "???";
}

QString ServerDialog::createStatusTip(RemoteAlignment &a)
{
    QString status = QString("Alignment: %1").arg(statText(a.stat));
    status.append(QString("; %1: %2").arg(a.v1, statText(a.v1Stat)));
    status.append(QString("; %1: %2; ").arg(a.v2, statText(a.v2Stat)));
    if (a.respUser>=0)
        status.append(tr("Responsible: %1; ").arg(userList.value(a.respUser).name));
    if (a.edUser>=0)
        status.append(tr("Editor: %1; ").arg(userList.value(a.edUser).name));
    status.append(tr("Status: "));
    if (a.status==Open)
        status.append(tr("open"));
    else if (a.status==Finished)
        status.append(tr("finished"));
    else if (a.status==Closed)
        status.append(tr("closed"));
    else if (a.status==Blocked)
        status.append(tr("blocked"));
    else if (a.status==Remote)
        status.append(tr("remote editor (%1)").arg(userList.value(a.remoteUser).name));
    /*if (!a.perm_chtext) {
    status.append("; ");
    status.append(tr("editing text disabled"));
  }
  if (!a.perm_cchstruct) {
    status.append("; ");
    status.append(tr("change of text structure disabled for central text"));
  }*/
    return status;
}

void ServerDialog::handleNetworkError(QNetworkReply * reply)
{
    //qDebug() << "Network connection error:" << reply->errorString();
    emit statusChanged(tr("Connection failed."));
    int errcode = reply->error();
    if (silent) {
        emit error(errcode, reply->errorString());
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Network connection failed. Error (%1): %2").arg(QString::number(errcode), reply->errorString()));
    }
    reply->deleteLater();
    emit failure();
}

void ServerDialog::handleRequestError(int errcode, QString message)
{
    //qDebug() << "Request error:" << errcode << message;
    emit statusChanged(tr("Request failed."));
    if (silent) {
        //lastErrCode = errcode;
        emit error(errcode, message);
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Error: %1").arg(message));
    }
    emit failure();
}

void ServerDialog::invalidResponse(QString addmessage)
{
    emit statusChanged(tr("Request failed."));
    QString message = tr("Invalid server response. ").append(addmessage);
    //qDebug()<<message;
    if (silent) {
        emit error(399, message);
    } else {
        QMessageBox::warning(this, tr("Error"), message);
    }
    emit failure();
}

void ServerDialog::syncCurrentAlignment()
{
    ItAlignment * a = window->model->alignment;
    QString key = alTitleFormat.arg(a->info.docId, a->info.ver[0].name, a->info.ver[1].name);
    if (!alignments.contains(key)) {
        key = alTitleFormat.arg(a->info.docId, a->info.ver[1].name, a->info.ver[0].name);
        if (!alignments.contains(key)) {
            QMessageBox::critical(this, tr("Error"), tr("Alignment not found on the server."));
            close();
            return;
        }
    }
    RemoteAlignment ra = alignments.value(key);
    //connect(this, SIGNAL(synced()), this, SLOT(close()));
    //connect(this, SIGNAL(failure()), this, SLOT(close()));
    alSyncProcess(ra);
}

void ServerDialog::alSync()
{
    if (!ui->alignmentList->currentItem())
        return;
    QString key = ui->alignmentList->currentItem()->text();
    RemoteAlignment a;
    int reload;
    if (!alignments.contains(key)) {
        a = localAlignments.value(key);
        reload = window->isDependent(a.text, a.v1, a.v2);
        if (reload==-1)
            return;
        alUpload(key);
        return;
    } else {
        a = alignments.value(key);
        reload = window->isDependent(a.text, a.v1, a.v2);
        if (reload==-1)
            return;
        if (a.stat==Conflict) {
            QMessageBox::critical(this, tr("Synchronization"), tr("The remote alignment or one of its documents conflicts with a local alignment (document). You have to rename or remove the conflicting alignment or document."));
            return;
        } else if (a.stat==NonLocal) {
            if (a.v1Stat!=NonLocal && a.v1Stat!=Synced) {
                QMessageBox::warning(this, tr("Synchronization"), tr("Version '%1' is already present locally as part of another alignment and must be synced first. Please, sync it through the corresponding alignment!").arg(a.v1));
                return;
            }
            if (a.v2Stat!=NonLocal && a.v2Stat!=Synced) {
                QMessageBox::warning(this, tr("Synchronization"), tr("Version '%1' is already present locally as part of another alignment and must be synced first. Please, sync it throught the corresponding alignment!").arg(a.v2));
                return;
            }
            alDownloadRequest(a);
        } else {
            alSyncProcess(a);
        }
    }
    if (reload==1)
        window->reloadAlignment(false);
}

void ServerDialog::alRelease()
{
    if (!ui->alignmentList->currentItem())
        return;
    QString key = ui->alignmentList->currentItem()->text();
    if (!alignments.contains(key) || !(alignments[key].status==Remote && alignments[key].remoteUser==userid))
        return;
    RemoteAlignment a = alignments[key];
    ItAlignment::alignmentInfo curinfo;
    if (window->model!=0)
        curinfo = window->model->alignment->info;
    if ((curinfo.docId==a.text && curinfo.ver[0].name==a.v1 && curinfo.ver[1].name==a.v2) || (curinfo.docId==a.text && curinfo.ver[1].name==a.v1 && curinfo.ver[0].name==a.v2)) {
        if (window->maybeSave())
            updateRAStatus(key);
    }
    while (a.stat != Synced) {
        int res = QMessageBox::question(this, tr("Release"),
                                        tr("This alignment is not synced with the server. Do you want to do a final sync before releasing the alignment?"),
                                        QMessageBox::Ok|QMessageBox::No);
        if (res==QMessageBox::Ok) {
            alSyncProcess(a);
        } else {
            break;
        }
        a = alignments[key];
    }

    int res = QMessageBox::question(this, tr("Release"),
                                    tr("You will not be able to submit any more changes to this alignment (without assistence of the InterText server administrator). Do you really want to release your lock on the alignment?"),
                                    QMessageBox::Ok|QMessageBox::No);
    if (res==QMessageBox::Ok)
        if (serverUnlockAlignment(alignments[key].aid)) {
            int res = QMessageBox::question(this, tr("Release"),
                                            tr("Do you want to remove the alignment from your local repository? (You can download it again from the server.)"),
                                            QMessageBox::Yes|QMessageBox::No);
            if (res==QMessageBox::Yes) {
                QString name = QString("%1.%2.%3").arg(a.text, a.v1, a.v2);
                ItAlignment::deleteAlignment(storagePath, name);
                emit alDeletedInRepo(name);
                loadAlList();
            }
        }
}

void ServerDialog::alDownloadRequest(RemoteAlignment &a)
{
    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("req", "al_download");
    postData.addQueryItem("id", QString::number(a.aid));
    //query.setQuery(postData);
    emit statusChanged(tr("Downloading new alignment..."));
    QEventLoop * loop = new QEventLoop();
    QNetworkReply *reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());
    //connect(net, SIGNAL(finished(QNetworkReply*)), this, SLOT(alDownloadReceive(QNetworkReply*)));
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(setProgressBar(qint64,qint64)));
    openProgressBar();
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    loop->exec(QEventLoop::ExcludeUserInputEvents);
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    closeProgressBar();
    //disconnect(net, SIGNAL(finished(QNetworkReply*)), this, SLOT(alDownloadReceive(QNetworkReply*)));
    QDomElement body;
    if (extractReply(reply, &body)) {
        /*QString dataAl = QString::fromUtf8(qUncompress(QByteArray::fromBase64(body.firstChildElement("alignment").text().toAscii())).data());
    QString dataDoc1 = QString::fromUtf8(qUncompress(QByteArray::fromBase64(body.firstChildElement("doc1").text().toAscii())).data());
    QString dataDoc2 = QString::fromUtf8(qUncompress(QByteArray::fromBase64(body.firstChildElement("doc2").text().toAscii())).data());
    ItAlignment * a = new ItAlignment(storagePath);*/
        //qDebug()<<qUncompress(body.firstChildElement("alignment").text().toAscii());
        //qDebug()<<body.firstChildElement("alignment").text();

        QDomElement infoEl = body.firstChildElement("info");
        QDomElement v1El = infoEl.firstChildElement("v1");
        QDomElement v2El = infoEl.firstChildElement("v2");
        QString tname = infoEl.firstChildElement("text").text();
        uint aid = infoEl.firstChildElement("id").text().toInt();
        QString v1name = v1El.firstChildElement("name").text();
        QString v2name = v2El.firstChildElement("name").text();
        QString fname_al = QString("%1.%2.%3.xml").arg(tname, v1name, v2name);
        QString fname_doc1 = QString("%1.%2.xml").arg(tname, v1name);
        QString fname_doc2 = QString("%1.%2.xml").arg(tname, v2name);
        QDateTime ts = QDateTime::fromString(body.attribute("ts"), Qt::ISODate).toLocalTime();
        QDateTime ts1;
        QDateTime ts2;
        if (!v1El.firstChildElement("ctime").text().isEmpty())
            ts1 = QDateTime::fromString(v1El.firstChildElement("ctime").text(), Qt::ISODate).toLocalTime();
        else
            ts1 = ts;
        if (!v2El.firstChildElement("ctime").text().isEmpty())
            ts2 = QDateTime::fromString(v2El.firstChildElement("ctime").text(), Qt::ISODate).toLocalTime();
        else
            ts2 = ts;

        RemoteAlignment ra;
        QString key = alKeyByAid(aid);
        if (key.isEmpty()) {
            emit statusChanged(tr("Download failed."));
            return;
        } else {
            ra = alignments[key];
        }

        QFile file;
        file.setFileName(QString("%1/%2").arg(storagePath, fname_al));
        if (!file.open(QIODevice::WriteOnly)) {
            QString errorMessage = QObject::tr("Error saving file '%1':\n%2").arg(fname_al, file.errorString());
            emit statusChanged(tr("Download failed."));
            QMessageBox::critical(this, tr("Error"), errorMessage);
            return;
        }
        //file.write(qUncompress(QByteArray::fromBase64(body.firstChildElement("alignment").text().toAscii())).data());
        file.write(QByteArray::fromBase64(body.firstChildElement("alignment").text().toLatin1()).data());
        file.close();

        if (ra.v1Stat==NonLocal) {
            file.setFileName(QString("%1/%2").arg(storagePath, fname_doc1));
            if (!file.open(QIODevice::WriteOnly)) {
                QFile::remove(QString("%1/%2").arg(storagePath, fname_al));
                QString errorMessage = QObject::tr("Error saving file '%1':\n%2").arg(fname_doc1, file.errorString());
                emit statusChanged(tr("Download failed."));
                QMessageBox::critical(this, tr("Error"), errorMessage);
                return;
            }
            //file.write(qUncompress(QByteArray::fromBase64(body.firstChildElement("doc1").text().toAscii())).data());
            file.write(QByteArray::fromBase64(body.firstChildElement("doc1").text().toLatin1()).data());
            file.close();
        }

        if (ra.v2Stat==NonLocal) {
            file.setFileName(QString("%1/%2").arg(storagePath, fname_doc2));
            if (!file.open(QIODevice::WriteOnly)) {
                QFile::remove(QString("%1/%2").arg(storagePath, fname_al));
                QFile::remove(QString("%1/%2").arg(storagePath, fname_doc1));
                QString errorMessage = QObject::tr("Error saving file '%1':\n%2").arg(fname_doc2, file.errorString());
                emit statusChanged(tr("Download failed."));
                QMessageBox::critical(this, tr("Error"), errorMessage);
                return;
            }
            //file.write(qUncompress(QByteArray::fromBase64(body.firstChildElement("doc2").text().toAscii())).data());
            file.write(QByteArray::fromBase64(body.firstChildElement("doc2").text().toLatin1()).data());
            file.close();
        }

        ItAlignment a(storagePath);
        a.info.docId = tname;
        a.info.ver[0].name = v1name;
        a.info.ver[1].name = v2name;
        a.info.source = serverUrl;
        a.info.ver[0].source = serverUrl;
        a.info.ver[1].source = serverUrl;
        a.info.synced = ts;
        a.info.changed = ts;
        a.info.ver[0].synced = ts;
        a.info.ver[1].synced = ts;
        if (v1El.firstChildElement("perm_chtext").text()=="1")
            a.info.ver[0].perm_chtext = true;
        else
            a.info.ver[0].perm_chtext = false;
        if (v1El.firstChildElement("perm_chstruct").text()=="1")
            a.info.ver[0].perm_chstruct = true;
        else
            a.info.ver[0].perm_chstruct = false;
        if (v2El.firstChildElement("perm_chtext").text()=="1")
            a.info.ver[1].perm_chtext = true;
        else
            a.info.ver[1].perm_chtext = false;
        if (v2El.firstChildElement("perm_chstruct").text()=="1")
            a.info.ver[1].perm_chstruct = true;
        else
            a.info.ver[1].perm_chstruct = false;
        //QString ctime;
        //ctime = v1El.firstChildElement("ctime").text();
        //if (!ctime.isEmpty())
        a.info.ver[0].changed = ts1;
        //ctime = v2El.firstChildElement("ctime").text();
        //if (!ctime.isEmpty())
        a.info.ver[1].changed = ts2;

        if (!a.loadFile(QString("%1/%2").arg(storagePath, fname_al))) {
            QFile::remove(QString("%1/%2").arg(storagePath, fname_al));
            QFile::remove(QString("%1/%2").arg(storagePath, fname_doc1));
            QFile::remove(QString("%1/%2").arg(storagePath, fname_doc2));
            emit statusChanged(tr("Download failed."));
            QMessageBox::critical(this, tr("Error"), a.errorMessage);
            return;
        }

        // check numbering - should be OK from InterText server, but... better safe than sorry...
        for (int d=0; d<=1; d++) {
            if (a.info.ver[d].numLevels<1) {
                QFile::remove(QString("%1/%2").arg(storagePath, fname_al));
                QFile::remove(QString("%1/%2").arg(storagePath, fname_doc1));
                QFile::remove(QString("%1/%2").arg(storagePath, fname_doc2));
                emit statusChanged(tr("Download failed."));
                QMessageBox::critical(this, tr("Error"), tr("Unknown ID numbering in document '%1'.").arg(a.info.ver[d].name));
                return;
            }
        }

        a.save();
        /*key = alKeyByAid(aid);
    if (!key.isEmpty()) {
      int n = ui->alignmentList->row(ui->alignmentList->findItems(key, Qt::MatchExactly).first());
      ui->alignmentList->item(n)->setIcon(QIcon(ICON_SYNCED));
      alignments[key].stat = Synced;
      alignments[key].v1Stat = Synced;
      alignments[key].v2Stat = Synced;
      QString status = createStatusTip(alignments[key]);
      ui->alignmentList->item(n)->setStatusTip(status);
      ui->alignmentList->item(n)->setToolTip(status);
    }*/
        emit statusChanged(tr("Alignment synchronized."));
        emit alRepoChanged();
        if (!serverLockAlignment(ra.aid))
            QMessageBox::warning(this, tr("Locking alignment"), tr("Locking alignment on the server failed. You will not be able to submit any changes."));
        loadAlList();
    }
}

bool ServerDialog::docDownload(ItAlignment *a, aligned_doc d)
{
    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("req", "doc_download");
    postData.addQueryItem("text", a->info.docId);
    postData.addQueryItem("ver", a->info.ver[d].name);
    //query.setQuery(postData);
    emit statusChanged(tr("Downloading document..."));
    QEventLoop * loop = new QEventLoop();
    QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
    openProgressBar();
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(setProgressBar(qint64,qint64)));
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    loop->exec(QEventLoop::ExcludeUserInputEvents);
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    closeProgressBar();
    delete loop;
    QDomElement body;
    if (!extractReply(reply, &body))
        return false;
    QDomElement infoEl = body.firstChildElement("info");
    QString alignable = infoEl.firstChildElement("al_elements").text();
    QDateTime ts = QDateTime::fromString(body.attribute("ts"), Qt::ISODate).toLocalTime();
    a->info.ver[d].synced = ts;
    QString ctime;
    ctime = infoEl.firstChildElement("ctime").text();
    if (!ctime.isEmpty())
        a->info.ver[d].changed = QDateTime::fromString(ctime, Qt::ISODate).toLocalTime();
    if (infoEl.firstChildElement("perm_chtext").text()=="1")
        a->info.ver[d].perm_chtext = true;
    else
        a->info.ver[d].perm_chtext = false;
    if (infoEl.firstChildElement("perm_chstruct").text()=="1")
        a->info.ver[d].perm_chstruct = true;
    else
        a->info.ver[d].perm_chstruct = false;

    QFile file;
    QString fname = QString("%1/%2.%3.xml").arg(storagePath, a->info.docId, a->info.ver[d].name);
    file.setFileName(fname);
    if (!file.open(QIODevice::WriteOnly)) {
        QString errorMessage = QObject::tr("Error saving file '%1':\n%2").arg(fname, file.errorString());
        emit statusChanged(tr("Download failed."));
        QMessageBox::critical(this, tr("Error"), errorMessage);
        return false;
    }
    //file.write(qUncompress(QByteArray::fromBase64(body.firstChildElement("doc").text().toAscii())).data());
    file.write(QByteArray::fromBase64(body.firstChildElement("doc").text().toLatin1()).data());
    file.close();

    if (!a->loadDoc(d)) {
        QMessageBox::critical(this, tr("Error"), tr("Error opening downloaded document."));
        return false;
    }

    a->createLinks(d, alignable.split(" ", QString::SkipEmptyParts));
    a->detectIdSystem(d);

    return true;
}

bool ServerDialog::serverLockAlignment(int aid)
{
    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("req", "al_lock");
    postData.addQueryItem("id", QString::number(aid));
    //query.setQuery(postData);
    QEventLoop * loop = new QEventLoop();
    QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    loop->exec(QEventLoop::ExcludeUserInputEvents);
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    delete loop;
    QDomElement body;
    if (!extractReply(reply, &body))
        return false;
    else {
        QString key = alKeyByAid(aid);
        if (!key.isEmpty()) {
            int n = ui->alignmentList->row(ui->alignmentList->findItems(key, Qt::MatchExactly).first());
            alignments[key].status = Remote;
            QString status = createStatusTip(alignments[key]);
            ui->alignmentList->item(n)->setStatusTip(status);
            ui->alignmentList->item(n)->setToolTip(status);
            ui->alignmentList->item(n)->setBackground(QBrush(STATCOLOR_REMOTE));
        }
        return true;
    }
}

bool ServerDialog::serverUnlockAlignment(int aid)
{
    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("req", "al_unlock");
    postData.addQueryItem("id", QString::number(aid));
    postData.addQueryItem("stat", "open");
    //query.setQuery(postData);
    QEventLoop * loop = new QEventLoop();
    QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    loop->exec(QEventLoop::ExcludeUserInputEvents);
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    delete loop;
    QDomElement body;
    if (!extractReply(reply, &body))
        return false;
    else {
        QString key = alKeyByAid(aid);
        if (!key.isEmpty()) {
            int n = ui->alignmentList->row(ui->alignmentList->findItems(key, Qt::MatchExactly).first());
            alignments[key].status = Open;
            QString status = createStatusTip(alignments[key]);
            ui->alignmentList->item(n)->setStatusTip(status);
            ui->alignmentList->item(n)->setToolTip(status);
            ui->alignmentList->item(n)->setBackground(QBrush(STATCOLOR_OPEN));
        }
        return true;
    }
}

QString ServerDialog::alKeyByAid(uint aid)
{
    QMapIterator<QString, RemoteAlignment> i(alignments);
    while (i.hasNext()) {
        i.next();
        if (i.value().aid==aid)
            return i.key();
    }
    return "";
}

ServerDialog::SyncStat ServerDialog::getDocStat(QString text, QString version, QString url, QDateTime srvLastChange, bool local_al)
{
    QDir dir(storagePath);
    QStringList list = dir.entryList(QStringList(QString("%1.%2.*.conf").arg(text, version)), QDir::Files, QDir::Name);
    if (list.isEmpty())
        list = dir.entryList(QStringList(QString("%1.*.%2.conf").arg(text, version)), QDir::Files, QDir::Name);
    if (list.isEmpty())
        return NonLocal;
    ItAlignment::alignmentInfo alinfo;
    ItAlignment::verInfo verInfo;
    // is it not currently open...?
    ItAlignment::alignmentInfo curinfo;
    if (window->model!=0)
        curinfo = window->model->alignment->info;
    if (curinfo.docId==text && (curinfo.ver[0].name==version || curinfo.ver[1].name==version))
        alinfo = curinfo;
    else
        ItAlignment::loadAlignmentConf(QString("%1/%2").arg(storagePath, list.first()), &alinfo);
    if (alinfo.ver[0].name==version)
        verInfo = alinfo.ver[0];
    else if (alinfo.ver[1].name==version)
        verInfo = alinfo.ver[1];
    if (local_al) {
        /*if (!verInfo.source.startsWith("http"))
        return LocalOnly;
      else */if (verInfo.source!=url) {
            if (!canUploadDoc(text, version) && lastErrCode!=ERR_PERM_DENIED)
                return Conflict;
            else
                return Unknown;
        } else {
            bool tmp = silent;
            silent = true;
            srvLastChange = docGetLastChange(text, version);
            silent = tmp;
            if (srvLastChange.isNull() && lastErrCode==ERR_NOT_FOUND)
                return LocalOnly;
            if (verInfo.synced < srvLastChange)
                return Obsolete;
            else if (verInfo.changed <= verInfo.synced)
                return Synced;
            else
                return NonSynced;
        }
    } else {
        if (verInfo.source!=url)
            return Conflict;
        else if (verInfo.synced < srvLastChange)
            return Obsolete;
        else if (verInfo.changed <= verInfo.synced)
            return Synced;
        else
            return NonSynced;
    }
}

/*ServerDialog::SyncStat ServerDialog::getDocSyncStat(QString text, ItAlignment::verInfo ver)
{
  QMapIterator<QString, RemoteAlignment> i(alignments);
  RemoteAlignment a;
  while (i.hasNext()) {
    i.next();
    a = i.value();
    if (a.text==text && a.v1==ver.name)
      return a.v1Stat;
    else if (a.text==text && a.v2==ver.name)
      return a.v2Stat;
  }
  return LocalOnly;
}*/

void ServerDialog::alSyncProcess(RemoteAlignment a)
{
    ItAlignment * alignment;
    ItAlignment::alignmentInfo curinfo;
    if (window->model!=0)
        curinfo = window->model->alignment->info;
    bool mustDelete = false;
    if ((curinfo.docId==a.text && curinfo.ver[0].name==a.v1 && curinfo.ver[1].name==a.v2) || (curinfo.docId==a.text && curinfo.ver[1].name==a.v1 && curinfo.ver[0].name==a.v2)) {
        alignment = window->model->alignment;
        connect(this, SIGNAL(synced()), window->model, SLOT(externalDataChange()));
        disconnect(&window->autoSaveTimer, SIGNAL(timeout()), window, SLOT(save())); // no autoSave while updating local texts, it may be aborted!
    } else {
        QString name = QString("%1.%2.%3").arg(a.text, a.v1, a.v2);
        alignment = new ItAlignment(storagePath, name, window->defaultIdNamespaceURI);
        if (alignment->info.docId.isEmpty()) {
            delete alignment;
            name = QString("%1.%2.%3").arg(a.text, a.v2, a.v1);
            alignment = new ItAlignment(storagePath, name, window->defaultIdNamespaceURI);
        }
        mustDelete = true;
    }

    ui->buttonBox->setEnabled(false);

    // renumbering - in case of troubles
    alignment->renumber(0, false);
    alignment->renumber(1, false);

    // download changes
    for (int d=0; d<2; d++) {
        if ((d==0 && a.v1Stat==Obsolete) || (d==1 && a.v2Stat==Obsolete)) {
            emit statusChanged(tr("Syncing document '%1' - downloading changes...").arg(alignment->info.ver[d].name));
            if (docDownloadChanges(alignment, d)) {
                if (!mustDelete) // => syncing currently open alignment!
                    window->model->undoStack->clear();
                alignment->save();
            } else {
                emit statusChanged(tr("Sync failed."));
                if (mustDelete)
                    delete alignment;
                emit reloadNeeded();
                if (hidden)
                    close();
                else
                    updateRAStatus(a.title);
                ui->buttonBox->setEnabled(true);
                if (!mustDelete) // => syncing currently open alignment!
                    connect(&window->autoSaveTimer, SIGNAL(timeout()), window, SLOT(save()));
                emit syncFinished();
                return;
            }
        }
    }
    if (!mustDelete) // => syncing currently open alignment!
        connect(&window->autoSaveTimer, SIGNAL(timeout()), window, SLOT(save()));

    // upload changes
    for (int d=0; d<2; d++) {
        if ((d==0 && a.v1Stat==NonSynced) || (d==1 && a.v2Stat==NonSynced)) {
            emit statusChanged(tr("Syncing document '%1' - uploading changes...").arg(alignment->info.ver[d].name));
            int res = 0;
            if (a.status==Remote && a.remoteUser==userid) {
                while ((res=docUploadChanges(a.aid, alignment, d))==-1) ;
                if (!mustDelete) // => syncing currently open alignment!
                    window->model->undoStack->clear();
                alignment->save();
            } else {
                QMessageBox::warning(this, tr("Synchronization"), tr("You don't have permission to upload any changes."));
            }
            if (res!=1) {
                if (alignment->info.ver[d].synced > alignment->info.ver[d].changed) {
                    alignment->setDocDepCTime(d, alignment->info.ver[d].synced.addSecs(1)); // there are still uncomitted changes!
                    alignment->save();
                }
                emit statusChanged(tr("Sync failed."));
                if (mustDelete)
                    delete alignment;
                //emit reloadNeeded(); // NOT useful more! Some changes may have been comitted.
                if (hidden)
                    close();
                else
                    updateRAStatus(a.title);
                ui->buttonBox->setEnabled(true);
                emit syncFinished();
                return;
            }
        }
        ui->buttonBox->setEnabled(true);

    }

    // upload alignment
    uploadAlignment(alignment, a.aid);
    if (mustDelete)
        delete alignment;
    if (hidden)
        close();
    else
        updateRAStatus(a.title);
    emit syncFinished();
    return;
}

bool ServerDialog::uploadAlignment(ItAlignment * alignment, int aid)
{
    emit statusChanged(tr("Uploading alignment..."));
    QFile * alfile = new QFile(QString("%1/%2").arg(alignment->storagePath, alignment->info.filename));
    alfile->open(QIODevice::ReadOnly);
    /*QString alxml;
    QTextStream str(&alxml);
    alignment->getAlignmentXML(&str);*/
    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    //request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    if (aid>0) {

        QHttpPart reqPart;
        reqPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"req\""));
        reqPart.setBody("update_alignment");
        multiPart->append(reqPart);

        QHttpPart aidPart;
        aidPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"id\""));
        aidPart.setBody(QString::number(aid).toUtf8());
        multiPart->append(aidPart);

        QHttpPart ts1Part;
        ts1Part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"lastsync1\""));
        ts1Part.setBody(alignment->info.ver[0].synced.toUTC().toString(Qt::ISODate).toUtf8());
        multiPart->append(ts1Part);

        QHttpPart ts2Part;
        ts2Part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"lastsync2\""));
        ts2Part.setBody(alignment->info.ver[1].synced.toUTC().toString(Qt::ISODate).toUtf8());
        multiPart->append(ts2Part);
    } else {
        QHttpPart reqPart;
        reqPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"req\""));
        reqPart.setBody("upload_alignment");
        multiPart->append(reqPart);

        QHttpPart tnamePart;
        tnamePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"text\""));
        tnamePart.setBody(alignment->info.docId.toUtf8());
        multiPart->append(tnamePart);

        QHttpPart vname1Part;
        vname1Part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"ver1\""));
        vname1Part.setBody(alignment->info.ver[0].name.toUtf8());
        multiPart->append(vname1Part);

        QHttpPart vname2Part;
        vname2Part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"ver2\""));
        vname2Part.setBody(alignment->info.ver[1].name.toUtf8());
        multiPart->append(vname2Part);
    }

    QHttpPart textPart;
    textPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/xml"));
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant::fromValue(QString("form-data; name=\"data\"; filename=\"%1.%2.%3.xml\"").arg(alignment->info.docId, alignment->info.ver[0].name, alignment->info.ver[1].name)));
    //textPart.setBody(alxml.toUtf8());
    textPart.setBodyDevice(alfile);
    alfile->setParent(multiPart);
    multiPart->append(textPart);

    QEventLoop * loop = new QEventLoop();
    QNetworkReply * reply = net->post(request, multiPart);
    multiPart->setParent(reply);
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
    openProgressBar(tr("Server is importing alignment (this may take several minutes)..."));
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(setProgressBar(qint64,qint64)));
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    loop->exec(QEventLoop::ExcludeUserInputEvents);
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    closeProgressBar();
    delete loop;
    alfile->close();
    QDomElement body;
    if (!extractReply(reply, &body)) {
        emit statusChanged(tr("Sync failed."));
        return false;
    }
    alignment->info.synced = QDateTime::fromString(body.attribute("ts"), Qt::ISODate).toLocalTime();

    /*a.stat=Synced;
alignment->info.synced = QDateTime::currentDateTime();*/
    /*QString key = alKeyByAid(a.aid);
if (!key.isEmpty()) {
  int n = ui->alignmentList->row(ui->alignmentList->findItems(key, Qt::MatchExactly).first());
  ui->alignmentList->item(n)->setIcon(QIcon(ICON_SYNCED));
  alignments[key].stat = Synced;
  alignments[key].v1Stat = Synced;
  alignments[key].v2Stat = Synced;
  QString status = createStatusTip(alignments[key]);
  ui->alignmentList->item(n)->setStatusTip(status);
  ui->alignmentList->item(n)->setToolTip(status);
}*/
    alignment->save();
    emit statusChanged(tr("Alignment synchronized."));
    emit synced();
    return true;
}

bool ServerDialog::uploadDoc(ItAlignment * a, aligned_doc d)
{
    QFile * file = new QFile(QString("%1/%2").arg(a->storagePath, a->info.ver[d].filename));
    if (!file->open(QIODevice::ReadOnly)) {
        emit statusChanged(tr("Upload failed. Cannot open document file."));
        return false;
    }
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart reqPart;
    reqPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"req\""));
    reqPart.setBody("upload_doc");
    multiPart->append(reqPart);

    QHttpPart textPart;
    textPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/xml"));
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant::fromValue(QString("form-data; name=\"data\"; filename=\"%1.%2.xml\"").arg(a->info.docId, a->info.ver[d].name)));
    //textPart.setBody(a->getDocXML(d).toUtf8());
    textPart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(textPart);

    QHttpPart tnamePart;
    tnamePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"text\""));
    tnamePart.setBody(a->info.docId.toUtf8());
    multiPart->append(tnamePart);

    QHttpPart vnamePart;
    vnamePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"ver\""));
    vnamePart.setBody(a->info.ver[d].name.toUtf8());
    multiPart->append(vnamePart);

    QHttpPart elementsPart;
    elementsPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"elements\""));
    elementsPart.setBody(a->getAlignableElementnames(d).join(" ").toUtf8());
    multiPart->append(elementsPart);

    emit statusChanged(tr("Uploading document version '%1' (it may take a minute or two)... ").arg(a->info.ver[d].name));

    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    QEventLoop * loop = new QEventLoop();
    QNetworkReply * reply = net->post(request, multiPart);
    multiPart->setParent(reply);
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
    openProgressBar();
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(setProgressBar(qint64,qint64)));
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    loop->exec(QEventLoop::ExcludeUserInputEvents);
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    closeProgressBar();
    delete loop;
    file->close();
    QDomElement body;
    if (!extractReply(reply, &body)) {
        emit statusChanged(tr("Upload failed."));
        return false;
    }

    QDateTime now = QDateTime::currentDateTime();
    a->setDocDepSTime(d, now);
    return true;
}

void ServerDialog::alUpload(QString name)
{
    RemoteAlignment a = localAlignments.value(name);

    ItAlignment * alignment;
    ItAlignment::alignmentInfo curinfo;
    if (window->model!=0)
        curinfo = window->model->alignment->info;
    bool mustDelete = false;
    if (curinfo.docId==a.text && curinfo.ver[0].name==a.v1 && curinfo.ver[1].name==a.v2) {
        alignment = window->model->alignment;
        connect(this, SIGNAL(synced()), window->model, SLOT(externalDataChange()));
    } else {
        QString name = QString("%1.%2.%3").arg(a.text, a.v1, a.v2);
        alignment = new ItAlignment(storagePath, name, window->defaultIdNamespaceURI);
        mustDelete = true;
    }

    // ensure correct numbering, the server is not so tolerant and imported texts might have anything as IDs...
    if (!hidden) showStatus(tr("Renumbering IDs before upload..."));
    //emit statusChanged(tr("Renumbering IDs before upload..."));
    openProgressBar();
    setProgressBar(0,2);
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    alignment->renumber(0, false);
    setProgressBar(1,2);
    alignment->renumber(1, false);
    setProgressBar(2,2);
    alignment->save();
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    closeProgressBar();
    if (!hidden) showStatus(tr("Starting upload..."));

    SyncStat mystat;
    for (int d=0; d<2; d++) {
        if (d==0)
            mystat = a.v1Stat;
        else
            mystat = a.v2Stat;
        bool skipupload = false;
        if (alignment->info.ver[d].source.startsWith("http")) {
            // the document version is already in sync with some server... is it a different one?
            if (alignment->info.ver[d].source!=serverUrl) {
                if (QMessageBox::question(this, tr("Upload"),
                                          tr("Document version '%1' is in synchronization with another server. You will not be able to synchronize it with the original server anymore. Do you want to continue?").arg(alignment->info.ver[d].name),
                                          QMessageBox::Ok|QMessageBox::Abort) != QMessageBox::Ok)
                    return;
            } else {
                // no, it is already kept in synchronization with the current server! check status of synchronization...
                skipupload = true;
                if (mystat!=Synced) {
                    // but currently not synchronized!

                    if (mystat==Obsolete) {
                        if (!mustDelete) // => syncing currently open alignment!
                            disconnect(&window->autoSaveTimer, SIGNAL(timeout()), window, SLOT(save())); // no autoSave while updating local texts, it may be aborted!
                        emit statusChanged(tr("Syncing document '%1' - downloading changes...").arg(alignment->info.ver[d].name));
                        if (docDownloadChanges(alignment, d)) {
                            if (!mustDelete) // => syncing currently open alignment!
                                window->model->undoStack->clear();
                            alignment->save();
                        } else {
                            emit statusChanged(tr("Sync failed."));
                            if (mustDelete)
                                delete alignment;
                            emit reloadNeeded();
                            if (hidden)
                                close();
                            else
                                updateRAStatus(a.title);
                            if (!mustDelete) // => syncing currently open alignment!
                                connect(&window->autoSaveTimer, SIGNAL(timeout()), window, SLOT(save()));
                            return;
                        }
                    }
                    if (!mustDelete) // => syncing currently open alignment!
                        connect(&window->autoSaveTimer, SIGNAL(timeout()), window, SLOT(save()));

                    emit statusChanged(tr("Syncing document '%1' - uploading changes...").arg(alignment->info.ver[d].name));
                    int res = 0;
                    while ((res=docUploadChanges(-1, alignment, d))==-1) ;
                    if (!mustDelete) // => syncing currently open alignment!
                        window->model->undoStack->clear();
                    alignment->save();
                    if (res!=1) {
                        if (alignment->info.ver[d].synced > alignment->info.ver[d].changed) {
                            alignment->setDocDepCTime(d, alignment->info.ver[d].synced.addSecs(1)); // there are still uncomitted changes!
                            alignment->save();
                        }
                        emit statusChanged(tr("Sync failed."));
                        if (mustDelete)
                            delete alignment;
                        //emit reloadNeeded(); // NOT anymore! Some changes may have been comitted.
                        if (hidden)
                            close();
                        else
                            updateRAStatus(a.title);
                        return;
                    }
                }
            }
        }

        if (!skipupload) {
            if (uploadDoc(alignment, d)) {
                alignment->setDocDepSource(d, serverUrl);
            } else {
                if (mustDelete)
                    delete alignment;
                emit reloadNeeded();
                if (hidden)
                    close();
                else
                    updateRAStatus(a.title);
                return;
            }
        }
    }

    if (uploadAlignment(alignment)) {
        alignment->info.source = serverUrl;
    } else
        return;

    alignment->save();

    if (mustDelete)
        delete alignment;
    if (hidden)
        close();
    else
        loadAlList();
    //updateRAStatus(a.title);

    return;
}

QDateTime ServerDialog::docGetLastChange(QString text, QString version)
{
    QString ver = version;
    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("req", "doc_lastchange");
    postData.addQueryItem("text", text);
    postData.addQueryItem("ver", ver);
    //query.setQuery(postData);
    QEventLoop * loop = new QEventLoop();
    QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());//qDebug()<<query.encodedQuery();
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
    loop->exec(QEventLoop::ExcludeUserInputEvents);
    delete loop;
    QDomElement body;
    if (!extractReply(reply, &body)) {
        return QDateTime();
    }
    QDomElement ctimeEl = body.firstChildElement("ctime");
    QDateTime ret;
    if (!ctimeEl.isNull() && !ctimeEl.text().isEmpty()) {
        ret = QDateTime::fromString(ctimeEl.text(), Qt::ISODate).toLocalTime();
    }
    return ret;
}

bool ServerDialog::docDownloadChanges(ItAlignment * a, aligned_doc d, int aid)
{
    emit statusChanged(tr("Downloading list of changes from the server..."));
    QString ver = a->info.ver[d].name;
    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("req", "doc_changelist");
    if (aid!=0)
        postData.addQueryItem("aid", QString::number(aid));
    else
        postData.addQueryItem("text", a->info.docId);
    postData.addQueryItem("ver", ver);
    postData.addQueryItem("since", a->info.ver[d].synced.toUTC().toString(Qt::ISODate));
    //query.setQuery(postData);
    QEventLoop * loop = new QEventLoop();
    QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());//qDebug()<<query.encodedQuery();
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
    openProgressBar();
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(setProgressBar(qint64,qint64)));
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    loop->exec(QEventLoop::ExcludeUserInputEvents);
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    closeProgressBar();
    delete loop;
    QDomElement body;
    if (!extractReply(reply, &body)) {
        return false;
    }

    QDomElement infoEl = body.firstChildElement("info");
    QDomElement permChtEl = infoEl.firstChildElement("perm_chtext");
    QDomElement permChsEl = infoEl.firstChildElement("perm_chstruct");
    if (!permChtEl.isNull()) {
        if (permChtEl.text()=="1")
            a->info.ver[d].perm_chtext = true;
        else
            a->info.ver[d].perm_chtext = false;
    }
    if (!permChsEl.isNull()) {
        if (permChsEl.text()=="1")
            a->info.ver[d].perm_chstruct = true;
        else
            a->info.ver[d].perm_chstruct = false;
    }

    QDomElement listEl = body.firstChildElement("changelist");
    QDateTime ts = QDateTime::fromString(body.attribute("ts"), Qt::ISODate).toLocalTime();
    QDomNodeList changeList = listEl.childNodes();

    if (changeList.size()>0) {
        emit statusChanged(tr("Applying changes from the server..."));
        ChangeDialog * cd = new ChangeDialog(this, window, a, d, changeList, markChanges);
        int res = cd->exec();
        if (res==QDialog::Rejected) {
            return false;
        }
    }

    a->setDocDepSTime(d, ts);
    if (a->getChangedElements(d).size()>0)
        a->setDocDepCTime(d, QDateTime::currentDateTime());
    else
        a->setDocDepCTime(d, ts);
    return true;
}

int ServerDialog::docUploadChanges(int aid, ItAlignment * a, aligned_doc d)
{
    QList<ItElement* > list = a->getChangedElements(d);
    ItElement * e;
    QString message;
    lastSyncTs = QDateTime();
    int corr=0;
    int num;//qDebug()<<"Uploading changes:";
    openProgressBar();
    for (int i=0; i<list.size(); i++) {
        emit statusChanged(tr("Commiting change %1 of %2...").arg(QString::number(i), QString::number(list.size())));//qDebug()<<tr("Commiting change %1 of %2...").arg(QString::number(i), QString::number(list.size()));
        setProgressBar(i, list.size());
        e = list.at(i);
        num = e->num+corr;
        int j = i;
        if (!e->isVirgin()) { // maybe just a change of parent break...?
            // merge possible?
            if (e->repl()>1 && !(canMerge(a, d, num, e->repl()-1, &message)==1)) {
                if (!handleMergeConflict(a, d, e, message))
                    return 0;
                else
                    return -1;
            }
            // merge
            while (e->repl()>1) {
                if (!serverRequestMerge(aid, a, d, num)) {
                    if (!handleMergeConflict(a, d, e, message))
                        return 0;
                    else
                        return -1;
                } else {
                    e->setRepl(e->repl()-1);
                    corr--;
                }
            }
            // update contents (&split)
            QString text = e->getContents(false);
            while ((j+1)<list.size() && list.at(j+1)->num==e->num) {
                j++;
                e = list.at(j);
                text.append("\n\n");
                text.append(e->getContents(false));
                corr++;
            }
            if (!serverRequestUpdate(aid, a, d, num, text))
                return 0;
            else {
                for (int k=i; k<=j; k++) {
                    list.at(k)->delRepl();
                }
            }
        }
        // update parent break
        for (int k=i; k<=j; k++) {
            e = list.at(k);
            if (e->parbr()=="n") {
                if (serverUpdateParbr(aid, a, d, num, true))
                    e->setParbr("");
                else
                    return 0;
            } else if (e->parbr()=="d"){
                if (serverUpdateParbr(aid, a, d, num, false))
                    e->setParbr("");
                else
                    return 0;
            }
            num++;
        }
        i = j;
    }
    closeProgressBar();
    // commit complete changes
    if (serverCloseUpdate(a, d)) {
        if (!lastSyncTs.isNull())
            a->setDocDepSTime(d, lastSyncTs);
        else
            a->setDocDepCTime(d, a->info.ver[d].synced.addSecs(-1));
        return 1;
    } else
        return 0;
}

bool ServerDialog::serverRequestMerge(int aid, ItAlignment * a, aligned_doc d, int n)
{
    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("req", "merge");
    postData.addQueryItem("aid", QString::number(aid));
    postData.addQueryItem("text", a->info.docId);
    postData.addQueryItem("ver", a->info.ver[d].name);
    postData.addQueryItem("l", QString::number(n));
    postData.addQueryItem("lastsync", a->info.ver[d].synced.toUTC().toString(Qt::ISODate));//qDebug()<<query.encodedQuery();
    //query.setQuery(postData);
    QEventLoop * loop = new QEventLoop();
    QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    loop->exec(QEventLoop::ExcludeUserInputEvents);
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    delete loop;
    QDomElement body;
    if (!extractReply(reply, &body)) {
        return false;
    }
    lastSyncTs = QDateTime::fromString(body.attribute("ts"), Qt::ISODate).toLocalTime();
    a->setDocDepSTime(d, lastSyncTs);
    return true;
}

bool ServerDialog::serverRequestUpdate(int aid, ItAlignment * a, aligned_doc d, int n, QString &text)
{
    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("req", "update");
    postData.addQueryItem("aid", QString::number(aid));
    postData.addQueryItem("text", a->info.docId);
    postData.addQueryItem("ver", a->info.ver[d].name);
    postData.addQueryItem("l", QString::number(n));
    postData.addQueryItem("newtext", text);
    postData.addQueryItem("lastsync", a->info.ver[d].synced.toUTC().toString(Qt::ISODate));//qDebug()<<query.encodedQuery();
    //query.setQuery(postData);
    QEventLoop * loop = new QEventLoop();
    QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    loop->exec(QEventLoop::ExcludeUserInputEvents);
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    delete loop;
    QDomElement body;
    if (!extractReply(reply, &body)) {
        return false;
    }
    lastSyncTs = QDateTime::fromString(body.attribute("ts"), Qt::ISODate).toLocalTime();
    a->setDocDepSTime(d, lastSyncTs);
    return true;
}

bool ServerDialog::serverUpdateParbr(int aid, ItAlignment * a, aligned_doc d, int n, bool add)
{
    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    if (add)
        postData.addQueryItem("req", "newpar");
    else
        postData.addQueryItem("req", "delpar");
    postData.addQueryItem("aid", QString::number(aid));
    postData.addQueryItem("text", a->info.docId);
    postData.addQueryItem("ver", a->info.ver[d].name);
    postData.addQueryItem("l", QString::number(n));
    postData.addQueryItem("lastsync", a->info.ver[d].synced.toUTC().toString(Qt::ISODate));//qDebug()<<query.encodedQuery();
    //query.setQuery(postData);
    QEventLoop * loop = new QEventLoop();
    QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    loop->exec(QEventLoop::ExcludeUserInputEvents);
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    delete loop;
    QDomElement body;
    if (!extractReply(reply, &body)) {
        return false;
    }
    lastSyncTs = QDateTime::fromString(body.attribute("ts"), Qt::ISODate).toLocalTime();
    a->setDocDepSTime(d, lastSyncTs);
    return true;
}

bool ServerDialog::serverCloseUpdate(ItAlignment * a, aligned_doc d)
{
    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("req", "close_update");
    postData.addQueryItem("text", a->info.docId);
    postData.addQueryItem("ver", a->info.ver[d].name);
    //query.setQuery(postData);
    QEventLoop * loop = new QEventLoop();
    QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());//qDebug()<<query.encodedQuery();
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    loop->exec(QEventLoop::ExcludeUserInputEvents);
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    delete loop;
    QDomElement body;
    if (!extractReply(reply, &body))
        return false;
    else
        return true;
}

bool ServerDialog::handleMergeConflict(ItAlignment * a, aligned_doc d, ItElement *e, QString &message)
{
    int res = QMessageBox::warning(this, tr("Merge conflict"), tr("Merge not accepted by the server: %1. Do you want to revert the conflicting part of the text to the state of the server?").arg(message), QMessageBox::Ok|QMessageBox::Abort);
    if (res==QMessageBox::Abort)
        return false;
    else {
        QStringList items;
        for (int i=e->num; i<(e->num+e->repl()-1); i++) {
            items << QString::number(i);
        }
        //QUrl query(serverUrl);
        QNetworkRequest request(serverUrl);
        request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
        QUrlQuery postData;
        postData.addQueryItem("req", "get_elements");
        //query.addQueryItem("id", QString::number(aid));
        postData.addQueryItem("text", a->info.docId);
        postData.addQueryItem("ver", a->info.ver[d].name);
        postData.addQueryItem("items", items.join(","));
        //query.setQuery(postData);
        QEventLoop * loop = new QEventLoop();
        QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());//qDebug()<<query.encodedQuery();
        connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
#ifndef QT_NO_CURSOR
        QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
        loop->exec(QEventLoop::ExcludeUserInputEvents);
#ifndef QT_NO_CURSOR
        QApplication::restoreOverrideCursor();
#endif
        delete loop;
        QDomElement body;
        if (!extractReply(reply, &body))
            return false;


        QDomElement listEl = body.firstChildElement("elements");
        //QDateTime ts = QDateTime::fromString(body.attribute("ts"), Qt::ISODate);
        QDomNodeList elements = listEl.childNodes();
        if (elements.size()<1)
            return false;
        ChangeDialog * cd = new ChangeDialog(this, window, a, d, elements, markChanges, true);
        int res = cd->exec();
        if (res==QDialog::Rejected) {
            return false;
        }

        return true;
    }
}

void ServerDialog::requestItems(QString text, QString ver, QList<int> list)
{
    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    QStringList strs;
    for (int i=0; i<list.size(); i++)
        strs.append(QString::number(list.at(i)));
    postData.addQueryItem("req", "get_elements");
    //query.addQueryItem("id", QString::number(aid));
    postData.addQueryItem("text", text);
    postData.addQueryItem("ver", ver);
    postData.addQueryItem("items", strs.join(","));
    //query.setQuery(postData);
    emit statusChanged(tr("Downloading additional elements..."));
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());//qDebug()<<query.encodedQuery();
    connect(net, SIGNAL(finished(QNetworkReply*)), this, SLOT(itemDownloadReceive(QNetworkReply*)));
}

void ServerDialog::itemDownloadReceive(QNetworkReply * reply)
{
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    disconnect(net, SIGNAL(finished(QNetworkReply*)), this, SLOT(itemDownloadReceive(QNetworkReply*)));
    QDomElement body;
    if (!extractReply(reply, &body))
        return;
    emit receivedItems(body.toElement().firstChildElement("elements").childNodes());
}

int ServerDialog::canMerge(ItAlignment * a, aligned_doc d, int n, int count, QString * message)
{
    QEventLoop * loop = new QEventLoop();
    if (connectionFailed)
        return -1;
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    if (!isConnected) {
        connect(this, SIGNAL(connected()), loop, SLOT(quit()));
        connect(this, SIGNAL(failure()), loop, SLOT(quit()));
        loop->exec(QEventLoop::ExcludeUserInputEvents);
        disconnect(this, SIGNAL(connected()), loop, SLOT(quit()));
        disconnect(this, SIGNAL(failure()), loop, SLOT(quit()));
    }
    emit statusChanged(tr("Checking acceptability of the merge on the server..."));
    QString key = alTitleFormat.arg(a->info.docId, a->info.ver[0].name, a->info.ver[1].name);
    int aid;
    if (!alignments.contains(key)) {
        aid = -1;
        /*#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    emit error(203, tr("Alignment not found on the server."));
    return -1;*/
    } else {
        aid = alignments.value(key).aid;
    }
    QString text = a->info.docId;
    QString ver = a->info.ver[d].name;
    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("req", "canmerge");
    postData.addQueryItem("exaid", QString::number(aid));
    postData.addQueryItem("text", text);
    postData.addQueryItem("ver", ver);
    postData.addQueryItem("l", QString::number(n));
    postData.addQueryItem("n", QString::number(count));
    postData.addQueryItem("lastsync", a->info.ver[d].synced.toUTC().toString(Qt::ISODate));
    //query.setQuery(postData);
    QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());//qDebug()<<query.encodedQuery();
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
    loop->exec(QEventLoop::ExcludeUserInputEvents);
    delete loop;
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    emit statusChanged(tr("Response received."));
    QDomElement body;
    if (!extractReply(reply, &body))
        return -1;
    int res = body.toElement().firstChildElement("result").text().toInt();
    if (res<0) {
        if (QMessageBox::warning(this, tr("Out of sync"), tr("The text on the server has been changed. You are highly advised to sync in order to prevent conflicting changes to the text. Do you want to sync?"), QMessageBox::Ok|QMessageBox::No)==QMessageBox::Ok) {
            syncCurrentAlignment();
            return -2;
        }
    }
    if (message!=0) {
        QString contents;
        QTextStream* str = new QTextStream(&contents);
        body.toElement().firstChildElement("message").save(*str,-1);
        delete str;
        contents.remove(QRegExp("^<[^>]+>"));
        contents.remove(QRegExp("</[^>]+>$"));
        contents.remove(QRegExp("^[^:]*:[^:]*:"));
        *message = contents;
    }
    return res;
}

bool ServerDialog::disconnected()
{
    if (net->networkAccessible()==0)
        return true;
    else
        return false;
}

ServerDialog::RemoteAlignment ServerDialog::getRemAlignmentInfo(ItAlignment * a)
{
    RemoteAlignment ra;
    QMapIterator<QString, RemoteAlignment> i(alignments);
    while (i.hasNext() && !(ra.text==a->info.docId && ra.v1==a->info.ver[0].name && ra.v2==a->info.ver[1].name)) {
        i.next();
        ra = i.value();
    }
    if (ra.text==a->info.docId && ra.v1==a->info.ver[0].name && ra.v2==a->info.ver[1].name)
        return ra;
    else {
        RemoteAlignment era;
        return era;
    }
}

void ServerDialog::selAlChange(const QString &key)
{
    if (alignments.contains(key) && alignments[key].status==Remote && alignments[key].remoteUser==userid) {
        alReleaseButton->setEnabled(true);
    } else {
        alReleaseButton->setEnabled(false);
    }
    if (userList.value(userid).type!=Editor && alignments.contains(key))
        alEditButton->setEnabled(true);
    else
        alEditButton->setEnabled(false);
    if (localAlignments.contains(key) || (alignments.contains(key) && alignments[key].stat!=NonLocal))
        propButton->setEnabled(true);
    else
        propButton->setEnabled(false);
}

void ServerDialog::alEdit()
{
    QString key = ui->alignmentList->currentItem()->text();
    RemoteAttrDialog * ed = new RemoteAttrDialog(this, key);
    ed->exec();
}

ServerDialog::RemoteUser ServerDialog::curUser() {
    return userList.value(userid);
}

bool ServerDialog::updateAlAttributes(RemoteAlignment &a)
{
    QString qstat;
    if (a.status==Open) qstat = ALSTAT_OPEN;
    else if (a.status==Finished) qstat = ALSTAT_FINISHED;
    else if (a.status==Closed) qstat = ALSTAT_CLOSED;
    else if (a.status==Blocked) qstat = ALSTAT_BLOCKED;
    else if (a.status==Remote) qstat = ALSTAT_REMOTE;
    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("req", "al_update_attrs");
    postData.addQueryItem("id", QString::number(a.aid));
    postData.addQueryItem("editor", QString::number(a.edUser));
    postData.addQueryItem("resp", QString::number(a.respUser));
    postData.addQueryItem("status", qstat);
    postData.addQueryItem("r_user", QString::number(a.remoteUser));
    postData.addQueryItem("pedit", QString::number(QVariant(a.perm_chtext).toInt()));
    postData.addQueryItem("pcchstr", QString::number(QVariant(a.perm_cchstruct).toInt()));
    //query.setQuery(postData);
    QEventLoop * loop = new QEventLoop();
    QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    loop->exec(QEventLoop::ExcludeUserInputEvents);
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    delete loop;
    loadAlList();
    if (!extractReply(reply))
        return false;
    else
        return true;
}

QStringList ServerDialog::getRemoteDocList()
{
    QStringList docs;
    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("req", "doc_list");
    //query.setQuery(postData);
    emit statusChanged(tr("Requesting document list..."));
    QNetworkReply *reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());
    QEventLoop * loop = new QEventLoop();
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
    openProgressBar();
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(setProgressBar(qint64,qint64)));
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    loop->exec(QEventLoop::ExcludeUserInputEvents);
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    closeProgressBar();
    delete loop;
    QDomElement body;
    emit statusChanged(tr("Response received."));
    if (!extractReply(reply, &body))
        return docs;

    QDomElement doclistEl = body.firstChildElement("docs");
    if (doclistEl.isNull())
        return docs;
    QDomNode n = doclistEl.firstChild();
    while(!n.isNull()) {
        QDomElement e = n.toElement();
        if(!e.isNull() && e.tagName()=="doc") {
            docs << QString("%1: %2").arg(e.firstChildElement("text").text(), e.firstChildElement("ver").text());
        }
        n = n.nextSibling();
    }
    return docs;
}

bool ServerDialog::canUploadDoc(QString textname, QString vername)
{
    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("req", "doc_canupload");
    postData.addQueryItem("text", textname);
    postData.addQueryItem("ver", vername);
    //query.setQuery(postData);
    QEventLoop * loop = new QEventLoop();
    QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());//qDebug()<<query.encodedQuery();
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    loop->exec(QEventLoop::ExcludeUserInputEvents);
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    delete loop;
    QDomElement body;
    if (!extractReply(reply, &body, true))
        return false;
    else
        return true;
}

bool ServerDialog::canUploadAlignment(QString textname, QString ver1name, QString ver2name)
{
    //QUrl query(serverUrl);
    QNetworkRequest request(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("req", "al_canupload");
    postData.addQueryItem("text", textname);
    postData.addQueryItem("ver1", ver1name);
    postData.addQueryItem("ver2", ver2name);
    //query.setQuery(postData);
    QEventLoop * loop = new QEventLoop();
    QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());//qDebug()<<query.encodedQuery();
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
    loop->exec(QEventLoop::ExcludeUserInputEvents);
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
    delete loop;
    QDomElement body;
    if (!extractReply(reply, &body, true))
        return false;
    else
        return true;
}


void ServerDialog::alProp()
{
    bool del = true;
    QString key = ui->alignmentList->currentItem()->text();
    RemoteAlignment ra;
    if (alignments.contains(key))
        ra = alignments.value(key);
    else
        ra = localAlignments.value(key);
    int reload = window->isDependent(ra.text, ra.v1, ra.v2);
    if (reload==-1)
        return;
    QString al = QString("%1.%2.%3").arg(ra.text, ra.v1, ra.v2);
    ItAlignment *a;
    if (window->model!=0 && al==window->model->alignment->info.name) {
        a = window->model->alignment;
        del = false;
    } else
        a = new ItAlignment(window->storagePath, al, window->defaultIdNamespaceURI);
    AlignmentAttrDialog *d = new AlignmentAttrDialog(this, a, del);
    if (!del)
        connect(d, SIGNAL(accepted()), window, SLOT(propertiesChanged()));
    if (reload==1)
        connect(d, SIGNAL(accepted()), window, SLOT(reloadAlignmentSilently()));
    d->exec();
}

void ServerDialog::handleSSLErrors(QNetworkReply * reply, const QList<QSslError> & errors)
{
    reply->ignoreSslErrors();
}

void ServerDialog::openProgressBar(QString fullmsg)
{
    progrBarFullMsg = fullmsg;
    ui->progressBar->setVisible(true);
    ui->progressBar->setRange(0,0);
    emit openingProgressBar();
    emit settingProgressBarRange(0, 0);
}

void ServerDialog::setProgressBar(qint64 val, qint64 max)
{
    if (ui->progressBar->maximum()==0 && max>=0) {
        ui->progressBar->setMaximum(1000);
        emit settingProgressBarRange(0, 1000);
    }
    if (max>=0) {
        int p = int(((val*1.0/max)*1000)+0.5);//fqDebug()<<val<<max<<p;
        ui->progressBar->setValue(p);
        emit settingProgressBarValue(p);
        if (p==1000 && !progrBarFullMsg.isEmpty()) {
            emit statusChanged(progrBarFullMsg);
            ui->progressBar->setRange(0,0);
            emit settingProgressBarRange(0, 0);
        }
    }
}

void ServerDialog::closeProgressBar()
{
    emit closingProgressBar();
    ui->progressBar->setVisible(false);
}
