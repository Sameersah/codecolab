#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QVector>
#include <QTextCharFormat>

class QTextDocument;

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QTextDocument *parent = nullptr);

    void setLanguage(const QString& language);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    void setupCppRules();
    void setupPythonRules();
    void setupJavaScriptRules();
    void setupJavaRules();
    void clearRules();

    QVector<HighlightingRule> highlightingRules;
    QString currentLanguage;

    // Format for different elements
    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat preprocessorFormat;

    // For multi-line comments
    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;
};

#endif // SYNTAXHIGHLIGHTER_H