- memory: Qt

Qt is a powerful cross-platform application library which is ideal for connected devices. In Sailfish OS systems it serves as the main application development environment and provides consistent APIs into most other commonly used device functions. Sailfish currently uses Qt version 5.6.

The following modules are used within Sailfish OS:

    QtCore
    QtWebkit
    QtDBus
    QtGui
    QtLocation
    QtPositioning
    QtMultimedia
    QtNetwork
    QtQuick
    QtQml
    QtSensors
    QtSql
    QtSvg
    QtXml
    QtXmlPatterns

There are many other Qt modules which are not used; most are obvious but these are worth noting:

    QtScript
    QtOpengl
    QtQuickWidgets
    qtWidgets
- memory a chaque nouveau string, il faut penser à le traduire via les fichiers de traductions
- memory a chaque nouveau tag git, il faut mettre à jour `Version:` dans rpm/harbour-lagoon.spec (source unique : le .pro la lit et la propage à UpdateChecker et aux pages Login/Settings via APP_VERSION)
- to memorize : tu dois faire des commits, tag, code et commanteaire de code an anglais