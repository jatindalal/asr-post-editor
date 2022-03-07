#include "changespeakerdialog.h"
#include "ui_changespeakerdialog.h"

#include <QDebug>

ChangeSpeakerDialog::ChangeSpeakerDialog(QWidget* parent)
    : QDialog(parent),
      ui(new Ui::ChangeSpeakerDialog)
{
    ui->setupUi(this);

    ui->comboBox_speaker->setEditable(true);
}

ChangeSpeakerDialog::~ChangeSpeakerDialog()
{
    delete ui;
}

QString ChangeSpeakerDialog::speaker()
{
    return ui->comboBox_speaker->currentText();
}

bool ChangeSpeakerDialog::replaceAll()
{
    return ui->checkBox_changeAllOccurences->isChecked();
}

void ChangeSpeakerDialog::addItems(const QStringList& speakers)
{
    ui->comboBox_speaker->addItems(speakers);
}
