#ifndef DRAFTMANAGER_H
#define DRAFTMANAGER_H

#include <QObject>
#include <QSettings>
#include <QString>

class DraftManager : public QObject
{
    Q_OBJECT

public:
    explicit DraftManager(QObject *parent = nullptr);

    Q_INVOKABLE QString getDraft(const QString &channelId) const;
    Q_INVOKABLE void saveDraft(const QString &channelId, const QString &text);
    Q_INVOKABLE void clearDraft(const QString &channelId);

private:
    QSettings m_settings;
};

#endif // DRAFTMANAGER_H
