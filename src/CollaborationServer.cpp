#include "CollaborationServer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

CollaborationServer::CollaborationServer(quint16 port, QObject *parent)
    : QObject(parent)
    , server(new QWebSocketServer(QStringLiteral("CodeColab Server"), QWebSocketServer::NonSecureMode, this))
{
    if (server->listen(QHostAddress::Any, port)) {
        qDebug() << "Server listening on port" << port;
        connect(server, &QWebSocketServer::newConnection, this, &CollaborationServer::onNewConnection);
    } else {
        qDebug() << "Server failed to start. Error:" << server->errorString();
    }
}

CollaborationServer::~CollaborationServer()
{
    stop();
}

bool CollaborationServer::start()
{
    return server->isListening();
}

void CollaborationServer::stop()
{
    // Close all client connections
    for (QWebSocket* client : clientUserIds.keys()) {
        client->close();
        client->deleteLater();
    }
    
    // Clear all maps
    clientUserIds.clear();
    documentClients.clear();
    userSessions.clear();
    
    // Close the server
    server->close();
}

void CollaborationServer::onNewConnection()
{
    QWebSocket *socket = server->nextPendingConnection();
    
    connect(socket, &QWebSocket::textMessageReceived, this, &CollaborationServer::onTextMessageReceived);
    connect(socket, &QWebSocket::disconnected, this, &CollaborationServer::onSocketDisconnected);
    
    qDebug() << "New client connected";
}

void CollaborationServer::onSocketDisconnected()
{
    QWebSocket *client = qobject_cast<QWebSocket *>(sender());
    if (!client) return;

    QString userId = clientUserIds.value(client);
    if (!userId.isEmpty()) {
        QString documentId = userSessions.value(userId);
        if (!documentId.isEmpty()) {
            documentClients[documentId].remove(client);
            if (documentClients[documentId].isEmpty()) {
                documentClients.remove(documentId);
            }
        }
        userSessions.remove(userId);
        clientUserIds.remove(client);
    }

    client->deleteLater();
    qDebug() << "Client disconnected";
}

void CollaborationServer::onTextMessageReceived(const QString &message)
{
    QWebSocket *client = qobject_cast<QWebSocket *>(sender());
    if (!client) return;

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "Invalid message format received";
        return;
    }

    QJsonObject jsonMsg = doc.object();
    QString type = jsonMsg["type"].toString();
    QJsonObject payload = jsonMsg["payload"].toObject();

    if (type == "join") {
        handleJoinMessage(client, payload);
    } else if (type == "leave") {
        handleLeaveMessage(client, payload);
    } else if (type == "edit") {
        handleEditMessage(client, payload);
    } else if (type == "cursor") {
        handleCursorMessage(client, payload);
    } else if (type == "chat") {
        handleChatMessage(client, payload);
    }
}

void CollaborationServer::handleJoinMessage(QWebSocket *client, const QJsonObject &payload)
{
    QString documentId = payload["documentId"].toString();
    QString userId = payload["userId"].toString();
    QString username = payload["username"].toString();

    // Store user information
    clientUserIds[client] = userId;
    userSessions[userId] = documentId;
    documentClients[documentId].insert(client);

    // Notify other clients in the document
    QJsonObject notification;
    notification["type"] = "user_joined";
    QJsonObject notificationPayload;
    notificationPayload["userId"] = userId;
    notificationPayload["username"] = username;
    notification["payload"] = notificationPayload;

    broadcastToDocument(documentId, QJsonDocument(notification).toJson(QJsonDocument::Compact), client);
}

void CollaborationServer::handleLeaveMessage(QWebSocket *client, const QJsonObject &payload)
{
    QString userId = clientUserIds.value(client);
    if (userId.isEmpty()) return;

    QString documentId = userSessions.value(userId);
    if (!documentId.isEmpty()) {
        documentClients[documentId].remove(client);
        if (documentClients[documentId].isEmpty()) {
            documentClients.remove(documentId);
        }

        // Notify other clients
        QJsonObject notification;
        notification["type"] = "user_left";
        QJsonObject notificationPayload;
        notificationPayload["userId"] = userId;
        notification["payload"] = notificationPayload;

        broadcastToDocument(documentId, QJsonDocument(notification).toJson(QJsonDocument::Compact));
    }

    userSessions.remove(userId);
    clientUserIds.remove(client);
}

void CollaborationServer::handleEditMessage(QWebSocket *client, const QJsonObject &payload)
{
    QString userId = clientUserIds.value(client);
    if (userId.isEmpty()) return;

    QString documentId = userSessions.value(userId);
    if (!documentId.isEmpty()) {
        QJsonObject message;
        message["type"] = "edit";
        QJsonObject messagePayload = payload;
        messagePayload["userId"] = userId;
        message["payload"] = messagePayload;

        broadcastToDocument(documentId, QJsonDocument(message).toJson(QJsonDocument::Compact), client);
    }
}

void CollaborationServer::handleCursorMessage(QWebSocket *client, const QJsonObject &payload)
{
    QString userId = clientUserIds.value(client);
    if (userId.isEmpty()) return;

    QString documentId = userSessions.value(userId);
    if (!documentId.isEmpty()) {
        QJsonObject message;
        message["type"] = "cursor";
        QJsonObject messagePayload = payload;
        messagePayload["userId"] = userId;
        message["payload"] = messagePayload;

        broadcastToDocument(documentId, QJsonDocument(message).toJson(QJsonDocument::Compact), client);
    }
}

void CollaborationServer::handleChatMessage(QWebSocket *client, const QJsonObject &payload)
{
    QString userId = clientUserIds.value(client);
    if (userId.isEmpty()) return;

    QString documentId = userSessions.value(userId);
    if (!documentId.isEmpty()) {
        QJsonObject message;
        message["type"] = "chat";
        QJsonObject messagePayload = payload;
        messagePayload["userId"] = userId;
        message["payload"] = messagePayload;

        broadcastToDocument(documentId, QJsonDocument(message).toJson(QJsonDocument::Compact));
    }
}

void CollaborationServer::broadcastToDocument(const QString &documentId, const QString &message, QWebSocket *exclude)
{
    if (!documentClients.contains(documentId)) return;

    for (QWebSocket *client : documentClients[documentId]) {
        if (client != exclude && client->isValid()) {
            client->sendTextMessage(message);
        }
    }
} 