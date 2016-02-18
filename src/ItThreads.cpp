#include "ItThreads.h"
#include "ItWindow.h"

ReplaceAllThread::ReplaceAllThread(ItWindow * parent) : QThread(parent)
{
  window = parent;
}

void ReplaceAllThread::run()
{/*
#ifndef QT_NO_CURSOR
  QApplication::setOverrideCursor(Qt::BusyCursor);
#endif
  uint count = 0;
  //window->statusBar()->showMessage(tr("Replace all in progress..."),0);*/

/* // ItWindow: replaceAll at this level
  search_wrapper(true, true);
  if (model->canReplace()) {
    double prog = 0;
    uint max = model->rowCount();
    uint cur = 0;
    stopButtonPressed = false;
    model->undoStack->beginMacro("Replace all");
    while (model->canReplace() && !stopButtonPressed) {
      cur = model->lastMatchIndex.parent().row();
      prog = ((cur*1.0) / max)*100;
      statusBar()->showMessage(tr("Replace all in progress... %1% (segment: %2 of %3; occurrence: %4)").arg(QString::number(prog,'f',1),
                                                                                                            QString::number(cur),
                                                                                                            QString::number(max),
                                                                                                            QString::number(count)),0);
      model->replace(searchBar->getReplacementString()); //replace();
      count++;
      //search_wrapper(true, false, true); //findNext();
      //find(INVALID_POSITION, true, searchBar->getSearchType(), searchBar->getSearchSide(), searchBar->getSearchString(), true);
      model->find(INVALID_POSITION, true, searchBar->getSearchSide(), searchBar->getSearchType(), searchBar->getSearchString());
    }
    model->undoStack->endMacro();
    //delete stop;
  }
*/

  // call the model to replace all
/*  if ((window->searchBar->getSearchType()==ItSearchBar::RegExp || window->searchBar->getSearchType()==ItSearchBar::RegExpCS) && !QRegExp(window->searchBar->getSearchString()).isValid()) {
    QMessageBox::warning(window, tr("Search"), tr("Invalid regular expression."));
    return;
  }
  window->newSearchQuery(0);
  window->searchBar->addCurrentQuery();
  if (!window->model->replaceAll(window->statusBar(), &count, window->searchBar->getSearchSide(), window->searchBar->getSearchType(), window->searchBar->getSearchString(), window->searchBar->getReplacementString()))
    QMessageBox::information(window, tr("Search"),
                             tr("Not found."), QMessageBox::Ok);

#ifndef QT_NO_CURSOR
  QApplication::restoreOverrideCursor();
#endif
  //window->statusBar()->removeWidget(stop);
  //window->statusBar()->showMessage(tr("Ready."),500);
  if (count>0)
    QMessageBox::information(window, tr("Replace"),
                           tr("Replaced %1 occurrences.").arg(QString::number(count)), QMessageBox::Ok);*/
}

