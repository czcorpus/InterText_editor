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

#include "ItSentenceSplitter.h"
#include <QDebug>
#include <QRegularExpression>

ItSentenceSplitter::ItSentenceSplitter()
{
  selectedProfile = 0;
}

QStringList ItSentenceSplitter::getProfileNames()
{
  QStringList list;
  SplitterProfile p;
  foreach(p, profiles) {
    list << p.name;
  }
  return list;
}

QString ItSentenceSplitter::split(QString text, QString sep)
{
  SplitterProfile p = profiles.at(selectedProfile);
  Replacement r;
  // apply all rules (regular expressions)
  foreach (r, p.expressions) {
    text.replace(QRegularExpression(r.src, QRegularExpression::UseUnicodePropertiesOption), r.repl);
  }
  // remove break after abbreviations
  QString abbr;
  foreach (abbr, p.abbrevs) {
    text.replace(QRegularExpression(QString("(\\b%1)#!#").arg(abbr.replace(".","\\.")), QRegularExpression::UseUnicodePropertiesOption),"\\1 ");
    //text.replace(QRegularExpression(QString("(\\b%1)#!#").arg(QRegularExpression::escape(abbr))),"\\1 ");
  }
  // fix broken XML tags
  text.replace("#!#", " #!# ");
  int pos = 0;
  int len;
  QString open, body, close;
  QRegExp re("(<([a-zA-Z]+)[^>]*>)(.*)(<\\/\\2>)");
  re.setMinimal(true);
  while ((pos=re.indexIn(text, pos))>=0) {
    len = re.capturedTexts().at(0).length();
    open = re.capturedTexts().at(1);
    body = re.capturedTexts().at(3);
    close = re.capturedTexts().at(4);
    if (body.contains("#!#")) {
      body.replace(QRegularExpression("((?:<\\/[^>]+>)*#!#(?:<[^\\/][^>]*>)*)"),QString("%1\\1%2").arg(close, open));
      text.replace(pos, len, QString("%1%2%3").arg(open, body, close));
    }
    pos++;
  }
  text.replace(QRegularExpression("\\s*((?:<\\/[^>]+>)*#!#(?:<[^\\/][^>]*>)*)\\s*"), "\\1");

  return text.replace("#!#", sep);
}

void ItSentenceSplitter::selectProfile(int n)
{
  selectedProfile = n;
}

void ItSentenceSplitter::setProfiles(QList<SplitterProfile> newprofiles)
{
  profiles = newprofiles;
}

QList<ItSentenceSplitter::SplitterProfile> ItSentenceSplitter::getProfiles()
{
  return profiles;
}
