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

#include "ItServerConn.h"
#include <QNetworkCookieJar>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>

ItServerConn::ItServerConn(QString url, QString username, QString passwd)
{
    server_url = url;
    server_username = username;
    server_passwd = passwd;
    net = new QNetworkAccessManager(this);
    net->setCookieJar(new QNetworkCookieJar(this));
    connect(net, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(handleSSLErrors(QNetworkReply*,QList<QSslError>)));
}

ItServerConn::~ItServerConn()
{
    delete net;
}

bool ItServerConn::login()
{
    if (!checkVersion())
        return false;
    QDomElement body;
    QUrl query(server_url);
    QNetworkRequest request(server_url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("req", "login");
    postData.addQueryItem("login", server_username);
    postData.addQueryItem("passwd", server_passwd);
    query.setQuery(postData);

    emit statusChanged(tr("Logging in to the server..."));
    QEventLoop * loop = new QEventLoop();
    QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
    loop->exec();//QEventLoop::ExcludeUserInputEvents);

    if (!extractReply(reply, &body))
        return false;
    server_userid = body.firstChildElement("userid").text().toInt();
    return true;
}

int ItServerConn::canMerge(int aid, QString text, QString ver, QString ver2, int n, int count, QDateTime lastsynced, QString * message)
{
    QEventLoop * loop = new QEventLoop();

    emit statusChanged(tr("Checking acceptability of the merge on the server..."));

    //QUrl query(server_url);
    QNetworkRequest request(server_url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("req", "canmerge");
    if (aid>0)
        postData.addQueryItem("exaid", QString::number(aid));
    postData.addQueryItem("text", text);
    postData.addQueryItem("ver", ver);
    postData.addQueryItem("ver2", ver2);
    postData.addQueryItem("l", QString::number(n));
    postData.addQueryItem("n", QString::number(count));
    lastsynced.setTimeSpec(Qt::OffsetFromUTC);
    postData.addQueryItem("lastsync", lastsynced.toUTC().toString(Qt::ISODate));
    //query.setQuery(postData);
    QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());//qDebug()<<query.encodedQuery();
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
    loop->exec();//QEventLoop::ExcludeUserInputEvents);
    delete loop;
    emit statusChanged(tr("Response received."));
    QDomElement body;
    if (!extractReply(reply, &body))
        return -1;
    int res = body.toElement().firstChildElement("result").text().toInt();
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

void ItServerConn::docGetLastChange(QString text, QString ver)
{
    emit statusChanged(tr("Checking for text changes on the server..."));
    //QUrl query(server_url);
    QNetworkRequest request(server_url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QUrlQuery postData;
    postData.addQueryItem("req", "doc_lastchange");
    postData.addQueryItem("text", text);
    postData.addQueryItem("ver", ver);
    //query.setQuery(postData);
    QEventLoop * loop = new QEventLoop();
    QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());//qDebug()<<query.encodedQuery();
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
    loop->exec();//QEventLoop::ExcludeUserInputEvents);
    delete loop;
    QDomElement body;
    if (!extractReply(reply, &body)) {
        return;
    }
    QDomElement ctimeEl = body.firstChildElement("ctime");
    QDateTime ret;
    if (!ctimeEl.isNull() && !ctimeEl.text().isEmpty()) {
        ret = QDateTime::fromString(ctimeEl.text(), Qt::ISODate).toLocalTime();
    }
    emit docGetLastChangeMessage(text, ver, ret);
    //return ret;
}

int ItServerConn::getLastErrCode()
{
    return lastErrCode;
}

bool ItServerConn::netAvailable()
{
    if (net->networkAccessible()==0)
        return false;
    else
        return true;
}

bool ItServerConn::checkVersion()
{
    //QUrl query(server_url);
    QUrlQuery postData;
    postData.addQueryItem("req", "version");
    //query.setQuery(postData);
    QNetworkRequest request(server_url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    emit statusChanged(tr("Connecting to the server..."));
    QEventLoop * loop = new QEventLoop();
    QNetworkReply * reply = net->post(request, postData.query(QUrl::EncodeUnicode).toLatin1());
    connect(reply, SIGNAL(finished()), loop, SLOT(quit()));
    loop->exec();//QEventLoop::ExcludeUserInputEvents);

    QDomElement body;
    if (!extractReply(reply, &body, true))
        return false;
    server_version = body.firstChildElement("version").text().toFloat();
    if (server_version<2) {
        invalidResponse(tr("Unsupported InterText server API version. Please, upgrade your InterText server installation."));
        return false;
    }
    return true;
}

bool ItServerConn::extractReply(QNetworkReply * reply, QDomElement * body, bool firsttouch)
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
        QString errorMessage = QObject::tr("Error parsing XML response at line %1, column %2: %3.").arg( QString().setNum(errorLine),
                                                                                                         QString().setNum(errorColumn), errorMsg);
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
        lastErrCode = retVal;
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

void ItServerConn::handleNetworkError(QNetworkReply * reply)
{
    //qDebug() << "Network connection error:" << reply->errorString();
    emit statusChanged(tr("Connection failed."));
    int errcode = reply->error();
    emit error(errcode, reply->errorString());
    reply->deleteLater();
    emit failure();
}

void ItServerConn::handleRequestError(int errcode, QString message)
{
    //qDebug() << "Request error:" << errcode << message;
    emit statusChanged(tr("Request failed."));
    //lastErrCode = errcode;
    emit error(errcode, message);
    emit failure();
}

void ItServerConn::handleSSLErrors(QNetworkReply * reply, const QList<QSslError> & errors)
{
    reply->ignoreSslErrors();
}

void ItServerConn::invalidResponse(QString addmessage)
{
    emit statusChanged(tr("Request failed."));
    QString message = tr("Invalid server response. ").append(addmessage);
    //qDebug()<<message;
    emit error(399, message);
    emit failure();
}
