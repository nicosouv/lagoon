#ifndef MOCKSLACKSERVER_H
#define MOCKSLACKSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QUrlQuery>

// Minimal in-process HTTP server that answers Slack API calls with canned
// JSON per endpoint and records every request for assertions.
class MockSlackServer : public QObject
{
    Q_OBJECT

public:
    struct Request {
        QString method;
        QString endpoint;   // e.g. "auth.test"
        QUrlQuery query;
        QByteArray body;
    };

    explicit MockSlackServer(QObject *parent = nullptr);

    bool start();
    QString baseUrl() const;  // "http://127.0.0.1:<port>/api/"

    void setResponse(const QString &endpoint, const QJsonObject &response);
    QList<Request> requests() const { return m_requests; }
    int requestCount(const QString &endpoint) const;
    void clearRequests() { m_requests.clear(); }

private slots:
    void handleNewConnection();
    void handleReadyRead();

private:
    void processBuffer(QTcpSocket *socket);
    void respond(QTcpSocket *socket, const QByteArray &body);

    QTcpServer m_server;
    QHash<QString, QByteArray> m_responses;  // endpoint -> JSON body
    QList<Request> m_requests;
    QHash<QTcpSocket *, QByteArray> m_buffers;
};

#endif // MOCKSLACKSERVER_H
