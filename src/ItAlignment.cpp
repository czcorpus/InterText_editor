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


#include "ItAlignment.h"

ItAlignment::ItAlignment(QString path, QString name, QString namespaceURI)
{
  mode = DEFAULT_MODE;
  alignableElementsOrder[0] = 0;
  alignableElementsOrder[1] = 0;
  storagePath = path;
  info.name = name;
  idNamespaceURI = namespaceURI;
    info.autoUpdateStatus = true;
  doc[0] = new ItDocument; doc[0]->setIdNamespaceURI(idNamespaceURI);
  doc[1] = new ItDocument; doc[1]->setIdNamespaceURI(idNamespaceURI);
  QDateTime now = QDateTime::currentDateTime();
  info.synced = now;
  info.changed = now;
  info.ver[0].synced = now;
  info.ver[0].changed = now;
  info.ver[0].perm_chstruct = true;
  info.ver[0].perm_chtext = true;
  info.ver[1].synced = now;
  info.ver[1].changed = now;
  info.ver[1].perm_chstruct = true;
  info.ver[1].perm_chtext = true;
  trackChanges[0] = false;
  trackChanges[1] = false;
  ignorePermissions = false;
  QString empty;
  setNumbering(0, 0, empty, empty);
  setNumbering(1, 0, empty, empty);
  resetLastMatch();
  if (!name.isEmpty()) {
    if (QFile::exists(infoFileName())) open();
		else errorMessage = QObject::tr("Alignment '%1' not found in repository.").arg(name);
  }
}

ItAlignment::~ItAlignment() {
    destroyAlignableElementsOrder(0);
    destroyAlignableElementsOrder(1);
	QList<Link*> * i;
	Link * l;
  for (int d=0; d<=1; d++) {
    while (!links[d].isEmpty()) {
      i = links[d].takeFirst();
      while (!i->isEmpty()) {
        l = i->takeFirst();
        delete l->element;
        qDeleteAll(l->depLinks);
        delete l;
      }
      delete i;
    }
  }
	QList<dependentLink*> * dl;
	for (int d=0; d<=1; d++) {
		for (int i=0; i<depLinks[d].size(); i++) {
			dl = depLinks[d].at(i);
			qDeleteAll(*dl);
			dl->clear();
			delete dl;
		}
		depLinks[d].clear();
	}
	qDeleteAll(depAlignments[0]);
	qDeleteAll(depAlignments[1]);
	depAlignments[0].clear();
	depAlignments[1].clear();
  delete doc[0];
  delete doc[1];
}

void ItAlignment::destroyAlignableElementsOrder(aligned_doc d)
{
    if (alignableElementsOrder[d]) {
        alignableElementsOrder[d]->clear();
        delete alignableElementsOrder[d];
    }
    alignableElementsOrder[d] = 0;
}

QString ItAlignment::infoFileName() {
  return QString("%1/%2.conf").arg(storagePath,info.name);
}

QString ItAlignment::createInfoFileName(QString path, QString text, QString v1, QString v2) {
  return QString("%1/%2.%3.%4.conf").arg(path,text,v1,v2);
}

bool ItAlignment::alDocExists() {
  QStringList list;
  QDir datadir(storagePath);
  datadir.setNameFilters(QStringList(QString("%1.*").arg(info.docId.replace(' ','_'))));
  list = datadir.entryList();
  if (list.empty())
    return false;
  else
    return true;
}

bool ItAlignment::alVerExists(int ver) {
  QStringList list;
  QDir datadir(storagePath);
  datadir.setNameFilters(QStringList(QString("%1.%2.*").arg(info.docId.replace(' ','_'), info.ver[ver].name)));
  list = datadir.entryList();
  if (list.empty())
    return false;
  else
    return true;
}

bool ItAlignment::alExists() {
  QStringList list;
  QDir datadir(storagePath);
  datadir.setNameFilters(QStringList(QString("%1.%2.%3.conf").arg(info.docId.replace(' ','_'), info.ver[0].name, info.ver[1].name)));
  list = datadir.entryList();
  if (list.empty()) {
      datadir.setNameFilters(QStringList(QString("%1.%3.%2.conf").arg(info.docId.replace(' ','_'), info.ver[0].name, info.ver[1].name)));
      list = datadir.entryList();
      if (list.empty())
          return false;
      else
          return true;
  } else
    return true;
}

bool ItAlignment::loadAlignmentConf(QString filename, alignmentInfo * myinfo) {
	QFileInfo file(filename);
  if (!file.exists()) return false;
  QSettings * myInfoFile = new QSettings(filename, QSettings::IniFormat);
  myinfo->name = myInfoFile->value("alignment/name","Unknown").toString();
  myinfo->filename = myInfoFile->value("alignment/filename","unknown_file").toString();
  myinfo->docId = myInfoFile->value("alignment/docId","Unknown document").toString();
  myinfo->source = myInfoFile->value("alignment/source","").toString();
  myinfo->autoUpdateStatus = myInfoFile->value("alignment/autoUpdStat","true").toBool();
  myinfo->synced = QDateTime::fromString(myInfoFile->value("alignment/synced",QDateTime::currentDateTime()).toString(), Qt::ISODate).toLocalTime();
  myinfo->changed = QDateTime::fromString(myInfoFile->value("alignment/changed",QDateTime::currentDateTime()).toString(), Qt::ISODate).toLocalTime();
  myinfo->ver[0].name = myInfoFile->value("ver1/name","ver1").toString();
  myinfo->ver[0].filename = myInfoFile->value("ver1/filename","unknown_file_1").toString();
  myinfo->ver[0].synced = QDateTime::fromString(myInfoFile->value("ver1/synced",QDateTime::currentDateTime()).toString(), Qt::ISODate).toLocalTime();
  myinfo->ver[0].changed = QDateTime::fromString(myInfoFile->value("ver1/changed",QDateTime::currentDateTime()).toString(), Qt::ISODate).toLocalTime();
  myinfo->ver[0].source = myInfoFile->value("ver1/source","").toString();
  myinfo->ver[0].perm_chtext = myInfoFile->value("ver1/permit_text_change","true").toBool();
  myinfo->ver[0].perm_chstruct = myInfoFile->value("ver1/permit_struct_change","true").toBool();
  myinfo->ver[0].numLevels = myInfoFile->value("ver1/id_levels",-1).toInt();
  myinfo->ver[0].numPrefix = myInfoFile->value("ver1/id_prefix","").toString();
  myinfo->ver[0].numParentPrefix = myInfoFile->value("ver1/id_parent_prefix","").toString();
  myinfo->ver[1].name = myInfoFile->value("ver2/name","ver2").toString();
  myinfo->ver[1].filename = myInfoFile->value("ver2/filename","unknown_file_1").toString();
  myinfo->ver[1].synced = QDateTime::fromString(myInfoFile->value("ver2/synced",QDateTime::currentDateTime()).toString(), Qt::ISODate).toLocalTime();
  myinfo->ver[1].changed = QDateTime::fromString(myInfoFile->value("ver2/changed",QDateTime::currentDateTime()).toString(), Qt::ISODate).toLocalTime();
  myinfo->ver[1].source = myInfoFile->value("ver2/source","").toString();
  myinfo->ver[1].perm_chtext = myInfoFile->value("ver2/permit_text_change","true").toBool();
  myinfo->ver[1].perm_chstruct = myInfoFile->value("ver2/permit_struct_change","true").toBool();
  myinfo->ver[1].numLevels = myInfoFile->value("ver2/id_levels",-1).toInt();
  myinfo->ver[1].numPrefix = myInfoFile->value("ver2/id_prefix","").toString();
  myinfo->ver[1].numParentPrefix = myInfoFile->value("ver2/id_parent_prefix","").toString();
  delete myInfoFile;
	return true;
}

void ItAlignment::saveAlignmentConf(QString filename, alignmentInfo * myinfo)
{
  QSettings * myInfoFile = new QSettings(filename, QSettings::IniFormat);
  myInfoFile->setValue("alignment/name", myinfo->name);
  myInfoFile->setValue("alignment/filename", myinfo->filename);
  myInfoFile->setValue("alignment/docId", myinfo->docId);
  myInfoFile->setValue("alignment/source", myinfo->source);
  myInfoFile->setValue("alignment/autoUpdStat", QVariant(myinfo->autoUpdateStatus).toString());
  myInfoFile->setValue("alignment/synced", myinfo->synced.toUTC().toString(Qt::ISODate));
  myInfoFile->setValue("alignment/changed", myinfo->changed.toUTC().toString(Qt::ISODate));
  myInfoFile->setValue("ver1/name", myinfo->ver[0].name);
  myInfoFile->setValue("ver1/filename", myinfo->ver[0].filename);
  myInfoFile->setValue("ver1/synced", myinfo->ver[0].synced.toUTC().toString(Qt::ISODate));
  myInfoFile->setValue("ver1/changed", myinfo->ver[0].changed.toUTC().toString(Qt::ISODate));
  myInfoFile->setValue("ver1/source", myinfo->ver[0].source);
  myInfoFile->setValue("ver1/permit_text_change", QVariant(myinfo->ver[0].perm_chtext).toString());
  myInfoFile->setValue("ver1/permit_struct_change", QVariant(myinfo->ver[0].perm_chstruct).toString());
  myInfoFile->setValue("ver1/id_levels", myinfo->ver[0].numLevels);
  myInfoFile->setValue("ver1/id_prefix", myinfo->ver[0].numPrefix);
  myInfoFile->setValue("ver1/id_parent_prefix", myinfo->ver[0].numParentPrefix);
  myInfoFile->setValue("ver2/name", myinfo->ver[1].name);
  myInfoFile->setValue("ver2/filename", myinfo->ver[1].filename);
  myInfoFile->setValue("ver2/synced", myinfo->ver[1].synced.toUTC().toString(Qt::ISODate));
  myInfoFile->setValue("ver2/changed", myinfo->ver[1].changed.toUTC().toString(Qt::ISODate));
  myInfoFile->setValue("ver2/source", myinfo->ver[1].source);
  myInfoFile->setValue("ver2/permit_text_change", QVariant(myinfo->ver[1].perm_chtext).toString());
  myInfoFile->setValue("ver2/permit_struct_change", QVariant(myinfo->ver[1].perm_chstruct).toString());
  myInfoFile->setValue("ver2/id_levels", myinfo->ver[1].numLevels);
  myInfoFile->setValue("ver2/id_prefix", myinfo->ver[1].numPrefix);
  myInfoFile->setValue("ver2/id_parent_prefix", myinfo->ver[1].numParentPrefix);
  myInfoFile->sync();
  delete myInfoFile;
}

bool ItAlignment::open() {
	QFileInfo file(infoFileName());
  info.infoName = file.fileName();
	loadAlignmentConf(infoFileName(), &info);
	setNumbering(0, info.ver[0].numLevels, info.ver[0].numPrefix, info.ver[0].numParentPrefix);
	setNumbering(1, info.ver[1].numLevels, info.ver[1].numPrefix, info.ver[1].numParentPrefix);
	return loadFile(file.canonicalPath()+'/'+info.filename);
}

bool ItAlignment::save() {
  if (info.name.isEmpty())
    info.name = QString("%1.%2.%3").arg(info.docId.replace(' ','_'), info.ver[0].name, info.ver[1].name);
  if (info.filename.isEmpty())
    info.filename = QString("%1.%2.%3.xml").arg(info.docId.replace(' ','_'), info.ver[0].name, info.ver[1].name);
  if (info.ver[0].filename.isEmpty())
    info.ver[0].filename = QString("%1.%2.xml").arg(info.docId.replace(' ','_'), info.ver[0].name);
  if (info.ver[1].filename.isEmpty())
    info.ver[1].filename = QString("%1.%2.xml").arg(info.docId.replace(' ','_'), info.ver[1].name);
  saveAlignmentConf(infoFileName(), &info);

  if (!saveFile(QString("%1/%2").arg(storagePath, info.filename))) return false;
  if (!doc[0]->save(QString("%1/%2").arg(storagePath, info.ver[0].filename))) {
    errorMessage = doc[0]->errorMessage;
    return false;
  }
  if (!doc[1]->save(QString("%1/%2").arg(storagePath,info.ver[1].filename))) {
    errorMessage = doc[1]->errorMessage;
    return false;
  }
  if (!saveDependentAlignments())
    return false;
  return true;
}

bool ItAlignment::loadFile(const QString &filename, unsigned short defaultStatus)
{
    unsigned short status;
    QString mark;
    QFileInfo fileinfo(filename);
    QStringList xtargets, lIDs, rIDs, ratio;
    Link *l;
    QDomDocument alDoc;

    //qDebug() << "Reading file:" << afilename;

    // Load alignment
    QString errorMsg; int errorLine, errorColumn;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        errorMessage = QObject::tr("Cannot open file '%1':\n%2.").arg(fileinfo.fileName(), file.errorString());
        return false;
    }
    if (!alDoc.setContent(&file, true, &errorMsg, &errorLine, &errorColumn)) {
        file.close();
        errorMessage = QObject::tr("Error parsing XML document '%1' at line %2, column %3: %4.").arg(fileinfo.fileName(), QString().setNum(errorLine), QString().setNum(errorColumn), errorMsg);
        return false;
    }
    file.close();

    // extract document filenames
    QDomElement docElem = alDoc.documentElement();
    if (docElem.tagName()!="linkGrp") {
        errorMessage = QObject::tr("No root 'linkGrp' element in alignment file.");
        return false;
    }
    info.ver[1].filename = docElem.attribute("toDoc","");
    info.ver[0].filename = docElem.attribute("fromDoc","");
    if (info.ver[0].source.isEmpty())
        info.ver[0].source = info.ver[0].filename;
    if (info.ver[1].source.isEmpty())
        info.ver[1].source = info.ver[1].filename;

    //qDebug() << "Reading file " << lfilename;

    if (!loadDoc(0, fileinfo.canonicalPath()))
        return false;
    if (!loadDoc(1, fileinfo.canonicalPath()))
        return false;

    /*QString tmpfname = QString("%1/%2.%3.xml").arg(storagePath, info.docId, info.ver[0].name);
  if (!QFile::exists(tmpfname)) tmpfname = QString("%1/%2").arg(fileinfo.canonicalPath(), info.ver[0].filename);
  if(!doc[0]->load(tmpfname)) {
    errorMessage = doc[0]->errorMessage;
        return false;
  }*/

    //qDebug() << "Reading file " << rfilename;

    /*tmpfname = QString("%1/%2.%3.xml").arg(storagePath, info.docId, info.ver[1].name);
  if (!QFile::exists(tmpfname)) tmpfname = QString("%1/%2").arg(fileinfo.canonicalPath(), info.ver[1].filename);
  if(!doc[1]->load(tmpfname)) {
    errorMessage = doc[1]->errorMessage;
        return false;
  }*/

  QDomNode n = docElem.firstChild();
  QStringList alignedIDs[2];
  int i = 0;
  while(!n.isNull()) {
      QDomElement e = n.toElement();
      if(!e.isNull() && e.tagName()=="link") {
          status = linkStatValue(e.attribute("status", "DEFAULT_STATUS"), defaultStatus);
          mark = e.attribute("mark","DEFAULT_MARK");
          xtargets = e.attribute("xtargets",";").split(";");
          ratio = e.attribute("type","-").split("-");
          if (ratio[1]!="0") lIDs = xtargets[1].split(" "); else lIDs = QStringList();
          if (ratio[0]!="0") rIDs = xtargets[0].split(" "); else rIDs = QStringList();
          alignedIDs[0].append(lIDs);
          alignedIDs[1].append(rIDs);
          QString id;
          //QString *pid;
          links[0] << new QList<Link*>;
          links[1] << new QList<Link*>;
          foreach (id, lIDs) {
              if (id.isEmpty()) continue;
              //pid = new QString(id);
              l = new Link;
              //qDebug() << "Adding to L pos" << i << "element id" << id;
              //l->pos = i;
              l->element = doc[0]->elementById(id);
              //l->element = pid;
              l->status = status;
              l->mark = mark.toUShort();
              if (l->element != 0) {
                  links[0].at(i)->append(l);
                  //sizeCache[0] << QSize();
              } else {
                  delete l;
                  errorMessage = QObject::tr("Element with id '%1' not found in document version '%2'.").arg(id,info.ver[0].name);
                  return false;
              }
          }
          foreach (id, rIDs) {
              if (id.isEmpty()) continue;
              //pid = new QString(id);
              l = new Link;
              //qDebug() << "Adding to R pos" << i << "element id" << id;
              //l->pos = i;
              l->element = doc[1]->elementById(id);
              //l->element = pid;
              l->status = status;
              l->mark = mark.toUShort();
              if (l->element != 0) {
                  links[1].at(i)->append(l);
                  //sizeCache[1] << QSize();
              } else {
                  delete l;
                  errorMessage = QObject::tr("Element with id '%1' not found in document version '%2'.").arg(id,info.ver[1].name);
                  return false;
              }
          }
          ++i;
      }
      n = n.nextSibling();
  }

  if (doc[0]->numLevels()<1) {
    doc[0]->detectIdSystem(alignedIDs[0]);
    info.ver[0].numLevels = doc[0]->numLevels();
    info.ver[0].numPrefix = doc[0]->numPrefix();
    info.ver[0].numParentPrefix = doc[0]->numParentPrefix();
  }
  //qDebug() << doc[0]->filename << "- levels:" << doc[0]->numbering_levels << "prefix:" << doc[0]->numbering_prefix << "parent prefix:" << doc[0]->numbering_parent_prefix;
  if (doc[1]->numLevels()<1) {
    doc[1]->detectIdSystem(alignedIDs[1]);
    info.ver[1].numLevels = doc[1]->numLevels();
    info.ver[1].numPrefix = doc[1]->numPrefix();
    info.ver[1].numParentPrefix = doc[1]->numParentPrefix();
  }
	doc[0]->destroyIndex(alignedIDs[0]);
	doc[1]->destroyIndex(alignedIDs[1]);
  //qDebug() << doc[1]->filename << "- levels:" << doc[1]->numbering_levels << "prefix:" << doc[1]->numbering_prefix << "parent prefix:" << doc[1]->numbering_parent_prefix;
	//qDebug() << "Finished";
  if (info.ver[0].source.startsWith("http"))
    trackChanges[0] = true;
  if (info.ver[1].source.startsWith("http"))
    trackChanges[1] = true;
	if (!loadDependentAlignments())
		return false;
	return true;
}

void ItAlignment::detectIdSystem(aligned_doc d)
{
  QStringList idlist;
  QString id;
  QList<Link* > * i;
  Link * l;
  foreach (i, links[d]) {
    foreach (l, *i) {
      id = l->element->getID(idNamespaceURI);
      //if (!id.isEmpty())
      idlist << id;
    }
  }
  doc[d]->detectIdSystem(idlist);
  info.ver[d].numLevels = doc[d]->numLevels();
  info.ver[d].numPrefix = doc[d]->numPrefix();
  info.ver[d].numParentPrefix = doc[d]->numParentPrefix();
}

bool ItAlignment::loadDoc(aligned_doc d, QString path)
{
  if (path.isEmpty())
    path = storagePath;
  QString fname = QString("%1/%2.%3.xml").arg(storagePath, info.docId, info.ver[d].name);
  if (!QFile::exists(fname)) {
    fname = QString("%1/%2").arg(path, info.ver[d].filename);
  } else {
    QDir dir(storagePath);
    aligned_doc od = 0;
    QStringList list = dir.entryList(QStringList(QString("%1.%2.*.conf").arg(info.docId, info.ver[d].name)), QDir::Files, QDir::Name);
    if (list.isEmpty()) {
      od = 1;
      list = dir.entryList(QStringList(QString("%1.*.%2.conf").arg(info.docId, info.ver[d].name)), QDir::Files, QDir::Name);
    }
    if (!list.isEmpty()) {
      //errorMessage = QObject::tr("Corrupt storage: an orphan document '%1' present in the storage. Please, delete it.").arg(fname);
      //return false;
    //} else {
        ItAlignment::alignmentInfo alinfo;
        ItAlignment::loadAlignmentConf(QString("%1/%2").arg(storagePath, list.first()), &alinfo);
        info.ver[d].filename = alinfo.ver[od].filename;
        info.ver[d].source = alinfo.ver[od].source;
        info.ver[d].synced = alinfo.ver[od].synced;
        info.ver[d].changed = alinfo.ver[od].changed;
        info.ver[d].perm_chtext = alinfo.ver[od].perm_chtext;
        info.ver[d].perm_chstruct = alinfo.ver[od].perm_chstruct;
        info.ver[d].numLevels = alinfo.ver[od].numLevels;
        info.ver[d].numPrefix = alinfo.ver[od].numPrefix;
        info.ver[d].numParentPrefix = alinfo.ver[od].numParentPrefix;
    }
  }
  if(!doc[d]->load(fname)) {
    errorMessage = doc[d]->errorMessage;
    return false;
  }
  return true;
}

bool ItAlignment::setDocXml(aligned_doc d, QString &xml)
{
  if (!doc[d]->setXml(xml)) {
    errorMessage = doc[d]->errorMessage;
    return false;
  }
  return true;
}

bool ItAlignment::setDocXml(aligned_doc d, QByteArray &xml)
{
  if (!doc[d]->setXml(xml)) {
    errorMessage = doc[d]->errorMessage;
    return false;
  }
  return true;
}

void ItAlignment::createLinks(aligned_doc d, QStringList alignable, QList<int> groups)
{
  QList<ItElement*> ellist;
  doc[d]->collectElements(&ellist, alignable);
  int i = 0, cnt = 0;
  Link * l;
  QList<Link* > * tmplist = 0;
  while (!ellist.isEmpty()) {
    if (cnt==0) {
      cnt = groups.value(i, 1);
      i++;
      if (tmplist)
        links[d].append(tmplist);
      tmplist = new QList<Link* >;
      if (cnt==0)
        continue;
    }
    l = new Link;
    l->element = ellist.takeFirst();
    l->status = LINK_PLAIN;
    l->mark = 0;
    tmplist->append(l);
    cnt--;
  }
  if (tmplist)
    links[d].append(tmplist);
}

bool ItAlignment::saveFile(const QString &filename, QString doc1name, QString doc2name)
{
	QFile file(filename);
  if (!file.open(QIODevice::WriteOnly)) {
    errorMessage = QObject::tr("Error saving file '%1':\n%2").arg(filename, file.errorString());
    return false;
  }

	//qDebug() << "File opened:" << filename;

  QTextStream out(&file);
  getAlignmentXML(&out, doc1name, doc2name);
	file.close();
	return true;
}

void ItAlignment::getAlignmentXML(QTextStream * out, QString doc1name, QString doc2name)
{
  int lcount, rcount, m;
  QString lids, rids, stat, mark;
  QString sep(" ");

  if (doc1name.isEmpty()) doc1name = info.ver[0].filename;//QString("%1.%2.xml").arg(info.docId, info.ver[0].name);//
  if (doc2name.isEmpty()) doc2name = info.ver[1].filename;//QString("%1.%2.xml").arg(info.docId, info.ver[1].name);//

  *out << "<?xml version='1.0' encoding='utf-8'?>\n";
  *out << "<linkGrp toDoc='" << doc2name << "' fromDoc='" << doc1name << "'>\n";

  int max = links[0].size();
  if (links[1].size()>max) max = links[1].size();
  for (int i=0; i<max; ++i) {
    lcount =0; rcount = 0;
    lids = getIDs(0,i).join(sep);
    rids = getIDs(1,i).join(sep);
    if (lids!="") lcount = lids.split(sep).size();
    if (rids!="") rcount = rids.split(sep).size();
    stat = linkStatName(getStat(i));
    m = getMark(i);
    if (m>0) mark = QString(" mark='%1'").arg(m);
    else mark = QString();
    *out << "<link type='" << rcount << "-" << lcount << "' xtargets='" << rids << ";" << lids << "' status='" << stat << "'" << mark << "/>\n";
  }
  *out << "</linkGrp>\n";
}

QString ItAlignment::getDocXML(aligned_doc d)
{
    return doc[d]->getXml();
}

bool ItAlignment::saveDoc(int i, const QString &filename) {
  if (!doc[i]->save(filename)) {
    errorMessage = doc[i]->errorMessage;
    return false;
  } else {
    return true;
  }
}

int ItAlignment::maxPos()
{
	if (links[0].size()>links[1].size())
		return links[0].size()-1;
	else
		return links[1].size()-1;
}

QVariant ItAlignment::getContents(aligned_doc doc, int pos, bool prepend, bool ignoreMarkup, const QList<Replacement> *transformations)
{
  //if (doc>1 || doc<0) return QStringList();
	int max = links[doc].size()-1;
  if (pos>max || pos<0) return QStringList();
	QList<Link*> list = *links[doc].at(pos);
	QStringList strList;
    QString text;
	for(int i=0; i<list.size(); ++i) 
	{
        text = list.at(i)->element->getContents(prepend);
        if (transformations && ignoreMarkup)
            text = transformText(text, ignoreMarkup, *transformations);
        if (breaksOrder(doc, pos, i)) {
            if (text.startsWith(QChar(0x25BA)))
                text.replace(0,1,QChar(0x25BB));
            else if (text.startsWith(QChar(0x25A0)))
                text.replace(0,1,QChar(0x25A1));

        }
        strList << text;
	}
	return strList;
}

bool ItAlignment::isFirst(aligned_doc doc, int pos, int el) {
	if (!validElement(doc, pos, el))
		return false;
	Link * link = links[doc].at(pos)->at(el);
	if (link->element->getParent().firstChildElement()==link->element->element)
		return true;
	else
		return false;
}

bool ItAlignment::canMergeParent(aligned_doc doc, int pos, int el) {
	if (!validElement(doc, pos, el))
		return false;
    if (!canChStruct(doc))
		return false;
	if (!isFirst(doc, pos, el))
		return false;
	Link * link = links[doc].at(pos)->at(el);
	QDomElement prevel = link->element->getParent().previousSiblingElement();
	if (!prevel.isNull() && prevel.tagName()==link->element->getParent().tagName())
		return true;
	else
		return false;
}

bool ItAlignment::canMerge(aligned_doc doc, int pos, int el, int count) {
	if (!validElement(doc, pos, el))
    return false;
	int max = links[doc].at(pos)->size()-1;
	if (count<1 || el+count>max)
    return false;
    if (!canChStruct(doc))
    return false;
    for (int i=1; i <= count; i++) {
        if (isFirst(doc, pos, el+i) && !canMergeParent(doc, pos, el+i))
            return false;
        if (breaksOrder(doc, pos, el+i))
            return false;
    }
	return true;
}

bool ItAlignment::canSplitParent(aligned_doc doc, int pos, int el) {
	if (!validElement(doc, pos, el))
		return false;
    if (!canChStruct(doc))
		return false;
	if (isFirst(doc, pos, el))
		return false;
	return true;
}

bool ItAlignment::updateContents(aligned_doc doc, int pos, int el, QString string)
{
  if (!ignorePermissions && ((doc==0 && !info.ver[0].perm_chtext) || (doc==1 && !info.ver[1].perm_chtext))) {
    //qDebug() << "Permission denied.";
    return false;
  }
  if (string==getContents(doc,pos,false).toStringList().at(el)) {
    //qDebug() << "No change, no update.";
    return true;
  }
  //qDebug() << "Would like to update:" << doc << pos << el << string;
	int max = links[doc].size()-1;
	if (pos<0 || pos>max) return false;
	Link * link = links[doc].at(pos)->at(el);
    string = string.trimmed();
    fixBrokenTags(string);
  bool res = link->element->updateContents(string, trackChanges[doc]);
  if (res) {
    setDocDepCTime(doc, QDateTime::currentDateTime());
    destroyAlignableElementsOrder(doc); // rebuilding the list just seems to be fast enough...
  }
  return res;
}

bool ItAlignment::isVirgin(aligned_doc doc, int pos, int el) {
  if (!validElement(doc, pos, el))
    return false;
  Link * link = links[doc].at(pos)->at(el);
  return link->element->isVirgin();
}

void ItAlignment::setVirgin(aligned_doc doc, int pos, int el) {
  if (!trackChanges[doc])
    return;
  if (!validElement(doc, pos, el))
    return;
  Link * link = links[doc].at(pos)->at(el);
  link->element->setVirgin();
}

bool ItAlignment::merge(aligned_doc doc, int pos, int el, int count) {
	if (!validElement(doc, pos, el))
    return false;
    if (!canChStruct(doc))
    return false;
  int max = links[doc].at(pos)->size()-1;
	if (count<1 || el+count>max)
    return false;
  QString text = links[doc].at(pos)->at(el)->element->getContents(false);
  int repl = links[doc].at(pos)->at(el)->element->repl();
  for (int i=1; i <= count; i++) {
    text.append(" ").append(links[doc].at(pos)->at(el+i)->element->getContents(false));
    repl += links[doc].at(pos)->at(el+i)->element->repl();
  }
  updateContents(doc, pos, el, text);
  if (trackChanges[doc]) {
    Link * link = links[doc].at(pos)->at(el);
    link->element->setRepl(repl);
  }
  removeAfter(doc, pos, el, count); // does rebuild of alignableElementsOrder and renumbering
	return true;
}

int ItAlignment::getRepl(aligned_doc doc, int pos, int el)
{
  if (!validElement(doc, pos, el))
    return -1;
  Link * link = links[doc].at(pos)->at(el);
  return link->element->repl();
}

void ItAlignment::setRepl(aligned_doc doc, int pos, int el, int val)
{
  if (!trackChanges[doc])
    return;
  if (!validElement(doc, pos, el))
    return;
  Link * link = links[doc].at(pos)->at(el);
  link->element->setRepl(val);
}

QString ItAlignment::getParbr(aligned_doc doc, int pos, int el)
{
  if (!validElement(doc, pos, el))
    return "";
  Link * link = links[doc].at(pos)->at(el);
  return link->element->parbr();
}

void ItAlignment::setParbr(aligned_doc doc, int pos, int el, QString val)
{
  if (!trackChanges[doc])
    return;
  if (!validElement(doc, pos, el))
    return;
  Link * link = links[doc].at(pos)->at(el);
  link->element->setParbr(val);
}

bool ItAlignment::split(aligned_doc doc, int pos, int el, QStringList newstrings) {
	if (!validElement(doc, pos, el))
		return false;
    if (!canChStruct(doc))
		return false;
    QStack<QString> inherittags;
    for (int i=0; i<newstrings.length(); i++) {
        newstrings[i] = newstrings[i].trimmed();
        fixBrokenTags(newstrings[i], &inherittags);
    }
	if (!updateContents(doc, pos, el, newstrings.takeFirst()))
		return false;
	while (!newstrings.isEmpty())
		if (!duplicate(doc, pos, el++, newstrings.takeFirst()))
			return false;
    destroyAlignableElementsOrder(doc); // rebuilding the list just seems to be fast enough...
	renumber(doc);
	return true;
}

void ItAlignment::fixBrokenTags(QString &str, QStack<QString> *inherited)
{
    QStack<QString> stack;
    QStack<QString> reopen;
    QString tmp, prepend;
    if (inherited) {
        reopen = *inherited;
        while (!reopen.isEmpty()) {
            tmp = reopen.pop();
            tmp = QString("<%1>").arg(tmp);
            prepend.append(tmp);
        }
        str.prepend(prepend);
    }
    prepend = "";
    QRegExp rx("<(/?)([^>]+)>");
    int pos = 0;
    int next = 0;
    QString captag;
    while ((next=rx.indexIn(str, pos)) != -1) {
        pos = next;
        captag = rx.capturedTexts().at(2);
        if (captag.endsWith("/")) {
            pos += rx.matchedLength();
            continue;
        }
        if (rx.capturedTexts().at(1).isEmpty()) {
            stack.push(captag);
            pos += rx.matchedLength();
        } else {
            if (stack.isEmpty()) {
                prepend.prepend( QString("<%1>").arg(captag) );
                pos += rx.matchedLength();
            } else {
                // close all  still open tags
                while (!stack.isEmpty() && (tmp=stack.pop().split(" ").at(0)) != captag) {
                    reopen.push(tmp);
                    tmp = QString("</%1>").arg(tmp);
                    str.insert(pos, tmp);
                    pos += tmp.length();
                }
                // if the tag ending here has no start...
                if (stack.isEmpty() && tmp != captag) {
                    prepend.prepend( QString("<%1>").arg(captag) );
                }
                pos += rx.matchedLength();
                // skip just following reopened (found) tag before reopening the other ones!
                // (=survive at least one level of element "breaks" - that should be enough for normal cases)
                QRegExp srx("\\s*<("+captag+"[^>]*)>");
                if (srx.indexIn(str, pos) == pos) {
                    stack.push(srx.capturedTexts().at(1));
                    pos += srx.matchedLength();
                }
                while (!reopen.isEmpty()) {
                    tmp = reopen.pop();
                    stack.push(tmp);
                    tmp = QString("<%1>").arg(tmp);
                    str.insert(pos, tmp);
                    pos += tmp.length();
                }
            }
        }
    }
    while (!stack.isEmpty()) {
        tmp = stack.pop();
        reopen.push(tmp);
        tmp = QString("</%1>").arg(tmp.split(" ").at(0));
        str.append(tmp);
    }
    str.prepend(prepend);
    str.replace(QRegExp("<([^>]+)>\\s*</\\1>"),"");//qDebug()<<str;
    if (inherited)
        *inherited = reopen;
}

bool ItAlignment::duplicate(aligned_doc doc, int pos, int el, QString newstring) {
	if (!validElement(doc, pos, el))
		return false;
    if (!canChStruct(doc))
		return false;
	Link * link = links[doc].at(pos)->at(el);
    newstring = newstring.trimmed();
    fixBrokenTags(newstring);
  ItElement * newel = link->element->clone(newstring, trackChanges[doc]);
	if (newel==0)
		return false;
	Link * l = new Link;
  //l->pos = link->pos;
	l->element = newel;
	l->status = link->status;
	l->mark = link->mark;
	//l->depLinks = link->depLinks;
	dependentLink * dl;
	for (int i=0; i<link->depLinks.size(); i++) {
		dl = new dependentLink;
		dl->pos = link->depLinks.at(i)->pos;
		dl->mark = link->depLinks.at(i)->mark;
		dl->status = link->depLinks.at(i)->status;
		dl->id = link->depLinks.at(i)->id;
		l->depLinks.append(dl);
	}
	links[doc].at(pos)->insert(el+1, l);
	return true;
}

/*bool ItAlignment::remove(aligned_doc doc, int pos, int el, int count) {
	if ((doc==0 && !info.ver[0].perm_chstruct) || (doc==1 && !info.ver[1].perm_chstruct))
		//qDebug() << "Permission denied.";
		return false;
	int max = links[doc].size()-1;
	if (pos<0 || pos>max)
		return false;
	max = links[doc].at(pos)->size()-1;
	if (el<0 || el>max)
		return false;
	if (count<1 || el+count-1>max)
		return false;
	Link * link;
	ItElement * element;
	QDomElement docel;
	for (int i=0; i < count; i++) {
		link = links[doc].at(pos)->at(el);
		element = link->element;
		docel = element->element;
		docel.parentNode().removeChild(docel);
		links[doc].at(pos)->removeAt(el);
		delete link;
		delete element;
	}
	renumber(doc);
	return true;
}*/

bool ItAlignment::removeAfter(aligned_doc doc, int pos, int el, int count) {
	if (!validElement(doc, pos, el))
    return false;
	if (!canChStruct(doc))
    return false;
	int max = links[doc].at(pos)->size()-1;
	if (count<1 || el+count>max)
    return false;
	Link * link = links[doc].at(pos)->at(el);
	ItElement * element;
	QDomElement docel, parel, lastparel;
	lastparel =  link->element->getParent();
	for (int i=1; i <= count; i++) {
		link = links[doc].at(pos)->at(el+1);
		element = link->element;
		docel = element->element;
		parel = element->getParent();
		parel.removeChild(docel);
		links[doc].at(pos)->removeAt(el+1);
		qDeleteAll(link->depLinks);
		delete link;
		delete element;
		/* merge parents if they differ! */
		if (parel!=lastparel && parel==lastparel.nextSiblingElement() && parel.tagName()==lastparel.tagName()) {
			QDomElement newchild, oldchild;
			Link * tmp_link;
			while (parel.hasChildNodes()) {
				oldchild = parel.firstChild().toElement();
				tmp_link = findLinkByDomElement(oldchild, doc, pos, el+1);
				newchild = lastparel.appendChild(oldchild).toElement();
				if (tmp_link!=0)
					tmp_link->element->element = newchild;
				else qDebug() << "Houston, we have a problem: corresponding link not found!!! What shall I do? Crash?";
			}
			parel.parentNode().removeChild(parel);
		}
    }
    destroyAlignableElementsOrder(doc); // rebuilding the list just seems to be fast enough...
    renumber(doc);
	return true;
}

bool ItAlignment::splitParent(aligned_doc doc, int pos, int el) {
	if (!validElement(doc, pos, el))
		return false;
	if (!canChStruct(doc))
		return false;
	Link * link = links[doc].at(pos)->at(el);
	QDomElement parel = link->element->getParent();
	if (parel.firstChildElement()==link->element->element)
    return false; // this is already the first element of its parent!
  if (trackChanges[doc]) {
    if (link->element->parbr()=="d")
      link->element->setParbr("");
    else
      link->element->setParbr("n");
  }
	QDomElement newparent = parel.cloneNode(false).toElement();
	parel.parentNode().insertAfter(newparent, parel);
	QDomNode refNode = link->element->element.previousSibling();
	QDomNode newnode;
	Link * tmp_link;
	for (QDomNode node = refNode.nextSibling(); !node.isNull(); node = refNode.nextSibling()) {
		tmp_link = findLinkByDomElement(node.toElement(), doc, pos, el);
		newnode = newparent.appendChild(node);
		if (tmp_link!=0)
			tmp_link->element->element = newnode.toElement();
		//else qDebug() << "Houston, we have a problem: corresponding link not found!!! What shall I do? Crash?";
	}
	renumber(doc);
	return true;
}

bool ItAlignment::mergeParent(aligned_doc doc, int pos, int el) {
	if (!validElement(doc, pos, el))
		return false;
	if (!canChStruct(doc))
		return false;
	Link * link = links[doc].at(pos)->at(el);
	QDomElement parel = link->element->getParent();
	QDomElement prevparel = parel.previousSiblingElement();
	if (prevparel.isNull() || parel.nodeName()!=prevparel.nodeName())
		return false;
  if (trackChanges[doc]) {
    if (link->element->parbr()=="n")
      link->element->setParbr("");
    else
      link->element->setParbr("d");
  }
	QDomNode newnode;
	Link * tmp_link;
	for (QDomNode node = parel.firstChild(); !node.isNull(); node = parel.firstChild()) {
		tmp_link = findLinkByDomElement(node.toElement(), doc, pos, el);
		newnode = prevparel.appendChild(node);
		if (tmp_link!=0)
			tmp_link->element->element = newnode.toElement();
		//else qDebug() << "Houston, we have a problem: corresponding link not found!!! What shall I do? Crash?";
	}
	parel.parentNode().removeChild(parel);
	renumber(doc);
	return true;
}

bool ItAlignment::validElement(aligned_doc doc, int pos, int el) {
	if (doc>1)
		return false;
	int max = links[doc].size()-1;
	if (pos<0 || pos>max)
		return false;
	max = links[doc].at(pos)->size()-1;
	if (el<0 || el>max)
		return false;
	return true;
}

bool ItAlignment::canChStruct(aligned_doc doc) {
    if (ignorePermissions)
        return true;
	if ((doc==0 && !info.ver[0].perm_chstruct) || (doc==1 && !info.ver[1].perm_chstruct))
		//qDebug() << "Permission denied.";
		return false;
	return true;
}

ItAlignment::Link * ItAlignment::findLinkByDomElement(QDomElement el, aligned_doc doc, int startPos, int startEl) {
	Link * link;
	for (int i=startPos; i < links[doc].size(); i++) {
		for (int j=startEl; j < links[doc].at(i)->size(); j++) {
			link = links[doc].at(i)->at(j);
			if (link->element->element==el) return link;
		}
		startEl = 0;
	}
	return 0;
}

QStringList ItAlignment::getIDs(aligned_doc doc, int pos)
{
    if (pos<0)
      return QStringList();
	int max = links[doc].size()-1;
	if (pos>max) return QStringList();
	QList<Link*> list = *links[doc].at(pos);
	QStringList strList;
	for(int i=0; i<list.size(); ++i) 
	{
        strList << list.at(i)->element->getID(idNamespaceURI);
	}
	return strList;
}

int ItAlignment::getSize(aligned_doc doc, int pos)
{
    if (pos<0)
      return -1;
    int max = links[doc].size()-1;
    if (pos>max) return -1;
    return links[doc].at(pos)->size();
}

ushort ItAlignment::getStat(int pos)
{
  ushort maxstat = 0;
  //Link * l;
  /*if (links[0].size()>pos)
		foreach (l, *links[0].at(pos)) {
			if (l->status > maxstat) maxstat = l->status;
		}
	if (links[1].size()>pos)
		foreach (l, *links[1].at(pos)) {
			if (l->status > maxstat) maxstat = l->status;
    }*/
  get1Stat(ushort(0), pos, &maxstat);
  get1Stat(1, pos, &maxstat);
  if (maxstat)
    return maxstat;
  else
    return LINK_PLAIN;
}

void ItAlignment::get1Stat(ushort doc, int pos, ushort * maxstat)
{
  Link * l;
  if (links[doc].size()>pos)
    foreach (l, *links[doc].at(pos)) {
      if (l->status > *maxstat) *maxstat = l->status;
    }
}

QString ItAlignment::getStatName(int pos)
{
	return linkStatName(getStat(pos));
}

ushort ItAlignment::getMark(int pos)
{
	ushort maxmark = 0;
  /*Link * l;
	if (links[0].size()>pos)
		foreach (l, *links[0].at(pos)) {
			if (l->mark > maxmark) maxmark = l->mark;
		}
	if (links[1].size()>pos)
		foreach (l, *links[1].at(pos)) {
			if (l->mark > maxmark) maxmark = l->mark;
    }*/
  get1Mark(0, pos, &maxmark);
  get1Mark(1, pos, &maxmark);
	return maxmark;
}

void ItAlignment::get1Mark(ushort doc, int pos, ushort * maxmark)
{
  Link * l;
  if (links[doc].size()>pos && pos>=0)
    foreach (l, *links[doc].at(pos)) {
      if (l->mark > *maxmark) *maxmark = l->mark;
    }
}

unsigned short ItAlignment::linkStatValue(QString name, unsigned short defaultVal)
{
	if (name == LINK_MANUAL_NAME) return LINK_MANUAL;
	else if (name == LINK_AUTO_NAME) return LINK_AUTO;
	else if (name == LINK_PLAIN_NAME) return LINK_PLAIN;
	else return defaultVal;
}

QString ItAlignment::linkStatName(unsigned short val)
{
	if (val== LINK_MANUAL) return QString(LINK_MANUAL_NAME);
	else if (val == LINK_AUTO) return QString(LINK_AUTO_NAME);
	else if (val == LINK_PLAIN) return QString(LINK_PLAIN_NAME);
	else return QString();
}

/*void ItAlignment::test()
{
	using namespace std;
	//qDebug() << "lLinks size:" << lLinks.size();
	QString string;
	//QTextStream* str = new QTextStream(&string);
	qDebug() << "Size:" << lLinks.size();
	for(int i=0; i <= maxPos(); ++i) {
		qDebug() << "Position" << i;
		qDebug() << getRData(i).toStringList().join("\n");
*		for(int j=0; j < lLinks.at(i)->size(); j++) {
			string = lLinks.at(i)->at(j)->element->getXML();
			qDebug() << "-" << string.toUtf8();
		}*
	}
}*/

bool ItAlignment::moveDown(aligned_doc doc, int pos)
{
	if (pos>links[doc].size()-1) return false;
  links[doc].insert(pos,new QList<Link*>);
  //updateStat(pos, slist);
  while (links[doc].last()->isEmpty())
      delete links[doc].takeLast();
  info.changed = QDateTime::currentDateTime();
	return true;
}

bool ItAlignment::moveUp(aligned_doc doc, int pos)
{
	QList<Link*> * newlist, * old1, * old2;
	if (pos>links[doc].size()-1 || pos<1) return false;
	old1 = links[doc].at(pos-1);
	old2 = links[doc].at(pos);
	newlist = new QList<Link*>(*old1 + *old2);
	links[doc].replace(pos-1, newlist);
  links[doc].removeAt(pos);
  //updateStat(pos-1, slist);
	delete old1; delete old2;
  info.changed = QDateTime::currentDateTime();
	return true;
}

bool ItAlignment::shift(aligned_doc doc, int pos)
{
	Link * lnk;
	if (pos>links[doc].size()-1 || pos<1) return false;
	if (links[doc].at(pos)->isEmpty()) return false;
	lnk = links[doc].at(pos)->first();
	links[doc].at(pos-1)->append(lnk);
	links[doc].at(pos)->removeFirst();
	if (pos == links[doc].size()-1 && links[doc].last()->isEmpty()) links[doc].removeLast();
  //updateStat(pos-1, slist);
  info.changed = QDateTime::currentDateTime();
	return true;
}

bool ItAlignment::pop(aligned_doc doc, int pos)
{
	Link * lnk;
	if (pos>links[doc].size()-1 || pos<0) return false;
	if (links[doc].at(pos)->isEmpty()) return false;
	if (pos == links[doc].size()-1) links[doc] << new QList<Link*>;
	lnk = links[doc].at(pos)->last();
	links[doc].at(pos+1)->prepend(lnk);
	links[doc].at(pos)->removeLast();
  //updateStat(pos, slist);
  info.changed = QDateTime::currentDateTime();
	return true;
}

bool ItAlignment::is1to1(int pos)
{
	if (pos>links[0].size()-1 || pos>links[1].size()-1) return false;
	if (links[0].at(pos)->size()==1 && links[1].at(pos)->size()==1) return true;
	else return false;
}

void ItAlignment::toggleMark(int pos)
{
	Link * l;
	int oldmark = getMark(pos);
	if (pos<links[0].size())
		foreach (l, *links[0].at(pos)) {
			if (oldmark > 0) l->mark = 0;
			else l->mark = 1;
		}
	if (pos<links[1].size())
		foreach (l, *links[1].at(pos)) {
			if (oldmark > 0) l->mark = 0;
			else l->mark = 1;
		}
  info.changed = QDateTime::currentDateTime();
}

void ItAlignment::setMark(int pos, ushort mark)
{
  Link * l;
  if (pos<links[0].size())
    foreach (l, *links[0].at(pos)) {
      l->mark = mark;
    }
  if (pos<links[1].size())
    foreach (l, *links[1].at(pos)) {
      l->mark = mark;
    }
  info.changed = QDateTime::currentDateTime();
}

void ItAlignment::toggleStat(int pos)
{
	if (getStat(pos) == LINK_MANUAL)
		setStat(pos,LINK_PLAIN);
	else
    setStat(pos,LINK_MANUAL);
}


void ItAlignment::setStat(int pos, ushort status)
{
	Link * l;
	if (pos<links[0].size())
		foreach (l, *links[0].at(pos))
			l->status = status;
	if (pos<links[1].size())
		foreach (l, *links[1].at(pos))
			l->status = status;
  info.changed = QDateTime::currentDateTime();
}

void ItAlignment::updateStat(int pos)
{
	ushort s;
	statRec r;
	if (mode > MODE_MANUAL)
		for (int i=0; i<=pos; ++i) {
			s = getStat(i);
            if (s != LINK_MANUAL) {
				setStat(i,LINK_MANUAL);
			}
		}
}

void ItAlignment::scanStat(int pos, QList<statRec> * slist)
{
    ushort s;
    statRec r;
    if (mode > MODE_MANUAL)
        for (int i=0; i<=pos; ++i) {
            s = getStat(i);
            if (s != LINK_MANUAL) {
                if (slist!=0) {
                    r.position = i;
                    r.status = s;
                    slist->append(r);
                }
            }
        }
}

void ItAlignment::undoStatusChanges(QList<statRec> &slist)
{
	statRec sr;
    foreach (sr, slist) {
		setStat(sr.position, sr.status);
	}
}

/*QSize ItAlignment::getSize(aligned_doc doc, int pos)
{
	if (pos<sizeCache[doc].size()) {
		//qDebug() << "fetching size for:" << doc << pos << "size:" << sizeCache[doc].at(pos).width() << sizeCache[doc].at(pos).height();
		return sizeCache[doc].at(pos);
	}
	else {
		//qDebug() << "fetching size for:" << doc << pos << "size: UNKNOWN" ;
		return QSize();
	}
}

void ItAlignment::cacheSize(aligned_doc doc, int pos, QSize &size)
{
	if (pos<sizeCache[doc].size()) { 
		sizeCache[doc].replace(pos,size);
		//qDebug() << "Caching size for: " << doc << pos << "size:" << sizeCache[doc].at(pos).width() << sizeCache[doc].at(pos).height(); 
	}
}

void ItAlignment::resetSize(aligned_doc doc, int pos)
{
	if (pos<sizeCache[doc].size()) { 
		sizeCache[doc].replace(pos,QSize());
		//qDebug() << "Resetting size for: " << doc << pos; 
	}
}

void ItAlignment::resetAllSizes()
{
	for (int i=0; i < sizeCache[0].size(); ++i)
		sizeCache[0].replace(i,QSize());
	for (int i=0; i < sizeCache[1].size(); ++i)
		sizeCache[1].replace(i,QSize());
}*/

void ItAlignment::getNumbering(int document, int *levels, QString *prefix, QString *parent_prefix) {
  *levels = doc[document]->numLevels();
  *prefix = doc[document]->numPrefix();
  *parent_prefix = doc[document]->numParentPrefix();
}

void ItAlignment::setNumbering(int document, int levels, QString &prefix, QString &parent_prefix) {
  doc[document]->setNumLevels(levels);
  info.ver[document].numLevels = levels;
  doc[document]->setNumPrefix(prefix);
  info.ver[document].numPrefix = prefix;
  doc[document]->setNumParentPrefix(parent_prefix);
  info.ver[document].numParentPrefix = parent_prefix;
}

void ItAlignment::renumber(int document, bool updateCTime) {
  int cnt = 0;
  int parCnt = 0;
  QDomNode lastParent;
  QString lastParentId;
  //QList< Link* > segment;
  //Link link;
  clearIndex(document);
  /*for (int i = 0; i < links[document].size(); i++) {
    segment = *links[document].at(i);
    for (int j = 0; j < segment.size(); j++) {
      link = *segment.at(j);
      cnt++;
      if (info.ver[document].numLevels==1) {
        link.element->setID(QString("%1%2").arg(info.ver[document].numPrefix, QString::number(cnt)), idNamespaceURI);
      } else {
        if (link.element->getParent()!=lastParent) {
          parCnt++; cnt = 1;
          lastParent = link.element->getParent();
          lastParentId = QString("%1%2").arg(info.ver[document].numParentPrefix, QString::number(parCnt)); //qDebug() << lastParentId;
          link.element->setParentID(lastParentId, idNamespaceURI);
        }
        link.element->setID(QString("%1%2%3").arg(lastParentId, info.ver[document].numPrefix, QString::number(cnt)), idNamespaceURI);
      }
    }

  }*/
  // new method usable for cross-order alignments
  QDomElement e;
  QString id;
  if (!alignableElementsOrder[document])
      createAlignableElementsOrder(document);
  for (int i = 0; i < alignableElementsOrder[document]->size(); i++) {
      e = alignableElementsOrder[document]->at(i);
      cnt++;
      if (info.ver[document].numLevels==1) {
          id = QString("%1%2").arg(info.ver[document].numPrefix, QString::number(cnt));
          if (idNamespaceURI.isEmpty())
              e.setAttribute("id", id);
          else
              e.setAttributeNS(idNamespaceURI, "id", id);
      } else {
          if (e.parentNode() != lastParent) {
              parCnt++; cnt = 1;
              lastParent = e.parentNode();
              lastParentId = QString("%1%2").arg(info.ver[document].numParentPrefix, QString::number(parCnt));
              if (idNamespaceURI.isEmpty())
                  e.parentNode().toElement().setAttribute("id", id);
              else
                  e.parentNode().toElement().setAttributeNS(idNamespaceURI, "id", id);
          }
          id = QString("%1%2%3").arg(lastParentId, info.ver[document].numPrefix, QString::number(cnt));
          if (idNamespaceURI.isEmpty())
              e.setAttribute("id", id);
          else
              e.setAttributeNS(idNamespaceURI, "id", id);
      }
  }

  if (updateCTime) {
      QDateTime now = QDateTime::currentDateTime();
      setDocDepCTime(document, now);
      setAlDepCTime(now);
  }
}

void ItAlignment::setDocDepCTime(int document, QDateTime time)
{
  /*if (!trackChanges[document])
    return;*/
  info.ver[document].changed = time;
  alignmentInfo * myinfo;
  for(int i=0; i<depAlignments[document].size(); i++) {
    myinfo = depAlignments[document].value(i);
    myinfo->ver[depAlSharedDoc[document].at(i)].changed = time;
  }
}

void ItAlignment::setDocDepSTime(int document, QDateTime time)
{
  info.ver[document].synced = time;
  alignmentInfo * myinfo;
  for(int i=0; i<depAlignments[document].size(); i++) {
    myinfo = depAlignments[document].value(i);
    myinfo->ver[depAlSharedDoc[document].at(i)].synced = time;
  }
}

void ItAlignment::setDocDepSource(int document, QString source)
{
  info.ver[document].source = source;
  alignmentInfo * myinfo;
  for(int i=0; i<depAlignments[document].size(); i++) {
    myinfo = depAlignments[document].value(i);
    myinfo->ver[depAlSharedDoc[document].at(i)].source = source;
  }
}

void ItAlignment::getAlDepCTimes(int document, QList<QDateTime> * ctimes)
{
  ctimes->clear();
  alignmentInfo * myinfo;
  for(int i=0; i<depAlignments[document].size(); i++) {
    myinfo = depAlignments[document].value(i);
    ctimes->append(myinfo->changed);
  }
}

void ItAlignment::setAlDepCTimes(int document, QList<QDateTime> * ctimes)
{
  alignmentInfo * myinfo;
  for(int i=0; i<depAlignments[document].size(); i++) {
    myinfo = depAlignments[document].value(i);
    myinfo->changed = ctimes->at(i);
  }
}

void ItAlignment::setAlDepCTime(QDateTime time)
{
  info.changed = time;
  alignmentInfo * myinfo;
  for (int d=0; d<2; d++) {
    for(int i=0; i<depAlignments[d].size(); i++) {
      myinfo = depAlignments[d].value(i);
      myinfo->changed = time;
    }
  }
}

/*void ItAlignment::setAlDepSource(QString source)
{
  info.source = source;
  alignmentInfo * myinfo;
  for (int d=0; d<2; d++) {
    for(int i=0; i<depAlignments[d].size(); i++) {
      myinfo = depAlignments[d].value(i);
      myinfo->source = source;
    }
  }
}*/

bool ItAlignment::loadDependentAlignments() {
	QStringList list;
	QDir datadir(storagePath);
	if (!datadir.exists())
		return false;
	datadir.setNameFilters(QStringList(info.docId+".*.conf"));
	list = datadir.entryList();
	alignmentInfo * tmp_info;
	QList<dependentLink*> * dlList;
	for (int i=0; i<list.size(); i++) {
    if (list.at(i)==info.infoName)
      continue;
		tmp_info = new alignmentInfo;
    loadAlignmentConf(QString("%1/%2").arg(storagePath, list.at(i)), tmp_info);
    ushort docNum, sharedNum;
		if (tmp_info->ver[0].name==info.ver[0].name) {
			docNum = 0;
			sharedNum = 0;
		} else if (tmp_info->ver[1].name==info.ver[0].name) {
			docNum = 0;
			sharedNum = 1;
		} else if (tmp_info->ver[0].name==info.ver[1].name) {
			docNum = 1;
			sharedNum = 0;
		} else if (tmp_info->ver[1].name==info.ver[1].name) {
			docNum = 1;
			sharedNum = 1;
		} else {
			delete tmp_info;
			continue;
    }
		depAlignments[docNum].append(tmp_info);
		depAlSharedDoc[docNum].append(sharedNum);
		dlList = new QList<dependentLink*>;
    if (!loadDepAlignment(QString("%1/%2").arg(storagePath, tmp_info->filename), docNum, sharedNum, dlList))
			return false;
		depLinks[docNum].append(dlList);
	}
	return true;
}

bool ItAlignment::loadDepAlignment(QString filename, ushort docNum, ushort sharedNum, QList<dependentLink*> * idlist) {
	unsigned short status;
	QString mark;
	QFileInfo fileinfo(filename);
	QStringList xtargets, lIDs, rIDs, ratio;
	Link *l;
	dependentLink * dl;
	QDomDocument alDoc;
  QString sep(" ");

	//qDebug() << "Reading file:" << afilename;

	// Load alignment
	QString errorMsg; int errorLine, errorColumn;
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly)) {
		errorMessage = QObject::tr("Cannot open dependent file '%1':\n%2.").arg(fileinfo.fileName(), file.errorString());
		return false;
	}
	if (!alDoc.setContent(&file, true, &errorMsg, &errorLine, &errorColumn)) {
		file.close();
		errorMessage = QObject::tr("Error parsing XML document '%1' at line %2, column %3: %4.").arg(fileinfo.fileName(), QString().setNum(errorLine), QString().setNum(errorColumn), errorMsg);
		return false;
	}
	file.close();

	// extract document filenames
	QDomElement docElem = alDoc.documentElement();
	if (docElem.tagName()!="linkGrp") {
		errorMessage = QObject::tr("No root 'linkGrp' element in dependent alignment file '%1'.").arg(fileinfo.fileName());
		return false;
	}

	QDomNode n = docElem.firstChild();
	int pos = 0;
	int el = 0;
	int depPos = 0;
	QStringList baseIds, depIds;
	while(!n.isNull()) {
		QDomElement e = n.toElement();
		if(!e.isNull() && e.tagName()=="link") {
			status = linkStatValue(e.attribute("status", "DEFAULT_STATUS"), DEFAULT_STATUS);
			mark = e.attribute("mark","DEFAULT_MARK");
			xtargets = e.attribute("xtargets",";").split(";");
			ratio = e.attribute("type","-").split("-");
			if (ratio[1]!="0") lIDs = xtargets[1].split(" "); else lIDs = QStringList();
			if (ratio[0]!="0") rIDs = xtargets[0].split(" "); else rIDs = QStringList();
			if (sharedNum==0) {
				baseIds = lIDs; depIds = rIDs;
			} else {
				baseIds = rIDs; depIds = lIDs;
            }
			/* base IDs - add to the shared links */
            for (int c=0; c<baseIds.size(); c++) {
                while (pos<links[docNum].size() && el == links[docNum].at(pos)->size()) {
                    el = 0; pos++;
                }
                if (!(pos<links[docNum].size()))
                    break;
                l = links[docNum].value(pos)->value(el);
                if (baseIds.at(c)!=l->element->getID(idNamespaceURI)) {
                    // fallback for cross-order alignments
                    l = getLinkByElId(docNum, baseIds.at(c), &pos, &el);
                    if (!l) { // does it really matter? could we not just ignore it and just fix the gap somehow?
                        errorMessage = QObject::tr("Element ID '%1' aligned by a dependent alignment '%2' not found in the current alignment.").arg(baseIds.at(c), fileinfo.fileName());
                        return false;
                    }
                }//qDebug()<<"=== Deplink"<<baseIds.at(c)<<"at depPos"<<depPos<<"added to link at pos"<<pos<<"el"<<el;
                dl = new dependentLink;
                dl->id = baseIds.at(c);
                dl->pos = depPos;
                dl->mark = mark.toInt();
                dl->status = status;
                l->depLinks.append(dl);
                el++;
            }
            /* dependent IDs - add to the prepared 'idlist' */
            //for (int c=0; c<depIds.size(); c++) {
            dl = new dependentLink;//qDebug()<<">>> Deplink ids"<<depIds.join(sep)<<"at depPos"<<depPos<<"appended to idlist.";
            dl->id = depIds.join(sep);//depIds.at(c);
            dl->pos = depPos;
            dl->mark = mark.toInt();
            dl->status = status;
            idlist->append(dl);
            //}
			depPos++;
      }
      n = n.nextSibling();
	}
	return true;
}

bool ItAlignment::saveDependentAlignments() {
  for (int d=0; d<=1; d++) {
    for (int i=0; i<depAlignments[d].size(); i++) {
      if (!saveDepAlignment(depAlignments[d].at(i), i, d, depAlSharedDoc[d].at(i)))
        return false;
    }
  }
  return true;
}

bool ItAlignment::saveDepAlignment(alignmentInfo * myinfo, int idx, ushort docNum, ushort sharedNum)
{
  int lcount, rcount;
  ushort s, m;
  QString docIds, depIds, lids, rids, stat, mark;
  QStringList docIdList;
  QString sep(" ");

  saveAlignmentConf(QString("%1/%2.conf").arg(storagePath, myinfo->name), myinfo);

  QFile file(QString("%1/%2").arg(storagePath, myinfo->filename));
  if (!file.open(QIODevice::WriteOnly)) {
    errorMessage = QObject::tr("Error saving dependent alignment '%1':\n%2").arg(myinfo->filename, file.errorString());
    return false;
  }

  //qDebug() << "File opened:" << filename;

  QTextStream out(&file);
  out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
  out << "<linkGrp toDoc=\"" << myinfo->ver[1].filename << "\" fromDoc=\"" << myinfo->ver[0].filename << "\">\n";

  //ushort depNum;
  dependentLink * dl = 0;
  Link * l = 0;
  int max = 0;
  int basePos = 0, baseEl = 0;
  //if (docNum==0) depNum=1; else depNum=0;
  // get max pos of the dependent links
  for (int b=links[docNum].size()-1; b>=0; b--) {
    if (!links[docNum].at(b)->isEmpty()) {
      l = links[docNum].at(b)->last();
      dependentLink * lastdl = l->depLinks.at(idx);
      max = lastdl->pos;
      break;
    }
  }
  if (depLinks[docNum].at(idx)->size()>max) max = depLinks[docNum].at(idx)->size()-1;
  for (ushort i=0; i<=max; ++i) {
    lcount =0; rcount = 0;
    s = 0; m = 0;
    // get IDs from the base document
    docIdList.clear();
    //if (l!=0) qDebug() << "pos" << i << "next link has" <<  l->depLinks.at(idx)->pos << "at index" << basePos << baseEl; else qDebug()<<"End reached";
    /*while (l!=0 && i == l->depLinks.at(idx)->pos) {
      //qDebug() << i << l->element->getID();
      docIdList.append(l->element->getID(idNamespaceURI));
      l = nextLink(docNum, basePos, baseEl);
    }*/
    // every time rescan completely for cross-order alignments
    basePos = 0; baseEl = 0;
    l = nextLink(docNum, basePos, baseEl);
    while (l!=0) {
        if (i == l->depLinks.at(idx)->pos) {
            docIdList.append(l->element->getID(idNamespaceURI));
        }
        l = nextLink(docNum, basePos, baseEl);
    }
    docIds = docIdList.join(sep);
    //get1Stat(docNum, i, &s); // NO!!!!!! propagation of status/mark from a different alignment!
    //get1Mark(docNum, i, &m); // NO!!!!!!
    // get IDs of the dependent document
    if (i<depLinks[docNum].at(idx)->size()) {
      dl = depLinks[docNum].at(idx)->at(i);
      depIds = dl->id;
      //if (dl->status > s) // NO!!!!!!
      s = dl->status;
      //if (dl->mark > m)  // NO!!!!!!
      m = dl->mark;
    } else
      depIds = "";
    if (sharedNum==0) {
      lids = docIds;
      rids = depIds;
    } else {
      lids = depIds;
      rids = docIds;
    }
    if (lids!="") lcount = lids.split(sep).size();
    if (rids!="") rcount = rids.split(sep).size();
    if (m>0) mark = QString(" mark=\"%1\"").arg(m);
    else mark = QString();
    stat = linkStatName(s);
    out << "<link type=\"" << rcount << "-" << lcount << "\" xtargets=\"" << rids << ";" << lids << "\" status=\"" << stat << "\"" << mark << "/>\n";
  }
  out << "</linkGrp>\n";
  file.close();
  return true;
}

bool ItAlignment::createCrossAlignment(aligned_doc baseDoc, QString remoteVersion, bool remoteAtRight)
{
    aligned_doc medDoc = 0;
    if (baseDoc==0)
        medDoc = 1;

    int depAl = -1;
    aligned_doc remDoc;
    alignmentInfo * depInfo;
    for (int i=0; i<depAlignments[medDoc].size(); i++) {
        depInfo = depAlignments[medDoc].at(i);
        remDoc = 1;
        if (depAlSharedDoc[medDoc].at(i)==1)
            remDoc = 0;
        if (depInfo->ver[remDoc].name == remoteVersion) {
            depAl = i;
            break;
        }
    }

    if (depAl<0) {
        errorMessage = QObject::tr("Version '%1' not found among alignments of '%2'.").arg(remoteVersion, info.ver[medDoc].name);
        return false;
    }



    alignmentInfo * remInfo = depAlignments[medDoc].at(depAl);
    remDoc = 1;
    if (depAlSharedDoc[medDoc].at(depAl)!=0)
        remDoc = 0;
    aligned_doc baseSide = 0;
    aligned_doc remSide = 1;
    if (!remoteAtRight) {
        baseSide = 1;
        remSide = 0;
    }
    // we can just rewrite the alignmentInfo, it will not be used anymore
    info.ver[baseSide] = info.ver[baseDoc];
    info.ver[remSide] = remInfo->ver[remDoc];
    info.source = QObject::tr("generated from alignments '%1' and '%2'").arg(info.name, depInfo->name);
    if (remoteAtRight)
        info.name = QString("%1.%2.%3").arg(info.docId, info.ver[baseSide].name, remoteVersion);
    else
        info.name = QString("%1.%3.%2").arg(info.docId, info.ver[baseSide].name, remoteVersion);
    info.filename = QString("%1.xml").arg(info.name);
    QDateTime now = QDateTime::currentDateTime();
    info.changed = now;
    info.synced = now;
    return saveCrossAlignment(&info, baseDoc, depAl, !remoteAtRight);
}

bool ItAlignment::saveCrossAlignment(alignmentInfo * myinfo, aligned_doc baseDoc, int depAl, bool swap)
{
  saveAlignmentConf(QString("%1/%2.conf").arg(storagePath, myinfo->name), myinfo);

  QFile file(QString("%1/%2").arg(storagePath, myinfo->filename));
  if (!file.open(QIODevice::WriteOnly)) {
    errorMessage = QObject::tr("Error saving cross alignment '%1':\n%2").arg(myinfo->filename, file.errorString());
    return false;
  }

  QTextStream out(&file);

  aligned_doc medDoc = 0;
  if (baseDoc==0)
      medDoc = 1;

  QStringList baseIDs, remIDs;
  QString lids, rids, stat, mark = "";
  stat = LINK_AUTO_NAME;
  //ushort s = 0, m = 0;
  int basePos = 0, remPos = 0, lcount, rcount, stopRemPos = 0, nextRemPos = 0;
  Link * lastMedLink;
  dependentLink * lastMedDepLink, * dl;
  QString sep(" ");

  out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
  out << "<linkGrp toDoc=\"" << myinfo->ver[1].filename << "\" fromDoc=\"" << myinfo->ver[0].filename << "\">\n";

  // anything in the remote document preceding the base alignment?
  if (!links[medDoc].at(basePos)->isEmpty() && links[medDoc].at(basePos)->first()->depLinks.at(depAl)->pos>0) {
      for (int i=0; i<links[medDoc].at(basePos)->first()->depLinks.at(depAl)->pos; i++) {
          dl = depLinks[medDoc].at(depAl)->at(i);
          remIDs.append(dl->id);
          remPos++;
      }
      if (!swap) {
          lids = "";
          rids = remIDs.join(sep);
      } else {
          rids = "";
          lids = remIDs.join(sep);
      }
      lcount = 0; rcount = 0;
      if (lids!="") lcount = lids.split(sep).size();
      if (rids!="") rcount = rids.split(sep).size();
      out << "<link type=\"" << rcount << "-" << lcount << "\" xtargets=\"" << rids << ";" << lids << "\" status=\"" << stat << "\"" << mark << "/>\n";
      remIDs.clear();
  }

  // let's process each segment and join the segments which overlap in the remote alignment...
  // put mark where medium-document has a hole (missing text)
  bool close = false;
  while (basePos<=maxPos()) {
      //qDebug()<<"Basepos:"<<basePos<<"remPos:"<<remPos<<"stopRemPos:"<<stopRemPos<<"nextRemPos"<<nextRemPos;
      close = false;
      baseIDs.append(getIDs(baseDoc,basePos));
      //get1Stat(baseDoc, basePos, &s);
      //get1Mark(baseDoc, basePos, &m);
      if (!links[medDoc].at(basePos)->isEmpty()) {
          lastMedLink = links[medDoc].at(basePos)->last();
          lastMedDepLink = lastMedLink->depLinks.at(depAl);
          stopRemPos = lastMedDepLink->pos;
      } else {
          mark = QString(" mark=\"%1\"").arg(1);
      }

      while (remPos<=stopRemPos) {
          dl = depLinks[medDoc].at(depAl)->at(remPos);
          remIDs.append(dl->id);
          /*if (m<dl->mark) m = dl->mark;
          if (s<dl->status) s = dl->status;*/
          remPos++;
      }

      int skip = 1;
      while (basePos+skip<=maxPos() && links[medDoc].at(basePos+skip)->isEmpty()) {
          skip++;
      }
      if (basePos+skip>maxPos())// || links[medDoc].at(basePos+skip)->isEmpty())
          close = true;
      else {
          lastMedLink = links[medDoc].at(basePos+skip)->first();
          lastMedDepLink = lastMedLink->depLinks.at(depAl);
          nextRemPos = lastMedDepLink->pos;
      }

      if (nextRemPos>stopRemPos+1)
          stopRemPos = nextRemPos-1;

      if (close || nextRemPos >= remPos) {
          if (!swap) {
              lids = baseIDs.join(sep);
              rids = remIDs.join(sep);
          } else {
              rids = baseIDs.join(sep);
              lids = remIDs.join(sep);
          }
          lcount = 0; rcount = 0;
          if (lids!="") lcount = lids.split(sep).size();
          if (rids!="") rcount = rids.split(sep).size();
          //if (m>0) mark = QString(" mark='%1'").arg(m); else mark = QString();
          //stat = linkStatName(s);
          out << "<link type=\"" << rcount << "-" << lcount << "\" xtargets=\"" << rids << ";" << lids << "\" status=\"" << stat << "\"" << mark << "/>\n";
          baseIDs.clear();
          remIDs.clear();
          mark = QString();
          //s = 0; m = 0;
          // add a 0:n segment with remote text which is just completely missing in the base alignment
          if (nextRemPos>remPos) {
              while (nextRemPos>remPos) {
                  dl = depLinks[medDoc].at(depAl)->at(remPos);
                  remIDs.append(dl->id);
                  /*if (m<dl->mark) m = dl->mark;
                  if (s<dl->status) s = dl->status;*/
                  remPos++;
              }
              if (!swap) {
                  lids = ""; rids = remIDs.join(sep);
              } else {
                  rids = ""; lids = remIDs.join(sep);
              }
              lcount = 0; rcount = 0;
              if (lids!="") lcount = lids.split(sep).size();
              if (rids!="") rcount = rids.split(sep).size();
              out << "<link type=\"" << rcount << "-" << lcount << "\" xtargets=\"" << rids << ";" << lids << "\" status=\"" << stat << "\"" << mark << "/>\n";
              remIDs.clear();
          }
      }
      basePos++;
  }

  // any more text in the remote document missing in the base alignment?
  if (depLinks[medDoc].at(depAl)->size()>remPos) {
      while (depLinks[medDoc].at(depAl)->size()>remPos) {
          dl = depLinks[medDoc].at(depAl)->at(remPos);
          remIDs.append(dl->id);
          remPos++;
      }
      if (!swap) {
          lids = ""; rids = remIDs.join(sep);
      } else {
          rids = ""; lids = remIDs.join(sep);
      }
      lcount = 0; rcount = 0;
      if (lids!="") lcount = lids.split(sep).size();
      if (rids!="") rcount = rids.split(sep).size();
      out << "<link type=\"" << rcount << "-" << lcount << "\" xtargets=\"" << rids << ";" << lids << "\" status=\"" << stat << "\"" << mark << "/>\n";
      remIDs.clear();
  }

  out << "</linkGrp>\n";
  file.close();
  return true;
}

ItAlignment::Link * ItAlignment::nextLink(aligned_doc doc, int &pos, int &el) {
  if (pos<0 || pos >= links[doc].size()) return 0;
  if (el >= links[doc].at(pos)->size()) {
    el = 0; pos++;
    return nextLink(doc, pos, el);
  } else {
    Link * l = links[doc].at(pos)->at(el);
    el++;
    return l;
  }
}

void ItAlignment::syncDepsPermissions()
{
  info.ver[0].perm_chtext = canDepsChtext(0);
  info.ver[1].perm_chtext = canDepsChtext(1);
  info.ver[0].perm_chstruct = canDepsChstruct(0);
  info.ver[1].perm_chstruct = canDepsChstruct(1);
}

bool ItAlignment::canDepsChtext(ushort doc) {
	alignmentInfo * myinfo;
	for (int i=0; i<depAlignments[doc].size(); i++) {
		myinfo = depAlignments[doc].at(i);
		if (!myinfo->ver[depAlSharedDoc[doc].at(i)].perm_chtext)
			return false;
	}
	return true;
}

bool ItAlignment::canDepsChstruct(ushort doc) {
	alignmentInfo * myinfo;
	for (int i=0; i<depAlignments[doc].size(); i++) {
		myinfo = depAlignments[doc].at(i);
		if (!myinfo->ver[depAlSharedDoc[doc].at(i)].perm_chstruct)
			return false;
	}
	return true;
}

bool ItAlignment::canMergeDeps(aligned_doc doc, int pos, int el, int count) {
  if (!validElement(doc, pos, el))
    return false;
  int max = links[doc].at(pos)->size()-1;
  if (count<1 || el+count>max) {
    errorMessage = QObject::tr("Cannot merge: Elements belong to different segments.");
    return false;
  }
  if (!canChStruct(doc))
    return false;
  Link * l = links[doc].at(pos)->at(el);
  QList<dependentLink*> firstDepLinks = l->depLinks;
  for (int i=1; i <= count; i++) {
    for (int dep=0; dep<firstDepLinks.size(); dep++) {
      l = links[doc].at(pos)->at(el+i);
      if (l->depLinks.at(dep)->pos != firstDepLinks.at(dep)->pos) {
        alignmentInfo * myinfo = depAlignments[doc].at(dep);
        errorMessage = QObject::tr("Cannot merge: Elements belong to different segments in the alignment '%1', position %2 and %3. Please, check and merge the segments manually first.").arg(myinfo->name, QString::number(firstDepLinks.at(dep)->pos+1), QString::number(l->depLinks.at(dep)->pos+1));
        return false;
      }
    }
  }
  return true;
}

uint ItAlignment::find(int startpos, bool forward, ItSearchBar::searchSide side, ItSearchBar::searchType stype, QString str, bool ignoreMarkup, QList<Replacement> &transform)
{
  int curpos = startpos;
  bool searchleft = false, searchright = false;
  switch (side) {
  case ItSearchBar::Left:
    searchleft = true;
    break;
  case ItSearchBar::Right:
    searchright = true;
    break;
  case ItSearchBar::Both:
    searchleft = true;
    searchright = true;
    break;
  }

  do {
    if (forward) {
      if (searchleft && search_segment(0, curpos, forward, stype, str, ignoreMarkup, transform))
          return lastMatch.pos;
      if (searchright && search_segment(1, curpos, forward, stype, str, ignoreMarkup, transform))
          return lastMatch.pos;
    } else {
      if (searchright && search_segment(1, curpos, forward, stype, str, ignoreMarkup, transform))
          return lastMatch.pos;
      if (searchleft && search_segment(0, curpos, forward, stype, str, ignoreMarkup, transform))
          return lastMatch.pos;
    }
    if (curpos==INVALID_POSITION)
      curpos = lastMatch.pos;
    if (forward)
      curpos++;
    else {
      if (curpos==0)
        break;
      else
        curpos--;
    }
  } while (curpos <= maxPos() );
  return INVALID_POSITION;
}

bool ItAlignment::search_segment(aligned_doc doc, int pos, bool forward, ItSearchBar::searchType stype, QString &str, bool ignoreMarkup, QList<Replacement> &transform)
{
  QStringList seg_contents;
  QString text;
  int nextpos = -1, count;
  // start from the beginning of the segment by default
  int el = 0, startpos = 0;

  if (pos==INVALID_POSITION) {
    // start from the last match
    if ((forward && doc<lastMatch.doc) || (!forward && doc>lastMatch.doc)) {
      // skip this position: we do not want to search here anymore
      //qDebug()<<"Not searching in" << doc << "anymore, just continue to the next position forwards/backwards";
      return false;
    }
    pos = lastMatch.pos;
    if (doc!=lastMatch.doc) {
      // searching another document, cannot start from the exact position of the last match
      if (!forward) {
        el = getIDs(doc,pos).size()-1;
        startpos = -1;
      }
    } else {
      // otherwise just continue from the last matched position
      el=lastMatch.el;
      if (el<0)
          el = 0;
      if (stype==ItSearchBar::ElementId) {
        if (forward)
          el = lastMatch.el+1;
        else
          el = lastMatch.el-1;
      } else if (stype==ItSearchBar::EmptySeg || stype==ItSearchBar::Non1Seg || stype==ItSearchBar::UnConfirmed) {
        return false;
      } else {
        if (forward)
          startpos = lastMatch.strpos+1;
        else
          startpos = lastMatch.strpos-1;
      }
      // check for start-position overflows
      if (startpos<0)
        el--;
      int max = getIDs(doc,pos).size()-1;
      if (el<0 || el>max)
        return false;
      text = transformText(getContents(doc, pos, false).toStringList().at(el), ignoreMarkup, transform);
      if (startpos>=text.size()) {
        el++; startpos = 0;
      }
      if (el<0 || el>max)
        return false;
    }
  } else {
    // start from the end if searching backwards
    if (!forward) {
      el = getIDs(doc,pos).size()-1;
      startpos = -1;
    }
  }

  count = getIDs(doc,pos).size();
  seg_contents = getContents(doc, pos, false).toStringList();

  // checking whole segments
  if (stype==ItSearchBar::EmptySeg) {
    if (count==0) {
      resetLastMatch();
      lastMatch.set = true;
      lastMatch.doc = doc;
      lastMatch.pos = pos;
      lastMatch.el = -1;
      return true;
    }
  } else if (stype==ItSearchBar::Non1Seg) {
    if (count!=1) {
      resetLastMatch();
      lastMatch.set = true;
      lastMatch.doc = doc;
      lastMatch.pos = pos;
      lastMatch.el = -1;
      return true;
    }
  } else if (stype==ItSearchBar::Bookmark) {
    if (getMark(pos)>0) {
      resetLastMatch();
      lastMatch.set = true;
      lastMatch.doc = doc;
      lastMatch.pos = pos;
      lastMatch.el = -1;
      return true;
    }
  } else if (stype==ItSearchBar::UnConfirmed) {
      if (getStat(pos)!=LINK_MANUAL) {
        resetLastMatch();
        lastMatch.set = true;
        lastMatch.doc = doc;
        lastMatch.pos = pos;
        lastMatch.el = -1;
        return true;
      }
   } else {
    QRegularExpression re;
    QRegularExpressionMatch match;
    // checking single elements
    while (el>=0 && el<count) {
      switch (stype) {
      case ItSearchBar::ElementId:
        if (getIDs(doc,pos).at(el).contains(QRegularExpression(str, QRegularExpression::CaseInsensitiveOption|QRegularExpression::UseUnicodePropertiesOption))){
          resetLastMatch();
          lastMatch.set = true;
          lastMatch.doc = doc;
          lastMatch.pos = pos;
          lastMatch.el = el;
          return true;
        }
        break;
      case ItSearchBar::SubStr:
        text = transformText(seg_contents.at(el), ignoreMarkup, transform);
        if (forward)
          nextpos = text.indexOf(str, startpos, Qt::CaseInsensitive);
        else
          nextpos = text.lastIndexOf(str, startpos, Qt::CaseInsensitive);
        break;
      case ItSearchBar::SubStrCS:
        text = transformText(seg_contents.at(el), ignoreMarkup, transform);
        if (forward)
          nextpos = text.indexOf(str, startpos, Qt::CaseSensitive);
        else
          nextpos = text.lastIndexOf(str, startpos, Qt::CaseSensitive);
        break;
      case ItSearchBar::RegExp:
        text = transformText(seg_contents.at(el), ignoreMarkup, transform);
        re.setPattern(str);
        //rx.setCaseSensitivity(Qt::CaseInsensitive);
        re.setPatternOptions(QRegularExpression::CaseInsensitiveOption|QRegularExpression::UseUnicodePropertiesOption);
        if (forward)
            nextpos = text.indexOf(re, startpos, &match);
          //nextpos = rx.indexIn(text,startpos);
          //nextpos = seg_contents.at(el).indexOf(QRegExp(str, Qt::CaseInsensitive), startpos);
        else
            nextpos = text.lastIndexOf(re, startpos, &match);
          //nextpos = rx.lastIndexIn(text,startpos);
          //nextpos = seg_contents.at(el).lastIndexOf(QRegExp(str, Qt::CaseInsensitive), startpos);
        break;
      case ItSearchBar::RegExpCS:
        text = transformText(seg_contents.at(el), ignoreMarkup, transform);
        re.setPattern(str);
        //rx.setCaseSensitivity(Qt::CaseSensitive);
        re.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
        if (forward)
            nextpos = text.indexOf(re, startpos, &match);
          //nextpos = rx.indexIn(text,startpos);
          //nextpos = seg_contents.at(el).indexOf(QRegExp(str, Qt::CaseSensitive), startpos);
        else
            nextpos = text.lastIndexOf(re, startpos, &match);
          //nextpos = rx.lastIndexIn(text,startpos);
          //nextpos = seg_contents.at(el).lastIndexOf(QRegExp(str, Qt::CaseSensitive), startpos);
        break;
      default:
        break;
      }
      if (nextpos!=-1) {
        resetLastMatch();
        lastMatch.set = true;
        lastMatch.replaced = false;
        lastMatch.doc = doc;
        lastMatch.pos = pos;
        lastMatch.el = el;
        lastMatch.strpos = nextpos;
        if (!match.hasMatch()) {
          lastMatch.capturedTexts = QStringList();
          lastMatch.len = str.length();
        } else {
          lastMatch.capturedTexts = match.capturedTexts();
          lastMatch.len = match.capturedLength(0);
        }
        return true;
      } else {
        if (forward) {
          el++;
          startpos = 0;
        } else {
          el--;
          startpos = -1;
        }
      }
    }
  }
  return false;
}

QString ItAlignment::transformText(QString text, bool ignoreMarkup, const QList<Replacement> &transform)
{
    if (ignoreMarkup) {
        Replacement r;
        // apply all rules (regular expressions)
        foreach (r, transform) {
            text.replace(QRegularExpression(r.src), r.repl);
        }
        text.replace(QRegExp("<[^>]*>"),"").replace(QRegExp("&[a-zA-Z0-9]*;"),"\x1a");
    }
    return text;
}

void ItAlignment::resetLastMatch()
{
  lastMatch.set = false;
  lastMatch.replaced = true;
  lastMatch.doc = 0;
  lastMatch.pos = 0;
  lastMatch.el = 0;
  lastMatch.strpos = 0;
  lastMatch.len = 0;
  lastMatch.capturedTexts = QStringList();
}

void ItAlignment::setLastMatch(bool replaced, aligned_doc doc, int pos, int el, int strpos, int len, QStringList captures)
{
  lastMatch.set = true;
  lastMatch.replaced = replaced;
  lastMatch.doc = doc;
  lastMatch.pos = pos;
  lastMatch.el = el;
  lastMatch.strpos = strpos;
  lastMatch.len = len;
  lastMatch.capturedTexts = captures;
}



bool ItAlignment::export_text(const QString &filename, aligned_doc doc, int startPos, int endPos, QString head, QString el_sep, QString par_sep, QString foot)
{
  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly)) {
    errorMessage = QObject::tr("Error saving file '%1':\n%2").arg(filename, file.errorString());
    return false;
  }
  QTextStream out(&file);
  out.setCodec("UTF-8");
  out << head;
  QString text;
  QStringList elements;
  //QStringList ids;
  for (int pos=startPos; pos<=endPos; pos++) {
    elements = getContents(doc, pos, false).toStringList();
    //ids = getIDs(doc, pos);
    for (int el=0; el<elements.size(); el++ ) {
      if (isFirst(doc, pos, el)) {
        out << par_sep;
      }
      text = elements.at(el);
      text.replace("\n"," ");
      out << text << el_sep;
    }
  }
  out << foot;
  file.close();
  return true;
}

void ItAlignment::clearIndex(aligned_doc d)
{
  if (doc[d]->hasIndex()) {
    QStringList ids;
    for (int i=0; i<links[d].size(); i++) {
      ids.append(getIDs(d, i));
    }
    doc[d]->destroyIndex(ids);
  }
}

bool ItAlignment::realign(int fromPos, int toPos, QList<QStringList> alignedIds [2], ushort status, QList<statRec> * slist)
{
  QList< QList<Link*>* > newlinks [DOCUMENTS];
  // copy links before the start position
  for (int i=0; i<fromPos; i++) {
    newlinks[0].append(links[0].at(i));
    newlinks[1].append(links[1].at(i));
  }
  // store status for undo
  if (slist) {
    for (int i=fromPos; i<=toPos; i++) {
      statRec r;
      r.position = i;
      r.status = getStat(i);
      slist->append(r);
    }
  }
  // create links for the realigned block
  QString id;
  Link * l;
  int oldpos = fromPos;
  int oldel = 0;
  for(int i=fromPos; i<fromPos+alignedIds[0].size(); i++) {
    newlinks[0] << new QList<Link*>;
    foreach (id, alignedIds[0].at(i-fromPos)) {
      if (id.isEmpty()) continue;
      while (oldel == links[0].at(oldpos)->size()) {
        oldel = 0; oldpos++;
      }
      l = links[0].at(oldpos)->at(oldel);
      if (id!=l->element->getID(idNamespaceURI)) {
        errorMessage = QObject::tr("Inconsistent alignment in the left side document at relative link #%1: expected ID '%2', but found '%3'.").arg(QString::number(oldpos+1), id, l->element->getID(idNamespaceURI));
        return false;
      }
      l->status = status;
      newlinks[0].at(i)->append(l);
      oldel++;
    }
  }
  oldpos = fromPos;
  oldel = 0;
  for(int i=fromPos; i<fromPos+alignedIds[1].size(); i++) {
    newlinks[1] << new QList<Link*>;
    foreach (id, alignedIds[1].at(i-fromPos)) {
      if (id.isEmpty()) continue;
      while (oldel == links[1].at(oldpos)->size()) {
        oldel = 0; oldpos++;
      }
      l = links[1].at(oldpos)->at(oldel);
      if (id!=l->element->getID(idNamespaceURI)) {
        errorMessage = QObject::tr("Inconsistent alignment in the right side document at relative link #%1: expected ID '%2', but found '%3'.").arg(QString::number(oldpos+1), id, l->element->getID(idNamespaceURI));
        return false;
      }
      l->status = status;
      newlinks[1].at(i)->append(l);
      oldel++;
    }
  }
  // append the rest of links
  for (int i=toPos+1; i<links[0].size(); i++) {
    newlinks[0].append(links[0].at(i));
  }
  for (int i=toPos+1; i<links[1].size(); i++) {
    newlinks[1].append(links[1].at(i));
  }
  // swap links and newlinks
  QList< QList<Link*>* > oldlinks [DOCUMENTS];
  oldlinks[0] = links[0];
  oldlinks[1] = links[1];
  links[0] = newlinks[0];
  links[1] = newlinks[1];
  // delete obsolete QList pointers from oldlinks
  for (int i=fromPos; i<=toPos; i++) {
    if (oldlinks[0].size()>i)
      delete oldlinks[0].at(i);
    if (oldlinks[1].size()>i)
      delete oldlinks[1].at(i);
  }
  info.changed = QDateTime::currentDateTime();
  return true;
}

void ItAlignment::applySentenceSplitter(aligned_doc d, ItSentenceSplitter * splitter, QStringList textElements, QString &elName)
{
  QList<ItElement* > list;
  doc[d]->collectElements(&list, textElements);
  ItElement * e;
  QString text;
  QStack<QString> inherittags;
  foreach (e, list) {
    text = e->getContents(false).trimmed();
    fixBrokenTags(text);
    text = splitter->split(text, QString("</%1>\n<%1>").arg(elName));
    text = QString("<%1>%2</%1>").arg(elName, text);
    fixBrokenTags(text, &inherittags);
    e->updateContents(text, false);
  }
}

/* find element linked by n-th link (num) in the alignment and return it
 * together with its position(rpos)/element(rel) coordinates */
ItElement * ItAlignment::getLinkedElement(aligned_doc d, int num, int * rpos, int * rel)
{
  int pos = 0;
  int cnt = links[d].at(pos)->size();
  num++; // size is never 0, unlike "num" index
  // find the position number
  while (cnt<num && pos<links[d].size()-1) {
    pos++;
    cnt += links[d].at(pos)->size();
  }
  cnt -= links[d].at(pos)->size();
  //pos--;
  // compute the element number at that position
  int el = num-cnt-1;
  // does such an element exist at all?
  if (links[d].at(pos)->size()<=el)
    return 0;
  else {
    // set coordinates if desired
    if (rpos!=0)
      *rpos = pos;
    if (rel!=0)
      *rel = el;
    // get the element at pos / el coordinates
    Link * l = links[d].at(pos)->at(el);
    return l->element;
  }
}

int ItAlignment::getNumOfOriginalNth(aligned_doc d, int on, int *pre)
{
  int pos=0;
  int cnt=0;
  *pre = 0;
  Link * l=0;
  int el=0;
  int i=0;
  on++;
  while (cnt<on && pos<links[d].size()) {
    for (el=0; el<links[d].at(pos)->size() && cnt<on; el++) {
      l = links[d].at(pos)->at(el);
      cnt += l->element->repl();
      i++;
    }
    pos++;
  }
  if (l->element->repl()>1)
    *pre = l->element->repl()-1-(cnt-on);
  i--;
  return i;

}

int ItAlignment::getOriginalNumOfElement(aligned_doc d, int pos, int el)
{
  int n=0, i;
  Link * l;
  for (i=0; i<pos; i++) {
    for (int j=0; j<links[d].at(i)->size(); j++) {
      l = links[d].at(i)->at(j);
      n += l->element->repl();
    }
  }
  //i++;
  for (int j=0; j<=el; j++) {
    l = links[d].at(i)->at(j);
    n += l->element->repl();
  }
  return n-1;
}

QList<ItElement* > ItAlignment::getChangedElements(aligned_doc d)
{
  QList<ItElement* > list;
  Link * l;
  ItElement * e;
  int n=0; int lastn = 0;
  for(int pos=0; pos<links[d].size(); pos++) {
    for(int el=0; el<links[d].at(pos)->size(); el++) {
      l = links[d].at(pos)->at(el);
      e = l->element;
      if (!l->element->isVirgin() || !l->element->parbr().isEmpty()) {
        if (e->repl()>0) {
          e->num = n;
          lastn = n;
        } else
          e->num = lastn;
        list.append(e);
      }
      n += e->repl();
    }
  }
  return list;
}

void ItAlignment::deleteAlignment(QString repository_path, QString al)
{
  QFile::remove(QFileInfo(repository_path, QString("%1.conf").arg(al)).canonicalFilePath());
  QFile::remove(QFileInfo(repository_path, QString("%1.xml").arg(al)).canonicalFilePath());
  QStringList c = al.split(".");
  QDir dir(repository_path);
  if (dir.entryList(QStringList(QString("%1.*%2.*conf").arg(c.at(0), c.at(1)))).isEmpty())
    QFile::remove(QFileInfo(repository_path, QString("%1.%2.xml").arg(c.at(0), c.at(1))).canonicalFilePath());
  if (dir.entryList(QStringList(QString("%1.*%2.*conf").arg(c.at(0), c.at(2)))).isEmpty())
    QFile::remove(QFileInfo(repository_path, QString("%1.%2.xml").arg(c.at(0), c.at(2))).canonicalFilePath());
}

QStringList ItAlignment::getAlignableElementnames(aligned_doc d)
{
    QStringList names;
    Link * l;
    QString n;
    for (int i=0; i<links[d].size(); i++) {
        for (int j=0; j<links[d].at(i)->size(); j++) {
            l = links[d].at(i)->at(j);
            n = l->element->element.nodeName();
            if (!names.contains(n))
                names.append(n);
        }
    }
    return names;
}

/*int ItAlignment::getDepAlignment(aligned_doc d, QString versionName)
{
    alignmentInfo * ai;
    aligned_doc rem;
    for (int i=0; i<depAlignments[d].length(); i++) {
        ai = depAlignments[d].at(i);
        rem = 0;
        if (depAlSharedDoc[d].at(i)==0)
            rem = 1;
        if (versionName == ai->ver[rem].name) {
            return i;
        }
    }
    return -1;
}*/

ItDocument * ItAlignment::getDocument(aligned_doc d)
{
    return doc[d];
}

ItElement * ItAlignment::getElement(aligned_doc doc, int pos, int el)
{
    return links[doc].at(pos)->at(el)->element;
}

void ItAlignment::setIdNamespaceURI(QString &uri) {
    idNamespaceURI = uri;
    doc[0]->setIdNamespaceURI(idNamespaceURI);
    doc[1]->setIdNamespaceURI(idNamespaceURI);
}

void ItAlignment::createAlignableElementsOrder(aligned_doc d)
{
    if (alignableElementsOrder[d]) {
        return;
    }
    alignableElementsOrder[d] = new QList<QDomElement>;
    QStringList elnames = getAlignableElementnames(d);
    QList<ItElement*> itelements;
    doc[d]->collectElements(&itelements, elnames);
    ItElement * e;
    while (itelements.size()) {
        e = itelements.takeFirst();
        alignableElementsOrder[d]->append(e->element);
        delete e;
    }
}

int ItAlignment::getAlElementOrder(aligned_doc d, QDomElement el)
{
    if (!alignableElementsOrder[d])
        createAlignableElementsOrder(d);
    return alignableElementsOrder[d]->indexOf(el);
}

bool ItAlignment::breaksOrder(aligned_doc d, int pos, int el)
{
    ItElement * e = links[d].at(pos)->at(el)->element;
    int order = getAlElementOrder(d, e->element);
    int prev = order - 1;
    e = getPrecedingAlignedElement(d, pos, el);
    if (e) {
        prev = getAlElementOrder(d, e->element);
    }
    if (order == prev + 1)
        return false;
    else {
        //if (d>0) qDebug()<<"Order broken by doc"<<d+1<<"pos"<<pos+1<<"el"<<el+1<<"order is"<<order<<"prev is"<<prev;
        return true;
    }
}

ItElement * ItAlignment::getPrecedingAlignedElement(aligned_doc d, int pos, int el)
{
    if (pos<links[d].size()) {
        while (pos>=0) {
            if (el>0) {
                el--;
                return links[d].at(pos)->at(el)->element;
            } else {
                pos--;
                if (pos>=0)
                    el = links[d].at(pos)->size();
            }
        }
    }
    return 0;
}

ItAlignment::Link * ItAlignment::getLinkByElId(aligned_doc d, QString id, int * fpos, int * fel)
{
    Link * l = 0;
    int pos = 0;
    int el = 0;
    while (pos < links[d].size()) {
        while (el < links[d].at(pos)->size()) {
            l = links[d].at(pos)->at(el);
            if (l->element->getID(idNamespaceURI) == id) {
                if (fpos)
                    *fpos = pos;
                if (fel)
                    *fel = el;
                return l;
            }
            el++;
        }
        pos++;
        el = 0;
    }
    return 0;
}

bool ItAlignment::swapWithPrevPosition(aligned_doc d, int pos)
{
    if (pos < 1)
        return false;
    if (pos > links[d].size() - 1)
        return false;
    QList<Link *> * tmp = links[d].at(pos);
    links[d].replace(pos, links[d].at(pos-1));
    links[d].replace(pos-1, tmp);
    return true;
}

bool ItAlignment::crossOrderAlignmentAllowed()
{
    if (info.source.startsWith("http"))
        return false;
    return true;
}
