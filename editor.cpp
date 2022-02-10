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
        settingContent = true;
        setPlainText(content);
        settingContent = false;
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
        setContent();

        emit message("Opened transcript " + fileUrl->fileName());
    }
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
                            struct word tempWord = {wordTimeStamp, wordText};

                            blockText += (wordText + " ");
                            line.words.append(tempWord);
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

void Editor::setContent()
{
    settingContent = true;
    QString content("");
    for (auto& block: qAsConst(m_blocks)) {
        auto blockText = "[" + block.speaker + "]: " + block.text + " [" + block.timeStamp.toString("hh:mm:ss.zzz") + "]";
        content.append(blockText + "\n");
    }
    setPlainText(content.trimmed());
    settingContent = false;
}

void Editor::contentChanged(int position, int charsRemoved, int charsAdded)
{
    if ((charsAdded || charsRemoved) && !settingContent) {
        if (m_highlighter)
            delete m_highlighter;
        m_highlighter = new Highlighter(this->document());

        auto currentBlockNumber = textCursor().blockNumber();
        auto blockText = "[" + m_blocks[currentBlockNumber].speaker + "]: " + m_blocks[currentBlockNumber].text + " [" + m_blocks[currentBlockNumber].timeStamp.toString("hh:mm:ss.zzz") + "]";

        if (textCursor().block().text().trimmed() != blockText)
            addBlock(currentBlockNumber);

        if(m_blocks.size() != blockCount()) {
            auto blocksChanged = m_blocks.size() - blockCount();
            if (blocksChanged > 0) { // Blocks deleted
                for (int i = 0; i < blocksChanged; i++) {
                    qDebug() << "delete block" << currentBlockNumber + i + 1;
                    m_blocks.removeAt(currentBlockNumber + i + 1);
                }
            }
            else { // Blocks added
                for (int i = 0; i < -blocksChanged; i++) {
                    qDebug() << "add block" << currentBlockNumber + i ;
                }
            }
        }
    }
}

Editor::block Editor::fromEditor(qint64 blockNumber)
{
    QRegularExpression timeStampExp("\\[(\\d?\\d:)?[0-5]?\\d:[0-5]?\\d(\\.\\d\\d?\\d?)?]");
    QRegularExpressionMatch match = timeStampExp.match(document()->findBlockByNumber(blockNumber).text());


}

void Editor::addBlock(qint64 blockNumber)
{
    qDebug() << "update block" << blockNumber;
}

void Editor::showBlocks()
{

}

