#ifndef USERDB_H
#define USERDB_H

#include <QList>
#include <QString>

// SQLite-backed user cache, one row per (teamId, userId). Replaces the old
// QSettings "users-full" INI cache which wrote 9 keys + sync() per user on
// the UI thread. A whole workspace is saved in a single transaction.
class UserDb
{
public:
    struct CachedUser {
        QString id;
        QString name;
        QString realName;
        QString displayName;
        QString avatar;
        QString statusText;
        QString statusEmoji;
        bool isBot;
    };

    // databasePath is a test hook; the default lives in AppDataLocation
    explicit UserDb(const QString &databasePath = QString());
    ~UserDb();

    bool isValid() const { return m_valid; }

    // Replaces all cached users of the team in one transaction.
    // timestampMs defaults to now; migrations pass the legacy timestamp.
    bool saveUsers(const QString &teamId, const QList<CachedUser> &users,
                   qint64 timestampMs = 0);
    QList<CachedUser> loadUsers(const QString &teamId) const;
    qint64 cacheTimestamp(const QString &teamId) const;  // ms since epoch, 0 if none

private:
    QString m_connectionName;
    bool m_valid;
};

#endif // USERDB_H
