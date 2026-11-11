#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QtMqtt/qmqttclient.h>
#include <QVector>
#include <QSet>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QMqttClient *cliente;

    QVector<int> valores;
    QSet<int> nodos;

    int idSeleccionado = -1;

    void actualizarGrafica();
};

#endif

