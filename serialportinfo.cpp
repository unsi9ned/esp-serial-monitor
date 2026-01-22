#include "serialportinfo.h"

SerialPortInfo::SerialPortInfo()
{

}

QList<QString> SerialPortInfo::standardBaudRates()
{
    QList<qint32> brList = QSerialPortInfo::standardBaudRates();
    QList<QString> brStrList;

    foreach(qint32 br, brList)
    {
        brStrList.append(QString::number(br));
    }

    return brStrList;
}

QSerialPortInfo SerialPortInfo::info(const QString &portName)
{
    QSerialPortInfo info;

    foreach(QSerialPortInfo curr, QSerialPortInfo::availablePorts())
    {
        if(curr.portName() == portName)
        {
            info = curr;
            break;
        }
    }

    return info;
}
