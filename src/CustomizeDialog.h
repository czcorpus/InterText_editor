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

#ifndef CUSTOMIZEDIALOG_H
#define CUSTOMIZEDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QMenu>

class ItWindow;

namespace Ui {
class CustomizeDialog;
}

class CustomizeDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit CustomizeDialog(ItWindow *parent = 0);
    ~CustomizeDialog();
public slots:
    void accept();

protected:
    bool eventFilter(QObject *obj, QEvent *ev);

private:
    Ui::CustomizeDialog *ui;
    ItWindow *window;
    int scanKeysToRow;
    QString lastKeys;
    QMenu ctxMenu;
    QAction *setAct, *delAct;
    void reconfigureToolBar();
    void reconfigureContextMenu();
    void reconfigureShortcuts();

private slots:
    void addTBItem();
    void delTBItem();
    void addCMItem();
    void delCMItem();
    void shortCutEdit(QModelIndex index);
    void shortCutChange(int row, int col, int prerow, int precol);
    void showShortcutsMenu(const QPoint& pos);
    void requestShortcutEdit();
    void requestShortcutDelete();
};

#endif // CUSTOMIZEDIALOG_H
