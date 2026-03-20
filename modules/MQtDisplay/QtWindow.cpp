#include "QtWindow.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QImage>

QtWindow::QtWindow()
    : m_label(nullptr),
      m_videoWidget(nullptr),
      m_timer(nullptr)
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

    m_timer = new QTimer(this);

    connect(m_timer, &QTimer::timeout, this, [this]() {
        const int width = 640;
        const int height = 480;

        QImage img(width, height, QImage::Format_RGB888);

        for (int y = 0; y < height; ++y)
        {
            uchar* line = img.scanLine(y);

            for (int x = 0; x < width; ++x)
            {
                const int i = x * 3;

                uchar r = static_cast<uchar>((255 * x) / (width - 1));
                uchar g = static_cast<uchar>((255 * y) / (height - 1));
                uchar b = static_cast<uchar>((255 * (x + y)) / (width + height - 2));

                line[i + 0] = r;
                line[i + 1] = g;
                line[i + 2] = b;
            }
        }

        m_videoWidget->setFrame(img);
    });

    m_timer->start(33);
}


#include <QApplication>
#include "QtWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QtWindow window;
    window.show();

    return app.exec();
}