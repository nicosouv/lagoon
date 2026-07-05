#include "mockslackserver.h"

#include <QJsonDocument>
#include <QUrl>

MockSlackServer::MockSlackServer(QObject *parent)
    : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection,
            this, &MockSlackServer::handleNewConnection);
}

bool MockSlackServer::start()
{
    return m_server.listen(QHostAddress::LocalHost, 0);
}

QString MockSlackServer::baseUrl() const
{
    return QString("http://127.0.0.1:%1/api/").arg(m_server.serverPort());
}

void MockSlackServer::setResponse(const QString &endpoint, const QJsonObject &response)
{
    m_responses[endpoint] = QJsonDocument(response).toJson(QJsonDocument::Compact);
}

int MockSlackServer::requestCount(const QString &endpoint) const
{
    int count = 0;
    for (const Request &request : m_requests) {
        if (request.endpoint == endpoint) {
            ++count;
        }
    }
    return count;
}

void MockSlackServer::handleNewConnection()
{
    while (m_server.hasPendingConnections()) {
        QTcpSocket *socket = m_server.nextPendingConnection();
        m_buffers.insert(socket, QByteArray());
        connect(socket, &QTcpSocket::readyRead,
                this, &MockSlackServer::handleReadyRead);
        connect(socket, &QTcpSocket::disconnected,
                socket, &QTcpSocket::deleteLater);
        connect(socket, &QObject::destroyed, this, [this, socket]() {
            m_buffers.remove(socket);
        });
    }
}

void MockSlackServer::handleReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket) {
        return;
    }
    m_buffers[socket] += socket->readAll();
    processBuffer(socket);
}

void MockSlackServer::processBuffer(QTcpSocket *socket)
{
    const QByteArray &buffer = m_buffers[socket];

    int headerEnd = buffer.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        return;  // headers not complete yet
    }

    QByteArray headers = buffer.left(headerEnd);
    QList<QByteArray> lines = headers.split('\n');
    if (lines.isEmpty()) {
        return;
    }

    // Request line: "GET /api/endpoint?query HTTP/1.1"
    QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
    if (requestLine.size() < 2) {
        return;
    }

    // Wait for the full body on POST
    int contentLength = 0;
    for (const QByteArray &line : lines) {
        if (line.toLower().startsWith("content-length:")) {
            contentLength = line.mid(line.indexOf(':') + 1).trimmed().toInt();
        }
    }
    QByteArray body = buffer.mid(headerEnd + 4);
    if (body.size() < contentLength) {
        return;  // body not complete yet
    }

    QUrl url = QUrl::fromEncoded(requestLine.at(1));

    Request request;
    request.method = QString::fromLatin1(requestLine.at(0));
    request.endpoint = url.path();
    request.endpoint.remove("/api/");
    request.query = QUrlQuery(url);
    request.body = body.left(contentLength);
    m_requests.append(request);

    QByteArray responseBody = m_responses.value(
        request.endpoint,
        QByteArray("{\"ok\":false,\"error\":\"unknown_method\"}"));
    respond(socket, responseBody);
}

void MockSlackServer::respond(QTcpSocket *socket, const QByteArray &body)
{
    QByteArray response;
    response += "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: application/json; charset=utf-8\r\n";
    response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += body;

    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();
}
