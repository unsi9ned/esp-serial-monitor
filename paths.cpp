#include "paths.h"

Paths::Paths(QObject *parent) : QObject(parent)
{

}

QString Paths::arduinoSecondaryBootloaderName()
{
    QString filename = Paths::arduinoSecondaryBootloader();
    int slashPos = filename.lastIndexOf(QChar('/'));

    slashPos = slashPos == -1 ? filename.lastIndexOf(QChar('\\')) : slashPos;
    slashPos = slashPos == -1 ? 0 : slashPos + 1;
    filename = filename.mid(slashPos);

    return filename;
}
