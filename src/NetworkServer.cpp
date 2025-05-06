#include "NetworkServer.h"
#include <QDebug>

NetworkServer::NetworkServer(QObject *parent) : QObject(parent) {
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &NetworkServer::onNewConnection);
}

bool NetworkServer::startServer(quint16 port) {
    if (!server->listen(QHostAddress::Any, port)) {
        qDebug() << "Server could not start!";
        return false;
    } else {
        qDebug() << "Server started on port:" << port;
        return true;
    }
}

void NetworkServer::onNewConnection() {
    QTcpSocket *client = server->nextPendingConnection();
    clients.append(client);
    connect(client, &QTcpSocket::readyRead, this, &NetworkServer::readClientData);
}

void NetworkServer::readClientData() {
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;
    QString data = client->readAll();
    emit dataReceived(data);
    sendToClients(data);
}

void NetworkServer::sendToClients(const QString &message) {
    for (QTcpSocket *client : std::as_const(clients)) {
        client->write(message.toUtf8());
    }
}
