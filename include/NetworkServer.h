#ifndef NETWORKSERVER_H
#define NETWORKSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>

class NetworkServer : public QObject {
    Q_OBJECT

public:
    explicit NetworkServer(QObject *parent = nullptr);
    bool startServer(quint16 port);
    bool isListening() const { return server->isListening(); }

signals:
    void dataReceived(const QString &data);

public slots:
    void sendToClients(const QString &message);

private slots:
    void onNewConnection();
    void readClientData();

private:
    QTcpServer *server;
    QList<QTcpSocket*> clients;
};

#endif // NETWORKSERVER_H
