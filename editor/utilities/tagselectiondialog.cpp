#include "tagselectiondialog.h"
#include "ui_tagselectiondialog.h"

#include <QDebug>

TagSelectionDialog::TagSelectionDialog(QWidget* parent)
    : QDialog(parent),
    ui (new Ui::TagSelectionDialog)
{
    ui->setupUi(this);
    ui->label->setHidden(true);
    ui->lineEdit_language->setHidden(true);

    connect(ui->checkBox_lang, &QCheckBox::stateChanged, this,
    [&]()
    {
        ui->label->setVisible(!ui->label->isVisible());
        ui->lineEdit_language->setVisible(!ui->lineEdit_language->isVisible());
    }
    );
}

TagSelectionDialog::~TagSelectionDialog()
{
    delete ui;
}


QStringList TagSelectionDialog::tagList() const
{
    QStringList currentTagList;

    if (ui->checkBox_invs->isChecked())
        currentTagList << "InvS";
    if (ui->checkBox_noisy->isChecked())
        currentTagList << "Noisy";
    if (ui->checkBox_slacked->isChecked())
        currentTagList << "Slacked";
    if (ui->checkBox_l1infl->isChecked())
        currentTagList << "L1Infl";
    if (ui->checkBox_mltsp->isChecked())
        currentTagList << "MltSp";
    if (ui->checkBox_lang->isChecked() && ui->lineEdit_language->text() != "") {
        currentTagList << QString("Lang_%1").arg(ui->lineEdit_language->text());
    }

    return currentTagList;
}

void TagSelectionDialog::markExistingTags(const QStringList& existingTagsList)
{
    if (existingTagsList.contains("InvS"))
        ui->checkBox_invs->setChecked(true);
    if (existingTagsList.contains("Noisy"))
        ui->checkBox_noisy->setChecked(true);
    if (existingTagsList.contains("Slacked"))
        ui->checkBox_slacked->setChecked(true);
    if (existingTagsList.contains("L1Infl"))
        ui->checkBox_l1infl->setChecked(true);
    if (existingTagsList.contains("MltSp"))
        ui->checkBox_mltsp->setChecked(true);

    auto subList = existingTagsList.filter("Lang_");
    if (!subList.isEmpty()) {
        ui->checkBox_lang->setChecked(true);

        auto langText = subList.first();
        ui->lineEdit_language->setText(langText.mid(5));
    }
}
