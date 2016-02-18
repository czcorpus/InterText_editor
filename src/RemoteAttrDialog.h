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

#ifndef REMOTEATTRDIALOG_H
#define REMOTEATTRDIALOG_H

#include <QDialog>
#include "ServerDialog.h"

namespace Ui {
class RemoteAttrDialog;
}

class RemoteAttrDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit RemoteAttrDialog(ServerDialog *parent, QString key);
    ~RemoteAttrDialog();
    
private:
    Ui::RemoteAttrDialog *ui;
    ServerDialog *sd;
    ServerDialog::RemoteAlignment al;
    void setRemoteUser(int userid);
    QSortFilterProxyModel *selEditorProxyModel;
    QAbstractItemModel *oldSelEditorModel;
    QSortFilterProxyModel *selRespProxyModel;
    QAbstractItemModel *oldSelRespModel;
    QSortFilterProxyModel *selREditorProxyModel;
    QAbstractItemModel *oldSelREditorModel;

private slots:
    void changeEditor(int n);
    void changeResp(int n);
    void changeRemoteUser(int n);
    void changeStatus(int n);
    void changeEdit(int state);
    void changeCChstruct(int state);
    void setAccess();
    void apply();
};

#endif // REMOTEATTRDIALOG_H
