#pragma once

#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QString>
#include <QByteArray>
#include <QUrl>
#include <functional>

namespace devdialogue::test {

class MockHttpServer : public QObject {
    Q_OBJECT
public:
    struct HttpRequest {
        QString method;
        QString path;
        QMap<QString, QString> headers;
        QByteArray body;
    };

    struct HttpResponse {
        int statusCode{200};
        QString statusText{"OK"};
        QByteArray body{"{}"};
        QString contentType{"application/json"};
        QMap<QString, QString> headers;
    };

    using Handler = std::function<HttpResponse(const HttpRequest&)>;

    explicit MockHttpServer(QObject* parent = nullptr) : QObject(parent) {
        connect(&m_server, &QTcpServer::newConnection, this, &MockHttpServer::onNewConnection);
    }

    ~MockHttpServer() override {
        stop();
    }

    bool start() {
        return m_server.listen(QHostAddress::LocalHost, 0);
    }

    void stop() {
        m_server.close();
    }

    quint16 port() const {
        return m_server.serverPort();
    }

    QString url() const {
        return QString("http://127.0.0.1:%1").arg(port());
    }

    void setHandler(Handler handler) {
        m_handler = std::move(handler);
    }

private slots:
    void onNewConnection() {
        while (m_server.hasPendingConnections()) {
            QTcpSocket* socket = m_server.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                processRequest(socket);
            });
        }
    }

private:
    void processRequest(QTcpSocket* socket) {
        QByteArray rawData = socket->readAll();
        if (rawData.isEmpty()) return;

        int headerEnd = rawData.indexOf("\r\n\r\n");
        if (headerEnd == -1) return;

        QByteArray headerPart = rawData.left(headerEnd);
        QByteArray body = rawData.mid(headerEnd + 4);

        QList<QByteArray> lines = headerPart.split('\n');
        if (lines.isEmpty()) return;

        QByteArray reqLine = lines[0].trimmed();
        QList<QByteArray> reqParts = reqLine.split(' ');
        if (reqParts.size() < 2) return;

        HttpRequest req;
        req.method = QString::fromUtf8(reqParts[0]);
        req.path = QString::fromUtf8(reqParts[1]);
        req.body = body;

        for (int i = 1; i < lines.size(); ++i) {
            QByteArray line = lines[i].trimmed();
            int colon = line.indexOf(':');
            if (colon != -1) {
                QString key = QString::fromUtf8(line.left(colon).trimmed()).toLower();
                QString val = QString::fromUtf8(line.mid(colon + 1).trimmed());
                req.headers[key] = val;
            }
        }

        HttpResponse resp{200, "OK", "{}", "application/json", {}};
        if (m_handler) {
            resp = m_handler(req);
        }

        QByteArray responseData;
        responseData.append(QString("HTTP/1.1 %1 %2\r\n").arg(resp.statusCode).arg(resp.statusText).toUtf8());
        responseData.append(QString("Content-Type: %1\r\n").arg(resp.contentType).toUtf8());
        responseData.append(QString("Content-Length: %1\r\n").arg(resp.body.size()).toUtf8());
        responseData.append("Connection: close\r\n");
        for (auto it = resp.headers.begin(); it != resp.headers.end(); ++it) {
            responseData.append(QString("%1: %2\r\n").arg(it.key()).arg(it.value()).toUtf8());
        }
        responseData.append("\r\n");
        responseData.append(resp.body);

        socket->write(responseData);
        socket->flush();
        socket->disconnectFromHost();
    }

    QTcpServer m_server;
    Handler m_handler;
};

} // namespace devdialogue::test
