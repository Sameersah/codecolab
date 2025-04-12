#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "CodeEditorWidget.h"
#include "LoginDialog.h"
#include "CollaborationClient.h"

#include <QSplitter>
#include <QTextEdit>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QTimer>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , collaborationManager(std::make_shared<CollaborationManager>())
{
    ui->setupUi(this);
    setupUI();
    setupConnections();

    // Show login dialog on startup
    QTimer::singleShot(500, this, &MainWindow::showLoginDialog);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupConnections()
{
    // Connect code editor signals
    connect(codeEditor.get(), &CodeEditorWidget::editorContentChanged,
            this, &MainWindow::onTextChanged);
    connect(codeEditor.get(), &CodeEditorWidget::cursorPositionChanged,
            this, &MainWindow::onCursorPositionChanged);

    // Connect chat input
    connect(chatInput.get(), &QTextEdit::textChanged, [this]() {
        // Enable/disable send button based on whether there's text
        QPushButton* sendButton = qobject_cast<QPushButton*>(
            chatInput->parentWidget()->layout()->itemAt(
                chatInput->parentWidget()->layout()->count() - 1)->widget());
        if (sendButton) {
            sendButton->setEnabled(!chatInput->toPlainText().trimmed().isEmpty());
        }
    });

    // Install event filter for chat input to handle Enter key
    chatInput->installEventFilter(this);
}

void MainWindow::setupUI()
{
    // Create central widget and layout
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    setCentralWidget(centralWidget);

    // Create main splitter (editor | right panel)
    mainSplitter = std::make_unique<QSplitter>(Qt::Horizontal);
    mainLayout->addWidget(mainSplitter.get());

    // Create code editor
    codeEditor = std::make_unique<CodeEditorWidget>();
    codeEditor->setCollaborationManager(collaborationManager);
    mainSplitter->addWidget(codeEditor.get());

    // Create right panel splitter (user list | chat)
    rightSplitter = std::make_unique<QSplitter>(Qt::Vertical);
    mainSplitter->addWidget(rightSplitter.get());

    // Create user list panel
    QWidget* userListPanel = new QWidget(rightSplitter.get());
    QVBoxLayout* userListLayout = new QVBoxLayout(userListPanel);
    userListLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* collaboratorsLabel = new QLabel("Collaborators:");
    userListLayout->addWidget(collaboratorsLabel);

    QListWidget* userList = new QListWidget();
    userListLayout->addWidget(userList);

    rightSplitter->addWidget(userListPanel);

    // Create chat panel
    QWidget* chatPanel = new QWidget(rightSplitter.get());
    QVBoxLayout* chatLayout = new QVBoxLayout(chatPanel);
    chatLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* chatLabel = new QLabel("Chat:");
    chatLayout->addWidget(chatLabel);

    chatBox = std::make_unique<QTextEdit>();
    chatBox->setReadOnly(true);
    chatLayout->addWidget(chatBox.get());

    chatInput = std::make_unique<QTextEdit>();
    chatInput->setMaximumHeight(60);
    chatLayout->addWidget(chatInput.get());

    QPushButton* sendButton = new QPushButton("Send");
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendChatMessage);
    chatLayout->addWidget(sendButton);

    rightSplitter->addWidget(chatPanel);

    // Set up splitter proportions
    mainSplitter->setSizes({600, 200});
    rightSplitter->setSizes({200, 400});

    // Set up status bar
    statusLabel = std::make_unique<QLabel>("Not logged in");
    statusBar()->addWidget(statusLabel.get());

    // Setup menus with newer Qt 6 syntax
    QMenu* fileMenu = menuBar()->addMenu("&File");
    QAction* newAction = new QAction("New", this);
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::onNewDocument);
    fileMenu->addAction(newAction);

    QAction* openAction = new QAction("Open", this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenDocument);
    fileMenu->addAction(openAction);

    QAction* saveAction = new QAction("Save", this);
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveDocument);
    fileMenu->addAction(saveAction);

    fileMenu->addSeparator();

    QAction* exitAction = new QAction("Exit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);

    QMenu* editMenu = menuBar()->addMenu("&Edit");
    QAction* cutAction = new QAction("Cut", this);
    cutAction->setShortcut(QKeySequence::Cut);
    connect(cutAction, &QAction::triggered, codeEditor.get(), &QPlainTextEdit::cut);
    editMenu->addAction(cutAction);

    QAction* copyAction = new QAction("Copy", this);
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, codeEditor.get(), &QPlainTextEdit::copy);
    editMenu->addAction(copyAction);

    QAction* pasteAction = new QAction("Paste", this);
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, codeEditor.get(), &QPlainTextEdit::paste);
    editMenu->addAction(pasteAction);

    QMenu* viewMenu = menuBar()->addMenu("&View");
    QAction* zoomInAction = new QAction("Zoom In", this);
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, codeEditor.get(), &QPlainTextEdit::zoomIn);
    viewMenu->addAction(zoomInAction);

    QAction* zoomOutAction = new QAction("Zoom Out", this);
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, codeEditor.get(), &QPlainTextEdit::zoomOut);
    viewMenu->addAction(zoomOutAction);

    QMenu* collaborationMenu = menuBar()->addMenu("&Collaboration");
    QAction* shareAction = new QAction("Share Document", this);
    connect(shareAction, &QAction::triggered, this, &MainWindow::onShareDocument);
    collaborationMenu->addAction(shareAction);

    QMenu* userMenu = menuBar()->addMenu("&User");
    QAction* loginAction = new QAction("Login", this);
    connect(loginAction, &QAction::triggered, this, &MainWindow::onLogin);
    userMenu->addAction(loginAction);

    QAction* logoutAction = new QAction("Logout", this);
    connect(logoutAction, &QAction::triggered, this, &MainWindow::onLogout);
    userMenu->addAction(logoutAction);

    // Set window properties
    resize(1024, 768);
    setWindowTitle("CodeColab - Collaborative Code Editor");
}

void MainWindow::showLoginDialog()
{
    LoginDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        currentUser = dialog.getUser();
        if (currentUser) {
            updateStatusBar();
            updateTitle();
            onNewDocument(); // Create a new document by default
        } else {
            QMessageBox::critical(this, "Login Failed", "Could not authenticate user.");
        }
    } else {
        // User cancelled login, close application if not already logged in
        if (!currentUser) {
            close();
        }
    }
}

void MainWindow::updateTitle()
{
    QString title = "CodeColab";
    if (currentDocument) {
        title += " - " + currentDocument->getTitle();
    }
    if (currentUser) {
        title += " - " + currentUser->getUsername();
    }
    setWindowTitle(title);
}

void MainWindow::updateStatusBar()
{
    if (currentUser) {
        statusLabel->setText("Logged in as: " + currentUser->getUsername());
    } else {
        statusLabel->setText("Not logged in");
    }
}

void MainWindow::updateUserList()
{
    QListWidget* userList = findChild<QListWidget*>();
    if (!userList) return;

    userList->clear();

    if (currentUser) {
        QListWidgetItem* ownerItem = new QListWidgetItem(currentUser->getUsername() + " (You)");
        ownerItem->setForeground(currentUser->getCursorColor());
        userList->addItem(ownerItem);
    }

    for (const auto& [userId, username] : connectedUsers.toStdMap()) {
        QListWidgetItem* userItem = new QListWidgetItem(username);
        userList->addItem(userItem);
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == chatInput.get() && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            onSendChatMessage();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// Slot implementations
void MainWindow::onLogin()
{
    // If already logged in, ask if user wants to logout first
    if (currentUser) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Already Logged In",
                                     "You are already logged in. Do you want to logout first?",
                                     QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            onLogout();
        } else {
            return;
        }
    }

    showLoginDialog();
}

void MainWindow::onLogout()
{
    if (!currentUser) return;

    // Check if we need to save the document
    if (currentDocument) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Unsaved Changes",
                                     "Do you want to save changes before logging out?",
                                     QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
        if (reply == QMessageBox::Cancel) {
            return;
        } else if (reply == QMessageBox::Yes) {
            onSaveDocument();
        }
    }

    // Clear current state
    currentUser = nullptr;
    currentDocument = nullptr;
    connectedUsers.clear();

    // Clear UI
    codeEditor->setDocument(nullptr);
    codeEditor->setPlainText("");
    chatBox->clear();
    chatInput->clear();

    // Update UI
    updateStatusBar();
    updateTitle();
    updateUserList();

    // Show login dialog
    QTimer::singleShot(100, this, &MainWindow::showLoginDialog);
}

void MainWindow::onNewDocument()
{
    if (!currentUser) {
        QMessageBox::warning(this, "Not Logged In", "You must be logged in to create a document.");
        return;
    }

    bool ok;
    QString title = QInputDialog::getText(this, "New Document",
                                         "Document title:", QLineEdit::Normal,
                                         "Untitled.cpp", &ok);
    if (ok && !title.isEmpty()) {
        // Create a new document
        currentDocument = std::make_shared<Document>(
            "doc_" + QString::number(QDateTime::currentSecsSinceEpoch()),
            title,
            currentUser);

        // Set initial content
        QString initialContent = "// " + title + "\n// Created by " + currentUser->getUsername() + "\n\n";
        if (title.endsWith(".cpp") || title.endsWith(".h")) {
            initialContent += "#include <iostream>\n\nint main() {\n    std::cout << \"Hello, World!\" << std::endl;\n    return 0;\n}\n";
            currentDocument->setLanguage("C++");
        } else if (title.endsWith(".py")) {
            initialContent += "def main():\n    print(\"Hello, World!\")\n\nif __name__ == \"__main__\":\n    main()\n";
            currentDocument->setLanguage("Python");
        } else if (title.endsWith(".js")) {
            initialContent += "console.log(\"Hello, World!\");\n";
            currentDocument->setLanguage("JavaScript");
        } else if (title.endsWith(".java")) {
            initialContent += "public class " + title.left(title.lastIndexOf('.')) + " {\n    public static void main(String[] args) {\n        System.out.println(\"Hello, World!\");\n    }\n}\n";
            currentDocument->setLanguage("Java");
        }

        currentDocument->setContent(initialContent);

        // Set up editor
        codeEditor->setDocument(currentDocument);
        codeEditor->setPlainText(initialContent);
        codeEditor->setLanguage(currentDocument->getLanguage());

        // Update UI
        updateTitle();
        updateUserList();

        // Clear chat
        chatBox->clear();
        chatInput->clear();

        QMessageBox::information(this, "Document Created", "New document '" + title + "' has been created.");
    }
}

void MainWindow::onOpenDocument()
{
    if (!currentUser) {
        QMessageBox::warning(this, "Not Logged In", "You must be logged in to open a document.");
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(this, "Open Document", "",
                                                  "C++ Files (*.cpp *.h);;Python Files (*.py);;JavaScript Files (*.js);;Java Files (*.java);;All Files (*)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();

            // Create document
            QFileInfo fileInfo(fileName);
            currentDocument = std::make_shared<Document>(
                "doc_" + QString::number(QDateTime::currentSecsSinceEpoch()),
                fileInfo.fileName(),
                currentUser);

            // Set content
            currentDocument->setContent(content);

            // Determine language based on file extension
            QString extension = fileInfo.suffix().toLower();
            if (extension == "cpp" || extension == "h") {
                currentDocument->setLanguage("C++");
            } else if (extension == "py") {
                currentDocument->setLanguage("Python");
            } else if (extension == "js") {
                currentDocument->setLanguage("JavaScript");
            } else if (extension == "java") {
                currentDocument->setLanguage("Java");
            } else {
                currentDocument->setLanguage("Plain");
            }

            // Set up editor
            codeEditor->setDocument(currentDocument);
            codeEditor->setPlainText(content);
            codeEditor->setLanguage(currentDocument->getLanguage());

            // Update UI
            updateTitle();
            updateUserList();

            // Clear chat
            chatBox->clear();
            chatInput->clear();
        } else {
            QMessageBox::critical(this, "Error", "Could not open file: " + fileName);
        }
    }
}

void MainWindow::onSaveDocument()
{
    if (!currentUser || !currentDocument) {
        QMessageBox::warning(this, "No Document", "No document is currently open.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Save Document",
                                                  currentDocument->getTitle(),
                                                  "C++ Files (*.cpp *.h);;Python Files (*.py);;JavaScript Files (*.js);;Java Files (*.java);;All Files (*)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << codeEditor->toPlainText();
            file.close();

            QMessageBox::information(this, "Saved", "Document saved successfully.");
        } else {
            QMessageBox::critical(this, "Error", "Could not save file: " + fileName);
        }
    }
}

void MainWindow::onShareDocument()
{
    if (!currentUser || !currentDocument) {
        QMessageBox::warning(this, "No Document", "No document is currently open.");
        return;
    }

    bool ok;
    QString collaboratorEmail = QInputDialog::getText(this, "Share Document",
                                                    "Enter collaborator's email:", QLineEdit::Normal,
                                                    "", &ok);
    if (ok && !collaboratorEmail.isEmpty()) {
        // In a real implementation, we would check if the user exists and share the document
        // For the prototype, we'll simulate successful sharing
        QMessageBox::information(this, "Document Shared",
                               "Document shared with " + collaboratorEmail + ".\n\n"
                               "For the prototype, this is a simulated action.");
    }
}

void MainWindow::onUserConnected(const QString& userId, const QString& username)
{
    if (!connectedUsers.contains(userId)) {
        connectedUsers[userId] = username;
        updateUserList();

        // Add message to chat
        QString message = "<b>" + username + " has joined the document.</b>";
        chatBox->append(message);
    }
}

void MainWindow::onUserDisconnected(const QString& userId)
{
    if (connectedUsers.contains(userId)) {
        QString username = connectedUsers[userId];
        connectedUsers.remove(userId);
        updateUserList();

        // Add message to chat
        QString message = "<b>" + username + " has left the document.</b>";
        chatBox->append(message);
    }
}

void MainWindow::onCursorPositionChanged()
{
    if (!currentUser || !currentDocument) return;

    // Get current position
    int position = codeEditor->textCursor().position();

    // Update status bar with position information
    int line = codeEditor->textCursor().blockNumber() + 1;
    int column = codeEditor->textCursor().columnNumber() + 1;
    statusLabel->setText(QString("Line: %1 Column: %2 | %3").arg(line).arg(column).arg(currentUser->getUsername()));

    // In a real implementation, we would send this position to other users
    // For the prototype, we'll simulate other users' cursors randomly
    if (isCollaborating) {
        static QElapsedTimer lastUpdate; // Replace QTime with QElapsedTimer

        // Periodically update simulated cursors for demo purposes
        if (!lastUpdate.isValid() || lastUpdate.elapsed() > 2000) {
            lastUpdate.start(); // Restart the timer

            for (const auto& [userId, username] : connectedUsers.toStdMap()) {
                // Generate random position within the document
                int maxPos = codeEditor->document()->characterCount();
                int randomPos = QRandomGenerator::global()->bounded(maxPos); // Replace qrand with QRandomGenerator

                // Update the remote cursor in the editor
                codeEditor->updateRemoteCursor(userId, username, randomPos);
            }
        }
    }
}

void MainWindow::onTextChanged()
{
    if (!currentUser || !currentDocument) return;

    // Update document content
    QString newContent = codeEditor->toPlainText();
    if (newContent != currentDocument->getContent()) {
        currentDocument->setContent(newContent);

        // In a real implementation, we would send these changes to other users
        // For the prototype, we'll just mark the document as modified
        setWindowModified(true);
    }
}

void MainWindow::onChatMessageReceived(const QString& userId, const QString& message)
{
    if (connectedUsers.contains(userId)) {
        QString username = connectedUsers[userId];
        QString formattedMessage = "<b>" + username + ":</b> " + message;
        chatBox->append(formattedMessage);
    } else if (currentUser && userId == currentUser->getUserId()) {
        QString formattedMessage = "<b>You:</b> " + message;
        chatBox->append(formattedMessage);
    }
}

void MainWindow::onSendChatMessage()
{{
    if (!currentUser || !currentDocument) return;

    QString message = chatInput->toPlainText().trimmed();
    if (!message.isEmpty()) {
        // Clear input
        chatInput->clear();

        // Display message locally
        onChatMessageReceived(currentUser->getUserId(), message);

        // In a real implementation, we would send this message to other users
        // For the prototype, we'll simulate a response
        QTimer::singleShot(1500, [this, message]() {
            // Get a random user to respond
            if (!connectedUsers.isEmpty()) {
                QStringList userIds = connectedUsers.keys();
                QString randomUserId = userIds[QRandomGenerator::global()->bounded(userIds.size())]; // Replace qrand

                // Generate a simulated response
                QStringList responses = {
                    "I see what you mean about " + message,
                    "That's interesting. Have you tried a different approach?",
                    "Good point about the code.",
                    "I think we should refactor that part.",
                    "Let me check that function you mentioned."
                };

                QString response = responses[QRandomGenerator::global()->bounded(responses.size())]; // Replace qrand
                onChatMessageReceived(randomUserId, response);
            }
        });
    }
}
}