#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QTcpSocket>

class NetworkClient : public QObject {
    Q_OBJECT

public:
    explicit NetworkClient(QObject *parent = nullptr);
    void connectToHost(const QString &host, quint16 port);
    void sendMessage(const QString &message);

signals:
    void messageReceived(const QString &message);

private slots:
    void readData();

private:
    QTcpSocket *socket;
};

#endif // NETWORKCLIENT_H
