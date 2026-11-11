
#ifndef GRAFICA_H
#define GRAFICA_H

#include <QWidget>
#include <QVector>

class Grafica : public QWidget
{
    Q_OBJECT

public:
    explicit Grafica(QWidget *parent = nullptr);
    void agregarEstado(bool encendido);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<int> valores;
};

#endif
