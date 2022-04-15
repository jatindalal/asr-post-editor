#pragma once

#include <QTableWidget>
#include "blockandword.h"

class WordEditor: public QTableWidget
{
    Q_OBJECT

public:
    explicit WordEditor(QWidget* parent = nullptr);
    QVector<word> currentWords() const;

public slots:
    void refreshWords(const QVector<word>& words);
    void insertTimeStamp(const QTime& timeToInsert);

private:
    void fitTableContents();
    static QTime getTime(const QString& text);
    static QStringList getTagList(const QString& tagString);
};
