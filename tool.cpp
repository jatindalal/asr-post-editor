#include "tool.h"
#include "./ui_tool.h"

#include "mediaplayer.h"

Tool::Tool(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Tool)
{
    ui->setupUi(this);

    player = new MediaPlayer(this);
    player->setVideoOutput(ui->m_videoWidget);

    m_activeEditor = ui->m_editor;

    ui->m_editor->installEventFilter(this);
    ui->m_wordEditor->installEventFilter(this);

    ui->m_wordEditor->setHidden(true);

    connect(ui->close, &QAction::triggered, this, &QMainWindow::close);

    // Connect Player Controls and Media Player
    connect(ui->player_open, &QAction::triggered, player, &MediaPlayer::open);
    connect(ui->player_togglePlay, &QAction::triggered, player, &MediaPlayer::togglePlayback);
    connect(ui->player_seekForward, &QAction::triggered, player, [&]() {player->seek(5);});
    connect(ui->player_seekBackward, &QAction::triggered, player, [&]() {player->seek(-5);});
    connect(ui->m_playerControls, &PlayerControls::play, player, &QMediaPlayer::play);
    connect(ui->m_playerControls, &PlayerControls::pause, player, &QMediaPlayer::pause);
    connect(ui->m_playerControls, &PlayerControls::stop, player, &QMediaPlayer::stop);
    connect(ui->m_playerControls, &PlayerControls::seekForward, player, [&]() {player->seek(5);});
    connect(ui->m_playerControls, &PlayerControls::seekBackward, player, [&]() {player->seek(-5);});
    connect(ui->m_playerControls, &PlayerControls::changeVolume, player, &QMediaPlayer::setVolume);
    connect(ui->m_playerControls, &PlayerControls::changeMuting, player, &QMediaPlayer::setMuted);
    connect(ui->m_playerControls, &PlayerControls::changeRate, player, &QMediaPlayer::setPlaybackRate);
    connect(ui->m_playerControls, &PlayerControls::stop, ui->m_videoWidget, QOverload<>::of(&QVideoWidget::update));
    connect(player, &QMediaPlayer::stateChanged, ui->m_playerControls, &PlayerControls::setState);
    connect(player, &QMediaPlayer::volumeChanged, ui->m_playerControls, &PlayerControls::setVolume);
    connect(player, &QMediaPlayer::mutedChanged, ui->m_playerControls, &PlayerControls::setMuted);
    connect(player, &MediaPlayer::message, this->statusBar(), &QStatusBar::showMessage);
    connect(player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &Tool::handleMediaPlayerError);

    // Connect components dependent on Player's position change to player
    connect(player, &QMediaPlayer::positionChanged, this,
        [&]()
        {
            ui->slider_position->setValue(player->position());
            ui->label_position->setText(player->getPositionInfo());
            ui->m_editor->highlightTranscript(player->elapsedTime());
        }
    );

    connect(player, &QMediaPlayer::durationChanged, this,
        [&]()
        {
            ui->slider_position->setRange(0, player->duration());
            ui->label_position->setText(player->getPositionInfo());
        }
    );

    // Connect edit menu actions
    connect(ui->edit_undo, &QAction::triggered, m_activeEditor, [&]() {m_activeEditor->undo();});
    connect(ui->edit_redo, &QAction::triggered, m_activeEditor, [&]() {m_activeEditor->redo();});
    connect(ui->edit_cut, &QAction::triggered, m_activeEditor, [&]() {m_activeEditor->cut();});
    connect(ui->edit_copy, &QAction::triggered, m_activeEditor, [&]() {m_activeEditor->copy();});
    connect(ui->edit_paste, &QAction::triggered, m_activeEditor, [&]() {m_activeEditor->paste();});
    connect(ui->edit_findReplace, &QAction::triggered, m_activeEditor, [&]() {m_activeEditor->findReplace();});

    // Connect view menu actions
    connect(ui->view_zoomIn, &QAction::triggered, m_activeEditor, [&]() {m_activeEditor->zoomIn();});
    connect(ui->view_zoomOut, &QAction::triggered, m_activeEditor, [&]() {m_activeEditor->zoomOut();});

    // Connect Editor menu actions and editor controls
    ui->m_editor->setWordEditor(ui->m_wordEditor);

    connect(ui->editor_openTranscript, &QAction::triggered, ui->m_editor, &Editor::openTranscript);
    connect(ui->editor_debugBlocks, &QAction::triggered, ui->m_editor, &Editor::showBlocksFromData);
    connect(ui->editor_save, &QAction::triggered, ui->m_editor, &Editor::saveTranscript);
    connect(ui->editor_jumpToLine, &QAction::triggered, ui->m_editor, &Editor::jumpToHighlightedLine);
    connect(ui->editor_splitLine, &QAction::triggered, ui->m_editor, [&]() {ui->m_editor->splitLine(player->elapsedTime());});
    connect(ui->editor_mergeUp, &QAction::triggered, ui->m_editor, &Editor::mergeUp);
    connect(ui->editor_mergeDown, &QAction::triggered, ui->m_editor, &Editor::mergeDown);
    connect(ui->editor_toggleWords, &QAction::triggered, ui->m_wordEditor, [&](){ui->m_wordEditor->setVisible(!ui->m_wordEditor->isVisible());});
    connect(ui->m_editor, &Editor::message, this->statusBar(), &QStatusBar::showMessage);
    connect(ui->m_editor, &Editor::jumpToPlayer, player, &MediaPlayer::setPositionToTime);

    // Connect position slider change to player position
    connect(ui->slider_position, &QSlider::sliderMoved, player, [&](){player->setPosition(ui->slider_position->value());});
}

Tool::~Tool()
{
    delete player;
    delete ui;
}

void Tool::handleMediaPlayerError()
{
    const QString errorString = player->errorString();
    QString message = "Error: ";
    if (errorString.isEmpty())
        message += " #" + QString::number(int(player->error()));
    else
        message += errorString;
    statusBar()->showMessage(message);
}

void Tool::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space && event->modifiers() == Qt::ControlModifier)
        player->togglePlayback();
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
    else if (event->key() == Qt::Key_Up && event->modifiers() == Qt::ControlModifier)
        ui->m_editor->mergeUp();
    else if (event->key() == Qt::Key_Down && event->modifiers() == Qt::ControlModifier)
        ui->m_editor->mergeDown();
    else if (event->key() == Qt::Key_W && event->modifiers() == Qt::ControlModifier)
        ui->m_wordEditor->setVisible(!ui->m_wordEditor->isVisible());
    else
        QMainWindow::keyPressEvent(event);
}

bool Tool::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::FocusIn) {
        if (watched == ui->m_wordEditor) {
            m_activeEditor = ui->m_wordEditor;
            qDebug() << "word editor";
        }
        else if (watched == ui->m_editor) {
            m_activeEditor = ui->m_editor;
            qDebug() << "editor";
        }
    }
    return false;
}
