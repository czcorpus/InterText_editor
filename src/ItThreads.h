#ifndef IT_THREADS_H
#define IT_THREADS_H

#include <QThread>

class ItWindow;

class ReplaceAllThread : public QThread
{
public:
    ReplaceAllThread(ItWindow * parent = 0);
    void run();
private:
    ItWindow * window;
};

#endif // IT_THREADS_H
