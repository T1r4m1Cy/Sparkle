#include <QApplication>
#include <QMainWindow>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Sparkle Editor");
    mainWindow.resize(1280, 720);
    mainWindow.show();

    return app.exec();
}