#include "tool.h"
#include "./ui_tool.h"
#include "editor/utilities/keyboardshortcutguide.h"

#include <QFontDialog>

Tool::Tool(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Tool)
{
    ui->setupUi(this);

    player = new MediaPlayer(this);
    player->setVideoOutput(ui->m_videoWidget);

    ui->splitter_tool->setCollapsible(0, false);
    ui->splitter_tool->setCollapsible(1, false);
    ui->splitter_tool->setSizes(QList<int>({static_cast<int>(0.15 * sizeHint().height()),
                                            static_cast<int>(0.85 * sizeHint().height())}));

    ui->splitter_editor->setCollapsible(0, false);
    ui->splitter_editor->setCollapsible(1, false);
    ui->splitter_editor->setSizes(QList<int>({static_cast<int>(0.7 * sizeHint().height()),
                                            static_cast<int>(0.3 * sizeHint().height())}));

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
    connect(ui->edit_undo, &QAction::triggered, ui->m_editor, [&]() {ui->m_editor->undo();});
    connect(ui->edit_redo, &QAction::triggered, ui->m_editor, [&]() {ui->m_editor->redo();});
    connect(ui->edit_cut, &QAction::triggered, ui->m_editor, [&]() {ui->m_editor->cut();});
    connect(ui->edit_copy, &QAction::triggered, ui->m_editor, [&]() {ui->m_editor->copy();});
    connect(ui->edit_paste, &QAction::triggered, ui->m_editor, [&]() {ui->m_editor->paste();});
    connect(ui->edit_findReplace, &QAction::triggered, ui->m_editor, [&]() {ui->m_editor->findReplace();});

    // Connect view menu actions
    connect(ui->view_zoomIn, &QAction::triggered, ui->m_editor, [&]() {ui->m_editor->zoomIn();});
    connect(ui->view_zoomOut, &QAction::triggered, ui->m_editor, [&]() {ui->m_editor->zoomOut();});
    connect(ui->view_font, &QAction::triggered, this, &Tool::changeEditorFont);
    connect(ui->view_toggleTagList, &QAction::triggered, this, [&]() {ui->m_tagListDisplay->setVisible(!ui->m_tagListDisplay->isVisible());});

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
    connect(ui->editor_changeSpeaker, &QAction::triggered, ui->m_editor, &Editor::createChangeSpeakerDialog);
    connect(ui->editor_propagateTime, &QAction::triggered, ui->m_editor, &Editor::createTimePropagationDialog);
    connect(ui->editor_editTags, &QAction::triggered, ui->m_editor, &Editor::createTagSelectionDialog);
    connect(ui->m_editor, &Editor::message, this->statusBar(), &QStatusBar::showMessage);
    connect(ui->m_editor, &Editor::jumpToPlayer, player, &MediaPlayer::setPositionToTime);
    connect(ui->m_editor, &Editor::refreshTagList, ui->m_tagListDisplay, &TagListDisplayWidget::refreshTags);

    connect(ui->help_keyboardShortcuts, &QAction::triggered, this, &Tool::createKeyboardShortcutGuide);

    // Connect position slider change to player position
    connect(ui->slider_position, &QSlider::sliderMoved, player, &MediaPlayer::setPosition);
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

     if (event->key() == Qt::Key_Up && event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))
        ui->m_editor->speakerWiseJump("up");
    else if (event->key() == Qt::Key_Down && event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))
        ui->m_editor->speakerWiseJump("down");
    else if (event->key() == Qt::Key_Left && event->modifiers() == Qt::AltModifier)
        ui->m_editor->wordWiseJump("left");
    else if (event->key() == Qt::Key_Right && event->modifiers() == Qt::AltModifier)
        ui->m_editor->wordWiseJump("right");
    else if (event->key() == Qt::Key_Up && event->modifiers() == Qt::AltModifier )
        ui->m_editor->blockWiseJump("up");
    else if (event->key() == Qt::Key_Down && event->modifiers() == Qt::AltModifier)
        ui->m_editor->blockWiseJump("down");
    else if (event->key() == Qt::Key_I && event->modifiers() == Qt::ControlModifier) {
        if (ui->m_editor->hasFocus())
            ui->m_editor->insertTimeStamp(player->elapsedTime());
        else if(ui->m_wordEditor->hasFocus())
            ui->m_wordEditor->insertTimeStamp(player->elapsedTime());
    }
    else
        QMainWindow::keyPressEvent(event);
}

void Tool::createKeyboardShortcutGuide()
{
    auto help_keyshortcuts = new KeyboardShortcutGuide(this);

    help_keyshortcuts->setAttribute(Qt::WA_DeleteOnClose);
    help_keyshortcuts->show();
}

void Tool::changeEditorFont()
{
    bool fontSelected;
    QFont font = QFontDialog::getFont(&fontSelected, QFont( "Arial", 18 ), this, tr("Pick a font") );

    if (fontSelected) {
        ui->m_editor->document()->setDefaultFont(font);
        ui->m_wordEditor->setFont(font);
        ui->m_tagListDisplay->setFont(font);
    }
}

