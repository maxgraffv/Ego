#include "QtWindow.h"

#include <QApplication>
#include <QDebug>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

QtWindow::QtWindow()
    : m_label(nullptr),
      m_videoWidget(nullptr),
      m_frameReadyNotifier(nullptr),
      shm_name(std::getenv(SharedFrameIPC::kShmNameEnv)),
      fd(-1),
      event_fd(-1),
      mem(nullptr)
{
    setWindowTitle("QtWindow Process");
    resize(800, 600);

    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    auto* layout = new QVBoxLayout(central);

    m_label = new QLabel("Separate Qt Process", central);
    m_label->setAlignment(Qt::AlignCenter);

    m_videoWidget = new VideoWidget(central);

    layout->addWidget(m_label);
    layout->addWidget(m_videoWidget, 1);

    if (shm_name == nullptr)
    {
        qFatal("Missing shared memory name in environment");
    }

    const char* event_fd_text = std::getenv(SharedFrameIPC::kEventFdEnv);
    if (event_fd_text == nullptr)
    {
        qFatal("Missing frame-ready eventfd in environment");
    }

    event_fd = std::atoi(event_fd_text);
    if (event_fd < 0)
    {
        qFatal("Invalid frame-ready eventfd");
    }

    fd = shm_open(shm_name, O_RDONLY, 0666);
    if (fd == -1)
    {
        qFatal("shm_open failed: %s", std::strerror(errno));
    }

    mem = mmap(nullptr,
               SharedFrameIPC::kMappingSize,
               PROT_READ,
               MAP_SHARED,
               fd,
               0);

    if (mem == MAP_FAILED)
    {
        mem = nullptr;
        qFatal("mmap failed: %s", std::strerror(errno));
    }

    frame.resize(SharedFrameIPC::kMaxFrameBytes);

    m_frameReadyNotifier = new QSocketNotifier(event_fd, QSocketNotifier::Read, this);
    connect(m_frameReadyNotifier, &QSocketNotifier::activated, this, [this]() {
        consumeFrame();
    });
}

QtWindow::~QtWindow()
{
    if (mem != nullptr)
    {
        munmap(mem, SharedFrameIPC::kMappingSize);
    }

    if (fd != -1)
    {
        ::close(fd);
    }
}

void QtWindow::consumeFrame()
{
    std::uint64_t pending_frames = 0;
    const ssize_t read_result = read(event_fd, &pending_frames, sizeof(pending_frames));
    if (read_result != sizeof(pending_frames))
    {
        qWarning() << "eventfd read failed:" << std::strerror(errno);
        return;
    }

    SharedFrameIPC::Header* meta = header();
    if (meta->channels != 3 || meta->size == 0 || meta->size > SharedFrameIPC::kMaxFrameBytes)
    {
        qWarning() << "Unsupported frame metadata"
                   << meta->width
                   << meta->height
                   << meta->channels
                   << meta->size;
        return;
    }

    frame.resize(meta->size);
    std::memcpy(frame.data(), payload(), meta->size);

    QImage img(frame.data(),
               static_cast<int>(meta->width),
               static_cast<int>(meta->height),
               static_cast<int>(meta->width * meta->channels),
               QImage::Format_BGR888);
    m_videoWidget->setFrame(img.copy());
}

SharedFrameIPC::Header* QtWindow::header()
{
    return static_cast<SharedFrameIPC::Header*>(mem);
}

std::uint8_t* QtWindow::payload()
{
    return static_cast<std::uint8_t*>(mem) + sizeof(SharedFrameIPC::Header);
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QtWindow window;
    window.show();

    return app.exec();
}
