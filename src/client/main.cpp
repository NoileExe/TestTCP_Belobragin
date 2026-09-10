
#include <QCoreApplication>
#include "network_client.h"

// ====================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    NetworkClient client;
    client.Start();

    return QCoreApplication::exec();
}
