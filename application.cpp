#include "application.h"

#include <QFileOpenEvent>

Application::Application( int & argc, char **argv ) : QApplication(argc, argv)
{
    w = new MainWindow();
    w->show();
}

void Application::loadFile(const QString &fileName)
{
    w->loadLicense(fileName);
}

bool Application::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::FileOpen:
        loadFile(static_cast<QFileOpenEvent *>(event)->file());
        return true;
    default:
        return QApplication::event(event);
    }
}

Application::~Application()
{
    delete w;
}

