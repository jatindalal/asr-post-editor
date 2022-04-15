#include "wordeditor.h"

#include <QHeaderView>

WordEditor::WordEditor(QWidget* parent)
    : QTableWidget(parent)
{
    setColumnCount(3);

    setHorizontalHeaderItem(0, new QTableWidgetItem("Text"));
    setHorizontalHeaderItem(1, new QTableWidgetItem("End Time"));
    setHorizontalHeaderItem(2, new QTableWidgetItem("Tags"));

    fitTableContents();
}

QVector<word> WordEditor::currentWords() const
{
    QVector<word> wordsToReturn;

    for (int i = 0; i < rowCount(); i++) {
        auto text = item(i, 0)->text();
        auto timeStamp = getTime(item(i, 1)->text());
        auto tagList = getTagList(item(i, 2)->text());

        wordsToReturn.append(word {timeStamp, text, tagList});
    }

    return wordsToReturn;
}

void WordEditor::refreshWords(const QVector<word>& words)
{
    clear();

    if (words.isEmpty())
        return;

    setRowCount(words.size());

    int counter = 0;
    for (auto& a_word: words) {
        auto text = a_word.text;
        auto timeStamp = a_word.timeStamp;
        auto tagList = a_word.tagList;

        setItem(counter, 0, new QTableWidgetItem(text));
        setItem(counter, 1, new QTableWidgetItem(timeStamp.toString("hh:mm:ss.zzz")));
        setItem(counter, 2, new QTableWidgetItem(tagList.join(",")));

        counter++;
    }

    fitTableContents();
}

void WordEditor::insertTimeStamp(const QTime& timeToInsert)
{
    item(currentRow(), 1)->setText(timeToInsert.toString("hh:mm:ss.zzz"));
}

void WordEditor::fitTableContents()
{
    resizeRowsToContents();
    resizeColumnsToContents();
    horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
}

QTime WordEditor::getTime(const QString& text)
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

QStringList WordEditor::getTagList(const QString& tagString)
{
    if (tagString == "")
        return {};

    QStringList tagsToReturn;

    if (tagString.contains("invw", Qt::CaseInsensitive))
        tagsToReturn << "InvW";
    if (tagString.contains("slacked", Qt::CaseInsensitive))
        tagsToReturn << "Slacked";

    return tagsToReturn;
}


