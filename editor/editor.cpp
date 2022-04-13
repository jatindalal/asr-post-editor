#include "editor.h"

#include <QPainter>
#include <QTextBlock>
#include <QFileDialog>
#include <QStandardPaths>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QStringListModel>
#include <QMessageBox>
#include <algorithm>
#include <QDebug>

Editor::Editor(QWidget *parent) : TextEditor(parent)
{
    timeStampExp = QRegularExpression(R"(\[(\d?\d:)?[0-5]?\d:[0-5]?\d(\.\d\d?\d?)?])");
    speakerExp = QRegularExpression(R"(\[.*]:)");

    connect(this->document(), &QTextDocument::contentsChange, this, &Editor::contentChanged);
    connect(this, &Editor::cursorPositionChanged, this, &Editor::updateWordEditor);
    connect(this, &Editor::cursorPositionChanged, this,
    [&]()
    {
        if (!m_blocks.isEmpty())
            emit refreshTagList(m_blocks[textCursor().blockNumber()].tagList);
    });

    m_speakerCompleter = new QCompleter(this);
    m_speakerCompleter->setWidget(this);
    m_speakerCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_speakerCompleter->setWrapAround(false);
    m_speakerCompleter->setCompletionMode(QCompleter::PopupCompletion);

    m_textCompleter = new QCompleter(this);
    m_textCompleter->setWidget(this);
    m_textCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_textCompleter->setWrapAround(false);
    m_textCompleter->setCompletionMode(QCompleter::PopupCompletion);
    m_textCompleter->setModelSorting(QCompleter::CaseInsensitivelySortedModel);

    connect(m_speakerCompleter, QOverload<const QString &>::of(&QCompleter::activated),
                     this, &Editor::insertSpeakerCompletion);
    connect(m_textCompleter, QOverload<const QString &>::of(&QCompleter::activated),
                     this, &Editor::insertTextCompletion);
}

void Highlighter::highlightBlock(const QString& text)
{
    if (invalidBlockNumbers.contains(currentBlock().blockNumber())) {
        QTextCharFormat format;
        format.setForeground(Qt::red);
        setFormat(0, text.size(), format);
        return;
    }
    if (invalidWords.contains(currentBlock().blockNumber())) {
        auto invalidWordNumbers = invalidWords.values(currentBlock().blockNumber());
        auto speakerEnd = 0;
        auto speakerMatch = QRegularExpression(R"(\[.*]:)").match(text);
        if (speakerMatch.hasMatch())
            speakerEnd = speakerMatch.capturedEnd();

        auto words = text.mid(speakerEnd + 1).split(" ");
        int start = speakerEnd;

        QTextCharFormat format;
        format.setFontUnderline(true);
        format.setUnderlineColor(Qt::red);
        format.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

        for (int i = 0; i < words.size() - 1; i++) {
            if (!invalidWordNumbers.contains(i))
                continue;
            for (int j = 0; j < i; j++) start += (words[j].size() + 1);
            int count = words[i].size();
            setFormat(start + 1, count, format);
            start = speakerEnd;
        }
    }
    if (blockToHighlight == -1)
        return;
    else if (currentBlock().blockNumber() == blockToHighlight) {
        int speakerEnd = 0;
        auto speakerMatch = QRegularExpression(R"(\[.*]:)").match(text);
        if (speakerMatch.hasMatch())
            speakerEnd = speakerMatch.capturedEnd();

        int timeStampStart = QRegularExpression(R"(\[(\d?\d:)?[0-5]?\d:[0-5]?\d(\.\d\d?\d?)?])").match(text).capturedStart();

        QTextCharFormat format;

        format.setForeground(QColor(Qt::blue).lighter(120));
        setFormat(0, speakerEnd, format);

        format.setForeground(Qt::green);
        setFormat(speakerEnd, timeStampStart, format);

        format.setForeground(Qt::red);
        setFormat(timeStampStart, text.size(), format);

        auto words = text.mid(speakerEnd + 1).split(" ");

        if (wordToHighlight != -1 && wordToHighlight < words.size()) {
            int start = speakerEnd;
            for (int i=0; i < wordToHighlight; i++) start += (words[i].size() + 1);
            int count = words[wordToHighlight].size();

            format.setFontUnderline(true);
            format.setUnderlineColor(Qt::green);
            format.setUnderlineStyle(QTextCharFormat::DashUnderline);
            format.setForeground(Qt::green);
            setFormat(start + 1, count, format);
        }
    }
}

void Editor::mousePressEvent(QMouseEvent *e)
{
    QPlainTextEdit::mousePressEvent(e);
    if (e->modifiers() == Qt::ControlModifier && !m_blocks.isEmpty())
        helpJumpToPlayer();
}

void Editor::keyPressEvent(QKeyEvent *event)
{

    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_R)
        createChangeSpeakerDialog();
    else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_T)
        createTimePropagationDialog();

    if ((m_speakerCompleter && m_speakerCompleter->popup()->isVisible())
            || (m_textCompleter && m_textCompleter->popup()->isVisible())) {
        // The following keys are forwarded by the completer to the widget
       switch (event->key()) {
       case Qt::Key_Enter:
       case Qt::Key_Return:
       case Qt::Key_Escape:
       case Qt::Key_Tab:
       case Qt::Key_Backtab:
            event->ignore();
            return; // let the completer do default behavior
       default:
           break;
       }
    }

    TextEditor::keyPressEvent(event);

    QString blockText = textCursor().block().text();
    QString textTillCursor = blockText.left(textCursor().positionInBlock()).trimmed();
    QString completionPrefix;

    const bool shortcutPressed = (event->key() == Qt::Key_N && event->modifiers() == Qt::ControlModifier);
    const bool hasModifier = (event->modifiers() != Qt::NoModifier);
    const bool containsSpeakerBraces = blockText.leftRef(blockText.indexOf(" ")).contains("]:");


    static QString eow("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-="); // end of word

    if (!shortcutPressed && (hasModifier || event->text().isEmpty() || eow.contains(event->text().right(1)))) {
        m_speakerCompleter->popup()->hide();
        m_textCompleter->popup()->hide();
        return;
    }

    QCompleter *m_completer = nullptr;

    if (!textTillCursor.count(" ") && !textTillCursor.contains("]:") && !textTillCursor.contains("]")
            && textTillCursor.size() && containsSpeakerBraces) {
        //complete speaker

        m_completer = m_speakerCompleter;

        completionPrefix = blockText.left(blockText.indexOf(" "));
        completionPrefix = completionPrefix.mid(1, completionPrefix.size() - 3);

        QList<QString> speakers;
        for (auto& a_block: qAsConst(m_blocks))
            if (!speakers.contains(a_block.speaker) && a_block.speaker != "")
                speakers.append(a_block.speaker);

        m_speakerCompleter->setModel(new QStringListModel(speakers, m_speakerCompleter));
    }
    else if (textTillCursor.count(" ") > 0) {
        //complete text
        if (m_blocks[textCursor().blockNumber()].timeStamp.isValid()
                && textTillCursor.count(" ") == blockText.count(" "))
            return;

        QTextCursor tc = textCursor();
        tc.select(QTextCursor::WordUnderCursor);
        completionPrefix = tc.selectedText();

        if (completionPrefix.size() < 2) {
            m_textCompleter->popup()->hide();
            return;
        }

        m_completer = m_textCompleter;
    }
    else
        return;

    if (!m_completer)
        return;


    if (completionPrefix != m_completer->completionPrefix()) {
        m_completer->setCompletionPrefix(completionPrefix);
        m_completer->popup()->setCurrentIndex(m_completer->completionModel()->index(0, 0));
    }

    QRect cr = cursorRect();
    cr.setWidth(m_completer->popup()->sizeHintForColumn(0)
                + m_completer->popup()->verticalScrollBar()->sizeHint().width());
    m_completer->complete(cr);
}

void Editor::openTranscript()
{
    QFileDialog fileDialog(this);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setWindowTitle(tr("Open File"));
    fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).value(0, QDir::homePath()));

    if (fileDialog.exec() == QDialog::Accepted) {
        QUrl *fileUrl = new QUrl(fileDialog.selectedUrls().constFirst());
        if (m_file)
            delete m_file;
        m_file = new QFile(fileUrl->toLocalFile());

        if (!m_file->open(QIODevice::ReadOnly)) {
            emit message(m_file->errorString());
            return;
        }

        loadTranscriptData(m_file);

        if (m_transcriptLang == "sanskrit") {
            m_dictionary = listFromFile(":/wordlists/sanskrit_wordlist.txt");
            m_textCompleter->setModel(new QStringListModel(m_dictionary, m_textCompleter));
            m_textCompletionName = "text_sanskrit";
        }
        else{
            m_dictionary = listFromFile(":/wordlists/english_wordlist.txt");
            m_textCompleter->setModel(new QStringListModel(m_dictionary, m_textCompleter));
            m_textCompletionName = "text_english";
        }

        setContent();

        if (m_transcriptLang != "")
            emit message("Opened transcript " + fileUrl->fileName() + " Language: " + m_transcriptLang);
        else
            emit message("Opened transcript " + fileUrl->fileName());
    }
}

void Editor::saveTranscript()
{
    QFileDialog fileDialog(this);
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.setWindowTitle(tr("Save Transcript"));
    fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).value(0, QDir::homePath()));

    if (fileDialog.exec() == QDialog::Accepted) {
        auto fileUrl = QUrl(fileDialog.selectedUrls().constFirst());

        if (!document()->isEmpty()) {
            auto *file = new QFile(fileUrl.toLocalFile());
            if (!file->open(QIODevice::WriteOnly)) {
                emit message(file->errorString());
                return;
            }
            saveXml(file);
        }
    }
}

void Editor::showBlocksFromData()
{
    for (auto& m_block: qAsConst(m_blocks)) {
        qDebug() << m_block.timeStamp << m_block.speaker << m_block.text << m_block.tagList;
        for (auto& m_word: qAsConst(m_block.words)) {
            qDebug() << "   " << m_word.timeStamp << m_word.text;
        }
    }
}

void Editor::highlightTranscript(const QTime& elapsedTime)
{
    qint64 blockToHighlight = -1;
    qint64 wordToHighlight = -1;

    if (!m_blocks.isEmpty()) {
        for (int i=0; i < m_blocks.size(); i++) {
            if (m_blocks[i].timeStamp > elapsedTime) {
                blockToHighlight = i;
                break;
            }
        }
    }

    if (blockToHighlight != highlightedBlock) {
        highlightedBlock = blockToHighlight;
        if (!m_highlighter)
            m_highlighter = new Highlighter(document());
        m_highlighter->setBlockToHighlight(blockToHighlight);
    }

    if (blockToHighlight == -1)
        return;

    for (int i = 0; i < m_blocks[blockToHighlight].words.size(); i++) {
        if (m_blocks[blockToHighlight].words[i].timeStamp > elapsedTime) {
            wordToHighlight = i;
            break;
        }
    }

    if (wordToHighlight != highlightedWord) {
        highlightedWord = wordToHighlight;
        m_highlighter->setWordToHighlight(wordToHighlight);
    }
}

QTime Editor::getTime(const QString& text)
{
    if (text.contains(".")) {
        if (text.count(":") == 2) return QTime::fromString(text, "h:m:s.z");
        return QTime::fromString(text, "m:s.z");
    }
    else {
        if (text.count(":") == 2) return QTime::fromString(text, "h:m:s");
        return QTime::fromString(text, "m:s");
    }
    return {};
}

word Editor::makeWord(const QTime& t, const QString& s)
{
    word w = {t, s};
    return w;
}

// TODO: Space in a speaker name breaks speaker detection
block Editor::fromEditor(qint64 blockNumber) const
{
    QTime timeStamp;
    QVector<word> words;
    QString text, speaker, blockText(document()->findBlockByNumber(blockNumber).text());

    QRegularExpressionMatch match = timeStampExp.match(blockText);
    if (match.hasMatch()) {
        QString matchedTimeStampString = match.captured();
        if (blockText.mid(match.capturedEnd()).trimmed() == "") {
            // Get timestamp for string after removing the enclosing []
            timeStamp = getTime(matchedTimeStampString.mid(1,matchedTimeStampString.size() - 2));
            text = blockText.split(matchedTimeStampString)[0];
        }
    }

    match = speakerExp.match(blockText);
    if (match.hasMatch()) {
        speaker = match.captured();
        if (text != "")
            text = text.split(speaker)[1];
        speaker = speaker.left(speaker.size() - 2);
        speaker = speaker.right(speaker.size() - 1);
    }

    if (text == "")
        text = blockText.trimmed();
    else
        text = text.trimmed();

    auto list = text.split(" ");
    for (auto& m_word: qAsConst(list)) {
        words.append(makeWord(QTime(), m_word));
    }

    block b = {timeStamp, text, speaker, QStringList(), words};
    return b;
}

void Editor::loadTranscriptData(QFile *file)
{
    QXmlStreamReader reader(file);
    m_transcriptLang = "";
    m_blocks.clear();
    if (reader.readNextStartElement()) {
        if (reader.name() == "transcript") {
            m_transcriptLang = reader.attributes().value("lang").toString();

            while(reader.readNextStartElement()) {
                if(reader.name() == "line") {
                    auto blockTimeStamp = getTime(reader.attributes().value("timestamp").toString());
                    auto blockText = QString("");
                    auto blockSpeaker = reader.attributes().value("speaker").toString();
                    auto tagString = reader.attributes().value("tags").toString();
                    QStringList tagList;
                    if (tagString != "")
                        tagList = tagString.split(",");

                    struct block line = {blockTimeStamp, "", blockSpeaker, tagList, QVector<word>()};
                    while(reader.readNextStartElement()){
                        if(reader.name() == "word"){
                            auto wordTimeStamp  = getTime(reader.attributes().value("timestamp").toString());
                            auto wordText       = reader.readElementText();

                            blockText += (wordText + " ");
                            line.words.append(makeWord(wordTimeStamp, wordText));
                        }
                        else
                            reader.skipCurrentElement();
                    }
                    line.text = blockText.trimmed();
                    m_blocks.append(line);
                }
                else
                    reader.skipCurrentElement();
            }
        }
        else
            reader.raiseError(QObject::tr("Incorrect file"));
    }
}

void Editor::saveXml(QFile* file)
{
    QXmlStreamWriter writer(file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("transcript");

    for (auto& a_block: qAsConst(m_blocks)) {
        if (a_block.text != "") {
            auto timeStamp = a_block.timeStamp;
            QString timeStampString = timeStamp.toString("hh:mm:ss.zzz");
            auto speaker = a_block.speaker;

            writer.writeStartElement("line");
            writer.writeAttribute("timestamp", timeStampString);
            writer.writeAttribute("speaker", speaker);

            for (auto& a_word: qAsConst(a_block.words)) {
                writer.writeStartElement("word");
                writer.writeAttribute("timestamp", a_word.timeStamp.toString("hh:mm:ss.zzz"));
                writer.writeCharacters(a_word.text);
                writer.writeEndElement();
            }
            writer.writeEndElement();
        }
    }
    writer.writeEndElement();
    file->close();
    delete file;
}

void Editor::helpJumpToPlayer()
{
    auto currentBlockNumber = textCursor().blockNumber();
    auto timeToJump = QTime(0, 0);

    if (m_blocks[currentBlockNumber].timeStamp.isNull())
        return;

    int positionInBlock = textCursor().positionInBlock();
    auto blockText = textCursor().block().text();
    auto textBeforeCursor = blockText.left(positionInBlock);
    int wordNumber = textBeforeCursor.count(" ");
    if (m_blocks[currentBlockNumber].speaker != "" || textCursor().block().text().contains("[]:"))
        wordNumber--;

    for (int i = currentBlockNumber - 1; i >= 0; i--) {
        if (m_blocks[i].timeStamp.isValid()) {
            timeToJump = m_blocks[i].timeStamp;
            break;
        }
    }

    // If we can jump to a word, then do so
    if (wordNumber >= 0 &&
        wordNumber < m_blocks[currentBlockNumber].words.size() &&
        m_blocks[currentBlockNumber].words[wordNumber].timeStamp.isValid()
        ) {
        for (int i = wordNumber - 1; i >= 0; i--) {
            if (m_blocks[currentBlockNumber].words[i].timeStamp.isValid()) {
                timeToJump = m_blocks[currentBlockNumber].words[i].timeStamp;
                emit jumpToPlayer(timeToJump);
                return;
            }
        }
    }

    emit jumpToPlayer(timeToJump);
}

QStringList Editor::listFromFile(const QString& fileName) const
{
    QStringList words;

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly))
        return {};

    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        if (!line.isEmpty())
            words << QString::fromUtf8(line.trimmed());
    }

    return words;
}

void Editor::setContent()
{
    if (!settingContent) {
        settingContent = true;

        if (m_highlighter)
            delete m_highlighter;

        QString content("");
        for (auto& a_block: qAsConst(m_blocks)) {
            auto blockText = "[" + a_block.speaker + "]: " + a_block.text + " [" + a_block.timeStamp.toString("hh:mm:ss.zzz") + "]";
            content.append(blockText + "\n");
        }
        setPlainText(content.trimmed());

        m_highlighter = new Highlighter(document());

        QList<int> invalidBlocks;
        QMultiMap<int, int> invalidWords;
        for (int i = 0; i < m_blocks.size(); i++) {
            if (m_blocks[i].timeStamp.isNull())
                invalidBlocks.append(i);
            else {
                for (int j = 0; j < m_blocks[i].words.size(); j++) {
                    auto wordText = m_blocks[i].words[j].text.toLower();
                    if (!std::binary_search(m_dictionary.begin(),
                                            m_dictionary.end(),
                                            wordText))
                        invalidWords.insert(i, j);
                }
            }
        }

        m_highlighter->setInvalidBlocks(invalidBlocks);
        m_highlighter->setInvalidWords(invalidWords);
        m_highlighter->setBlockToHighlight(highlightedBlock);
        m_highlighter->setWordToHighlight(highlightedWord);

        settingContent = false;
    }
}

void Editor::contentChanged(int position, int charsRemoved, int charsAdded)
{
    // If chars aren't added or deleted then return
    if (!(charsAdded || charsRemoved) || settingContent)
        return;
    else if (!m_blocks.size()) { // If block data is empty (i.e. no file opened) just fill them from editor
        for (int i = 0; i < document()->blockCount(); i++)
            m_blocks.append(fromEditor(i));
        return;
    }

    qInfo() << "[Content Modified]"
            << QString("position: %1").arg(position)
            << QString("chars-removed: %1").arg(charsRemoved)
            << QString("chars-added: %1").arg(charsAdded)
            << QString("current-block-number: %1").arg(textCursor().blockNumber())
            << QString("block-count-change: %1").arg(m_blocks.size() - blockCount());

    if (m_highlighter)
        delete m_highlighter;
    m_highlighter = new Highlighter(this->document());

    int currentBlockNumber = textCursor().blockNumber();

    if(m_blocks.size() != blockCount()) {
        auto blocksChanged = m_blocks.size() - blockCount();
        if (blocksChanged > 0) { // Blocks deleted
            for (int i = 1; i <= blocksChanged; i++)
                m_blocks.removeAt(currentBlockNumber + 1);
        }
        else { // Blocks added
            for (int i = 1; i <= -blocksChanged; i++) {
                if (document()->findBlockByNumber(currentBlockNumber + blocksChanged).text().trimmed() == "")
                    m_blocks.insert(currentBlockNumber + blocksChanged, fromEditor(currentBlockNumber - i));
                else
                    m_blocks.insert(currentBlockNumber + blocksChanged + 1, fromEditor(currentBlockNumber - i + 1));
            }
        }
    }
    
    auto currentBlockFromEditor = fromEditor(currentBlockNumber);
    auto& currentBlockFromData = m_blocks[currentBlockNumber];

    if (currentBlockFromData.speaker != currentBlockFromEditor.speaker)
        currentBlockFromData.speaker = currentBlockFromEditor.speaker;

    if (currentBlockFromData.timeStamp != currentBlockFromEditor.timeStamp)
        currentBlockFromData.timeStamp = currentBlockFromEditor.timeStamp;

    if (currentBlockFromData.text != currentBlockFromEditor.text) {
        currentBlockFromData.text = currentBlockFromEditor.text;
        auto tagList = currentBlockFromData.tagList;

        auto& wordsFromEditor = currentBlockFromEditor.words;
        auto& wordsFromData = currentBlockFromData.words;

        int wordsDifference = wordsFromEditor.size() - wordsFromData.size();
        int diffStart{-1}, diffEnd{-1};

        for (int i = 0; i < wordsFromEditor.size() && i < wordsFromData.size(); i++)
            if (wordsFromEditor[i].text != wordsFromData[i].text) {
                diffStart = i;
                break;
            }

        if (diffStart == -1)
            diffStart = wordsFromEditor.size() - 1;
        for (int i = 0; i <= diffStart; i++)
            if (i < wordsFromData.size())
                wordsFromEditor[i].timeStamp = wordsFromData[i].timeStamp;
        if (!wordsDifference) {
            for (int i = diffStart; i < wordsFromEditor.size(); i++)
                wordsFromEditor[i].timeStamp = wordsFromData[i].timeStamp;
        }

        if (wordsDifference > 0) {
            for (int i = wordsFromEditor.size() - 1, j = wordsFromData.size() - 1; j > diffStart; i--, j--)
                if (wordsFromEditor[i].text == wordsFromData[j].text)
                    wordsFromEditor[i].timeStamp = wordsFromData[j].timeStamp;
        }
        else if (wordsDifference < 0) {
            for (int i = wordsFromEditor.size() - 1, j = wordsFromData.size() - 1; i > diffStart; i--, j--)
                if (wordsFromEditor[i].text == wordsFromData[j].text)
                    wordsFromEditor[i].timeStamp = wordsFromData[j].timeStamp;
        }

        currentBlockFromData = currentBlockFromEditor;
        currentBlockFromData.tagList = tagList;
    }

    m_highlighter->setBlockToHighlight(highlightedBlock);
    m_highlighter->setWordToHighlight(highlightedWord);

    QList<int> invalidBlocks;
    QMultiMap<int, int> invalidWords;
    for (int i = 0; i < m_blocks.size(); i++) {
        if (m_blocks[i].timeStamp.isNull())
            invalidBlocks.append(i);
        else {
            for (int j = 0; j < m_blocks[i].words.size(); j++) {
                auto wordText = m_blocks[i].words[j].text.toLower();
                if (!std::binary_search(m_dictionary.begin(),
                                        m_dictionary.end(),
                                        wordText))
                    invalidWords.insert(i, j);
            }
        }
    }
    m_highlighter->setInvalidBlocks(invalidBlocks);
    m_highlighter->setInvalidWords(invalidWords);

    updateWordEditor();
}

void Editor::jumpToHighlightedLine()
{
    if (highlightedBlock == -1)
        return;
    QTextCursor cursor(document()->findBlockByNumber(highlightedBlock));
    setTextCursor(cursor);
}

void Editor::splitLine(const QTime& elapsedTime)
{
    auto cursor = textCursor();
    if (cursor.blockNumber() != highlightedBlock)
        return;

    int positionInBlock = cursor.positionInBlock();
    auto blockText = cursor.block().text();
    auto textBeforeCursor = blockText.left(positionInBlock);
    auto textAfterCursor = blockText.right(blockText.size() - positionInBlock);
    auto cutWordLeft = textBeforeCursor.split(" ").last();
    auto cutWordRight = textAfterCursor.split(" ").first();
    int wordNumber = textBeforeCursor.count(" ");

    if (m_blocks[highlightedBlock].speaker != "" || blockText.contains("[]:"))
        wordNumber--;
    if (wordNumber < 0 || wordNumber >= m_blocks[highlightedBlock].words.size())
        return;

    if (textBeforeCursor.contains("]:"))
        textBeforeCursor = textBeforeCursor.split("]:").last();
    if (textAfterCursor.contains("["))
        textAfterCursor = textAfterCursor.split("[").first();

    auto timeStampOfCutWord = m_blocks[highlightedBlock].words[wordNumber].timeStamp;
    QVector<word> words;
    int sizeOfWordsAfter = m_blocks[highlightedBlock].words.size() - wordNumber - 1;

    if (cutWordRight != "")
        words.append(makeWord(timeStampOfCutWord, cutWordRight));
    for (int i = 0; i < sizeOfWordsAfter; i++) {
        words.append(m_blocks[highlightedBlock].words[wordNumber + 1]);
        m_blocks[highlightedBlock].words.removeAt(wordNumber + 1);
    }

    if (cutWordLeft == "")
        m_blocks[highlightedBlock].words.removeAt(wordNumber);
    else {
        m_blocks[highlightedBlock].words[wordNumber].text = cutWordLeft;
        m_blocks[highlightedBlock].words[wordNumber].timeStamp = elapsedTime;
    }

    block blockToInsert = {m_blocks[highlightedBlock].timeStamp,
                           textAfterCursor.trimmed(),
                           m_blocks[highlightedBlock].speaker,
                           m_blocks[highlightedBlock].tagList,
                           words};
    m_blocks.insert(highlightedBlock + 1, blockToInsert);

    m_blocks[highlightedBlock].text = textBeforeCursor.trimmed();
    m_blocks[highlightedBlock].timeStamp = elapsedTime;

    setContent();
    updateWordEditor();
}

void Editor::mergeUp()
{
    auto blockNumber = textCursor().blockNumber();
    auto previousBlockNumber = blockNumber - 1;

    if (m_blocks.isEmpty() || blockNumber == 0 || m_blocks[blockNumber].speaker != m_blocks[previousBlockNumber].speaker)
        return;

    auto currentWords = m_blocks[blockNumber].words;

    m_blocks[previousBlockNumber].words.append(currentWords);                 // Add current words to previous block
    m_blocks[previousBlockNumber].timeStamp = m_blocks[blockNumber].timeStamp;  // Update time stamp of previous block
    m_blocks[previousBlockNumber].text.append(" " + m_blocks[blockNumber].text);// Append text to previous block

    m_blocks.removeAt(blockNumber);
    setContent();
    updateWordEditor();

    QTextCursor cursor(document()->findBlockByNumber(previousBlockNumber));
    setTextCursor(cursor);
    centerCursor();
}

void Editor::mergeDown()
{
    auto blockNumber = textCursor().blockNumber();
    auto nextBlockNumber = blockNumber + 1;

    if (m_blocks.isEmpty() || blockNumber == m_blocks.size() - 1 || m_blocks[blockNumber].speaker != m_blocks[nextBlockNumber].speaker)
        return;

    auto currentWords = m_blocks[blockNumber].words;

    auto temp = m_blocks[nextBlockNumber].words;
    m_blocks[nextBlockNumber].words = currentWords;
    m_blocks[nextBlockNumber].words.append(temp);

    auto tempText = m_blocks[nextBlockNumber].text;
    m_blocks[nextBlockNumber].text = m_blocks[blockNumber].text;
    m_blocks[nextBlockNumber].text.append(" " + tempText);

    m_blocks.removeAt(blockNumber);
    setContent();
    updateWordEditor();

    QTextCursor cursor(document()->findBlockByNumber(blockNumber));
    setTextCursor(cursor);
    centerCursor();
}

void Editor::createChangeSpeakerDialog()
{
    if (m_changeSpeaker || !m_blocks.size())
        return;

    m_changeSpeaker = new ChangeSpeakerDialog(this);
    m_changeSpeaker->setModal(true);

    QSet<QString> speakers;
    for (auto& a_block: qAsConst(m_blocks))
        speakers.insert(a_block.speaker);

    m_changeSpeaker->addItems(speakers.values());
    m_changeSpeaker->setCurrentSpeaker(m_blocks.at(textCursor().blockNumber()).speaker);

    connect(m_changeSpeaker,
            &ChangeSpeakerDialog::accepted,
            this,
            [&]() {
                changeSpeaker(m_changeSpeaker->speaker(), m_changeSpeaker->replaceAll());
                delete m_changeSpeaker;
                m_changeSpeaker = nullptr;
            }
    );
    connect(m_changeSpeaker, &ChangeSpeakerDialog::rejected, this, [&]() {delete m_changeSpeaker; m_changeSpeaker = nullptr;});
    connect(m_changeSpeaker, &ChangeSpeakerDialog::finished, this, [&]() {delete m_changeSpeaker; m_changeSpeaker = nullptr;});

    m_changeSpeaker->show();
}

void Editor::createTimePropagationDialog()
{
    if (m_propagateTime || !m_blocks.size())
        return;

    m_propagateTime = new TimePropagationDialog(this);
    m_propagateTime->setModal(true);

    m_propagateTime->setBlockRange(textCursor().blockNumber() + 1, blockCount());

    connect(m_propagateTime,
            &TimePropagationDialog::accepted,
            this,
            [&]() {
                propagateTime(m_propagateTime->time(),
                              m_propagateTime->blockStart(),
                              m_propagateTime->blockEnd(),
                              m_propagateTime->negateTime());
                delete m_propagateTime;
                m_propagateTime = nullptr;
            }
    );
    connect(m_propagateTime, &TimePropagationDialog::rejected, this, [&]() {delete m_propagateTime; m_propagateTime = nullptr;});
    connect(m_propagateTime, &TimePropagationDialog::finished, this, [&]() {delete m_propagateTime; m_propagateTime = nullptr;});

    m_propagateTime->show();
}

void Editor::createTagSelectionDialog()
{
    if (m_selectTag || !m_blocks.size())
        return;

    m_selectTag = new TagSelectionDialog(this);
    m_selectTag->setModal(true);

    m_selectTag->markExistingTags(m_blocks[textCursor().blockNumber()].tagList);

    connect(m_selectTag,
            &TagSelectionDialog::accepted,
            this,
            [&]() {
                selectTags(m_selectTag->tagList());
                delete m_selectTag;
                m_selectTag = nullptr;
            }
    );
    connect(m_selectTag, &TimePropagationDialog::rejected, this, [&]() {delete m_selectTag; m_selectTag = nullptr;});
    connect(m_selectTag, &TimePropagationDialog::finished, this, [&]() {delete m_selectTag; m_selectTag = nullptr;});

    m_selectTag->show();
}

void Editor::insertTimeStamp(const QTime& elapsedTime)
{
    auto blockNumber = textCursor().blockNumber();

    if (m_blocks.size() <= blockNumber)
        return;

    m_blocks[blockNumber].timeStamp = elapsedTime;

    dontUpdateWordEditor = true;
    setContent();
    QTextCursor cursor(document()->findBlockByNumber(blockNumber));
    setTextCursor(cursor);
    centerCursor();
    dontUpdateWordEditor = false;

}

void Editor::insertTimeStampInWordEditor(const QTime& elapsedTime)
{
    auto blockNumber = textCursor().blockNumber();
    auto wordEditorBlockNumber = m_wordEditor->textCursor().blockNumber();

    if (m_blocks.size() <= blockNumber || m_blocks[blockNumber].words.size() <= wordEditorBlockNumber)
        return;

    m_blocks[blockNumber].words[wordEditorBlockNumber].timeStamp = elapsedTime;

    updateWordEditor();

    QTextCursor cursor(m_wordEditor->document()->findBlockByNumber(wordEditorBlockNumber));
    m_wordEditor->setTextCursor(cursor);
    m_wordEditor->centerCursor();
}

void Editor::speakerWiseJump(const QString& jumpDirection)
{
    auto& blockNumber = highlightedBlock;

    if (blockNumber == -1) {
        emit message("Highlighted block not present");
        return;
    }

    auto speakerName = m_blocks[blockNumber].speaker;
    int blockToJump{-1};

    if (jumpDirection == "up") {
        for (int i = blockNumber - 1; i >= 0; i--)
            if (speakerName == m_blocks[i].speaker) {
                blockToJump = i;
                break;
            }
    }
    else if (jumpDirection == "down") {
        for (int i = blockNumber + 1; i < m_blocks.size(); i++)
            if (speakerName == m_blocks[i].speaker) {
                blockToJump = i;
                break;
            }
    }

    if (blockToJump == -1) {
        emit message("Couldn't find a block to jump");
        return;
    }

    QTime timeToJump(0, 0);

    for (int i = blockToJump - 1; i >= 0; i--) {
        if (m_blocks[i].timeStamp.isValid()) {
            timeToJump = m_blocks[i].timeStamp;
            break;
        }
    }

    emit jumpToPlayer(timeToJump);
}

void Editor::wordWiseJump(const QString& jumpDirection)
{
    auto& wordNumber = highlightedWord;

    if (highlightedBlock == -1 || wordNumber == -1) {
        emit message("Highlighted block or word not present");
        return;
    }

    auto& highlightedBlockWords = m_blocks[highlightedBlock].words;
    QTime timeToJump;
    int wordToJump{-1};

    if (jumpDirection == "left")
        wordToJump = wordNumber - 1;
    else if (jumpDirection == "right")
        wordToJump = wordNumber + 1;

    if (wordToJump < 0 || wordToJump >= highlightedBlockWords.size()) {
        emit message("Can't jump, end of block reached!", 2000);
        return;
    }

    if (jumpDirection == "left") {
        if (wordToJump == 0){
            timeToJump = QTime(0, 0);
            for (int i = highlightedBlock - 1; i >= 0; i--) {
                if (m_blocks[i].timeStamp.isValid()) {
                    timeToJump = m_blocks[i].timeStamp;
                    break;
                }
            }
        }
        else {
            for (int i = wordToJump - 1; i >= 0; i--)
                if (highlightedBlockWords[i].timeStamp.isValid()) {
                    timeToJump = highlightedBlockWords[i].timeStamp;
                    break;
                }
        }
    }
    
    if (jumpDirection == "right")
        timeToJump = highlightedBlockWords[wordToJump - 1].timeStamp;

    if (timeToJump.isNull()) {
        emit message("Couldn't find a word to jump to");
        return;
    }

    emit jumpToPlayer(timeToJump);
}


void Editor::blockWiseJump(const QString& jumpDirection)
{
    if (highlightedBlock == -1)
        return;

    int blockToJump{-1};

    if (jumpDirection == "up")
        blockToJump = highlightedBlock - 1;
    else if (jumpDirection == "down")
        blockToJump = highlightedBlock + 1;

    if (blockToJump == -1 || blockToJump == blockCount())
        return;

    QTime timeToJump;

    if (jumpDirection == "up") {
        timeToJump = QTime(0, 0);
        for (int i = blockToJump - 1; i >= 0; i--) {
            if (m_blocks[i].timeStamp.isValid()) {
                timeToJump = m_blocks[i].timeStamp;
                break;
            }
        }
    }
    else if (jumpDirection == "down")
        timeToJump = m_blocks[highlightedBlock].timeStamp;

    emit jumpToPlayer(timeToJump);

}

word Editor::fromWordEditor(qint64 blockNumber) const
{
    QTime timeStamp;
    QString text, blockText(m_wordEditor->document()->findBlockByNumber(blockNumber).text());

    QRegularExpressionMatch match = timeStampExp.match(blockText);
    if (match.hasMatch()) {
        QString matchedTimeStampString = match.captured();
        if (blockText.mid(match.capturedEnd()).trimmed() == "") {
            timeStamp = getTime(matchedTimeStampString.mid(1,matchedTimeStampString.size() - 2));
            text = blockText.split(matchedTimeStampString)[0];
        }
    }

    if (text == "")
        text = blockText.trimmed();
    else
        text = text.trimmed();

    if (text.contains(" "))
        text = text.split(" ").first();

    word w = {timeStamp, text};
    return w;
}

void Editor::updateWordEditor()
{
    if (!m_wordEditor || dontUpdateWordEditor)
        return;

    updatingWordEditor = true;

    auto blockNumber = textCursor().blockNumber();

    if (blockNumber >= m_blocks.size()) {
        m_wordEditor->clear();
        return;
    }

    auto& words = m_blocks[blockNumber].words;
    QString content;

    for (auto& a_word: qAsConst(words)) {
        content += a_word.text + " [" + a_word.timeStamp.toString("hh:mm:ss.zzz") + "]\n";
    }

    m_wordEditor->setPlainText(content.trimmed());

    updatingWordEditor = false;
}

void Editor::wordEditorChanged(int position, int charsRemoved, int charsAdded)
{
    auto editorBlockNumber = textCursor().blockNumber();

    if (document()->isEmpty() || m_blocks.isEmpty())
        m_blocks.append(fromEditor(0));

    if (!(charsAdded || charsRemoved) || settingContent || updatingWordEditor || editorBlockNumber >= m_blocks.size())
        return;

    auto& block = m_blocks[editorBlockNumber];
    if (!block.words.size()) {
        for (int i = 0; i < m_wordEditor->document()->blockCount(); i++)
            block.words.append(fromWordEditor(i));
        return;
    }

    auto& words = block.words;
    words.clear();

    QString blockText;
    for (int i = 0; i < m_wordEditor->blockCount(); i++) {
        auto wordToAppend = fromWordEditor(i);
        blockText += wordToAppend.text + " ";
        words.append(wordToAppend);
    }
    block.text = blockText.trimmed();

    dontUpdateWordEditor = true;
    setContent();
    QTextCursor cursor(document()->findBlockByNumber(editorBlockNumber));
    setTextCursor(cursor);
    centerCursor();
    dontUpdateWordEditor = false;
}

void Editor::changeSpeaker(const QString& newSpeaker, bool replaceAllOccurrences)
{
    if (m_blocks.isEmpty())
        return;
    auto blockNumber = textCursor().blockNumber();
    auto blockSpeaker = m_blocks[blockNumber].speaker;

    if (!replaceAllOccurrences)
        m_blocks[blockNumber].speaker = newSpeaker;
    else {
        for (auto& a_block: m_blocks){
            if (a_block.speaker == blockSpeaker)
                a_block.speaker = newSpeaker;
        }
    }

    setContent();
    QTextCursor cursor(document()->findBlockByNumber(blockNumber));
    setTextCursor(cursor);
    centerCursor();
}

void Editor::propagateTime(const QTime& time, int start, int end, bool negateTime)
{
    if (time.isNull()) {
        QMessageBox errorBox(QMessageBox::Critical, "Error", "Invalid Time Selected", QMessageBox::Ok);
        errorBox.exec();
        return;
    }
    else if (start < 1 || end > blockCount() || start > end) {
        QMessageBox errorBox(QMessageBox::Critical, "Error", "Invalid Block Range Selected", QMessageBox::Ok);
        errorBox.exec();
        return;
    }

    for (int i = start - 1; i < end; i++) {
        auto& currentTimeStamp = m_blocks[i].timeStamp;

        if (currentTimeStamp.isNull())
            currentTimeStamp = QTime(0, 0, 0);

        int secondsToAdd = time.hour() * 3600 + time.minute() * 60 + time.second();
        int msecondsToAdd = time.msec();

        if (negateTime) {
            secondsToAdd = -secondsToAdd;
            msecondsToAdd = -msecondsToAdd;
        }

        currentTimeStamp = currentTimeStamp.addMSecs(msecondsToAdd);
        currentTimeStamp = currentTimeStamp.addSecs(secondsToAdd);

    }

    int blockNumber = textCursor().blockNumber();

    setContent();
    QTextCursor cursor(document()->findBlockByNumber(blockNumber));
    setTextCursor(cursor);
    centerCursor();
}

void Editor::selectTags(const QStringList& newTagList)
{
    m_blocks[textCursor().blockNumber()].tagList = newTagList;

    emit refreshTagList(newTagList);
}

void Editor::insertSpeakerCompletion(const QString& completion)
{
    if (m_speakerCompleter->widget() != this)
        return;
    QTextCursor tc = textCursor();
    int extra = completion.length() - m_speakerCompleter->completionPrefix().length();

    if (m_speakerCompleter->completionPrefix().length()) {
        tc.movePosition(QTextCursor::Left);
        tc.movePosition(QTextCursor::EndOfWord);
    }

    tc.insertText(completion.right(extra));
    setTextCursor(tc);
}

void Editor::insertTextCompletion(const QString& completion)
{
    if (m_textCompleter->widget() != this)
        return;
    QTextCursor tc = textCursor();
    int extra = completion.length() - m_textCompleter->completionPrefix().length();

    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra));

    setTextCursor(tc);
}
