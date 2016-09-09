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

#ifndef SERVERDIALOG_H
#define SERVERDIALOG_H

#include <QDialog>
#include <QtNetwork>
#include <QtXml>
#include "ItAlignment.h"

#define ICON_SYNCED ":/images/16/confirmed.png"
#define ICON_NONSYNCED ":/images/16/locally-modified.png"
#define ICON_OBSOLETE ":/images/16/update-required.png"
#define ICON_CONFLICT ":/images/16/conflicting.png"
#define ICON_LOCAL ":/images/16/local.png"
#define ICON_NONLOCAL ":/images/16/nonlocal.png"

#define USER_ADMIN "admin"
#define USER_RESP "resp"
#define USER_EDITOR "editor"

#define ALSTAT_OPEN "open"
#define ALSTAT_FINISHED "finished"
#define ALSTAT_CLOSED "closed"
#define ALSTAT_BLOCKED "blocked"
#define ALSTAT_REMOTE "remote editor"

#define STATCOLOR_OPEN "#fff"
#define STATCOLOR_FINISHED "#aea"
#define STATCOLOR_CLOSED "#ccc"
#define STATCOLOR_BLOCKED "#fdd"
#define STATCOLOR_REMOTE "#bbf"
#define STATCOLOR_REMOTE_NA "#fbf"

#define ERR_NOERR 0
#define ERR_UNAUTH_USER 1
#define ERR_UNKNOWN_CMD 2
#define ERR_PERM_DENIED 3
#define ERR_NOT_FOUND   4
#define ERR_TEXT_CHANGED 5
#define ERR_OTHER       65535

class ItWindow;

namespace Ui {
class ServerDialog;
}

/* TODO: Separate server communication from ServerDialog object! (>ItServer object) This is really dirty! */

class ServerDialog : public QDialog
{
    Q_OBJECT

public:
    bool isConnected, connectionFailed;
    int lastErrCode;
    QString alTitleFormat;
    enum SyncStat { NonLocal = 0, LocalOnly, Synced, NonSynced, Obsolete, Conflict, Unknown };
    enum RemoteAlStatus { Open = 0, Finished, Closed, Blocked, Remote};
    enum UserType { Admin = 0, Resp, Editor };
    struct RemoteUser
    {
        int id;
        UserType type;
        QString name;
        RemoteUser() : id(0), type(Editor), name("???") {}
    };
    QMap<int, RemoteUser> userList;
    struct RemoteAlignment
    {
        QString title;
        uint aid;
        QString text;
        QString v1;
        QString v2;
        QString v1LastChng;
        QString v2LastChng;
        SyncStat stat;
        SyncStat v1Stat;
        SyncStat v2Stat;
        int respUser;
        int edUser;
        int remoteUser;
        bool perm_cchstruct;
        bool perm_chtext;
        RemoteAlStatus status;
        RemoteAlignment() : title(""), aid(0), text(""), v1(""), v2(""), v1LastChng(""), v2LastChng(""), stat(Unknown), v1Stat(Unknown), v2Stat(Unknown),respUser(-1), edUser(-1), remoteUser(-1), perm_cchstruct(true), perm_chtext(true), status(Open) {}
    };
    QMap<QString, RemoteAlignment> alignments, localAlignments;
    explicit ServerDialog(ItWindow *parent, QString path, QString url, QString username, QString passwd, bool stayhidden = false, bool besilent = false);
    ~ServerDialog();
    void connectToServer();
    int canMerge(ItAlignment * a, aligned_doc d, int n, int count, QString * message = 0); // 1=yes; 0=no; -1=don't know (cannot connect, etc.)
    void requestItems(QString text, QString ver, QList<int> list);
    bool disconnected();
    RemoteAlignment getRemAlignmentInfo(ItAlignment * a);
    RemoteUser curUser();
    bool updateAlAttributes(RemoteAlignment &a);
    QStringList getRemoteDocList();
    bool docDownload(ItAlignment *a, aligned_doc d);
    bool docDownloadChanges(ItAlignment *a, aligned_doc d, int aid=0);
    bool canUploadDoc(QString textname, QString vername);
    bool canUploadAlignment(QString textname, QString ver1name, QString ver2name);
    QDateTime docGetLastChange(QString text, QString version);
public slots:
    void syncCurrentAlignment();
    void openProgressBar(QString fullmsg = QString());
    void setProgressBar(qint64 val, qint64 max);
    void closeProgressBar();
signals:
    void alDeletedInRepo(QString alname);
    void statusChanged(QString status);
    void connected();
    void synced();
    void failure();
    void reloadNeeded();
    void error(int code, QString message);
    void receivedItems(QDomNodeList elements);
    void syncFinished();
    void openingProgressBar();
    void settingProgressBarRange(int min, int max);
    void settingProgressBarValue(int value);
    void closingProgressBar();
    void alRepoChanged();
private:
    Ui::ServerDialog *ui;
    ItWindow * window;
    QNetworkAccessManager * net;
    QString serverUrl;
    int serverVersion;
    QString user, pwd;
    int userid;
    QString storagePath;
    QPushButton * alSyncButton;
    QPushButton * alReleaseButton;
    QPushButton * alEditButton;
    QPushButton * propButton;
    QString verInSync;
    QDateTime lastSyncTs;
    QString progrBarFullMsg;
    bool * markChanges;
    bool silent, hidden;
    bool loadAlList();
    bool extractReply(QNetworkReply * reply, QDomElement * body = 0, bool silentOnReqErr = false, bool firsttouch = false);
    void insertAlignment(QDomElement &e);
    void insertLocalAlignment(QString text, QString v1, QString v2);
    void alDownloadRequest(RemoteAlignment &a);
    QString alKeyByAid(uint aid);
    SyncStat getDocStat(QString text, QString version, QString url, QDateTime srvLastChange, bool local_al=false);
    /*SyncStat getDocSyncStat(QString text, ItAlignment::verInfo ver);*/
    QString statText(SyncStat stat);
    QString createStatusTip(RemoteAlignment &a);
    void updateRAStatus(QString key);
    void alSyncProcess(RemoteAlignment a);
    void alUpload(QString name);
    void syncFailure(QString text);
    bool handleMergeConflict(ItAlignment *a, aligned_doc d, ItElement *e, QString &message);
    int docUploadChanges(int aid, ItAlignment * a, aligned_doc d);
    bool serverRequestMerge(int aid, ItAlignment * a, aligned_doc d, int n);
    bool serverRequestUpdate(int aid, ItAlignment * a, aligned_doc d, int n, QString &text);
    bool serverUpdateParbr(int aid, ItAlignment * a, aligned_doc d, int n, bool add);
    bool serverCloseUpdate(ItAlignment *a, aligned_doc d);
    bool serverLockAlignment(int aid);
    bool serverUnlockAlignment(int aid);
    bool uploadAlignment(ItAlignment * alignment, int aid = -1);
    bool uploadDoc(ItAlignment * a, aligned_doc d);
private slots:
    void handleNetworkError(QNetworkReply * reply);
    void handleRequestError(int errcode, QString message);
    void handleSSLErrors(QNetworkReply * reply, const QList<QSslError> & errors);
    void invalidResponse(QString addmessage);
    void alSync();
    void alRelease();
    void alEdit();
    //void alDownloadReceive(QNetworkReply * reply);
    void itemDownloadReceive(QNetworkReply * reply);
    void showStatus(QString status);
    void selAlChange(const QString &key);
    void alProp();
};

#endif // SERVERDIALOG_H
