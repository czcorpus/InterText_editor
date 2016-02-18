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

#ifndef ITFLOATCONTROLS_H
#define ITFLOATCONTROLS_H

#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QTimer>

class ItFloatControls : public QWidget
{
    Q_OBJECT
public:
    explicit ItFloatControls(QWidget *parent = 0);
    ~ItFloatControls();
    void setRow(int row);
    int row();
    void setEnabledLeft(bool set = true);
    void setEnabledRight(bool set = true);
signals:
    void sglUp(int row, int doc);
    void sglDown(int row, int doc);
    void txtUp(int row, int doc);
    void txtDown(int row, int doc);
    void bothUp(int row);
    void bothDown(int row);
public slots:
    void show(int autohide = 0);
    void setIconSize(int size = 16);

private:
    QGridLayout *layout;
    QPushButton *lSglUp, *lSglDown, *lDblUp, *lDblDown, *bUp, *bDown, *rDblUp, *rDblDown, *rSglUp, *rSglDown;
    QTimer timer;
    QIcon composeIcon(QString fname);
    int currow;
private slots:
    void lsglUpPressed();
    void lsglDownPressed();
    void ldblUpPressed();
    void ldblDownPressed();
    void rsglUpPressed();
    void rsglDownPressed();
    void rdblUpPressed();
    void rdblDownPressed();
    void bUpPressed();
    void bDownPressed();
};

#endif // ITFLOATCONTROLS_H
