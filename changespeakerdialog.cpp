#include "changespeakerdialog.h"
#include "ui_changespeakerdialog.h"

#include <QDebug>

ChangeSpeakerDialog::ChangeSpeakerDialog(QWidget* parent)
    : QDialog(parent),
      ui(new Ui::ChangeSpeakerDialog)
{
    ui->setupUi(this);

    ui->comboBox_speaker->addItem("Custom");

    connect(ui->comboBox_speaker, &QComboBox::currentTextChanged, this, &ChangeSpeakerDialog::speakerChanged);
}

ChangeSpeakerDialog::~ChangeSpeakerDialog()
{
    delete ui;
}

void ChangeSpeakerDialog::speakerChanged(const QString &speaker)
{
    if (speaker == "Custom") {
        ui->label_newSpeaker->setVisible(true);
        ui->lineEdit_newSpeaker->setVisible(true);
    }
    else {
        ui->label_newSpeaker->setHidden(true);
        ui->lineEdit_newSpeaker->setHidden(true);
    }
}

QString ChangeSpeakerDialog::speaker()
{
    QString selectedSpeaker = ui->comboBox_speaker->currentText();

    if (selectedSpeaker == "Custom")
        selectedSpeaker = ui->lineEdit_newSpeaker->text();

    return selectedSpeaker;
}

bool ChangeSpeakerDialog::replaceAll()
{
    return ui->checkBox_changeAllOccurences->isChecked();
}

void ChangeSpeakerDialog::addItems(const QStringList& speakers)
{
    ui->comboBox_speaker->addItems(speakers);
}
