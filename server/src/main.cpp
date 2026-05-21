#include <QCoreApplication>

#include "gobang_server.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("GobangServer");

    GobangServer server;
    if (!server.start(7777)) {
        return 1;
    }

    return app.exec();
}
