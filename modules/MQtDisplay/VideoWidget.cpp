#include "VideoWidget.h"

#include <QPainter>
#include <QPaintEvent>

VideoWidget::VideoWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(1920, 1080);
}

void VideoWidget::setFrame(const QImage& frame)
{
    m_frame = frame;
    update();
}

void VideoWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (!m_frame.isNull())
    {
        painter.drawImage(rect(), m_frame);
    }
}