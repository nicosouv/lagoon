#include "draftmanager.h"

DraftManager::DraftManager(QObject *parent)
    : QObject(parent)
    , m_settings("harbour-lagoon", "drafts")
{
}

QString DraftManager::getDraft(const QString &channelId) const
{
    return m_settings.value(QString("draft_%1").arg(channelId), "").toString();
}

void DraftManager::saveDraft(const QString &channelId, const QString &text)
{
    if (text.trimmed().isEmpty()) {
        clearDraft(channelId);
    } else {
        m_settings.setValue(QString("draft_%1").arg(channelId), text);
    }
}

void DraftManager::clearDraft(const QString &channelId)
{
    m_settings.remove(QString("draft_%1").arg(channelId));
}
