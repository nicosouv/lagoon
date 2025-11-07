#include "slackshipdaemon.h"
#include "dbusadaptor.h"
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    app.setOrganizationName("harbour-slackship");
    app.setApplicationName("harbour-slackship-daemon");
    app.setApplicationVersion("0.1.0");

    qDebug() << "Starting SlackShip Daemon...";

    // Create daemon instance
    SlackShipDaemon daemon;

    // Initialize daemon
    if (!daemon.initialize()) {
        qCritical() << "Failed to initialize daemon";
        return 1;
    }

    // Register D-Bus service
    QDBusConnection connection = QDBusConnection::sessionBus();

    if (!connection.registerService("org.harbour.slackship")) {
        qCritical() << "Failed to register D-Bus service:" << connection.lastError().message();
        return 2;
    }

    // Create and register D-Bus adaptor
    new DBusAdaptor(&daemon);

    if (!connection.registerObject("/org/harbour/slackship", &daemon)) {
        qCritical() << "Failed to register D-Bus object:" << connection.lastError().message();
        return 3;
    }

    qDebug() << "D-Bus service registered: org.harbour.slackship";
    qDebug() << "D-Bus object path: /org/harbour/slackship";

    // Start daemon
    daemon.start();

    qDebug() << "SlackShip Daemon started successfully";

    return app.exec();
}
