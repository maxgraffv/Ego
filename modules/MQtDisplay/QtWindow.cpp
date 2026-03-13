#include <QApplication>
#include <QMainWindow>
#include <QLabel>

class QtWindow : public QMainWindow
{
public:
    QtWindow()
    {
        setWindowTitle("QtWindow Process");
        resize(400, 200);

        QLabel* label = new QLabel("Seperate Qt Process", this);
        label->setAlignment(Qt::AlignCenter);
        setCentralWidget(label);
    }
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QtWindow window;
    window.show();

    return app.exec();
}