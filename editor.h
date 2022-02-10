#pragma once

#include <QPlainTextEdit>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QSize>
#include <QWidget>
#include <QTime>
#include <QXmlStreamReader>

#include "highlighter.h"

class LineNumberArea;

class Editor : public QPlainTextEdit
{
    Q_OBJECT

    struct word;
    struct block;

public:
    Editor(QWidget *parent = nullptr);
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *e) override;

signals:
    void editorMouseClicked();
    void message(const QString& text, int timeout = 5000);

public slots:
    void open();
    void openTranscript();

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
    void contentChanged(int position, int charsRemoved, int charsAdded);

private:
    void showBlocks();
    void addBlock(qint64 blockNumber);
    void setContent();
    void loadTranscriptData(QFile* file);
    block fromEditor(qint64 blockNumber);
    QTime getTime(const QString& text);

    bool settingContent{false};
    QWidget *lineNumberArea;
    QFile *m_file = nullptr;
    QList<block> m_blocks;
    Highlighter *m_highlighter = nullptr;
};


struct Editor::word
{
    QTime timeStamp;
    QString text;
};

struct Editor::block
{
    QTime timeStamp;
    QString text;
    QString speaker;
    QList<word> words;
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

