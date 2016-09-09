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


#ifndef IT_ALIGNMENT_H
#define IT_ALIGNMENT_H

#include <QtXml>
#include "ItCommon.h"
#include "ItDocument.h"
#include "ItSearchBar.h"
#include "ItSentenceSplitter.h"

#define DOCUMENTS 2

#define LINK_MANUAL 1
#define LINK_AUTO 2
#define LINK_PLAIN 3
#define LINK_MANUAL_NAME "man"
#define LINK_AUTO_NAME "auto"
#define LINK_PLAIN_NAME "plain"
#define DEFAULT_STATUS 1
#define DEFAULT_MARK 0

#define MODE_MANUAL 1
#define MODE_AUTOUPDATE 2
#define MODE_AUTOROLL_5 3
#define MODE_AUTOROLL_2 4
#define MODE_AUTOROLL_0 5
#define DEFAULT_MODE 4

#define INVALID_POSITION 2147483647

typedef int aligned_doc;

class ItAlignment
{

public:

    //    struct Note
    //    {
    //        QString author;
    //        QDateTime ts;
    //        QString Note;
    //    };

    struct dependentLink
    {
        unsigned int pos;
        unsigned short status;
        unsigned short mark;
        //QList<Note*> notes;
        QString id;
    };

    struct Link
    {
        //unsigned int pos;
        QList<dependentLink*> depLinks;
        ItElement *element;
        unsigned short status;
        unsigned short mark;
        //QList<Note*> notes;
    };

    struct verInfo
    {
        QString name;
        QString filename;
        QDateTime synced;
        QDateTime changed;
        QString source;
        int numLevels;
        QString numPrefix;
        QString numParentPrefix;
        bool perm_chstruct;
        bool perm_chtext;
    };

    struct alignmentInfo
    {
        QString docId;
        QString name;
        QString filename;
        QString source;
        QString infoName;
        QDateTime synced;
        QDateTime changed;
        verInfo ver [2];
        bool autoUpdateStatus;
    };

    struct statRec
    {
        int position;
        ushort status;
    };

    struct searchMatch {
        bool set;
        bool replaced;
        aligned_doc doc;
        int pos;
        int el;
        int strpos;
        int len;
        QStringList capturedTexts;
    };

    QString storagePath;
    alignmentInfo info;
    ushort mode;
    QString errorMessage;
    searchMatch lastMatch;
    bool trackChanges [2];
    bool ignorePermissions;
    QString idNamespaceURI;

    ItAlignment(QString path = QString(), QString name = QString(), QString namespaceURI=QString());
    ~ItAlignment();
    QString infoFileName();
    void setIdNamespaceURI(QString &uri);
    static QString createInfoFileName(QString path, QString text, QString v1, QString v2);
    static bool loadAlignmentConf(QString filename, alignmentInfo * myinfo);
    static void saveAlignmentConf(QString filename, alignmentInfo * myinfo);
    static void deleteAlignment(QString repository_path, QString al);
    bool open();
    bool save();
    bool loadFile(const QString &filename, unsigned short defaultStatus = DEFAULT_STATUS);
    bool saveFile(const QString &filename, QString doc1name = QString(), QString doc2name = QString());
    bool loadDoc(aligned_doc d, QString path = "");
    bool setDocXml(aligned_doc d, QString &xml);
    bool setDocXml(aligned_doc d, QByteArray &xml);
    void createLinks(aligned_doc d, QStringList alignable, QList<int> groups = QList<int>());
    bool saveDoc(int i, const QString &filename);
    bool alExists();
    bool alDocExists();
    bool alVerExists(int ver = 0);
    //void test();
    int maxPos();
    QVariant getContents(aligned_doc doc, int pos, bool prepend = true, bool ignoreMarkup=false, const QList<Replacement> *transformations=0);
    QStringList getIDs(aligned_doc doc, int pos);
    int getSize(aligned_doc doc, int pos);
    bool updateContents(aligned_doc doc, int pos, int el, QString string);
    ushort getStat(int pos);
    QString getStatName(int pos);
    ushort getMark(int pos);
    bool is1to1(int pos);
    void undoStatusChanges(QList<statRec> &slist);
    void setDocDepCTime(int document, QDateTime time);
    void setDocDepSTime(int document, QDateTime time);
    void setAlDepCTime(QDateTime time); // set new common ctime for the alignment and all deps
    void getAlDepCTimes(int document, QList<QDateTime> * ctimes);
    void setAlDepCTimes(int document, QList<QDateTime> * ctimes); // (re)set (old) individual ctime for single deps
    void setDocDepSource(int document, QString source);
    //void setAlDepSource(QString source);

    bool moveUp(aligned_doc doc, int pos);
    bool moveDown(aligned_doc doc, int pos);
    bool shift(aligned_doc doc, int pos);
    bool pop(aligned_doc doc, int pos);
    void toggleMark(int pos);
    void setMark(int pos, ushort mark);
    void toggleStat(int pos);
    void setStat(int pos, ushort status);
    bool isFirst(aligned_doc doc, int pos, int el);
    bool canMerge(aligned_doc doc, int pos, int el, int count = 1);
    bool canMergeDeps(aligned_doc doc, int pos, int el, int count = 1);
    bool canMergeParent(aligned_doc doc, int pos, int el);
    bool canSplitParent(aligned_doc doc, int pos, int el);
    void updateStat(int pos);
    void scanStat(int pos, QList<statRec> * slist = 0);
    void getNumbering(int document, int *levels, QString *prefix, QString *parent_prefix);
    void setNumbering(int document, int levels, QString &prefix, QString &parent_prefix);
    void detectIdSystem(aligned_doc d);
    bool merge(aligned_doc doc, int pos, int el, int count = 1);
    bool split(aligned_doc doc, int pos, int el, QStringList newstrings);
    bool removeAfter(aligned_doc doc, int pos, int el, int count = 1);
    bool splitParent(aligned_doc doc, int pos, int el); // split at (just before) the given element
    bool mergeParent(aligned_doc doc, int pos, int el); // merge to the previous sibling element
    void syncDepsPermissions();
    bool canDepsChtext(ushort doc);
    bool canDepsChstruct(ushort doc);
    bool crossOrderAlignmentAllowed();

    bool realign(int fromPos, int toPos, QList<QStringList> alignedIds [2], ushort status, QList<statRec> * slist = 0);
    void applySentenceSplitter(aligned_doc d, ItSentenceSplitter * splitter, QStringList textElements, QString &elName);

    void renumber(int document, bool updateCTime=true);

    uint find(int startpos, bool forward, ItSearchBar::searchSide side, ItSearchBar::searchType stype, QString str, bool ignoreMarkup, QList<Replacement> &transform);
    void resetLastMatch();
    void setLastMatch(bool replaced, aligned_doc doc, int pos, int el, int strpos, int len, QStringList captures = QStringList());

    bool export_text(const QString &filename, aligned_doc doc, int startPos, int endPos, QString head="", QString el_sep="\n", QString par_sep="<p>\n", QString foot="");

    // tracking changes
    bool isVirgin(aligned_doc doc, int pos, int el);
    void setVirgin(aligned_doc doc, int pos, int el);
    int getRepl(aligned_doc doc, int pos, int el);
    void setRepl(aligned_doc doc, int pos, int el, int val);
    QString getParbr(aligned_doc doc, int pos, int el);
    void setParbr(aligned_doc doc, int pos, int el, QString val);

    // sync
    ItElement * getLinkedElement(aligned_doc d, int num, int * rpos = 0, int * rel = 0);
    int getNumOfOriginalNth(aligned_doc d, int on, int *pre);
    int getOriginalNumOfElement(aligned_doc d, int pos, int el);
    QList<ItElement* > getChangedElements(aligned_doc d);
    void getAlignmentXML(QTextStream * out, QString doc1name = "", QString doc2name = "");
    QString getDocXML(aligned_doc d);
    QStringList getAlignableElementnamesForDoc(aligned_doc d);
    QStringList getAlignableElementnames(aligned_doc d);
    ItDocument * getDocument(aligned_doc d);

    bool createCrossAlignment(aligned_doc baseDoc, QString remoteVersion, bool remoteAtRight =  true);
    bool saveCrossAlignment(alignmentInfo * myinfo, aligned_doc baseDoc, int depAl, bool swap = false);
    //int getDepAlignment(aligned_doc d, QString versionName);
    void fixBrokenTags(QString &str, QStack<QString> *inherited = 0);

    ItElement * getElement(aligned_doc doc, int pos, int el);

    bool swapWithPrevPosition(aligned_doc d, int pos);
    int getAlElementOrder(aligned_doc d, QDomElement el);
    bool breaksOrder(aligned_doc d, int pos, int el);

    bool loadDependentAlignments();

private:
    QList< QList<Link*>* > links [DOCUMENTS];
    QList< QList<dependentLink*>* > depLinks [DOCUMENTS];
    QList< alignmentInfo* > depAlignments [DOCUMENTS];
    QList< unsigned short > depAlSharedDoc [DOCUMENTS];
    ItDocument * doc [DOCUMENTS];
    bool duplicate(aligned_doc doc, int pos, int el, QString newstring);
    unsigned short linkStatValue(QString name, unsigned short defaultVal = DEFAULT_STATUS);
    QString linkStatName(unsigned short val);
    Link * findLinkByDomElement(QDomElement el, aligned_doc doc, int startPos = 0, int startEl = 0);
    bool validElement(aligned_doc doc, int pos, int el);
    bool canChStruct(aligned_doc doc);
    bool loadDepAlignment(QString filename, ushort docNum, ushort sharedNum, QList<dependentLink*> * idlist);
    bool saveDependentAlignments();
    bool saveDepAlignment(alignmentInfo * myinfo, int idx, ushort docNum, ushort sharedNum);
    void get1Stat(ushort doc, int pos, ushort * maxstat);
    void get1Mark(ushort doc, int pos, ushort * maxmark);
    Link * nextLink(aligned_doc doc, int &pos, int &el);
    bool search_segment(aligned_doc doc, int pos, bool forward, ItSearchBar::searchType stype, QString &str, bool ignoreMarkup, QList<Replacement> &transform);
    void clearIndex(aligned_doc d);
    QString transformText(QString text, bool ignoreMarkup, const QList<Replacement> &transform);
    QList<QDomElement> * alignableElementsOrder[2];
    void createAlignableElementsOrder(aligned_doc d);
    ItElement * getPrecedingAlignedElement(aligned_doc d, int pos, int el);
    Link * getLinkByElId(aligned_doc d, QString id, int *fpos=0, int *fel=0);
    void destroyAlignableElementsOrder(aligned_doc d);
};


#endif
