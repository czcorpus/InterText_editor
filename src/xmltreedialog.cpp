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

#include "xmltreedialog.h"
#include "ui_xmltreedialog.h"
#include <QDebug>

XMLTreeDialog::XMLTreeDialog(ItAlignment *alignment, aligned_doc doc, QWidget *parent):
    QDialog(parent),
    ui(new Ui::XMLTreeDialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);
    model = new ItDomModel(alignment, doc, this);
    ui->treeView->setModel(model);
}

XMLTreeDialog::~XMLTreeDialog()
{
    delete ui;
    delete model;
}

void XMLTreeDialog::openPath(QList<QDomElement> path, int expandNext)
{
    QModelIndex paridx = QModelIndex();
    QDomNode item = model->getRootNode();
    QDomNode el;
    int i = 0;
    while (path.count()) {
        el = item;
        QDomElement step = path.takeFirst();
        i = 0;
        while (item != step && i<el.childNodes().count()) {
            item = el.childNodes().item(i).toElement();
            i++;
        }
        if (item==step) {
            i = i + el.attributes().count() - 1;
            paridx = model->index(i, 0, paridx);
            ui->treeView->expand(paridx);
        }
    }
    while (expandNext) {
        expandNext--;
        i++;
        QModelIndex idx = model->index(i, 0, paridx.parent());
        ui->treeView->expand(idx);
    }
    ui->treeView->setCurrentIndex(paridx);
    ui->treeView->scrollTo(paridx);
}
