#include "NetworkClient.h"

NetworkClient::NetworkClient(QObject *parent) : QObject(parent) {
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::readyRead, this, &NetworkClient::readData);
}

void NetworkClient::connectToHost(const QString &host, quint16 port) {
    socket->connectToHost(host, port);
}

void NetworkClient::sendMessage(const QString &message) {
    socket->write(message.toUtf8());
}

void NetworkClient::readData() {
    QString message = socket->readAll();
    emit messageReceived(message);
}



