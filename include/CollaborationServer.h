#ifndef COLLABORATIONSERVER_H
#define COLLABORATIONSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QMap>
#include <QSet>
#include <memory>
#include "Document.h"
#include "User.h"

class CollaborationServer : public QObject
{
    Q_OBJECT

public:
    explicit CollaborationServer(quint16 port, QObject *parent = nullptr);
    ~CollaborationServer();

    bool start();
    void stop();

private slots:
    void onNewConnection();
    void onSocketDisconnected();
    void onTextMessageReceived(const QString &message);

private:
    void handleJoinMessage(QWebSocket *client, const QJsonObject &payload);
    void handleLeaveMessage(QWebSocket *client, const QJsonObject &payload);
    void handleEditMessage(QWebSocket *client, const QJsonObject &payload);
    void handleCursorMessage(QWebSocket *client, const QJsonObject &payload);
    void handleChatMessage(QWebSocket *client, const QJsonObject &payload);
    void handleContentRequest(QWebSocket *client, const QJsonObject &payload);
    void broadcastToDocument(const QString &documentId, const QString &message, QWebSocket *exclude = nullptr);

    QWebSocketServer *server;
    QMap<QWebSocket*, QString> clientUserIds;  // Maps WebSocket clients to user IDs
    QMap<QString, QSet<QWebSocket*>> documentClients;  // Maps document IDs to connected clients
    QMap<QString, QString> userSessions;  // Maps user IDs to their current document ID
};

#endif // COLLABORATIONSERVER_H 