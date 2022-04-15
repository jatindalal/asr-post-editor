#pragma once

#include <QMainWindow>
#include "mediaplayer/mediaplayer.h"
#include "editor/texteditor.h"


QT_BEGIN_NAMESPACE
namespace Ui { class Tool; }
QT_END_NAMESPACE

class Tool : public QMainWindow
{
    Q_OBJECT

public:
    explicit Tool(QWidget *parent = nullptr);
    ~Tool();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void handleMediaPlayerError();
    void createKeyboardShortcutGuide();
    void changeEditorFont();

private:
    MediaPlayer *player = nullptr;
    Ui::Tool *ui;
};
