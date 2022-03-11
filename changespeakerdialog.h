#pragma once

#include <QDialog>
#include "ui_changespeakerdialog.h"

namespace Ui {
    class ChangeSpeakerDialog;
}

class ChangeSpeakerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChangeSpeakerDialog(QWidget* parent = nullptr): QDialog(parent), ui(new Ui::ChangeSpeakerDialog)
    {
        ui->setupUi(this);
    }

    ~ChangeSpeakerDialog() {delete ui;}

    QString speaker() 
    {
        return ui->comboBox_speaker->currentText();
    }
    
    bool replaceAll() 
    {
        return ui->checkBox_changeAllOccurences->isChecked();
    }
    
    void addItems(const QStringList& speakers) 
    {
        ui->comboBox_speaker->addItems(speakers);
    }

private:
    Ui::ChangeSpeakerDialog* ui;
};
