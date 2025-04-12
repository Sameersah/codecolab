#include "SyntaxHighlighter.h"

#include <QTextDocument>
#include <QRegularExpression>
#include <QDebug>
#include <algorithm>
SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , currentLanguage("Plain")
{
    // Initialize formats
    keywordFormat.setForeground(Qt::blue);
    keywordFormat.setFontWeight(QFont::Bold);
    
    classFormat.setForeground(Qt::darkMagenta);
    classFormat.setFontWeight(QFont::Bold);
    
    functionFormat.setForeground(Qt::darkGreen);
    
    quotationFormat.setForeground(Qt::darkRed);
    
    singleLineCommentFormat.setForeground(Qt::darkGray);
    singleLineCommentFormat.setFontItalic(true);
    
    multiLineCommentFormat.setForeground(Qt::darkGray);
    multiLineCommentFormat.setFontItalic(true);
    
    numberFormat.setForeground(Qt::darkCyan);
    
    preprocessorFormat.setForeground(Qt::darkBlue);
}

void SyntaxHighlighter::setLanguage(const QString& language)
{
    if (currentLanguage == language) return;
    
    currentLanguage = language;
    
    // Clear existing rules
    clearRules();
    
    // Setup rules for the selected language
    if (language == "C++") {
        setupCppRules();
    } else if (language == "Python") {
        setupPythonRules();
    } else if (language == "JavaScript") {
        setupJavaScriptRules();
    } else if (language == "Java") {
        setupJavaRules();
    }
    
    // Apply highlighting
    rehighlight();
}

void SyntaxHighlighter::clearRules()
{
    highlightingRules.clear();
    commentStartExpression = QRegularExpression();
    commentEndExpression = QRegularExpression();
}

void SyntaxHighlighter::setupCppRules()
{
    // Keywords
    QStringList keywordPatterns = {
        "\\bchar\\b", "\\bclass\\b", "\\bconst\\b", "\\bdouble\\b",
        "\\benum\\b", "\\bexplicit\\b", "\\bfriend\\b", "\\binline\\b",
        "\\bint\\b", "\\blong\\b", "\\bnamespace\\b", "\\boperator\\b",
        "\\bprivate\\b", "\\bprotected\\b", "\\bpublic\\b", "\\bshort\\b",
        "\\bsignals\\b", "\\bsigned\\b", "\\bslots\\b", "\\bstatic\\b",
        "\\bstruct\\b", "\\btemplate\\b", "\\btypedef\\b", "\\btypename\\b",
        "\\bunion\\b", "\\bunsigned\\b", "\\bvirtual\\b", "\\bvoid\\b",
        "\\bvolatile\\b", "\\bbool\\b", "\\breturn\\b", "\\bif\\b",
        "\\belse\\b", "\\bfor\\b", "\\bwhile\\b", "\\bdo\\b", "\\bswitch\\b",
        "\\bcase\\b", "\\bbreak\\b", "\\bcontinue\\b", "\\bgoto\\b"
    };
    
    for (const QString &pattern : keywordPatterns) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }
    
    // Class names
    HighlightingRule rule;
    rule.pattern = QRegularExpression("\\bQ[A-Za-z]+\\b");
    rule.format = classFormat;
    highlightingRules.append(rule);
    
    // Functions
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);
    
    // Quotations
    rule.pattern = QRegularExpression("\".*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);
    
    // Single-line comments
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);
    
    // Numbers
    rule.pattern = QRegularExpression("\\b[0-9]+\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);
    
    // Preprocessor directives
    rule.pattern = QRegularExpression("#[^\n]*");
    rule.format = preprocessorFormat;
    highlightingRules.append(rule);
    
    // Multi-line comments
    commentStartExpression = QRegularExpression("/\\*");
    commentEndExpression = QRegularExpression("\\*/");
}

void SyntaxHighlighter::setupPythonRules()
{
    // Keywords
    QStringList keywordPatterns = {
        "\\bFalse\\b", "\\bNone\\b", "\\bTrue\\b", "\\band\\b",
        "\\bas\\b", "\\bassert\\b", "\\bbreak\\b", "\\bclass\\b",
        "\\bcontinue\\b", "\\bdef\\b", "\\bdel\\b", "\\belif\\b",
        "\\belse\\b", "\\bexcept\\b", "\\bfinally\\b", "\\bfor\\b",
        "\\bfrom\\b", "\\bglobal\\b", "\\bif\\b", "\\bimport\\b",
        "\\bin\\b", "\\bis\\b", "\\blambda\\b", "\\bnonlocal\\b",
        "\\bnot\\b", "\\bor\\b", "\\bpass\\b", "\\braise\\b",
        "\\breturn\\b", "\\btry\\b", "\\bwhile\\b", "\\bwith\\b",
        "\\byield\\b"
    };
    
    for (const QString &pattern : keywordPatterns) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }
    
    // Class names
    HighlightingRule rule;
    rule.pattern = QRegularExpression("\\bclass\\s+([A-Za-z_][A-Za-z0-9_]*)");
    rule.format = classFormat;
    highlightingRules.append(rule);
    
    // Functions
    rule.pattern = QRegularExpression("\\bdef\\s+([A-Za-z_][A-Za-z0-9_]*)\\b");
    rule.format = functionFormat;
    highlightingRules.append(rule);
    
    // Function calls
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);
    
    // Double-quoted strings
    rule.pattern = QRegularExpression("\"[^\"]*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);
    
    // Single-quoted strings
    rule.pattern = QRegularExpression("'[^']*'");
    rule.format = quotationFormat;
    highlightingRules.append(rule);
    
    // Single-line comments
    rule.pattern = QRegularExpression("#[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);
    
    // Numbers
    rule.pattern = QRegularExpression("\\b[0-9]+\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);
    
    // No multi-line comments in Python, but we handle docstrings as comments
    commentStartExpression = QRegularExpression("\"\"\"");
    commentEndExpression = QRegularExpression("\"\"\"");
}

void SyntaxHighlighter::setupJavaScriptRules()
{
    // Keywords
    QStringList keywordPatterns = {
        "\\bbreak\\b", "\\bcase\\b", "\\bcatch\\b", "\\bclass\\b",
        "\\bconst\\b", "\\bcontinue\\b", "\\bdebugger\\b", "\\bdefault\\b",
        "\\bdelete\\b", "\\bdo\\b", "\\belse\\b", "\\benum\\b",
        "\\bexport\\b", "\\bextends\\b", "\\bfalse\\b", "\\bfinally\\b",
        "\\bfor\\b", "\\bfunction\\b", "\\bif\\b", "\\bimport\\b",
        "\\bin\\b", "\\binstanceof\\b", "\\bnew\\b", "\\bnull\\b",
        "\\breturn\\b", "\\bsuper\\b", "\\bswitch\\b", "\\bthis\\b",
        "\\bthrow\\b", "\\btrue\\b", "\\btry\\b", "\\btypeof\\b",
        "\\bvar\\b", "\\bvoid\\b", "\\bwhile\\b", "\\bwith\\b",
        "\\byield\\b", "\\blet\\b", "\\bconst\\b", "\\bawait\\b",
        "\\basync\\b"
    };
    
    for (const QString &pattern : keywordPatterns) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }
    
    // Functions
    HighlightingRule rule;
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);
    
    // Double-quoted strings
    rule.pattern = QRegularExpression("\"[^\"]*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);
    
    // Single-quoted strings
    rule.pattern = QRegularExpression("'[^']*'");
    rule.format = quotationFormat;
    highlightingRules.append(rule);
    
    // Template strings
    rule.pattern = QRegularExpression("`[^`]*`");
    rule.format = quotationFormat;
    highlightingRules.append(rule);
    
    // Single-line comments
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);
    
    // Numbers
    rule.pattern = QRegularExpression("\\b[0-9]+\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);
    
    // Multi-line comments
    commentStartExpression = QRegularExpression("/\\*");
    commentEndExpression = QRegularExpression("\\*/");
}

void SyntaxHighlighter::setupJavaRules()
{
    // Keywords
    QStringList keywordPatterns = {
        "\\babstract\\b", "\\bassert\\b", "\\bboolean\\b", "\\bbreak\\b",
        "\\bbyte\\b", "\\bcase\\b", "\\bcatch\\b", "\\bchar\\b",
        "\\bclass\\b", "\\bconst\\b", "\\bcontinue\\b", "\\bdefault\\b",
        "\\bdo\\b", "\\bdouble\\b", "\\belse\\b", "\\benum\\b",
        "\\bextends\\b", "\\bfinal\\b", "\\bfinally\\b", "\\bfloat\\b",
        "\\bfor\\b", "\\bgoto\\b", "\\bif\\b", "\\bimplements\\b",
        "\\bimport\\b", "\\binstanceof\\b", "\\bint\\b", "\\binterface\\b",
        "\\blong\\b", "\\bnative\\b", "\\bnew\\b", "\\bpackage\\b",
        "\\bprivate\\b", "\\bprotected\\b", "\\bpublic\\b", "\\breturn\\b",
        "\\bshort\\b", "\\bstatic\\b", "\\bstrictfp\\b", "\\bsuper\\b",
        "\\bswitch\\b", "\\bsynchronized\\b", "\\bthis\\b", "\\bthrow\\b",
        "\\bthrows\\b", "\\btransient\\b", "\\btry\\b", "\\bvoid\\b",
        "\\bvolatile\\b", "\\bwhile\\b"
    };
    
    for (const QString &pattern : keywordPatterns) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }
    
    // Class names
    HighlightingRule rule;
    rule.pattern = QRegularExpression("\\bclass\\s+([A-Za-z_][A-Za-z0-9_]*)");
    rule.format = classFormat;
    highlightingRules.append(rule);
    
    // Functions
    rule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*\\(");
    rule.format = functionFormat;
    highlightingRules.append(rule);
    
    // Method calls
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);
    
    // Quotations
    rule.pattern = QRegularExpression("\"[^\"]*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);
    
    // Single-line comments
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);
    
    // Numbers
    rule.pattern = QRegularExpression("\\b[0-9]+\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);
    
    // Multi-line comments
    commentStartExpression = QRegularExpression("/\\*");
    commentEndExpression = QRegularExpression("\\*/");
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    // Apply regular expression highlighting rules
    // Replace qAsConst with std::as_const
    for (const HighlightingRule &rule : std::as_const(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // Handle multi-line comments if applicable
    if (!commentStartExpression.pattern().isEmpty() && !commentEndExpression.pattern().isEmpty()) {
        setCurrentBlockState(0);

        int startIndex = 0;
        if (previousBlockState() != 1) {
            startIndex = text.indexOf(commentStartExpression);
        }

        while (startIndex >= 0) {
            QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
            int endIndex = match.capturedStart();
            int commentLength = 0;

            if (endIndex == -1) {
                setCurrentBlockState(1);
                commentLength = text.length() - startIndex;
            } else {
                commentLength = endIndex - startIndex + match.capturedLength();
            }

            setFormat(startIndex, commentLength, multiLineCommentFormat);
            startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
        }
    }
}