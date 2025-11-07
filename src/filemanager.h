#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>

class FileManager : public QObject
{
    Q_OBJECT

public:
    explicit FileManager(QObject *parent = nullptr);
    ~FileManager();

    Q_INVOKABLE void setToken(const QString &token);

public slots:
    // Upload files
    void uploadFile(const QString &channelId,
                   const QString &filePath,
                   const QString &comment = QString());

    void uploadImage(const QString &channelId,
                    const QString &imagePath,
                    const QString &comment = QString());

    // Download files
    void downloadFile(const QString &fileId,
                     const QString &privateUrl,
                     const QString &savePath);

    void downloadImage(const QString &imageUrl,
                      const QString &savePath);

    // Cancel operations
    void cancelUpload();
    void cancelDownload(const QString &fileId);

signals:
    // Upload signals
    void uploadStarted(const QString &fileName);
    void uploadProgress(const QString &fileName, qint64 bytesSent, qint64 bytesTotal);
    void uploadFinished(const QString &fileId, const QJsonObject &fileInfo);
    void uploadError(const QString &error);

    // Download signals
    void downloadStarted(const QString &fileName);
    void downloadProgress(const QString &fileName, qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(const QString &filePath);
    void downloadError(const QString &error);

private slots:
    void handleUploadFinished();
    void handleUploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void handleUploadError(QNetworkReply::NetworkError error);

    void handleDownloadFinished();
    void handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleDownloadError(QNetworkReply::NetworkError error);

private:
    QNetworkAccessManager *m_networkManager;
    QString m_token;

    QNetworkReply *m_currentUpload;
    QNetworkReply *m_currentDownload;
    QString m_currentFileName;

    QByteArray createMultipartData(const QString &filePath,
                                   const QString &channelId,
                                   const QString &comment,
                                   QByteArray &boundary);
};

#endif // FILEMANAGER_H
