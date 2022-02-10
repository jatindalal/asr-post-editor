#pragma once

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QDebug>

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    Highlighter(QTextDocument *parent = nullptr);
    void clearHighlight();
    void setBlockToHighlight(qint64 blockNumber);
    void setWordToHighlight(int wordNumber);

protected:
    void highlightBlock(const QString &text) override;

private:
    int blockToHighlight{-1};
    int wordToHighlight{-1};
    QTextCharFormat blockHighlightFormat, wordHighlightFormat, timeStampHighlightFormat;
};

