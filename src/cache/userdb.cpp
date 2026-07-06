#include "userdb.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

UserDb::UserDb(const QString &databasePath)
    : m_valid(false)
{
    // Unique connection name per instance (QSqlDatabase connections are global)
    m_connectionName = QString("userdb-%1").arg(reinterpret_cast<quintptr>(this));

    QString path = databasePath;
    if (path.isEmpty()) {
        QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);
        path = dir + QStringLiteral("/users.db");
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(path);
    if (!db.open()) {
        qWarning() << "[UserDb] Cannot open" << path << ":" << db.lastError().text();
        return;
    }

    QSqlQuery query(db);
    m_valid = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS users ("
        " team_id TEXT NOT NULL,"
        " user_id TEXT NOT NULL,"
        " name TEXT, real_name TEXT, display_name TEXT, avatar TEXT,"
        " status_text TEXT, status_emoji TEXT, is_bot INTEGER,"
        " PRIMARY KEY (team_id, user_id))"));
    m_valid = m_valid && query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS cache_meta ("
        " team_id TEXT PRIMARY KEY, updated_at INTEGER)"));
    if (!m_valid) {
        qWarning() << "[UserDb] Cannot create tables:" << query.lastError().text();
    }
}

UserDb::~UserDb()
{
    QSqlDatabase::database(m_connectionName).close();
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool UserDb::saveUsers(const QString &teamId, const QList<CachedUser> &users,
                       qint64 timestampMs)
{
    if (!m_valid || teamId.isEmpty()) {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction()) {
        qWarning() << "[UserDb] Cannot start transaction:" << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral("DELETE FROM users WHERE team_id = ?"));
    query.addBindValue(teamId);
    query.exec();

    query.prepare(QStringLiteral(
        "INSERT INTO users (team_id, user_id, name, real_name, display_name,"
        " avatar, status_text, status_emoji, is_bot)"
        " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    for (const CachedUser &user : users) {
        query.addBindValue(teamId);
        query.addBindValue(user.id);
        query.addBindValue(user.name);
        query.addBindValue(user.realName);
        query.addBindValue(user.displayName);
        query.addBindValue(user.avatar);
        query.addBindValue(user.statusText);
        query.addBindValue(user.statusEmoji);
        query.addBindValue(user.isBot ? 1 : 0);
        if (!query.exec()) {
            qWarning() << "[UserDb] Insert failed:" << query.lastError().text();
            db.rollback();
            return false;
        }
    }

    query.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO cache_meta (team_id, updated_at) VALUES (?, ?)"));
    query.addBindValue(teamId);
    query.addBindValue(timestampMs > 0 ? timestampMs : QDateTime::currentMSecsSinceEpoch());
    query.exec();

    return db.commit();
}

QList<UserDb::CachedUser> UserDb::loadUsers(const QString &teamId) const
{
    QList<CachedUser> users;
    if (!m_valid || teamId.isEmpty()) {
        return users;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral(
        "SELECT user_id, name, real_name, display_name, avatar,"
        " status_text, status_emoji, is_bot FROM users WHERE team_id = ?"));
    query.addBindValue(teamId);
    if (!query.exec()) {
        qWarning() << "[UserDb] Load failed:" << query.lastError().text();
        return users;
    }

    while (query.next()) {
        CachedUser user;
        user.id = query.value(0).toString();
        user.name = query.value(1).toString();
        user.realName = query.value(2).toString();
        user.displayName = query.value(3).toString();
        user.avatar = query.value(4).toString();
        user.statusText = query.value(5).toString();
        user.statusEmoji = query.value(6).toString();
        user.isBot = query.value(7).toInt() != 0;
        users.append(user);
    }
    return users;
}

qint64 UserDb::cacheTimestamp(const QString &teamId) const
{
    if (!m_valid || teamId.isEmpty()) {
        return 0;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare(QStringLiteral("SELECT updated_at FROM cache_meta WHERE team_id = ?"));
    query.addBindValue(teamId);
    if (query.exec() && query.next()) {
        return query.value(0).toLongLong();
    }
    return 0;
}
