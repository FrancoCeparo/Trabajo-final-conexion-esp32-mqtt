
#include "grafica.h"
#include <QPainter>

Grafica::Grafica(QWidget *parent)
    : QWidget(parent)
{
    valores.append(0);
}

void Grafica::agregarEstado(bool encendido)
{
    valores.append(encendido ? 1 : 0);

    if (valores.size() > 30)
        valores.removeFirst();

    update();
}

void Grafica::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter pintor(this);

    pintor.setRenderHint(QPainter::Antialiasing);

    pintor.fillRect(rect(), Qt::white);

    int margen = 20;
    int ancho = width() - 2 * margen;
    int alto = height() - 2 * margen;

    pintor.setPen(Qt::black);

    pintor.drawLine(margen, margen,
                    margen, margen + alto);

    pintor.drawLine(margen, margen + alto,
                    margen + ancho, margen + alto);

    if (valores.size() < 2)
        return;

    QPen lapiz(Qt::green, 3);
    pintor.setPen(lapiz);

    double paso = ancho / double(valores.size() - 1);

    for (int i = 1; i < valores.size(); i++) {

        int x1 = margen + (i - 1) * paso;
        int x2 = margen + i * paso;

        int y1 = margen + alto - valores[i - 1] * alto;
        int y2 = margen + alto - valores[i] * alto;

        pintor.drawLine(x1, y1, x2, y1);
        pintor.drawLine(x2, y1, x2, y2);
    }
}
