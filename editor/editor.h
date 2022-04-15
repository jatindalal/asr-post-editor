#pragma once

#include "blockandword.h"
#include "texteditor.h"
#include "wordeditor.h"
#include "utilities/changespeakerdialog.h"
#include "utilities/timepropagationdialog.h"
#include "utilities/tagselectiondialog.h"

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

    void setWordEditor(WordEditor* wordEditor)
    {
        m_wordEditor = wordEditor;
        connect(m_wordEditor, &QTableWidget::itemChanged, this, &Editor::wordEditorChanged);
    }

    QRegularExpression timeStampExp, speakerExp;

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;

signals:
    void jumpToPlayer(const QTime& time);
    void refreshTagList(const QStringList& tagList);

public slots:
    void openTranscript();
    void saveTranscript();
    void highlightTranscript(const QTime& elapsedTime);

    void showBlocksFromData();
    void jumpToHighlightedLine();
    void splitLine(const QTime& elapsedTime);
    void mergeUp();
    void mergeDown();
    void createChangeSpeakerDialog();
    void createTimePropagationDialog();
    void createTagSelectionDialog();
    void insertTimeStamp(const QTime& elapsedTime);

    void speakerWiseJump(const QString& jumpDirection);
    void wordWiseJump(const QString& jumpDirection);
    void blockWiseJump(const QString& jumpDirection);

private slots:
    void contentChanged(int position, int charsRemoved, int charsAdded);
    void wordEditorChanged();

    void updateWordEditor();

    void changeSpeaker(const QString& newSpeaker, bool replaceAllOccurrences);
    void propagateTime(const QTime& time, int start, int end, bool negateTime);
    void selectTags(const QStringList& newTagList);

    void insertSpeakerCompletion(const QString& completion);
    void insertTextCompletion(const QString& completion);

private:
    static QTime getTime(const QString& text);
    static word makeWord(const QTime& t, const QString& s, const QStringList& tagList);

    void loadTranscriptData(QFile* file);
    void setContent();
    void saveXml(QFile* file);
    void helpJumpToPlayer();

    block fromEditor(qint64 blockNumber) const;
    QStringList listFromFile(const QString& fileName) const;

    bool settingContent{false}, updatingWordEditor{false}, dontUpdateWordEditor{false};
    QFile* m_file = nullptr;
    QVector<block> m_blocks;
    QString m_transcriptLang;
    Highlighter* m_highlighter = nullptr;
    qint64 highlightedBlock = -1, highlightedWord = -1;
    WordEditor* m_wordEditor = nullptr;
    ChangeSpeakerDialog* m_changeSpeaker = nullptr;
    TimePropagationDialog* m_propagateTime = nullptr;
    TagSelectionDialog* m_selectTag = nullptr;
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
    void setInvalidWords(const QMultiMap<int, int>& invalidWordsMap)
    {
        invalidWords = invalidWordsMap;
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
    QMultiMap<int, int> invalidWords;
};

