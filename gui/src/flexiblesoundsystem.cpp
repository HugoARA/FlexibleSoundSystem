#include "flexiblesoundsystem.h"
#include "ui_flexiblesoundsystem.h"

#include "gui.h"

#include <QSpinBox>
#include <QRadioButton>
#include <QLabel>
#include <QPushButton>
#include <QTreeView>
#include <QDial>

FlexibleSoundSystem::FlexibleSoundSystem(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::FlexibleSoundSystem),
    m_gui(new CGui(this)),
    m_nclients(0),
    m_jack_mode(true)
{
    ui->setupUi(this);
    setup();
}

FlexibleSoundSystem::~FlexibleSoundSystem()
{
    delete ui;
    delete m_gui;
}

void FlexibleSoundSystem::setup() {
    ui->spinBoxNDevices->setReadOnly(true);
    ui->radioButtonJack->setChecked(true);

    ui->labelArtistEdit->setText("none");
    ui->labelNameEdit->setText("none");
    ui->labelAlbumEdit->setText("none");
    ui->labelGenreEdit->setText("none");

    ui->pushButtonPlayPause->setDisabled(true);
    ui->pushButtonStop->setDisabled(true);

    ui->dialEQHigh->setRange(0, 127);
    ui->dialEQMid->setRange(0, 127);
    ui->dialEQLow->setRange(0, 127);
    ui->dialFXFlanger->setRange(0, 127);
    ui->dialFXHPF->setRange(0, 127);
    ui->dialFXLPF->setRange(0, 127);

    setWindowState(Qt::WindowMaximized);
}

void FlexibleSoundSystem::incrementNClient() {
    ui->spinBoxNDevices->setValue(++m_nclients);
}

void FlexibleSoundSystem::decrementNClient() {
    if (m_nclients > 0) {
        ui->spinBoxNDevices->setValue(--m_nclients);
    }
}

void FlexibleSoundSystem::toggleOutputMode() {
    m_jack_mode = !m_jack_mode;

    if (m_jack_mode == true) {
        ui->radioButtonJack->setChecked(true);
    } else {
        ui->radioButtonStream->setChecked(true);
    }
}

void FlexibleSoundSystem::moveCursorUp() {
    ui->treeView->moveUp();
}

void FlexibleSoundSystem::moveCursorDown() {
    ui->treeView->moveDown();
}

void FlexibleSoundSystem::selectItem() {
    std::string name = ui->treeView->select();

    if (name.empty()) {
        // is a directory
    } else {
        // new music chosen
        if (m_gui->isMusicLoaded()) {
            stop();
            m_gui->removeMusic();
        }
        if (m_gui->loadMusic(name)) {
            setMusicInfo(m_gui->getMusicInfo());
            ui->graphicsViewArtwork->setImage(m_gui->getArtwork());
            m_gui->startDecoding();
            ui->pushButtonPlayPause->setEnabled(true);
        } else {
            std::cerr << "couldn't load music..." << std::endl;
        }
    }
}

void FlexibleSoundSystem::updateKnob(int knob, int value) {
    switch (knob) {
    case 1:
        ui->dialEQHigh->setValue(value);
        break;
    case 2:
        ui->dialEQMid->setValue(value);
        break;
    case 3:
        ui->dialEQLow->setValue(value);
        break;
    case 4:
        ui->dialFXFlanger->setValue(value);
        break;
    case 5:
        ui->dialFXHPF->setValue(value);
        break;
    case 6:
        ui->dialFXLPF->setValue(value);
        break;
    default:
        break;
    }
}

void FlexibleSoundSystem::play() {
    if (m_gui->isReproducing()) {
        m_gui->pauseMusic();
        ui->pushButtonPlayPause->setText("Play");
    } else {
        m_gui->playMusic();
        ui->pushButtonPlayPause->setText("Pause");
        ui->pushButtonStop->setEnabled(true);
    }
}

void FlexibleSoundSystem::stop() {
    if (m_gui->isReproducing()) {
        m_gui->stopMusic();
        handleStop();
    }
}

void FlexibleSoundSystem::handleStop() {
    ui->pushButtonPlayPause->setText("Play");
    ui->pushButtonStop->setEnabled(false);
}

void FlexibleSoundSystem::setMusicInfo(const SMusic& music) {
    ui->labelArtistEdit->setText(tr(music.m_artist.c_str()));
    ui->labelNameEdit->setText(tr(music.m_title.c_str()));
    ui->labelAlbumEdit->setText(tr(music.m_album.c_str()));
    ui->labelGenreEdit->setText(tr(music.m_genre.c_str()));
}
