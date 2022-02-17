#include "highlighter.h"

#include <QRegularExpression>

Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    blockHighlightFormat.setForeground(Qt::green);
    wordHighlightFormat.setBackground(Qt::yellow);
    timeStampHighlightFormat.setForeground(Qt::red);
    speakerHighlightFormat.setForeground(QColor(Qt::blue).lighter(120));
}

void Highlighter::highlightBlock(const QString &text)
{
    if (blockToHighlight == -1)
        return;
    else if (currentBlock().blockNumber() == blockToHighlight) {
        int speakerEnd = 0;
        auto speakerMatch = QRegularExpression("\\[[\\w\\.]*]:").match(text);
        if (speakerMatch.hasMatch())
            speakerEnd = speakerMatch.capturedEnd();

        int timeStampStart = QRegularExpression("\\[(\\d?\\d:)?[0-5]?\\d:[0-5]?\\d(\\.\\d\\d?\\d?)?]").match(text).capturedStart();

        setFormat(0, speakerEnd, speakerHighlightFormat);
        setFormat(speakerEnd, timeStampStart, blockHighlightFormat);
        setFormat(timeStampStart, text.size(), timeStampHighlightFormat);

        auto words = text.mid(speakerEnd + 1).split(" ");

        if (wordToHighlight != -1 && wordToHighlight < words.size()) {
            int start = speakerEnd;
            for (int i=0; i < wordToHighlight; i++) start += (words[i].size() + 1);
            int count = words[wordToHighlight].size();
            setFormat(start + 1, count, wordHighlightFormat);
        }
    }
}

void Highlighter::clearHighlight()
{
    blockToHighlight = -1;
    wordToHighlight = -1;
}

void Highlighter::setBlockToHighlight(qint64 blockNumber)
{
    blockToHighlight = blockNumber;
    rehighlight();
}

void Highlighter::setWordToHighlight(int wordNumber)
{
    wordToHighlight = wordNumber;
    rehighlight();
}
