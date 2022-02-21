#pragma once

#include <QDialog>

#include "editor.h"

namespace Ui {
    class FindReplaceDialog;
}

class FindReplaceDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FindReplaceDialog(Editor *parentEditor);
    ~FindReplaceDialog();

private slots:
    void updateFlags();
    void findPrevious();
    void findNext();
    void replace();
    void replaceAll();

private:
    Editor *m_Editor = nullptr;
    Ui::FindReplaceDialog *ui;
    QTextDocument::FindFlags flags;
};
