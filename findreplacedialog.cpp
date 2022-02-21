#include "findreplacedialog.h"
#include "ui_findreplacedialog.h"

FindReplaceDialog::FindReplaceDialog(Editor *parentEditor)
    : QDialog (parentEditor),
      m_Editor(parentEditor),
      ui (new Ui::FindReplaceDialog)
{
    ui->setupUi(this);
    connect(ui->button_find_next, &QPushButton::clicked, this, &FindReplaceDialog::findNext);
    connect(ui->button_find_previous, &QPushButton::clicked, this, &FindReplaceDialog::findPrevious);
    connect(ui->button_replace, &QPushButton::clicked, this, &FindReplaceDialog::replace);
    connect(ui->button_replace_all, &QPushButton::clicked, this, &FindReplaceDialog::replaceAll);

    connect(ui->whole_words, &QCheckBox::toggled, this, &FindReplaceDialog::updateFlags);
    connect(ui->case_sensitive, &QCheckBox::toggled, this, &FindReplaceDialog::updateFlags);

    flags = flags | QTextDocument::FindCaseSensitively;

    QTextCursor textCursor = m_Editor->textCursor();
    if (textCursor.hasSelection())
        ui->text_find->setText(textCursor.selectedText());
    textCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor,1);
    m_Editor->setTextCursor(textCursor);
}

FindReplaceDialog::~FindReplaceDialog()
{
    delete ui;
}

void FindReplaceDialog::updateFlags()
{
    QTextDocument::FindFlags tmp;

    if (ui->whole_words->isChecked())
        tmp = tmp | QTextDocument::FindWholeWords;
    if (ui->case_sensitive->isChecked())
        tmp = tmp | QTextDocument::FindCaseSensitively;

    flags = tmp;
}

void FindReplaceDialog::findNext()
{
    QString query = ui->text_find->text();
    m_Editor->find(query, flags);
}

void FindReplaceDialog::findPrevious()
{
    QString query = ui->text_find->text();
    m_Editor->find(query, QTextDocument::FindBackward | flags);
}

void FindReplaceDialog::replace()
{
    QString replacementString = ui->text_replace->text();
    if (replacementString != "" && m_Editor->textCursor().hasSelection())
        m_Editor->textCursor().insertText(replacementString);
}

void FindReplaceDialog::replaceAll()
{
    QTextCursor textCursor = m_Editor->textCursor();
    textCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor,1);
    m_Editor->setTextCursor(textCursor);

    QString query = ui->text_find->text();
    QString replacementString = ui->text_replace->text();

    while (m_Editor->find(query, flags))
        m_Editor->textCursor().insertText(replacementString);

    ui->text_find->setText(replacementString);
}
