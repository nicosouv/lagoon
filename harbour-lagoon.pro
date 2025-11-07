TARGET = harbour-lagoon

CONFIG += sailfishapp
QT += network websockets sql dbus
CONFIG += link_pkgconfig
PKGCONFIG += sailfishapp nemonotifications-qt5

# OAuth credentials - loaded from .qmake.conf
DEFINES += LAGOON_CLIENT_ID=\\\"$$LAGOON_CLIENT_ID_VALUE\\\"
DEFINES += LAGOON_CLIENT_SECRET=\\\"$$LAGOON_CLIENT_SECRET_VALUE\\\"

SOURCES += \
    src/main.cpp \
    src/slackapi.cpp \
    src/websocketclient.cpp \
    src/notificationmanager.cpp \
    src/workspacemanager.cpp \
    src/filemanager.cpp \
    src/dbusclient.cpp \
    src/oauthmanager.cpp \
    src/models/conversationmodel.cpp \
    src/models/messagemodel.cpp \
    src/models/usermodel.cpp \
    src/cache/cachemanager.cpp \
    src/settings/appsettings.cpp

HEADERS += \
    src/slackapi.h \
    src/websocketclient.h \
    src/notificationmanager.h \
    src/workspacemanager.h \
    src/filemanager.h \
    src/dbusclient.h \
    src/oauthmanager.h \
    src/models/conversationmodel.h \
    src/models/messagemodel.h \
    src/models/usermodel.h \
    src/cache/cachemanager.h \
    src/settings/appsettings.h

DISTFILES += \
    qml/harbour-lagoon.qml \
    qml/cover/CoverPage.qml \
    qml/pages/FirstPage.qml \
    qml/pages/ConversationPage.qml \
    qml/pages/SettingsPage.qml \
    qml/pages/LoginPage.qml \
    qml/pages/OAuthWebViewPage.qml \
    qml/components/MessageDelegate.qml \
    qml/components/ChannelDelegate.qml \
    qml/components/ReactionBubble.qml \
    qml/components/ImageAttachment.qml \
    qml/components/FileAttachment.qml \
    qml/dialogs/EmojiPicker.qml \
    qml/pages/ImageViewerPage.qml \
    qml/pages/WorkspaceSwitcher.qml \
    qml/js/storage.js \
    rpm/harbour-lagoon.spec \
    rpm/harbour-lagoon.yaml \
    rpm/harbour-lagoon.changes \
    translations/*.ts \
    harbour-lagoon.desktop

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

# Translations disabled for now - files don't exist yet
# CONFIG += sailfishapp_i18n
# TRANSLATIONS += \
#     translations/harbour-lagoon-en.ts \
#     translations/harbour-lagoon-fr.ts
