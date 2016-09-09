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

#ifndef ALIGNMENTMANAGER_H
#define ALIGNMENTMANAGER_H

#include <QDialog>
#include <QAbstractListModel>
#include "AlignmentAttrDialog.h"

namespace Ui {
class AlignmentManager;
}

class ItWindow;

class AlListModel : public QAbstractListModel
{
    Q_OBJECT

public:

    AlListModel(QString repository, QObject *parent);
    ~AlListModel();

    QVariant data(const QModelIndex &index, int role) const;
    //bool setData(const QModelIndex &index, const QVariant &value, int role);
    //Qt::ItemFlags flags(const QModelIndex &index) const;
    //QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    //QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    //QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QString getName(int i);
    void deleteAl(QString al);
    QStringList listTexts();
    QStringList listVersions();

public slots:

    void externalChange();

private:
    QString repository_path;

};


class AlignmentManager : public QDialog
{
    Q_OBJECT

public:
    explicit AlignmentManager(QString repository, QWidget *parent);
    ~AlignmentManager();
    QStringList listTexts();
    QStringList listVersions();

public slots:
    void show(bool openonly=false);
    void externalChange();

signals:
    void alDeletedInRepo(QString alname);

private:
    Ui::AlignmentManager *ui;
    AlListModel * model;
    ItWindow * window;
    QString repository_path;

    QPushButton * openButton;
    QPushButton * newButton;
    QPushButton * importButton;
    QPushButton * delButton;
    QPushButton * propButton;

private slots:
    void alDelete();
    void alOpen();
    void alNew();
    void alProp();
    void alImport();
    void curAlChanged(QModelIndex index);
};


#endif // ALIGNMENTMANAGER_H
