#pragma once

#include <QMainWindow>
#include "mediaplayer.h"


QT_BEGIN_NAMESPACE
namespace Ui { class tool; }
QT_END_NAMESPACE

class tool : public QMainWindow
{
    Q_OBJECT

public:
    tool(QWidget *parent = nullptr);
    ~tool();

private slots:
    void handleMediaPlayerError();

private:

    MediaPlayer *player = nullptr;
    Ui::tool *ui;
};
