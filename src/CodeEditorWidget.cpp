// CodeEditorWidget.cpp
#include "CodeEditorWidget.h"
#include "SyntaxHighlighter.h"
#include "Document.h"
#include "CollaborationManager.h"

#include <QPainter>
#include <QTextBlock>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QScrollBar>
#include <QDebug>
#include <QResizeEvent>

CodeEditorWidget::CodeEditorWidget(QWidget *parent)
    : QPlainTextEdit(parent)
    , lineNumberArea(new LineNumberArea(this))
    , syntaxHighlighter(nullptr)
    , currentLanguage("Plain")
    , ignoreChanges(false)
{
    setLineWrapMode(QPlainTextEdit::NoWrap);

    // Enable line numbers
    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditorWidget::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditorWidget::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditorWidget::highlightCurrentLine);
    connect(this, &QPlainTextEdit::textChanged, this, &CodeEditorWidget::onTextChanged);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditorWidget::onCursorPositionChanged);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    // Set a monospaced font
    QFont font("Courier New", 10);
    font.setFixedPitch(true);
    setFont(font);

    // Set tab width to 4 spaces (updated for Qt 6)
    QFontMetricsF metrics(font);
    setTabStopDistance(4 * metrics.horizontalAdvance(' '));
}

CodeEditorWidget::~CodeEditorWidget()
{
    delete syntaxHighlighter;
    // lineNumberArea is automatically deleted as a child widget
}

void CodeEditorWidget::setDocument(std::shared_ptr<Document> doc)
{
    currentDocument = doc;
    if (currentDocument) {
        if (syntaxHighlighter) {
            delete syntaxHighlighter;
            syntaxHighlighter = nullptr;
        }

        // Create a new syntax highlighter
        syntaxHighlighter = new SyntaxHighlighter(document());

        // Call the method on our custom SyntaxHighlighter class
        dynamic_cast<SyntaxHighlighter*>(syntaxHighlighter)->setLanguage(currentDocument->getLanguage());
    }
}

void CodeEditorWidget::setCollaborationManager(std::shared_ptr<CollaborationManager> manager)
{
    collaborationManager = manager;
}

void CodeEditorWidget::setLanguage(const QString& language)
{
    currentLanguage = language;
    if (syntaxHighlighter) {
        // Call the method on our custom SyntaxHighlighter class
        dynamic_cast<SyntaxHighlighter*>(syntaxHighlighter)->setLanguage(language);
    }
}

void CodeEditorWidget::highlightSyntax()
{
    if (syntaxHighlighter) {
        syntaxHighlighter->rehighlight();
    }
}

void CodeEditorWidget::updateRemoteCursor(const QString& userId, const QString& username, int position)
{
    // Create or update remote cursor
    RemoteCursor cursor;
    cursor.userId = userId;
    cursor.username = username;
    cursor.position = position;

    // Assign a color if it doesn't have one
    if (!remoteCursors.contains(userId)) {
        // Generate a color based on userId hash
        int hash = 0;
        for (QChar c : userId) {
            hash = (hash * 31) + c.unicode();
        }

        // Create a hue between 0 and 359 (exclude red which is for errors)
        int hue = (hash % 300) + 30;
        QColor color = QColor::fromHsv(hue, 255, 255);
        cursor.color = color;
    } else {
        cursor.color = remoteCursors[userId].color;
    }

    // Store the cursor
    remoteCursors[userId] = cursor;

    // Trigger repaint
    viewport()->update();
}

void CodeEditorWidget::removeRemoteCursor(const QString& userId)
{
    if (remoteCursors.contains(userId)) {
        remoteCursors.remove(userId);
        viewport()->update();
    }
}

void CodeEditorWidget::paintEvent(QPaintEvent* event)
{
    // First do the standard painting
    QPlainTextEdit::paintEvent(event);

    // Now draw remote cursors
    QPainter painter(viewport());

    for (const auto& cursor : remoteCursors) {
        // Get the position of the cursor in the document
        QTextCursor textCursor = QTextCursor(document());
        textCursor.setPosition(cursor.position);

        // Get the cursor rectangle using the correct method for Qt 6.9.0
        // Use QPlainTextEdit:: prefix to avoid naming conflict with local variable
        QRect cursorRectangle = QPlainTextEdit::cursorRect(textCursor);

        // Draw a vertical line for the cursor
        painter.setPen(QPen(cursor.color, 2));
        painter.drawLine(cursorRectangle.topLeft(), cursorRectangle.bottomLeft());

        // Draw the username above the cursor
        QFont font = painter.font();
        font.setBold(true);
        painter.setFont(font);

        QRect nameRect = cursorRectangle;
        nameRect.setHeight(fontMetrics().height());
        nameRect.moveTop(cursorRectangle.top() - nameRect.height());
        nameRect.setLeft(cursorRectangle.left());
        nameRect.setWidth(fontMetrics().horizontalAdvance(cursor.username) + 10);

        // Draw a background rectangle
        painter.fillRect(nameRect, cursor.color);

        // Draw the text
        painter.setPen(Qt::white);
        painter.drawText(nameRect, Qt::AlignCenter, cursor.username);
    }
}

void CodeEditorWidget::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(Qt::lightGray).lighter(120));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::darkGray);
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

int CodeEditorWidget::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditorWidget::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditorWidget::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditorWidget::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditorWidget::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::yellow).lighter(180);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditorWidget::keyPressEvent(QKeyEvent *event)
{
    // Handle special keys for editing
    if (event->key() == Qt::Key_Tab) {
        // Insert 4 spaces instead of a tab
        textCursor().insertText("    ");
        event->accept();
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // Auto-indent: copy whitespace from the current line
        QTextCursor cursor = textCursor();
        QString currentLine = cursor.block().text();

        // Extract leading whitespace
        QString indent;
        for (QChar c : currentLine) {
            if (c.isSpace()) {
                indent += c;
            } else {
                break;
            }
        }

        // Add extra indent after brackets
        if (currentLine.contains('{') && !currentLine.contains('}')) {
            indent += "    ";
        }

        // Insert newline with indent
        QPlainTextEdit::keyPressEvent(event);
        textCursor().insertText(indent);
    } else {
        QPlainTextEdit::keyPressEvent(event);
    }
}

void CodeEditorWidget::onTextChanged()
{
    if (ignoreChanges || !currentDocument) return;

    // Track local changes
    ignoreChanges = true;
    QString content = toPlainText();

    // Notify about content changes
    emit editorContentChanged(content);

    // Update document content
    currentDocument->setContent(content);

    // Create an edit operation
    if (collaborationManager) {
        EditOperation op;
        op.userId = "local";
        op.documentId = currentDocument->getId();
        op.position = textCursor().position();
        
        // For the prototype, we'll just send the entire content as an insertion
        // In a real implementation, we would calculate the actual changes
        op.insertion = content;
        op.deletionLength = 0;

        // Send the operation to the collaboration manager
        collaborationManager->synchronizeChanges(op);
    }

    ignoreChanges = false;
}

void CodeEditorWidget::onCursorPositionChanged()
{
    // Update UI
    highlightCurrentLine();

    // Notify about cursor position changes
    emit cursorPositionChanged(textCursor().position());
}

void CodeEditorWidget::mousePressEvent(QMouseEvent *event)
{
    QPlainTextEdit::mousePressEvent(event);
}

void CodeEditorWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QPlainTextEdit::mouseReleaseEvent(event);
}

void CodeEditorWidget::applyRemoteEdit(const EditOperation& operation)
{
    if (!currentDocument || ignoreChanges) return;

    // Set flag to avoid triggering local change events
    ignoreChanges = true;

    // Get current content
    QString content = toPlainText();

    // Apply the edit operation
    if (operation.position >= 0 && operation.position <= content.length()) {
        // Remove deleted text if any
        if (operation.deletionLength > 0) {
            content.remove(operation.position, operation.deletionLength);
        }
        
        // Insert new text
        if (!operation.insertion.isEmpty()) {
            content.insert(operation.position, operation.insertion);
        }
        
        // Update the editor
        setPlainText(content);
    }

    // Reset flag
    ignoreChanges = false;
}