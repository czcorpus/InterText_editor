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

#include <QApplication>
#include "ItWindow.h"

int main (int argc, char *argv[])
{
  //QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
  //QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
  //QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
  QApplication itApp(argc, argv);
  itApp.setApplicationName("InterText");
  //itApp.setAttribute(Qt::AA_DontShowIconsInMenus);
#ifndef Q_OS_MAC
  itApp.setWindowIcon(QIcon(":/images/16/InterText.ico"));
#endif
  ItWindow itWin;
  return itApp.exec();
}
