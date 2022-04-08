#pragma once

#include "blockandword.h"
#include "texteditor.h"
#include "changespeakerdialog.h"
#include "timepropagationdialog.h"

#include <QXmlStreamReader>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QCompleter>
#include <QAbstractItemModel>

class Highlighter;

class Editor : public TextEditor
{
    Q_OBJECT

public:
    explicit Editor(QWidget *parent = nullptr);

    void setWordEditor(TextEditor* wordEditor)
    {
        m_wordEditor = wordEditor;        
        connect(m_wordEditor->document(), &QTextDocument::contentsChange, this, &Editor::wordEditorChanged);
    }

    QRegularExpression timeStampExp, speakerExp;

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;

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
    void createChangeSpeakerDialog();
    void createTimePropagationDialog();
    void insertTimeStamp(const QTime& elapsedTime);
    void insertTimeStampInWordEditor(const QTime& elapsedTime);
    void speakerWiseJump(const QString& jumpDirection);
    void wordWiseJump(const QString& jumpDirection);
    void blockWiseJump(const QString& jumpDirection);

private slots:
    void contentChanged(int position, int charsRemoved, int charsAdded);
    void wordEditorChanged(int position, int charsRemoved, int charsAdded);
    void updateWordEditor();
    void changeSpeaker(const QString& newSpeaker, bool replaceAllOccurrences);
    void propagateTime(const QTime& time, int start, int end, bool negateTime);
    void insertSpeakerCompletion(const QString& completion);
    void insertTextCompletion(const QString& completion);

private:
    static QTime getTime(const QString& text);
    block fromEditor(qint64 blockNumber);
    static word makeWord(const QTime& t, const QString& s);
    word fromWordEditor(qint64 blockNumber);
    void loadTranscriptData(QFile* file);
    void setContent();
    void saveXml(QFile* file);
    void helpJumpToPlayer();
    QStringList listFromFile(const QString& fileName);

    bool settingContent{false}, updatingWordEditor{false}, dontUpdateWordEditor{false};
    QFile* m_file = nullptr;
    QVector<block> m_blocks;
    QString m_transcriptLang;
    Highlighter* m_highlighter = nullptr;
    qint64 highlightedBlock = -1, highlightedWord = -1;
    TextEditor* m_wordEditor = nullptr;
    ChangeSpeakerDialog* m_changeSpeaker = nullptr;
    TimePropagationDialog* m_propagateTime = nullptr;
    QCompleter *m_speakerCompleter = nullptr, *m_textCompleter = nullptr;
    QString m_textCompletionName;
    QStringList m_dictionary;
};




class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit Highlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent) {};

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
    void setInvalidBlocks(const QList<int>& invalidBlocks)
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

