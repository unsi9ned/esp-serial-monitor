#ifndef SERIALPORTINFO_H
#define SERIALPORTINFO_H

#include <QObject>
#include <QString>
#include <QSerialPortInfo>

class SerialPortInfo : public QSerialPortInfo
{
public:
    SerialPortInfo();
    static QList<QString> standardBaudRates();
    static QSerialPortInfo info(const QString &portName);
};

#endif // SERIALPORTINFO_H

