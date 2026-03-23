#ifndef QT_WINDOW_H
#define QT_WINDOW_H

#include <QMainWindow>
#include <QSocketNotifier>
#include "VideoWidget.h"
#include "SharedFrameIPC.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QImage>
#include <cstdint>
#include <vector>

class QLabel;

class QtWindow : public QMainWindow
{
    Q_OBJECT

    private:
        QLabel* m_label;
        VideoWidget* m_videoWidget;
        QSocketNotifier* m_frameReadyNotifier;
        const char* shm_name;
        int fd;
        int event_fd;
        void* mem;
        std::vector<uint8_t> frame;
        void consumeFrame();
        SharedFrameIPC::Header* header();
        std::uint8_t* payload();

    public:
        QtWindow();
        ~QtWindow();

};

#endif // QT_WINDOW_H
