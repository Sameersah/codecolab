// CollaborationClient.cpp
#include "CollaborationClient.h"
#include "User.h"
#include "Document.h"
#include "EditOperation.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QTimer>
#include <QDateTime>

CollaborationClient::CollaborationClient(QObject *parent)
    : QObject(parent)
    , isDocumentJoined(false)
{
    // Connect WebSocket signals
    QObject::connect(&webSocket, &QWebSocket::connected, this, &CollaborationClient::onConnected);
    QObject::connect(&webSocket, &QWebSocket::disconnected, this, &CollaborationClient::onDisconnected);
    QObject::connect(&webSocket, &QWebSocket::textMessageReceived, this, &CollaborationClient::onTextMessageReceived);
    QObject::connect(&webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
                     this, &CollaborationClient::onError);
}

CollaborationClient::~CollaborationClient()
{
    if (webSocket.isValid()) {
        webSocket.close();
    }
}

bool CollaborationClient::connect(const QString& url)
{
    serverUrl = url;
    
    // For the prototype, simulate successful connection
    QTimer::singleShot(500, this, &CollaborationClient::onConnected);
    
    return true;
    
    // In a real implementation, we would connect to the WebSocket server
    // webSocket.open(QUrl(serverUrl));
    // return true;
}

void CollaborationClient::disconnect()
{
    // For the prototype, simulate disconnection
    QTimer::singleShot(300, this, &CollaborationClient::onDisconnected);
    
    // In a real implementation, we would close the WebSocket connection
    // if (webSocket.isValid()) {
    //     webSocket.close();
    // }
}

bool CollaborationClient::isConnected() const
{
    // For the prototype, always return true
    return true;
    
    // In a real implementation, we would check the WebSocket state
    // return webSocket.state() == QAbstractSocket::ConnectedState;
}

void CollaborationClient::setUser(std::shared_ptr<User> user)
{
    currentUser = user;
}

void CollaborationClient::setDocument(std::shared_ptr<Document> document)
{
    currentDocument = document;
}

void CollaborationClient::joinDocument(const QString& documentId)
{
    if (!currentUser) {
        emit error("No user is set");
        return;
    }

    // Construct a join message
    QJsonObject payload;
    payload["documentId"] = documentId;
    payload["userId"] = currentUser->getUserId();
    payload["username"] = currentUser->getUsername();

    sendMessage("join", payload);

    // Simulate document join
    QTimer::singleShot(500, [this, documentId]() {
        isDocumentJoined = true;
        emit documentJoined(documentId);

        // Simulate other users already in the document
        QStringList userIds = {"user1", "user2", "user3"};
        QStringList usernames = {"Alice", "Bob", "Charlie"};

        // Remove self
        for (int i = 0; i < userIds.size(); ++i) {
            if (currentUser && userIds[i] == currentUser->getUserId()) {
                userIds.removeAt(i);
                usernames.removeAt(i);
                --i;
            }
        }

        int numUsers = 1 + QRandomGenerator::global()->bounded(2);
        for (int i = 0; i < numUsers && i < userIds.size(); ++i) {
            connectedUsers[userIds[i]] = usernames[i];
            emit userConnected(userIds[i], usernames[i]);

            int randomPos = QRandomGenerator::global()->bounded(100);
            emit cursorPositionReceived(userIds[i], usernames[i], randomPos);
        }
    }); // ✅ This closes the outer QTimer lambda
}


void CollaborationClient::leaveDocument()
{
    if (!isDocumentJoined) {
        return;
    }
    
    // Construct a leave message
    QJsonObject payload;
    if (currentDocument) {
        payload["documentId"] = currentDocument->getId();
    }
    if (currentUser) {
        payload["userId"] = currentUser->getUserId();
    }
    
    // Send the message
    sendMessage("leave", payload);
    
    // For the prototype, simulate successful leave
    QTimer::singleShot(300, [this]() {
        isDocumentJoined = false;
        emit documentLeft();
    });
}

void CollaborationClient::sendEdit(const EditOperation& operation)
{
    if (!isDocumentJoined) {
        emit error("Not joined to a document");
        return;
    }
    
    // Construct an edit message
    QJsonObject payload = operation.toJson();
    
    // Send the message
    sendMessage("edit", payload);
}

void CollaborationClient::sendCursorPosition(int position)
{
    if (!isDocumentJoined || !currentUser || !currentDocument) {
        return;
    }
    
    // Construct a cursor message
    QJsonObject payload;
    payload["documentId"] = currentDocument->getId();
    payload["userId"] = currentUser->getUserId();
    payload["username"] = currentUser->getUsername();
    payload["position"] = position;
    
    // Send the message
    sendMessage("cursor", payload);
}

void CollaborationClient::sendChatMessage(const QString& message)
{
    if (!isDocumentJoined || !currentUser || !currentDocument) {
        return;
    }
    
    // Construct a chat message
    QJsonObject payload;
    payload["documentId"] = currentDocument->getId();
    payload["userId"] = currentUser->getUserId();
    payload["username"] = currentUser->getUsername();
    payload["message"] = message;
    
    // Send the message
    sendMessage("chat", payload);
}

void CollaborationClient::onConnected()
{
    emit connected();
    
    // For the prototype, simulate some users being available
    connectedUsers.clear();
}

void CollaborationClient::onDisconnected()
{
    isDocumentJoined = false;
    connectedUsers.clear();
    emit disconnected();
}

void CollaborationClient::onTextMessageReceived(const QString& message)
{
    // Parse the message
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        emit error("Invalid message format");
        return;
    }
    
    QJsonObject jsonMsg = doc.object();
    handleMessage(jsonMsg);
}

void CollaborationClient::onError(QAbstractSocket::SocketError error)
{
    QString errorMessage = "WebSocket error: " + webSocket.errorString();
    emit this->error(errorMessage);
}

void CollaborationClient::sendMessage(const QString& type, const QJsonObject& payload)
{
    // Construct the message
    QJsonObject message;
    message["type"] = type;
    message["payload"] = payload;
    
    // Convert to JSON string
    QJsonDocument doc(message);
    QString jsonString = doc.toJson(QJsonDocument::Compact);
    
    // For the prototype, we'll just log the message
    qDebug() << "Sending message:" << jsonString;
    
    // In a real implementation, we would send the message over WebSocket
    // if (webSocket.isValid()) {
    //     webSocket.sendTextMessage(jsonString);
    // } else {
    //     emit error("WebSocket not connected");
    // }
}

void CollaborationClient::handleMessage(const QJsonObject& message)
{
    // Extract message type and payload
    QString type = message["type"].toString();
    QJsonObject payload = message["payload"].toObject();
    
    if (type == "join_response") {
        // Handle join response
        bool success = payload["success"].toBool();
        if (success) {
            QString documentId = payload["documentId"].toString();
            isDocumentJoined = true;
            emit documentJoined(documentId);
        } else {
            QString errorMsg = payload["error"].toString();
            emit error("Failed to join document: " + errorMsg);
        }
    } else if (type == "user_joined") {
        // Handle user joined notification
        QString userId = payload["userId"].toString();
        QString username = payload["username"].toString();
        
        connectedUsers[userId] = username;
        emit userConnected(userId, username);
    } else if (type == "user_left") {
        // Handle user left notification
        QString userId = payload["userId"].toString();
        
        if (connectedUsers.contains(userId)) {
            connectedUsers.remove(userId);
            emit userDisconnected(userId);
        }
    } else if (type == "edit") {
        // Handle edit operation
        EditOperation op = EditOperation::fromJson(payload);
        emit editReceived(op);
    } else if (type == "cursor") {
        // Handle cursor position update
        QString userId = payload["userId"].toString();
        QString username = payload["username"].toString();
        int position = payload["position"].toInt();
        
        emit cursorPositionReceived(userId, username, position);
    } else if (type == "chat") {
        // Handle chat message
        QString userId = payload["userId"].toString();
        QString username = payload["username"].toString();
        QString message = payload["message"].toString();
        
        emit chatMessageReceived(userId, username, message);
    } else {
        // Unknown message type
        emit error("Unknown message type: " + type);
    }
}