#pragma once

#include <QMediaPlayer>
#include <QWidget>
#include <QAbstractButton>
#include <QAbstractSlider>
#include <QComboBox>
#include <QLabel>

class PlayerControls : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerControls(QWidget *parent = nullptr);

    QMediaPlayer::State state() const;
    int volume() const;
    bool isMuted() const;
    qreal playbackRate() const;

public slots:
    void setState(QMediaPlayer::State state);
    void setVolume(int volume);
    void setMuted(bool muted);
    void setPlaybackRate(float rate);
    void setPositionInfo(QString info);
    void setPositionSliderDuration(qint64 duration);
    void setPositionSliderPosition(qint64 position);

signals:
    void play();
    void pause();
    void stop();
    void seekForward();
    void seekBackward();
    void changeVolume(int volume);
    void changeMuting(bool muting);
    void changeRate(qreal rate);
    void changePosition(qint64 position);

private slots:
    void playClicked();
    void muteClicked();
    void updateRate();
    void onVolumeSliderValueChanged();
    void onPositionSliderMoved(int position);

private:
    QMediaPlayer::State m_playerState = QMediaPlayer::StoppedState;
    bool m_playerMuted = false;
    QAbstractButton *m_playButton = nullptr;
    QAbstractButton *m_stopButton = nullptr;
    QAbstractButton *m_seekForwardButton = nullptr;
    QAbstractButton *m_seekBackwardButton = nullptr;
    QAbstractButton *m_muteButton = nullptr;
    QAbstractSlider *m_volumeSlider = nullptr;
    QAbstractSlider *m_positionSlider = nullptr;
    QLabel *m_positionLabel = nullptr;
    QComboBox *m_rateBox = nullptr;
};
