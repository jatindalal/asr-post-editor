#include "editor.h"

#include <QPainter>
#include <QTextBlock>
#include <QFileDialog>
#include <QStandardPaths>
#include <QRegularExpression>

Editor::Editor(QWidget *parent) : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, &Editor::blockCountChanged, this, &Editor::updateLineNumberAreaWidth);
    connect(this, &Editor::updateRequest, this, &Editor::updateLineNumberArea);
    connect(this, &Editor::cursorPositionChanged, this, &Editor::highlightCurrentLine);
    connect(this->document(), &QTextDocument::contentsChange, this, &Editor::contentChanged);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int Editor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;

    return space;
}

void Editor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void Editor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void Editor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void Editor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(150, 150, 150);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void Editor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);


    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());


    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void Editor::mousePressEvent(QMouseEvent *e)
{
    QPlainTextEdit::mousePressEvent(e);
    emit editorMouseClicked();
}

void Editor::open()
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

        QString content = m_file->readAll();

        if (!settingContent) {
            settingContent = true;
            setPlainText(content);
            settingContent = false;
        }
        emit message("Opened file " + fileUrl->fileName());
    }

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
        if (!settingContent) {
            settingContent = true;
            QString content("");
            for (auto& block: qAsConst(m_blocks)) {
                auto blockText = "[" + block.speaker + "]: " + block.text + " [" + block.timeStamp.toString("hh:mm:ss.zzz") + "]";
                content.append(blockText + "\n");
            }
            setPlainText(content.trimmed());
            settingContent = false;
        }

        emit message("Opened transcript " + fileUrl->fileName());
    }
}

void Editor::showBlocksFromData()
{
    for (auto& m_block: qAsConst(m_blocks)) {
        qDebug() << m_block.timeStamp << m_block.speaker << m_block.text;
        for (auto& m_word: qAsConst(m_block.words)) {
            qDebug() << "   " << m_word.timeStamp << m_word.text;
        }
    }
}

void Editor::highlightTranscript(const QTime& elapsedTime)
{
    qint64 blockToHighlight = -1;
    // Check if we need to highlight different block
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

Editor::word Editor::makeWord(const QTime& t, const QString& s)
{
    word w = {t, s};
    return w;
}

Editor::block Editor::fromEditor(qint64 blockNumber)
{
    QTime timeStamp;
    QList<word> words;
    QString text, speaker, blockText(document()->findBlockByNumber(blockNumber).text());

    QRegularExpression timeStampExp("\\[(\\d?\\d:)?[0-5]?\\d:[0-5]?\\d(\\.\\d\\d?\\d?)?]");
    QRegularExpressionMatch match = timeStampExp.match(blockText);
    if (match.hasMatch()) {
        QString matchedTimeStampString = match.captured();
        if (blockText.mid(match.capturedEnd()).trimmed() == "") {
            timeStamp = getTime(matchedTimeStampString.mid(1,matchedTimeStampString.size() - 2));
            text = blockText.split(matchedTimeStampString)[0];
        }
    }

    QRegularExpression speakerExp("\\[[\\w\\.]*]:");
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

    block b = {timeStamp, text, speaker, words};
    return b;
}

void Editor::loadTranscriptData(QFile *file)
{
    QXmlStreamReader reader(file);
    m_blocks.clear();
    if (reader.readNextStartElement()) {
        if (reader.name() == "transcript") {
            while(reader.readNextStartElement()) {
                if(reader.name() == "line") {
                    auto blockTimeStamp = getTime(reader.attributes().value("timestamp").toString());
                    auto blockText = QString("");
                    auto blockSpeaker = reader.attributes().value("speaker").toString();

                    struct block line = {blockTimeStamp, "", blockSpeaker, QList<word>()};
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

void Editor::contentChanged(int position, int charsRemoved, int charsAdded)
{
    if ((charsAdded || charsRemoved) && !settingContent && m_blocks.size()) {
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
                if (!(m_blocks[currentBlockNumber + blocksChanged] == fromEditor(currentBlockNumber + blocksChanged)))
                    m_blocks.replace(currentBlockNumber + blocksChanged, fromEditor(currentBlockNumber + blocksChanged));
                for (int i = 1; i <= -blocksChanged; i++) {
                    if (currentBlockNumber == 1)
                        m_blocks.insert(currentBlockNumber + blocksChanged, fromEditor(currentBlockNumber - i));
                    else
                        m_blocks.insert(currentBlockNumber + blocksChanged + 1, fromEditor(currentBlockNumber - i + 1));
                }
            }
        }
        // If current block's text is changed then replace it with new data
        // TODO: If text of some word is changed or new word is added then only that should have invalid timestamp
        auto blockText = "[" + m_blocks[currentBlockNumber].speaker + "]: " +
                          m_blocks[currentBlockNumber].text +
                         " [" + m_blocks[currentBlockNumber].timeStamp.toString("hh:mm:ss.zzz") + "]";
        if (textCursor().block().text().trimmed() != blockText)
            m_blocks.replace(currentBlockNumber, fromEditor(currentBlockNumber));
    }
    else if (!m_blocks.size()) {
        for (int i = 0; i < document()->blockCount(); i++)
            m_blocks.append(fromEditor(i));
    }
}
