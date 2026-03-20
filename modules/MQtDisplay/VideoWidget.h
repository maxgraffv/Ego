#ifndef VIDEO_WIDGET_H
#define VIDEO_WIDGET_H

#include <QWidget>
#include <QImage>

class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoWidget(QWidget* parent = nullptr);

    void setFrame(const QImage& frame);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage m_frame;
};

#endif // VIDEO_WIDGET_H