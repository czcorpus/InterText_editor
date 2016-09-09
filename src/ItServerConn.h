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

#ifndef ITSERVER_H
#define ITSERVER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QtXml>

#define ERR_NOERR 0
#define ERR_UNAUTH_USER 1
#define ERR_UNKNOWN_CMD 2
#define ERR_PERM_DENIED 3
#define ERR_NOT_FOUND   4
#define ERR_TEXT_CHANGED 5
#define ERR_OTHER       65535

class ItServerConn : public QObject
{
    Q_OBJECT

public:
    ItServerConn(QString url, QString username, QString passwd);
    ~ItServerConn();

    int getLastErrCode();
    bool netAvailable();
    bool login();
    int canMerge(int aid, QString text, QString ver, QString ver2, int n, int count, QDateTime lastsynced, QString * message = 0);
    // 1=yes; 0=no; -1=don't know (cannot connect, etc.)
    void docGetLastChange(QString text, QString ver);

signals:
    void statusChanged(QString status);
    void error(int code, QString message);
    void failure();
    void connected();
    void docGetLastChangeMessage(QString text, QString ver, QDateTime ret);

protected:
    bool checkVersion();

private:
    QString server_url;
    QString server_username;
    QString server_passwd;
    int server_version;
    int server_userid;
    QNetworkAccessManager * net;
    int lastErrCode;

    bool extractReply(QNetworkReply * reply, QDomElement * body = 0, bool firsttouch = false);

private slots:
    void handleNetworkError(QNetworkReply * reply);
    void handleRequestError(int errcode, QString message);
    void handleSSLErrors(QNetworkReply * reply, const QList<QSslError> & errors);
    void invalidResponse(QString addmessage);
};

#endif // ITSERVER_H
