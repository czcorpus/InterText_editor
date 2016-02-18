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

#include "CustomizeDialog.h"
#include "ui_CustomizeDialog.h"
#include <QDebug>
#include "ItWindow.h"

CustomizeDialog::CustomizeDialog(ItWindow *parent) :
    QDialog(parent),
    ui(new Ui::CustomizeDialog)
{
    window = parent;
    scanKeysToRow = -1;
    ui->setupUi(this);
    ui->shortcutsTable->installEventFilter(this);
    ui->tbCurrentList->setDragDropMode(QAbstractItemView::InternalMove);
    ui->cmCurrentList->setDragDropMode(QAbstractItemView::InternalMove);

    setAct = new QAction(tr("Set new shortcut"), this);
    setAct->setStatusTip(tr("Set new shortcut"));
    connect(setAct, SIGNAL(triggered()), this, SLOT(requestShortcutEdit()));
    delAct = new QAction(tr("Delete shortcut"), this);
    delAct->setStatusTip(tr("Delete shortcut"));
    connect(delAct, SIGNAL(triggered()), this, SLOT(requestShortcutDelete()));
    ctxMenu.addAction(setAct);
    ctxMenu.addAction(delAct);

    //setContextMenuPolicy(Qt::CustomContextMenu);
    ui->shortcutsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->shortcutsTable, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showShortcutsMenu(const QPoint&)));

    connect(ui->addTBItem, SIGNAL(clicked()), this, SLOT(addTBItem()));
    connect(ui->delTBItem, SIGNAL(clicked()), this, SLOT(delTBItem()));
    connect(ui->addCMItem, SIGNAL(clicked()), this, SLOT(addCMItem()));
    connect(ui->delCMItem, SIGNAL(clicked()), this, SLOT(delCMItem()));

    // toolbar
    QListWidgetItem *sep = new QListWidgetItem("---");
    sep->setData(Qt::UserRole, QVariant(0));
    ui->tbAvailableList->addItem(sep);
    QListWidgetItem *i;
    QAction *a;
    foreach(a, parent->toolBar->actions()) {
        if (a->text().isEmpty())
            i = new QListWidgetItem(*sep);
        else {
            i = new QListWidgetItem(a->icon(), a->text());
            i->setData(Qt::UserRole, QVariant((qulonglong)a));
        }
        ui->tbCurrentList->addItem(i);
    }

    foreach(a, parent->toolBarActions) {
        if (!parent->toolBar->actions().contains(a)) {
            i = new QListWidgetItem(a->icon(), a->text());
            i->setData(Qt::UserRole, QVariant((qulonglong)a));
            ui->tbAvailableList->addItem(i);
        }
    }

    // context menu
    sep = new QListWidgetItem("---");
    sep->setData(Qt::UserRole, QVariant(0));
    ui->cmAvailableList->addItem(sep);
    foreach(a, parent->contextMenuCurActions) {
        if (!a)
            i = new QListWidgetItem(*sep);
        else {
            i = new QListWidgetItem(a->icon(), a->text());
            i->setData(Qt::UserRole, QVariant((qulonglong)a));
        }
        ui->cmCurrentList->addItem(i);
    }
    foreach(a, parent->ctxmenuActions) {
        if (!parent->contextMenuCurActions.contains(a)) {
            i = new QListWidgetItem(a->icon(), a->text());
            i->setData(Qt::UserRole, QVariant((qulonglong)a));
            ui->cmAvailableList->addItem(i);
        }
    }

    // shortcuts
    int r=0;
    QStringList labels;
    labels << tr("Action") << tr("Keyboard shortcut");
    ui->shortcutsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->shortcutsTable->setRowCount(parent->allActions.size()+4);
    ui->shortcutsTable->setColumnCount(2);
    ui->shortcutsTable->setHorizontalHeaderLabels(labels);
    ui->shortcutsTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->shortcutsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->shortcutsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->shortcutsTable->verticalHeader()->hide();
    connect(ui->shortcutsTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(shortCutEdit(QModelIndex)));
    connect(ui->shortcutsTable, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(shortCutChange(int,int,int,int)));
    QTableWidgetItem *ti;
    foreach (a, parent->allActions) {
        ti = new QTableWidgetItem(a->text());
        ti->setFlags(ti->flags() & ~Qt::ItemIsEditable);
        ui->shortcutsTable->setItem(r, 0, ti);
        ti = new QTableWidgetItem(a->shortcut().toString());
        ti->setFlags(ti->flags() & ~Qt::ItemIsEditable);
        ui->shortcutsTable->setItem(r, 1, ti);
        r++;
    }
    ti = new QTableWidgetItem(tr("Editor: save and exit"));
    ti->setFlags(ti->flags() & ~Qt::ItemIsEditable);
    ui->shortcutsTable->setItem(r, 0, ti);
    ti = new QTableWidgetItem(window->editorKeys.saveExit.toString());
    ti->setFlags(ti->flags() & ~Qt::ItemIsEditable);
    ui->shortcutsTable->setItem(r, 1, ti);
    r++;
    ti = new QTableWidgetItem(tr("Editor: discard and exit"));
    ti->setFlags(ti->flags() & ~Qt::ItemIsEditable);
    ui->shortcutsTable->setItem(r, 0, ti);
    ti = new QTableWidgetItem(window->editorKeys.discardExit.toString());
    ti->setFlags(ti->flags() & ~Qt::ItemIsEditable);
    ui->shortcutsTable->setItem(r, 1, ti);
    r++;
    ti = new QTableWidgetItem(tr("Editor: save and open next"));
    ti->setFlags(ti->flags() & ~Qt::ItemIsEditable);
    ui->shortcutsTable->setItem(r, 0, ti);
    ti = new QTableWidgetItem(window->editorKeys.saveNext.toString());
    ti->setFlags(ti->flags() & ~Qt::ItemIsEditable);
    ui->shortcutsTable->setItem(r, 1, ti);
    r++;
    ti = new QTableWidgetItem(tr("Editor: save and open previous"));
    ti->setFlags(ti->flags() & ~Qt::ItemIsEditable);
    ui->shortcutsTable->setItem(r, 0, ti);
    ti = new QTableWidgetItem(window->editorKeys.savePrev.toString());
    ti->setFlags(ti->flags() & ~Qt::ItemIsEditable);
    ui->shortcutsTable->setItem(r, 1, ti);
    r++;
}

CustomizeDialog::~CustomizeDialog()
{
    delete ui;
}

void CustomizeDialog::addTBItem()
{
    if (!ui->tbAvailableList->currentIndex().isValid())
        return;
    QListWidgetItem *cur = ui->tbAvailableList->currentItem();
    if (cur->data(Qt::DisplayRole).toString() != "---") {
        ui->tbAvailableList->takeItem(ui->tbAvailableList->row(cur));
        ui->tbCurrentList->addItem(cur);
    } else {
        QListWidgetItem * newitem = new QListWidgetItem(*cur);
        ui->tbCurrentList->addItem(newitem);
    }
}

void CustomizeDialog::delTBItem()
{
    if (!ui->tbCurrentList->currentIndex().isValid())
        return;
    QListWidgetItem *cur = ui->tbCurrentList->currentItem();
    ui->tbCurrentList->takeItem(ui->tbCurrentList->row(cur));
    if (cur->data(Qt::DisplayRole).toString() == "---") {
        delete cur;
    } else {
        ui->tbAvailableList->addItem(cur);
    }
}

void CustomizeDialog::addCMItem()
{
    if (!ui->cmAvailableList->currentIndex().isValid())
        return;
    QListWidgetItem *cur = ui->cmAvailableList->currentItem();
    if (cur->data(Qt::DisplayRole).toString() != "---") {
        ui->cmAvailableList->takeItem(ui->cmAvailableList->row(cur));
        ui->cmCurrentList->addItem(cur);
    } else {
        QListWidgetItem * newitem = new QListWidgetItem(*cur);
        ui->cmCurrentList->addItem(newitem);
    }
}

void CustomizeDialog::delCMItem()
{
    if (!ui->cmCurrentList->currentIndex().isValid())
        return;
    QListWidgetItem *cur = ui->cmCurrentList->currentItem();
    ui->cmCurrentList->takeItem(ui->cmCurrentList->row(cur));
    if (cur->data(Qt::DisplayRole).toString() == "---") {
        delete cur;
    } else {
        ui->cmAvailableList->addItem(cur);
    }
}

void CustomizeDialog::accept()
{
    reconfigureToolBar();
    reconfigureContextMenu();
    reconfigureShortcuts();
    QDialog::accept();
}

void CustomizeDialog::reconfigureToolBar()
{
    QAction *a;
    QListWidgetItem *it;
    window->toolBar->clear();
    for (int i=0; i<ui->tbCurrentList->count(); i++) {
        it = ui->tbCurrentList->item(i);
        a = (QAction*)it->data(Qt::UserRole).toULongLong();
        if (a)
            window->toolBar->addAction(a);
        else
            window->toolBar->addSeparator();
    }
}

void CustomizeDialog::reconfigureContextMenu()
{
    QAction *a;
    QListWidgetItem *it;
    window->contextMenuCurActions.clear();
    for (int i=0; i<ui->cmCurrentList->count(); i++) {
        it = ui->cmCurrentList->item(i);
        a = (QAction*)it->data(Qt::UserRole).toULongLong();
        window->contextMenuCurActions.append(a);
    }
    window->view->createContextMenu();
}

void CustomizeDialog::reconfigureShortcuts()
{
    QAction *a;
    int i=0;
    foreach(a, window->allActions) {
        if (a->shortcut().toString() != ui->shortcutsTable->item(i, 1)->text()) {
            a->setShortcut(QKeySequence::fromString(ui->shortcutsTable->item(i, 1)->text()));
        }
        i++;
    }
    window->editorKeys.saveExit = QKeySequence::fromString(ui->shortcutsTable->item(i, 1)->text()); i++;
    window->editorKeys.discardExit = QKeySequence::fromString(ui->shortcutsTable->item(i, 1)->text()); i++;
    window->editorKeys.saveNext = QKeySequence::fromString(ui->shortcutsTable->item(i, 1)->text()); i++;
    window->editorKeys.savePrev = QKeySequence::fromString(ui->shortcutsTable->item(i, 1)->text()); i++;
    window->view->setEditorKeys(window->editorKeys);
}

void CustomizeDialog::shortCutEdit(QModelIndex index)
{
    QTableWidgetItem *item;
    if (scanKeysToRow>=0) {
        item = ui->shortcutsTable->item(scanKeysToRow, 1);
        item->setText(lastKeys);
    }
    scanKeysToRow = index.row();
    item = ui->shortcutsTable->item(index.row(), 1);
    lastKeys =  item->text();
    item->setText(tr("Press desired keys..."));
}

void CustomizeDialog::shortCutChange(int row, int col, int prerow, int precol)
{
    QTableWidgetItem *item;
    if (scanKeysToRow>=0 && row!=scanKeysToRow) {
        item = ui->shortcutsTable->item(scanKeysToRow, 1);
        item->setText(lastKeys);
    }

}

bool CustomizeDialog::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj != ui->shortcutsTable)
        return false;

    if (ev->type() != QEvent::KeyPress || scanKeysToRow<0){
        return false;
    }

    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(ev);
    int keyInt = keyEvent->key();
    Qt::Key key = static_cast<Qt::Key>(keyInt);
    if(key == Qt::Key_unknown){
        //qDebug() << "Unknown key from a macro probably";
        return false;
    }
    // the user have clicked just and only the special keys Ctrl, Shift, Alt, Meta.
    if(key == Qt::Key_Control ||
            key == Qt::Key_Shift ||
            key == Qt::Key_Alt ||
            key == Qt::Key_Meta)
    {
        //qDebug() << "Single click of special key: Ctrl, Shift, Alt or Meta";
        //qDebug() << "New KeySequence:" << QKeySequence(keyInt).toString(QKeySequence::NativeText);
        return false;
    }

    // check for a combination of user clicks
    Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
    QString keyText = keyEvent->text();
    // if the keyText is empty than it's a special key like F1, F5, ...
    //qDebug() << "Pressed Key:" << keyText;

    //QList<Qt::Key> modifiersList;
    if(modifiers & Qt::ShiftModifier)
        keyInt += Qt::SHIFT;
    if(modifiers & Qt::ControlModifier)
        keyInt += Qt::CTRL;
    if(modifiers & Qt::AltModifier)
        keyInt += Qt::ALT;
    if(modifiers & Qt::MetaModifier)
        keyInt += Qt::META;

    //qDebug() << "New KeySequence:" << QKeySequence(keyInt).toString(QKeySequence::PortableText);
    QTableWidgetItem *item;
    item = ui->shortcutsTable->item(scanKeysToRow, 1);
    item->setText(QKeySequence(keyInt).toString(QKeySequence::PortableText));
    scanKeysToRow = -1;
    lastKeys = "";
    return true;
}

void CustomizeDialog::showShortcutsMenu(const QPoint& pos)
{
    QPoint globalPos = ui->shortcutsTable->viewport()->mapToGlobal(pos);
    ui->shortcutsTable->setCurrentIndex(ui->shortcutsTable->model()->index(ui->shortcutsTable->rowAt(pos.y()), ui->shortcutsTable->columnAt(pos.x()), QModelIndex()));

    ctxMenu.exec(globalPos);
}

void CustomizeDialog::requestShortcutEdit()
{
    shortCutEdit(ui->shortcutsTable->currentIndex());
}

void CustomizeDialog::requestShortcutDelete()
{
    ui->shortcutsTable->currentItem()->setText("");
}
