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

#ifndef ITCOMMON_H
#define ITCOMMON_H

#define TINY 16
#define SMALL 22
#define MEDIUM 32
#define LARGE 48
#define XLARGE 64
#define XXLARGE 72
#define XXXLARGE 96
#define ULARGE 128

#include <QKeySequence>

enum AutoState { AutoNo = 0, AutoYes, AutoAsk };
enum ShowControlsType { HiddenCtrl = 0, OnMove, OnClick };

/*struct EditorKeys
{
    int saveExit;
    int discardExit;
    int saveNext;
    int savePrev;
};*/

struct EditorKeys
{
    QKeySequence saveExit;
    QKeySequence discardExit;
    QKeySequence saveNext;
    QKeySequence savePrev;
};

struct Replacement
{
  QString src;
  QString repl;
};

#endif // ITCOMMON_H
