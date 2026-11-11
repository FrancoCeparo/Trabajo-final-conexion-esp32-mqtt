#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QTimer>
#include <QPushButton>
#include <QComboBox>

#include <QtMqtt/qmqtttopicfilter.h>
#include <QtMqtt/qmqtttopicname.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cliente = new QMqttClient(this);

    valores.append(0);
    actualizarGrafica();

    // CONECTAR AL BROKER
    connect(ui->bconectar, &QPushButton::clicked, this, [this]() {

        cliente->setHostname(ui->lineip->text());
        cliente->setPort(ui->linepuerto->text().toUInt());
        cliente->setUsername(ui->lineuser->text());
        cliente->setPassword(ui->linecontra->text());

        cliente->connectToHost();

        ui->comboBox->clear();
        ui->comboBox->addItem("Conectando...");
    });

    // DESCONECTAR
    connect(ui->bdesconectar, &QPushButton::clicked, this, [this]() {

        cliente->disconnectFromHost();
    });

    // CUANDO CONECTA
    connect(cliente, &QMqttClient::connected, this, [this]() {

        ui->comboBox->clear();
        ui->comboBox->addItem("Seleccionar nodo...");

        cliente->subscribe(QMqttTopicFilter("/ej02/+/sensor"));
        cliente->subscribe(QMqttTopicFilter("/ej02/id"));
    });

    // CUANDO SE DESCONECTA
    connect(cliente, &QMqttClient::disconnected, this, [this]() {

        ui->comboBox->clear();
        ui->comboBox->addItem("Desconectado");

        idSeleccionado = -1;
        valores.clear();
        valores.append(0);
        actualizarGrafica();
    });

    // ERRORES MQTT
    connect(cliente, &QMqttClient::errorChanged, this,
            [this](QMqttClient::ClientError error) {

        qDebug() << "Error MQTT:" << error;
    });

    // RECIBIR MENSAJES
    connect(cliente, &QMqttClient::messageReceived,
            this, [this](const QByteArray &mensaje,
                         const QMqttTopicName &topico) {

        QString tema = topico.name();

        // RECIBIR ID
        if (tema == "/ej02/id") {

            int id = mensaje.toInt();

            if (id > 0 && !nodos.contains(id)) {

                nodos.insert(id);

                ui->comboBox->addItem(QString::number(id));
            }
        }

        // RECIBIR SENSOR DEL NODO SELECCIONADO aplicando el split de prfoe
        QStringList partes = tema.split("/");

        int id = partes[2].toInt();

        if (id == idSeleccionado) {
            valores.append(mensaje.toInt());

            if (valores.size() > 30)
                valores.removeFirst();

            actualizarGrafica();
        }
    });

    // BUSCAR NODOS
    connect(ui->bactualizar, &QPushButton::clicked, this, [this]() {

        if (cliente->state() != QMqttClient::Connected)
            return;

        nodos.clear();

        idSeleccionado = -1;
        valores.clear();
        valores.append(0);

        ui->comboBox->clear();
        ui->comboBox->addItem("Seleccionar nodo...");

        actualizarGrafica();

        cliente->publish(
            QMqttTopicName("/ej02/cmd"),
            QByteArray("getId")
        );

        QTimer::singleShot(1000, this, [this]() {

            if (nodos.isEmpty())
                ui->comboBox->addItem("No se encontraron nodos");
        });
    });

    // SELECCIONAR NODO
    connect(ui->comboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {

        bool ok = false;

        int id = ui->comboBox->currentText().toInt(&ok);

        idSeleccionado = ok ? id : -1;

        valores.clear();
        valores.append(0);

        actualizarGrafica();
    });

    // PRENDER LED
    connect(ui->bprender, &QPushButton::clicked, this, [this]() {

        if (cliente->state() == QMqttClient::Connected &&
            idSeleccionado != -1) {

            QString tema = QString("/ej02/%1/cmd").arg(idSeleccionado);

            cliente->publish(
                QMqttTopicName(tema),
                QByteArray("ledOn")
            );
        }
    });

    // APAGAR LED
    connect(ui->bapagar, &QPushButton::clicked, this, [this]() {

        if (cliente->state() == QMqttClient::Connected &&
            idSeleccionado != -1) {

            QString tema = QString("/ej02/%1/cmd").arg(idSeleccionado);

            cliente->publish(
                QMqttTopicName(tema),
                QByteArray("ledOff")
            );
        }
    });
}

// DIBUJAR GRAFICA
void MainWindow::actualizarGrafica()
{
    int ancho = ui->widget->width();
    int alto = ui->widget->height();

    QPixmap imagen(ancho, alto);
    imagen.fill(Qt::white);

    QPainter pintor(&imagen);
    pintor.setRenderHint(QPainter::Antialiasing);

    // EJES
    pintor.setPen(Qt::black);

    pintor.drawLine(20, 10, 20, alto - 20);
    pintor.drawLine(20, alto - 20, ancho - 10, alto - 20);

    // SEÑAL
    pintor.setPen(QPen(Qt::green, 3));

    if (valores.size() >= 2) {

        double paso = (ancho - 35.0) / (valores.size() - 1);

        int maximo = 1;

        for (int valor : valores) {
            if (valor > 1)
                maximo = 100;
        }

        for (int i = 1; i < valores.size(); i++) {

            int x1 = 20 + (i - 1) * paso;
            int x2 = 20 + i * paso;

            int y1 = alto - 20 -
                     valores[i - 1] * (alto - 50) / maximo;

            int y2 = alto - 20 -
                     valores[i] * (alto - 50) / maximo;

            pintor.drawLine(x1, y1, x2, y2);
        }
    }

    ui->widget->setPixmap(imagen);
}

MainWindow::~MainWindow()
{
    delete ui;
}

