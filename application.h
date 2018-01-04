#ifndef APPLICATION_H
#define APPLICATION_H

#include "mainwindow.h"
#include <QApplication>

class Application : public QApplication
{
Q_OBJECT
public:
    Application(int & argc, char **argv);
    virtual ~Application();
protected:
    bool event(QEvent *);
private:
    void loadFile(const QString &fileName);

public:
    MainWindow *w;

};

#endif // APPLICATION_H
