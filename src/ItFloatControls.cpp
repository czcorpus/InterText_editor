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

#include "ItFloatControls.h"

ItFloatControls::ItFloatControls(QWidget *parent) :
    QWidget(parent)
{
    layout = new QGridLayout(this);
    layout->setSpacing(0);
    lSglUp = new QPushButton(composeIcon("arrow-up.png"), QString(), this);
    lSglDown = new QPushButton(composeIcon("arrow-down.png"), QString(), this);
    rSglUp = new QPushButton(composeIcon("arrow-up.png"), QString(), this);
    rSglDown = new QPushButton(composeIcon("arrow-down.png"), QString(), this);
    lDblUp = new QPushButton(composeIcon("arrow-up-double.png"), QString(), this);
    lDblDown = new QPushButton(composeIcon("arrow-down-double.png"), QString(), this);
    rDblUp = new QPushButton(composeIcon("arrow-up-double.png"), QString(), this);
    rDblDown = new QPushButton(composeIcon("arrow-down-double.png"), QString(), this);
    bUp = new QPushButton(composeIcon("go-up.png"), QString(), this);
    bDown = new QPushButton(composeIcon("go-down.png"), QString(), this);

    connect(lSglUp, SIGNAL(pressed()), this, SLOT(lsglUpPressed()));
    connect(lSglDown, SIGNAL(pressed()), this, SLOT(lsglDownPressed()));
    connect(lDblUp, SIGNAL(pressed()), this, SLOT(ldblUpPressed()));
    connect(lDblDown, SIGNAL(pressed()), this, SLOT(ldblDownPressed()));
    connect(rSglUp, SIGNAL(pressed()), this, SLOT(rsglUpPressed()));
    connect(rSglDown, SIGNAL(pressed()), this, SLOT(rsglDownPressed()));
    connect(rDblUp, SIGNAL(pressed()), this, SLOT(rdblUpPressed()));
    connect(rDblDown, SIGNAL(pressed()), this, SLOT(rdblDownPressed()));
    connect(bUp, SIGNAL(pressed()), this, SLOT(bUpPressed()));
    connect(bDown, SIGNAL(pressed()), this, SLOT(bDownPressed()));

    layout->addWidget(lSglUp, 0, 0);
    layout->addWidget(lSglDown, 1, 0);
    layout->addWidget(lDblUp, 0, 1);
    layout->addWidget(lDblDown, 1, 1);
    layout->addWidget(bUp, 0, 2);
    layout->addWidget(bDown, 1, 2);
    layout->addWidget(rDblUp, 0, 3);
    layout->addWidget(rDblDown, 1, 3);
    layout->addWidget(rSglUp, 0, 4);
    layout->addWidget(rSglDown, 1, 4);
    setLayout(layout);
    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), this, SLOT(hide()));
}

ItFloatControls::~ItFloatControls()
{
    delete lSglUp;
    delete lSglDown;
    delete lDblUp;
    delete lDblDown;
    delete bUp;
    delete bDown;
    delete rDblUp;
    delete rDblDown;
    delete rSglUp;
    delete rSglDown;
    delete layout;
}

void ItFloatControls::setRow(int row)
{
    currow = row;
    if (row == 0) {
        lSglUp->setEnabled(false);
        lDblUp->setEnabled(false);
        bUp->setEnabled(false);
        rSglUp->setEnabled(false);
        rDblUp->setEnabled(false);
    } else {
        lSglUp->setEnabled(true);
        lDblUp->setEnabled(true);
        bUp->setEnabled(true);
        rSglUp->setEnabled(true);
        rDblUp->setEnabled(true);
    }
}

int ItFloatControls::row()
{
    return currow;
}

void ItFloatControls::setEnabledLeft(bool set)
{
    if (currow != 0) {
        lSglUp->setEnabled(set);
        lDblUp->setEnabled(set);
    }
    lSglDown->setEnabled(set);
    lDblDown->setEnabled(set);
}

void ItFloatControls::setEnabledRight(bool set)
{
    if (currow != 0) {
        rSglUp->setEnabled(set);
        rDblUp->setEnabled(set);
    }
    rSglDown->setEnabled(set);
    rDblDown->setEnabled(set);
}

QIcon ItFloatControls::composeIcon(QString fname)
{
    QIcon i = QIcon();
    i.addFile(QString(":/images/").append(fname));
    i.addFile(QString(":/images/16/").append(fname));
    i.addFile(QString(":/images/22/").append(fname));
    i.addFile(QString(":/images/32/").append(fname));
    i.addFile(QString(":/images/48/").append(fname));
    i.addFile(QString(":/images/svg/").append(fname.replace(".png",".svgz")));
    return i;
}

void ItFloatControls::setIconSize(int size)
{
    lSglUp->setIconSize(QSize(size, size));
    lSglDown->setIconSize(QSize(size, size));
    lDblUp->setIconSize(QSize(size, size));
    lDblDown->setIconSize(QSize(size, size));
    rSglUp->setIconSize(QSize(size, size));
    rSglDown->setIconSize(QSize(size, size));
    rDblUp->setIconSize(QSize(size, size));
    rDblDown->setIconSize(QSize(size, size));
    bUp->setIconSize(QSize(size, size));
    bDown->setIconSize(QSize(size, size));
}

void ItFloatControls::lsglUpPressed()
{
    hide();
    emit sglUp(currow, 0);
}

void ItFloatControls::lsglDownPressed()
{
    hide();
    emit sglDown(currow, 0);
}

void ItFloatControls::ldblUpPressed()
{
    hide();
    emit txtUp(currow, 0);
}

void ItFloatControls::ldblDownPressed()
{
    hide();
    emit txtDown(currow, 0);
}

void ItFloatControls::rsglUpPressed()
{
    hide();
    emit sglUp(currow, 1);
}

void ItFloatControls::rsglDownPressed()
{
    hide();
    emit sglDown(currow, 1);
}

void ItFloatControls::rdblUpPressed()
{
    hide();
    emit txtUp(currow, 1);
}

void ItFloatControls::rdblDownPressed()
{
    hide();
    emit txtDown(currow, 1);
}

void ItFloatControls::bUpPressed()
{
    hide();
    emit bothUp(currow);
}

void ItFloatControls::bDownPressed()
{
    hide();
    emit bothDown(currow);
}

void ItFloatControls::show(int autohide)
{
    if (autohide>0)
        timer.start(autohide);
    QWidget::show();
}
