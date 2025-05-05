// CodeEditorWidget.h
#ifndef CODEEDITORWIDGET_H
#define CODEEDITORWIDGET_H

#include <QPlainTextEdit>
#include <QMap>
#include <QColor>
#include <memory>
#include "Document.h"
#include "CollaborationManager.h"
#include "EditOperation.h"

class QSyntaxHighlighter;
class QPaintEvent;
class QResizeEvent;
class QSize;

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
    void applyRemoteEdit(const EditOperation& operation);

    void removeRemoteCursor(const QString& userId);

signals:
    void editorContentChanged(const QString& content);
    void cursorPositionChanged(int position);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onTextChanged();
    void onCursorPositionChanged();
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    class LineNumberArea : public QWidget
    {
    public:
        LineNumberArea(CodeEditorWidget *editor) : QWidget(editor), codeEditor(editor) {}

        QSize sizeHint() const override {
            return QSize(codeEditor->lineNumberAreaWidth(), 0);
        }

    protected:
        void paintEvent(QPaintEvent *event) override {
            codeEditor->lineNumberAreaPaintEvent(event);
        }

    private:
        CodeEditorWidget *codeEditor;
    };

    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

    LineNumberArea *lineNumberArea;
    std::shared_ptr<Document> currentDocument;
    std::shared_ptr<CollaborationManager> collaborationManager;
    QSyntaxHighlighter* syntaxHighlighter;
    QString currentLanguage;

    // Remote cursors for visualization
    QMap<QString, RemoteCursor> remoteCursors;

    // Track local changes to avoid loops
    bool ignoreChanges;
};

#endif // CODEEDITORWIDGET_H