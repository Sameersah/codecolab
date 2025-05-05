// MainWindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QString>
#include <memory>
#include <QElapsedTimer>    // Add for Qt 6
#include <QRandomGenerator> // Add for Qt 6

#include "User.h"
#include "Document.h"
#include "CollaborationManager.h"
#include "CollaborationClient.h"

class CodeEditorWidget;
class LoginDialog;
class QTextEdit;
class QSplitter;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Add document to the editor
    void addDocument(std::shared_ptr<Document> document);

    private slots:
        void onLogin();
    void onLogout();
    void onNewDocument();
    void onOpenDocument();
    void onSaveDocument();
    void onShareDocument();
    void onTextChanged();
    void onCursorPositionChanged();
    void onSendChatMessage();
    void onUserConnected(const QString& userId, const QString& username);
    void onUserDisconnected(const QString& userId);
    void onChatMessageReceived(const QString& userId, const QString& username, const QString& message);

private:
    void setupUI();
    void setupConnections();
    void updateTitle();
    void updateStatusBar();
    void updateUserList();
    void showLoginDialog();
    bool eventFilter(QObject *obj, QEvent *event) override;

    Ui::MainWindow *ui;
    std::unique_ptr<CodeEditorWidget> codeEditor;
    std::unique_ptr<QTextEdit> chatBox;
    std::unique_ptr<QTextEdit> chatInput;
    std::unique_ptr<QLabel> statusLabel;
    std::unique_ptr<QSplitter> mainSplitter;
    std::unique_ptr<QSplitter> rightSplitter;

    // Core objects
    std::shared_ptr<User> currentUser;
    std::shared_ptr<Document> currentDocument;
    std::shared_ptr<CollaborationManager> collaborationManager;
    std::unique_ptr<CollaborationClient> collaborationClient;

    // Track collaboration state
    QMap<QString, QString> connectedUsers; // userId -> username
    bool isCollaborating = false;
};

#endif // MAINWINDOW_H