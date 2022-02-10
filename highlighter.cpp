#include "highlighter.h"

Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    blockHighlightFormat.setForeground(Qt::green);
    wordHighlightFormat.setForeground(Qt::yellow);
    timeStampHighlightFormat.setForeground(Qt::red);
}

void Highlighter::highlightBlock(const QString &text)
{
    if (blockToHighlight == -1)
        return;
    else if (currentBlock().blockNumber() == blockToHighlight) {
        int textEndingPosition = text.split("[")[0].size();
        int timeStampStartPosition = text.split("[")[1].size();

        setFormat(0, textEndingPosition, blockHighlightFormat);
        setFormat(textEndingPosition, timeStampStartPosition + 1, timeStampHighlightFormat);
        auto words = text.split(" ");
        if (wordToHighlight != -1 && wordToHighlight < words.size()) {
            int start{0};
            for (int i=0; i < wordToHighlight; i++) start += (words[i].size() + 1);
            int count = words[wordToHighlight].size();
            setFormat(start, count, wordHighlightFormat);
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
