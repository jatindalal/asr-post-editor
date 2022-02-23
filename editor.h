#pragma once

#include <QPlainTextEdit>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QSize>
#include <QWidget>
#include <QTime>
#include <QXmlStreamReader>
#include <QRegularExpression>

#include "highlighter.h"

class LineNumberArea;

class Editor : public QPlainTextEdit
{
    Q_OBJECT

    struct word;
    struct block;

public:
    explicit Editor(QWidget *parent = nullptr);
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

    QRegularExpression timeStampExp, speakerExp;
    void findReplace();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *e) override;

signals:
    void jumpToPlayer(const QTime& time);
    void message(const QString& text, int timeout = 5000);

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
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
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
    QWidget *lineNumberArea;
    QFile *m_file = nullptr;
    QList<block> m_blocks;
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
    QList<word> words;

    inline bool operator==(block b) const
    {
          if(b.timeStamp==timeStamp && b.text==text && b.speaker==speaker && b.words==words)
             return true;
          return false;
    }
};

class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(Editor *parentEditor) : QWidget(parentEditor), m_editor(parentEditor)
    {}

    QSize sizeHint() const override
    {
        return QSize(m_editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        m_editor->lineNumberAreaPaintEvent(event);
    }

private:
    Editor *m_editor;
};

