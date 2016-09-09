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

#ifndef IT_ALIGNMENT_VIEW_H
#define IT_ALIGNMENT_VIEW_H

#include <QtWidgets>
#include "ItCommon.h"
#include "ItAlignmentModel.h"
#include "ItAlignmentDelegate.h"
#include "ItCommands.h"
#include "ItPlainTextEdit.h"
#include "ItSegmentView.h"
#include "ItFloatControls.h"

#define BACKWARD_RESIZE_ROWS 0
#define FORWARD_RESIZE_ROWS 2
#define INVALID_ROW 2147483647


class ItWindow;

class ItAlignmentView : public QTableView
{
    Q_OBJECT

public:
    ItAlignmentView(QWidget * parent = 0 );
    ~ItAlignmentView();
    void createContextMenu();
    //void cacheAllSizeHints();
    //void updateSizeHints(int from, int to);
    void setModel(QAbstractItemModel * model);
    bool isEditing();
    void setSegView(ItSegmentView * cursegview);
    void setEditor(ItPlainTextEdit * cureditor);
    bool htmlView();
    void setHtmlView(bool set);
    void setFont( const QFont &f );
    bool realign(int fromPos, int toPos, QList<QStringList> alignedIDs [2]);
    void setHighlNon11(bool set);
    void setHighlMarked(bool set);
    bool highlNon11();
    bool highlMarked();
    int skipMargin;
    int controlsHideTimeOut;
    void setAutoSaveElement(AutoState value);
    AutoState getAutoSaveElement();
    void setShowControls(ShowControlsType mode);
    void setSizeControls(int size);
    ShowControlsType getShowControls();
    int getSizeControls();
    void setEditorKeys(EditorKeys keys);
    EditorKeys getEditorKeys() const;
    //void setRowHeight(int row, int height);
    QAbstractItemDelegate::EndEditHint nexthint;
    ItSegmentView * segview;
    ItPlainTextEdit * txteditor;
    bool insertingElement;
public slots:
    void resizeRowToContents ( int row );
    void moveUp(int row = INVALID_ROW, int doc = 0);
    void moveDown(int row = INVALID_ROW, int doc = 0);
    void moveBothUp(int row = INVALID_ROW);
    void moveBothDown(int row = INVALID_ROW);
    void moveText();
    void shift(int row = INVALID_ROW, int doc = 0);
    void pop(int row = INVALID_ROW, int doc = 0);
    void swapSegments(int row = INVALID_ROW, int doc = 0);
    void toggleStat();
    void toggleMark();
    void confirmAll();
    bool openEditor();
    void insertElement();
    void splitParent();
    void mergeParent();
    void edit(const QModelIndex &index);
    void closeEditor(QWidget* editor,QAbstractItemDelegate::EndEditHint hint);
    void updateRowSize();
    void focusIndex(QModelIndex idx);
    void optimizeSize(QStatusBar * statusBar);
    void resizeRows();
    void showContextMenu(const QPoint& pos);
    void keepMargin();
    //void updateRowHeight(QModelIndex index, int height);
signals:
    void editingStarted();
    void editingFinished();
    void cursorChanged();
    void segViewChanged(ItSegmentView * cursegview);
    void focusChanged();
protected:
    void keyPressEvent (QKeyEvent * event);
    void mousePressEvent (QMouseEvent * event);
    void mouseMoveEvent ( QMouseEvent * event );
    void wheelEvent(QWheelEvent * event);
    void leaveEvent(QEvent * event);
    void focusOutEvent( QFocusEvent * event );
    void focusInEvent( QFocusEvent * event );
    void currentChanged ( const QModelIndex & current, const QModelIndex & previous );
    //void resizeEvent ( QResizeEvent * event );
    bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *event);
    QIcon composeIcon(QString fname);
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);
    void handleCloseHint(QAbstractItemDelegate::EndEditHint hint, bool insertNext = false);
protected slots:
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void commitData (QWidget * editor);
    void commitData ( QWidget * editor, QAbstractItemDelegate::EndEditHint hint );
    void mayCloseEditor ( QWidget * editor, QAbstractItemDelegate::EndEditHint hint = QAbstractItemDelegate::NoHint );
    //void currentChanged (const QModelIndex & current, const QModelIndex & previous);
    void insertNextRequested();
private:
    ItWindow * window;
    ItAlignmentDelegate * delegate;
    bool editorOpen;
    ItAlignmentModel * itmodel;
    bool hNon11, hMarked;
    //int nextToResize;
    int resizerow, llastrow, lfirstrow;
    bool resizeNextTime;
    bool keepMarginNextTime;
    QTimer timer;
    ShowControlsType showControls;
    int sizeControls;
    AutoState autoSaveElement;
    ItFloatControls * floatControl;
    QMenu ctxMenu;
    EditorKeys edKeys;
    //int calcItemHeight(const QModelIndex &index);
};

#endif
