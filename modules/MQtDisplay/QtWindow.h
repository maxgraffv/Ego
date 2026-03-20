#ifndef QT_WINDOW_H
#define QT_WINDOW_H

#include <QMainWindow>
#include "VideoWidget.h"

class QLabel;
class QTimer;

class QtWindow : public QMainWindow
{
    Q_OBJECT

public:
    QtWindow();

private:
    QLabel* m_label;
    VideoWidget* m_videoWidget;
    QTimer* m_timer;
};

#endif // QT_WINDOW_H