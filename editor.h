#pragma once

#include "texteditor.h"
#include "findreplacedialog.h"

#include <QTime>
#include <QXmlStreamReader>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextDocument>

class Highlighter;

class Editor : public TextEditor
{
    Q_OBJECT

    struct word;
    struct block;

public:
    explicit Editor(QWidget *parent = nullptr);

    QRegularExpression timeStampExp, speakerExp;

protected:
    void mousePressEvent(QMouseEvent *e) override;

signals:
    void jumpToPlayer(const QTime& time);

public slots:
    void openTranscript();
    void saveTranscript();
    void showBlocksFromData();
    void highlightTranscript(const QTime& elapsedTime);
    void jumpToHighlightedLine();
    void splitLine(const QTime& elapsedTime);
    void mergeUp();
    void mergeDown();

private slots:
    void contentChanged(int position, int charsRemoved, int charsAdded);

private:
    QTime getTime(const QString& text);
    word makeWord(const QTime& t, const QString& s);
    block fromEditor(qint64 blockNumber);
    void loadTranscriptData(QFile* file);
    void setContent();
    void saveXml(QFile* file);
    void helpJumpToPlayer();

    bool settingContent{false};
    QFile *m_file = nullptr;
    QVector<block> m_blocks;
    Highlighter *m_highlighter = nullptr;
    qint64 highlightedBlock = -1, highlightedWord = -1;
};


struct Editor::word
{
    QTime timeStamp;
    QString text;

    inline bool operator==(word w) const
    {
        if (w.timeStamp == timeStamp && w.text == text)
            return true;
        return false;
    }
};

struct Editor::block
{
    QTime timeStamp;
    QString text;
    QString speaker;
    QVector<word> words;

    inline bool operator==(block b) const
    {
          if(b.timeStamp==timeStamp && b.text==text && b.speaker==speaker && b.words==words)
             return true;
          return false;
    }
};


class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    Highlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent) {};

    void clearHighlight()
    {
        blockToHighlight = -1;
        wordToHighlight = -1;
    }
    void setBlockToHighlight(qint64 blockNumber)
    {
        blockToHighlight = blockNumber;
        rehighlight();
    }
    void setWordToHighlight(int wordNumber)
    {
        wordToHighlight = wordNumber;
        rehighlight();
    }

    void setInvalidBlocks(const QList<int> invalidBlocks)
    {
        invalidBlockNumbers = invalidBlocks;
        rehighlight();
    }
    void clearInvalidBlocks()
    {
        invalidBlockNumbers.clear();
    }

    void highlightBlock(const QString&) override;

private:
    int blockToHighlight{-1};
    int wordToHighlight{-1};
    QList<int> invalidBlockNumbers;
};

