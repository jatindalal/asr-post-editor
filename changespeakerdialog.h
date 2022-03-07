#pragma once

#include <QDialog>

namespace Ui {
    class ChangeSpeakerDialog;
}

class ChangeSpeakerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChangeSpeakerDialog(QWidget* parent = nullptr);
    ~ChangeSpeakerDialog();

    QString speaker();
    bool replaceAll();
    void addItems(const QStringList& speakers);

private:
    Ui::ChangeSpeakerDialog* ui;
};
