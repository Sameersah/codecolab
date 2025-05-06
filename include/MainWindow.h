#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QString>
#include <memory>
#include <QElapsedTimer>    // Qt 6
#include <QRandomGenerator> // Qt 6
#include <QMap>
#include <QJsonDocument>
#include <QTimer>
#include "ui_MainWindow.h"
#include "CodeEditorWidget.h"
#include "Document.h"
#include "User.h"
#include "CollaborationClient.h"
#include "CollaborationManager.h"
#include "NetworkServer.h"
#include "NetworkClient.h"

// Forward declarations
class LoginDialog;
class QTextEdit;
class QSplitter;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Add document to the editor
    void addDocument(std::shared_ptr<Document> document);

private slots:
    void showLoginDialog();
    void onLogin();
    void onLogout();
    void onNewDocument();
    void onOpenDocument();
    void onSaveDocument();
    void onShareDocument();
    void onUserConnected(const QString &userId, const QString &username);
    void onUserDisconnected(const QString &userId);
    void onTextChanged();
    void onChatMessageReceived(const QString &userId, const QString &username, const QString &message);
    void onSendChatMessage();
    void onRemoteTextChanged(const QString &message);

private:
    void setupUI();
    void setupConnections();
    void updateTitle();
    void updateStatusBar();
    void updateUserList();
    bool eventFilter(QObject *obj, QEvent *event) override;

    std::unique_ptr<Ui::MainWindow> ui;
    std::shared_ptr<CollaborationManager> collaborationManager;
    std::unique_ptr<CollaborationClient> collaborationClient;
    std::shared_ptr<Document> currentDocument;
    std::shared_ptr<User> currentUser;
    std::unique_ptr<CodeEditorWidget> codeEditor;
    std::unique_ptr<QSplitter> mainSplitter;
    std::unique_ptr<QSplitter> rightSplitter;
    std::unique_ptr<QTextEdit> chatBox;
    std::unique_ptr<QTextEdit> chatInput;
    std::unique_ptr<QLabel> statusLabel;
    QMap<QString, QString> connectedUsers;
    bool isCollaborating;
    NetworkServer *server;
    NetworkClient *client;
    QString lastSentText;
    QTimer *typingTimer;
};

#endif // MAINWINDOW_H
