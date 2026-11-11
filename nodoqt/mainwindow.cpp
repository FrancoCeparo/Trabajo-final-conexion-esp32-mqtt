
#include "ui_mainwindow.h"
#include "mainwindow.h"

#include <QDial>
#include <QLabel>
#include <QTimer>
#include <QRandomGenerator>

#include <QtMqtt/qmqtttopicfilter.h>
#include <QtMqtt/qmqtttopicname.h>

#define BROKER "126.0.0.9"
#define PUERTO 1873
#define USUARIO "tu"
#define CLAVE "sabe"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    idNodo = 0;
    idAsignado = false;

    cliente = new QMqttClient(this);

    cliente->setHostname(BROKER);
    cliente->setPort(PUERTO);
    cliente->setUsername(USUARIO);
    cliente->setPassword(CLAVE);

    connect(cliente, &QMqttClient::connected, this, [this]() {

        ui->conexionmqtt->setText("conectaditoo");

        cliente->subscribe(QMqttTopicFilter("/ej02/cmd"));
        cliente->subscribe(QMqttTopicFilter("/ej02/id"));

        idsOcupados.clear();
        idAsignado = false;

        cliente->publish(QMqttTopicName("/ej02/cmd"), "getId");

        QTimer::singleShot(1000, this, [this]() {

            do {
                idNodo = QRandomGenerator::global()->bounded(6, 1025);
            } while (idsOcupados.contains(idNodo));

            idAsignado = true;

            QString tema = QString("/ej02/%1/cmd").arg(idNodo);
            cliente->subscribe(QMqttTopicFilter(tema));

            ui->conexionmqtt->setText(
                QString("Conectado - ID: %1").arg(idNodo)
            );
        });
    });

    connect(cliente, &QMqttClient::disconnected, this, [this]() {
        ui->conexionmqtt->setText("desconectado");
    });

    connect(cliente, &QMqttClient::messageReceived,
            this, [this](const QByteArray &mensaje,
                         const QMqttTopicName &topico) {

        QString tema = topico.name();
        QString contenido = QString::fromUtf8(mensaje);

        if (tema == "/ej02/id") {
            int id = contenido.toInt();

            if (!idsOcupados.contains(id))
                idsOcupados.append(id);
        }

        if (tema == "/ej02/cmd" &&
            contenido == "getId" && idAsignado) {

            cliente->publish(
                QMqttTopicName("/ej02/id"),
                QByteArray::number(idNodo)
            );
        }

        QString comando = QString("/ej02/%1/cmd").arg(idNodo);

        if (tema == comando && contenido == "ledOn") {
            ui->labeled->setText("led prendido");
            ui->labeled->setStyleSheet("background-color: green;");
        }

        if (tema == comando && contenido == "ledOff") {
            ui->labeled->setText("led apagado");
            ui->labeled->setStyleSheet("background-color: gray;");
        }
    });

    connect(ui->dial, &QDial::valueChanged,
            this, [this](int valor) {

        if (cliente->state() == QMqttClient::Connected &&
            idAsignado) {

            QString tema = QString("/ej02/%1/sensor").arg(idNodo);

            cliente->publish(
                QMqttTopicName(tema),
                QByteArray::number(valor)
            );
        }
    });

    cliente->connectToHost();
}

MainWindow::~MainWindow()
{
    delete ui;
}

