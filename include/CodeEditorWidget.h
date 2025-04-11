#ifndef CODEEDITORWIDGET_H
#define CODEEDITORWIDGET_H

#include <QPlainTextEdit>
#include <QMap>
#include <QColor>
#include <memory>

class QSyntaxHighlighter;
class Document;
class CollaborationManager;

struct RemoteCursor {
    QString userId;
    QString username;
    int position;
    QColor color;
};

class CodeEditorWidget : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditorWidget(QWidget *parent = nullptr);
    ~CodeEditorWidget();

    void setDocument(std::shared_ptr<Document> doc);
    void setCollaborationManager(std::shared_ptr<CollaborationManager> manager);
    void setLanguage(const QString& language);
    void highlightSyntax();
    
    void updateRemoteCursor(const QString& userId, const QString& username, int position);
    void removeRemoteCursor(const QString& userId);

signals:
    void editorContentChanged(const QString& content);
    void cursorPositionChanged(int position);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onTextChanged();
    void onCursorPositionChanged();
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    class LineNumberArea;
    
    void createLineNumberArea();
    void setupDocument();
    void setupSyntaxHighlighter();
    int lineNumberAreaWidth() const;
    
    std::unique_ptr<LineNumberArea> lineNumberArea;
    std::shared_ptr<Document> currentDocument;
    std::shared_ptr<CollaborationManager> collaborationManager;
    QSyntaxHighlighter* syntaxHighlighter;
    QString currentLanguage;
    
    // Remote cursors for visualization
    QMap<QString, RemoteCursor> remoteCursors;
    
    // Track local changes to avoid loops
    bool ignoreChanges;
};

// Inner class for line numbers display
class CodeEditorWidget::LineNumberArea : public QWidget
{
public:
    LineNumberArea(CodeEditorWidget *editor) : QWidget(editor), editor(editor) {}
    
    QSize sizeHint() const override {
        return QSize(editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    CodeEditorWidget *editor;
};

#endif // CODEEDITORWIDGET_H