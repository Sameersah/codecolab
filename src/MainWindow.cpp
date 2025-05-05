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
#include <QColor>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , collaborationManager(std::make_shared<CollaborationManager>())
    , collaborationClient(std::make_unique<CollaborationClient>())
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

    // Connect collaboration client signals
    connect(collaborationClient.get(), &CollaborationClient::connected,
            this, [this]() {
                statusLabel->setText("Connected to server");
                if (currentDocument) {
                    collaborationClient->joinDocument(currentDocument->getId());
                }
            });
            
    connect(collaborationClient.get(), &CollaborationClient::disconnected,
            this, [this]() {
                statusLabel->setText("Disconnected from server");
                connectedUsers.clear();
                updateUserList();
            });
            
    connect(collaborationClient.get(), &CollaborationClient::error,
            this, [this](const QString& error) {
                QMessageBox::warning(this, "Collaboration Error", error);
            });
            
    connect(collaborationClient.get(), &CollaborationClient::editReceived,
            this, [this](const EditOperation& op) {
                if (currentDocument && codeEditor) {
                    codeEditor->applyRemoteEdit(op);
                }
            });
            
    connect(collaborationClient.get(), &CollaborationClient::cursorPositionReceived,
            this, [this](const QString& userId, const QString& username, int position) {
                if (codeEditor) {
                    codeEditor->updateRemoteCursor(userId, username, position);
                }
            });
            
    connect(collaborationClient.get(), &CollaborationClient::userConnected,
            this, &MainWindow::onUserConnected);
            
    connect(collaborationClient.get(), &CollaborationClient::userDisconnected,
            this, &MainWindow::onUserDisconnected);

    connect(collaborationClient.get(), &CollaborationClient::chatMessageReceived,
            this, &MainWindow::onChatMessageReceived);

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
            // Set the user in the collaboration client
            collaborationClient->setUser(currentUser);
            
            updateStatusBar();
            updateTitle();
            
            // Create a default document for the user
            currentDocument = std::make_shared<Document>(
                "shared_doc",  // Use a fixed ID for the shared document
                "Test Document.cpp",
                currentUser);

            // Set initial content
            QString initialContent = "// Test Document.cpp\n"
                                   "// Shared document for collaboration\n\n"
                                   "#include <iostream>\n\n"
                                   "int main() {\n"
                                   "    std::cout << \"Hello, World!\" << std::endl;\n"
                                   "    return 0;\n"
                                   "}\n";
            
            currentDocument->setContent(initialContent);
            currentDocument->setLanguage("C++");

            // Set up editor
            codeEditor->setDocument(currentDocument);
            codeEditor->setPlainText(initialContent);
            codeEditor->setLanguage("C++");
            codeEditor->highlightSyntax(); // Force syntax highlighting update

            // Update UI
            updateTitle();
            updateUserList();

            // Clear chat
            chatBox->clear();
            chatInput->clear();

            // Try to connect to the collaboration server
            if (collaborationClient->connect("ws://localhost:8080")) {
                statusLabel->setText("Connected to server");
            } else {
                statusLabel->setText("Not connected to server");
            }
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

    // Send cursor position to other users
    if (collaborationClient && collaborationClient->isConnected()) {
        collaborationClient->sendCursorPosition(position);
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

void MainWindow::onChatMessageReceived(const QString& userId, const QString& username, const QString& message)
{
    // Use the same color scheme as the cursors
    static const QColor colors[] = {
        QColor(255, 0, 0),     // Red
        QColor(0, 255, 0),     // Green
        QColor(0, 0, 255),     // Blue
        QColor(255, 255, 0),   // Yellow
        QColor(255, 0, 255),   // Magenta
        QColor(0, 255, 255),   // Cyan
        QColor(255, 128, 0),   // Orange
        QColor(128, 0, 255)    // Purple
    };

    // Calculate a more deterministic color index based on the entire user ID
    int colorIndex = 0;
    if (!userId.isEmpty()) {
        // Sum up all characters in the user ID
        int sum = 0;
        for (const QChar& c : userId) {
            sum += c.unicode();
        }
        colorIndex = sum % 8;
    }
    QColor userColor = colors[colorIndex];

    QString formattedMessage;
    if (currentUser && userId == currentUser->getUserId()) {
        formattedMessage = QString("<span style='color: %1'><b>You:</b> %2</span>")
            .arg(userColor.name())
            .arg(message);
    } else {
        formattedMessage = QString("<span style='color: %1'><b>%2:</b> %3</span>")
            .arg(userColor.name())
            .arg(username)
            .arg(message);
    }
    chatBox->append(formattedMessage);
}

void MainWindow::onSendChatMessage()
{
    if (!currentUser || !currentDocument) return;

    QString message = chatInput->toPlainText().trimmed();
    if (!message.isEmpty()) {
        // Clear input
        chatInput->clear();

        // Send message through collaboration client
        if (collaborationClient && collaborationClient->isConnected()) {
            collaborationClient->sendChatMessage(message);
        }
    }
}

void MainWindow::addDocument(std::shared_ptr<Document> document)
{
    if (!document) return;
    
    currentDocument = document;
    codeEditor->setDocument(document);
    codeEditor->setPlainText(document->getContent());
    codeEditor->setLanguage(document->getLanguage());
    codeEditor->highlightSyntax(); // Force syntax highlighting update
    
    // Set up collaboration
    if (collaborationClient) {
        // Set the document in the collaboration client
        collaborationClient->setDocument(document);
        
        // If connected, join the document
        if (collaborationClient->isConnected()) {
            collaborationClient->joinDocument(document->getId());
        } else {
            // Try to connect to the collaboration server
            collaborationClient->connect("ws://localhost:8080");
        }
    }
    
    updateTitle();
    updateStatusBar();
}