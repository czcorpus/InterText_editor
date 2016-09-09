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

#ifndef IT_SEARCHBAR_H
#define IT_SEARCHBAR_H

#include <QWidget>
#include <QtWidgets>

class ItWindow;

class ItSearchBar : public QWidget
{
    Q_OBJECT

public:
    enum searchType { SubStr = 0, SubStrCS, RegExp, RegExpCS, ElementId, Bookmark, EmptySeg, Non1Seg, UnConfirmed };
    enum searchSide { Left = 1, Right = 2, Both = 0 };
    enum barMode { Replace = 0, Search };
    ItSearchBar(ItWindow *parent);
    ~ItSearchBar();
    searchType getSearchType();
    searchSide getSearchSide();
    QString getSearchString();
    QString getReplacementString();
    void addCurrentQuery();
    bool emptySearch();

public slots:
    void showSearch();
    void showReplace();
    void hide();
    void enableReplace(bool enable);

signals:
    void findNext();
    void findPrev();
    void replFind();
    void hiding();

private:
    QComboBox * findEdit;
    QComboBox * replEdit;
    QComboBox * searchTypeSel;
    QComboBox * searchSideSel;
    QToolButton * toggleButton;
    QPushButton * findNextButton;
    QPushButton * findPrevButton;
    QPushButton * findAllButton;
    QPushButton * replaceButton;
    QPushButton * replFindButton;
    QPushButton * replAllButton;
    barMode mode;
    void showAs(barMode startMode);
    bool eventFilter(QObject *obj, QEvent *ev);
    ItWindow * window;
    bool ensureNoHtmlView();

private slots:
    void toggleMode();
    void searchTypeChanged(int index);
};

#endif // IT_SEARCHBAR_H
