#ifndef FLEXIBLESOUNDSYSTEM_H
#define FLEXIBLESOUNDSYSTEM_H

#include <QMainWindow>
#include <vector>

class CGui;
struct SMusic;

namespace Ui {
class FlexibleSoundSystem;
}

class FlexibleSoundSystem : public QMainWindow
{
    Q_OBJECT

public:
    explicit FlexibleSoundSystem(QWidget *parent = 0);
    ~FlexibleSoundSystem();

    void incrementNClient();
    void decrementNClient();
    void setMusicInfo(const SMusic&);
public slots:
    void toggleOutputMode();
    void moveCursorUp();
    void moveCursorDown();
    void selectItem();
    void updateKnob(int, int);
    void play();
    void stop();

    void handleStop();
private:
    Ui::FlexibleSoundSystem *ui;
    CGui* m_gui;
    int m_nclients;
    bool m_jack_mode;

    void setup();
    void setArtwork(const std::vector<uint8_t>& path);
};

#endif // FLEXIBLESOUNDSYSTEM_H
