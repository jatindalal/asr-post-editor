#include "tool.h"
#include "./ui_tool.h"

#include "mediaplayer.h"

tool::tool(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::tool)
{
    ui->setupUi(this);
    ui->m_playerControls->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    player = new MediaPlayer(this);
    player->setVideoOutput(ui->m_videoWidget);

    connect(ui->close, &QAction::triggered, this, &QMainWindow::close);

    // Connect Player Controls and Media Player
    connect(ui->player_open, &QAction::triggered, player, &MediaPlayer::open);
    connect(ui->player_togglePlay,
            &QAction::triggered,
            player,
            [=]() {
                    if (player->state() == MediaPlayer::PausedState || player->state() == MediaPlayer::StoppedState)
                        player->play();
                    else if (player->state() == MediaPlayer::PlayingState)
                        player->pause();
                  }
            );
    connect(ui->m_playerControls, &PlayerControls::play, player, &QMediaPlayer::play);
    connect(ui->m_playerControls, &PlayerControls::pause, player, &QMediaPlayer::pause);
    connect(ui->m_playerControls, &PlayerControls::stop, player, &QMediaPlayer::stop);
    connect(ui->m_playerControls, &PlayerControls::seekForward, player, [=]() {player->seek(5);});
    connect(ui->m_playerControls, &PlayerControls::seekBackward, player, [=]() {player->seek(-5);});
    connect(ui->m_playerControls, &PlayerControls::changeVolume, player, &QMediaPlayer::setVolume);
    connect(ui->m_playerControls, &PlayerControls::changeMuting, player, &QMediaPlayer::setMuted);
    connect(ui->m_playerControls, &PlayerControls::changeRate, player, &QMediaPlayer::setPlaybackRate);
    connect(ui->m_playerControls, &PlayerControls::stop, ui->m_videoWidget, QOverload<>::of(&QVideoWidget::update));
    connect(player, &QMediaPlayer::stateChanged, ui->m_playerControls, &PlayerControls::setState);
    connect(player, &QMediaPlayer::volumeChanged, ui->m_playerControls, &PlayerControls::setVolume);
    connect(player, &QMediaPlayer::mutedChanged, ui->m_playerControls, &PlayerControls::setMuted);
    connect(player, &MediaPlayer::positionChanged, ui->m_playerControls, &PlayerControls::setPositionSliderPosition);
    connect(player, &MediaPlayer::durationChanged, ui->m_playerControls, &PlayerControls::setPositionSliderDuration);
    connect(ui->m_playerControls, &PlayerControls::changePosition, player, &MediaPlayer::setPosition);
    connect(player, &MediaPlayer::positionChanged, ui->m_playerControls, [=]() {ui->m_playerControls->setPositionInfo(player->getPositionInfo());});
    connect(player, &MediaPlayer::message, this->statusBar(), &QStatusBar::showMessage);
    connect(player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &tool::handleMediaPlayerError);

    // Connect Editor controls
    connect(ui->editor_openTranscript, &QAction::triggered, ui->m_editor, &Editor::openTranscript);
    connect(ui->editor_debugBlocks, &QAction::triggered, ui->m_editor, &Editor::showBlocksFromData);
    connect(ui->editor_save, &QAction::triggered, ui->m_editor, &Editor::saveTranscript);
    connect(ui->editor_jumpToLine, &QAction::triggered, ui->m_editor, &Editor::jumpToHighlightedLine);
    connect(ui->editor_splitLine, &QAction::triggered, ui->m_editor, [=]() {ui->m_editor->splitLine(player->elapsedTime());});
    connect(ui->editor_mergeUp, &QAction::triggered, ui->m_editor, &Editor::mergeUp);
    connect(ui->editor_mergeDown, &QAction::triggered, ui->m_editor, &Editor::mergeDown);
    connect(ui->m_editor, &Editor::message, this->statusBar(), &QStatusBar::showMessage);
    connect(ui->m_editor, &Editor::jumpToPlayer, player, &MediaPlayer::setPositionToTime);

    // Connect Player elapsed time to highlight Editor
    connect(player, &MediaPlayer::positionChanged, ui->m_editor, [=]() {ui->m_editor->highlightTranscript(player->elapsedTime());});
}

tool::~tool()
{
    delete player;
    delete ui;
}

void tool::handleMediaPlayerError()
{
    const QString errorString = player->errorString();
    QString message = "Error: ";
    if (errorString.isEmpty())
        message += " #" + QString::number(int(player->error()));
    else
        message += errorString;
    statusBar()->showMessage(message);
}

void tool::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space && event->modifiers() == Qt::ControlModifier) {
        if (player->state() == MediaPlayer::PausedState || player->state() == MediaPlayer::StoppedState)
            player->play();
        else if (player->state() == MediaPlayer::PlayingState)
            player->pause();
    }
    else if (event->key() == Qt::Key_Period && event->modifiers() == Qt::ControlModifier)
        player->seek(5);
    else if (event->key() == Qt::Key_Comma && event->modifiers() == Qt::ControlModifier)
        player->seek(-5);
    else if (event->key() == Qt::Key_Semicolon && event->modifiers() == Qt::ControlModifier)
        ui->m_editor->splitLine(player->elapsedTime());
    else if (event->key() == Qt::Key_J && event->modifiers() == Qt::ControlModifier)
        ui->m_editor->jumpToHighlightedLine();
    else if (event->key() == Qt::Key_S && event->modifiers() == Qt::ControlModifier)
        ui->m_editor->saveTranscript();
    else if (event->key() == Qt::Key_F && event->modifiers() == Qt::ControlModifier)
        ui->m_editor->findReplace();
    else
        QMainWindow::keyPressEvent(event);
}
