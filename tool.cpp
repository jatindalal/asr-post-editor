#include "tool.h"
#include "./ui_tool.h"

#include <QDebug>

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

    //connect Editor controls
    connect(ui->editor_open, &QAction::triggered, ui->m_editor, &Editor::open);
    connect(ui->editor_openTranscript, &QAction::triggered, ui->m_editor, &Editor::openTranscript);
    connect(ui->m_editor, &Editor::message, this->statusBar(), &QStatusBar::showMessage);
}

tool::~tool()
{
    delete player;
    delete ui;
}

