#include "application.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("StreamPlay");
    QCoreApplication::setOrganizationDomain("streamplay-app.com");
    QCoreApplication::setApplicationName("StreamPlay");

    Application a(argc, argv);
    return a.exec();
}
