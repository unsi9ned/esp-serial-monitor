#include <QMessageBox>
#include <QFile>
#include <QDebug>
#include <QFileDialog>
#include <QProcess>
#include <QScrollBar>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "version.h"
#include "paths.h"
#include "serialportinfo.h"

//------------------------------------------------------------------------------
// Конструктор главного окна
//------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    serialPort(nullptr),
    esptoolProcess(nullptr),
    portStateBeforeBurn(false),
    ui(new Ui::MainWindow)
{
    QStringList stdBaudRates = SerialPortInfo::standardBaudRates();

    ui->setupUi(this);
    this->aboutDlg = new AboutDialog(this);
    this->setWindowIcon(QIcon(":/img/hamlab.png"));
    this->setWindowTitle(this->windowTitle() + QString(" (v%1)").arg(APP_VERSION));
    ui->cbxSpeed->insertItems(0, stdBaudRates);
    ui->cbxFlashSpeed->insertItems(0, stdBaudRates);
    ui->cbxFlashSpeed->addItem("921600");
    ui->cbxFlashSpeed->setCurrentIndex(ui->cbxFlashSpeed->count() - 1);
    this->serialPort = new QSerialPort();
    this->settings.setFileName(Paths::appSettings());

    //
    // Каждую секунду проверяем были ли подключены/отключены устройства и
    // при необходимости обновляем список доступных портов
    //
    this->updatePortList();

    connect(&this->portScanTimer,
            SIGNAL(timeout()),
            SLOT(updatePortList()));
    this->portScanTimer.start(1000);

    //
    // Загрузка настроек программы
    //
    this->loadSettings();

    //
    // Обработчик принятых данных от устройства
    //
    connect(this->serialPort,
            SIGNAL(readyRead()),
            this,
            SLOT(comPortReceive()));

    //
    // Процесс для запуска утилиты esptool.exe
    //
    this->esptoolProcess = new QProcess(this);
    this->esptoolProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(this->esptoolProcess,
            SIGNAL(readyReadStandardOutput()),
            SLOT(showBurnStdOut()));

    connect(this->esptoolProcess,
            SIGNAL(readyReadStandardError()),
            SLOT(showBurnStdErr()));

    connect(this->esptoolProcess,
            SIGNAL(started()),
            SLOT(burnStart()));

    connect(this->esptoolProcess,
            SIGNAL(finished(int)),
            SLOT(burnStop()));

    //
    // Процесс для запуска утилиты чтения таблицы разделов gen_esp32part.exe
    //
    this->esp32partProcess = new QProcess(this);
    this->esp32partProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(this->esp32partProcess,
            SIGNAL(readyReadStandardOutput()),
            SLOT(showEsp32PartStdErr()));

    connect(this->esp32partProcess,
            SIGNAL(readyReadStandardError()),
            SLOT(showEsp32PartStdOut()));

    connect(ui->pushButtonAbout,
            SIGNAL(clicked()),
            this->aboutDlg,
            SLOT(open()));

    connect(ui->pushButtonClear,
            SIGNAL(clicked(bool)),
            ui->plainTextEdit,
            SLOT(clear()));

    connect(this,
            SIGNAL(statusChanged(QString)),
            ui->statusBar,
            SLOT(showMessage(QString)));
}

MainWindow::~MainWindow()
{
    this->saveSettings();

    if(this->serialPort)
    {
        this->serialPort->close();
        delete this->serialPort;
    }

    if(this->esptoolProcess)
    {
        this->esptoolProcess->close();
        delete this->esptoolProcess;
        this->esptoolProcess = nullptr;
    }

    if(this->esp32partProcess)
    {
        this->esp32partProcess->close();
        delete this->esp32partProcess;
        this->esp32partProcess = nullptr;
    }

    delete this->aboutDlg;
    delete ui;
}

//------------------------------------------------------------------------------
// Загрузка настроек программы из файла
//------------------------------------------------------------------------------
void MainWindow::loadSettings()
{
    QStringList appSettings;

    if(this->settings.open(QFile::ReadOnly))
    {
        char line[10240];
        qint64 len;

        do
        {
            len = this->settings.readLine(line, sizeof(line));

            if(len > 0)
            {
                QString qLine = QString(line).trimmed();
                appSettings.append(qLine);
            }
        }
        while(len > 0);

        this->settings.close();
    }

    for(int i = appSettings.length(); i < 5; i++)
    {
        appSettings.append("");
    }

    QString portName = appSettings.at(0);
    QString portSpeed = appSettings.at(1);
    QString chipName = appSettings.at(3);
    QString ideName = appSettings.at(4);

    portSpeed = portSpeed.isEmpty() ? "115200" : portSpeed;
    chipName = chipName.isEmpty() ? "ESP32" : chipName;
    ideName = ideName.isEmpty() ? "Arduino" : ideName;

    ui->lineEditFirmware->setText(appSettings.at(2));

    for (int i = 0; i < ui->cbxChip->count(); i++)
    {
        if(ui->cbxChip->itemText(i) == chipName)
        {
            ui->cbxChip->setCurrentIndex(i);
            break;
        }
    }

    for(int i = 0; i < ui->cbxPort->count(); i++)
    {
        if(ui->cbxPort->itemText(i) == portName)
        {
            ui->cbxPort->setCurrentIndex(i);
            break;
        }
    }

    for(int i = 0; i < ui->cbxSpeed->count(); i++)
    {
        if(ui->cbxSpeed->itemText(i) == portSpeed)
        {
            ui->cbxSpeed->setCurrentIndex(i);
            break;
        }
    }

    for(int i = 0; i < ui->cbxIDE->count(); i++)
    {
        if(ui->cbxIDE->itemText(i) == ideName)
        {
            ui->cbxIDE->setCurrentIndex(i);
            break;
        }
    }
}

//------------------------------------------------------------------------------
// Сохранение настроект программы в файл
//------------------------------------------------------------------------------
void MainWindow::saveSettings()
{
    if(this->settings.open(QFile::WriteOnly))
    {
        this->settings.write(ui->cbxPort->currentText().toLatin1().constData());
        this->settings.write("\r\n");
        this->settings.write(ui->cbxSpeed->currentText().toLatin1().constData());
        this->settings.write("\r\n");
        this->settings.write(ui->lineEditFirmware->text().toLatin1().constData());
        this->settings.write("\r\n");
        this->settings.write(ui->cbxChip->currentText().toLatin1().constData());
        this->settings.write("\r\n");
        this->settings.write(ui->cbxIDE->currentText().toLatin1().constData());
        this->settings.write("\r\n");
        this->settings.close();
    }
}

//------------------------------------------------------------------------------
// Чтение файла flash_args, который создает среда Arduino или Espressif
// Извлечение из файла параметров программирования, таких как:
//    - Режим программирования (QIO, DIO, QOUT, DOUT, FASTRD)
//    - Частота SPI-шины памяти (20МГц, 26.7МГц, 40МГц, 80МГц)
//    - Объем памяти (2МБ, 4МБ и т.д.)
// Если файл не найден или не найден какой-либо параметр, то он не будет изменен
//------------------------------------------------------------------------------
void MainWindow::parseFlashArgs(QString &flashMode,
                                QString &flashFreq,
                                QString &flashSize,
                                uint32_t &bootloaderAddr,
                                uint32_t &partTableAddr,
                                uint32_t &factoryAddr,
                                uint32_t &secondBootloderAddr)
{
    //
    // Сначала из пути к прошивке выделяем каталог, где она находится
    // и название файла прошивки
    //
    QString baseName = ui->lineEditFirmware->text();
    QString firmwareDir;

    int slashPos = baseName.lastIndexOf(QChar('/'));
    int extensionBegin = baseName.lastIndexOf(QChar('.'));

    slashPos = slashPos == -1 ? baseName.lastIndexOf(QChar('\\')) : slashPos;
    slashPos = slashPos == -1 ? 0 : slashPos + 1;
    extensionBegin = extensionBegin <= slashPos ? baseName.length() : extensionBegin;

    firmwareDir = baseName.mid(0, slashPos);
    baseName = baseName.mid(slashPos, extensionBegin - slashPos);

    if(ui->cbxIDE->currentText().toLower() == "espressif" ||
       ui->cbxIDE->currentText().toLower() == "arduino")
    {
        QFile flashArgs;
        QString flashModeKey = "flash_mode";
        QString flashFreqKey = "flash_freq";
        QString flashSizeKey = "flash_size";

        if(ui->cbxIDE->currentText().toLower() == "espressif")
        {
            flashArgs.setFileName(firmwareDir + Paths::espressifFlashArgs());
            flashModeKey = "flash_mode";
            flashFreqKey = "flash_freq";
            flashSizeKey = "flash_size";
        }
        else if(ui->cbxIDE->currentText().toLower() == "arduino")
        {
            flashArgs.setFileName(firmwareDir + Paths::arduinoFlashArgs());
            flashModeKey = "flash-mode";
            flashFreqKey = "flash-freq";
            flashSizeKey = "flash-size";
        }

        if(QFile::exists(flashArgs.fileName()) && flashArgs.open(QFile::ReadOnly))
        {
            char line[10240];
            QString qLine;

            //
            // Разбираем построчно файл flash_args
            //
            while(flashArgs.readLine(line, sizeof(line)) > 0)
            {
                qLine = QString(line).trimmed();

                //
                // Разбор параметров flash_mode, flash_freq, flash_size
                //
                if(qLine.contains("--" + flashModeKey) ||
                   qLine.contains("--" + flashFreqKey) ||
                   qLine.contains("--" + flashSizeKey))
                {
                    QStringList args = qLine.split("--");

                    foreach(QString a, args)
                    {
                        QStringList subarg;

                        a = a.trimmed();
                        subarg = a.split(" ", QString::SkipEmptyParts);

                        if(subarg.length() != 2)
                        {
                            continue;
                        }

                        if(subarg.at(0).trimmed().toLower() == flashModeKey.trimmed())
                        {
                            flashMode = subarg.at(1).trimmed();
                        }
                        else if(subarg.at(0).trimmed().toLower() == flashFreqKey.trimmed())
                        {
                            flashFreq = subarg.at(1).trimmed();
                        }
                        else if(subarg.at(0).trimmed().toLower() == flashSizeKey.trimmed())
                        {
                            flashSize = subarg.at(1).trimmed();
                        }
                    }
                }
                //
                // Чтение адресов для каждого отдельного компонента программы:
                // загрузчика, таблицы разделов, вторичного загрузчика для OTA
                // оснойного приложения и т.д.
                //
                else if (qLine.contains("bootloader"))
                {
                    int end = qLine.indexOf(" ");
                    bootloaderAddr = qLine.mid(0, end).trimmed().toUInt(nullptr, 16);
                }
                else if (qLine.contains("partition"))
                {
                    int end = qLine.indexOf(" ");
                    partTableAddr = qLine.mid(0, end).trimmed().toUInt(nullptr, 16);
                }
                else if (qLine.contains(Paths::arduinoSecondaryBootloaderName()))
                {
                    int end = qLine.indexOf(" ");
                    secondBootloderAddr = qLine.mid(0, end).trimmed().toUInt(nullptr, 16);
                }
                else if (qLine.contains(baseName))
                {
                    int end = qLine.indexOf(" ");
                    factoryAddr = qLine.mid(0, end).trimmed().toUInt(nullptr, 16);
                }
            }

            flashArgs.close();
        }
    }
}

//------------------------------------------------------------------------------
// Вывод таблицы разделов в окно логирования
//------------------------------------------------------------------------------
void MainWindow::printPartitionTable(const QList<PartitionInfo> &partitionTable,
                                     uint32_t bootloaderAddr,
                                     qint64 bootloaderSize,
                                     uint32_t partTableAddr,
                                     qint64 partTableSize)
{
    this->printlog(QString("Partition table:\n"));
    this->printlog(QString("-------------------------------------"
                           "-------------------------------------"
                           "-------------------------------------"
                           "-------------------------------------\n"));

    this->printlog(QString("%1\t%2\t%3\t%4\t%5\n").
                   arg("Label").
                   arg("Type").
                   arg("Subtype").
                   arg("Address").
                   arg("Size"));

    this->printlog(QString("-------------------------------------"
                           "-------------------------------------"
                           "-------------------------------------"
                           "-------------------------------------\n"));

    this->printlog(QString("%1\t%2\t%3\t0x%4\t0x%5\n").
                   arg("bootloader").
                   arg("").
                   arg("").
                   arg(bootloaderAddr, 0, 16, QChar('0')).
                   arg(bootloaderSize, 0, 16, QChar('0')));

    this->printlog(QString("%1\t%2\t%3\t0x%4\t0x%5\n").
                   arg("partition table").
                   arg("").
                   arg("").
                   arg(partTableAddr, 0, 16, QChar('0')).
                   arg(partTableSize, 0, 16, QChar('0')));

    if(!partitionTable.isEmpty())
    {
        foreach(PartitionInfo rec, partitionTable)
        {
            this->printlog(QString("%1\t%2\t%3\t0x%4\t0x%5\n").
                           arg(rec.label()).
                           arg(rec.typeName()).
                           arg(rec.subTypeName()).
                           arg(rec.address(), 0, 16, QChar('0')).
                           arg(rec.size(), 0, 16, QChar('0')));
        }
    }
    else if(ui->cbxChip->currentText().toLower() == "esp32")
    {
        this->printlog(QString("\nRead partition table ERROR\n"));
    }

    this->printlog(QString("-------------------------------------"
                           "-------------------------------------"
                           "-------------------------------------"
                           "-------------------------------------\n"));
}

//------------------------------------------------------------------------------
// Чтение таблицы разделов из bin- или csv-файла, в зависимости от IDE
//------------------------------------------------------------------------------
QList<PartitionInfo> MainWindow::readPartitionTable()
{
    QList<PartitionInfo> partitions;
    QStringList args;
    QString baseName = ui->lineEditFirmware->text();
    QString firmwareDir;
    QString bootloaderFile;
    QString partitionFile;
    QString partitionFileCsv;
    QString bootApp0File;
    QString firmwareFile = ui->lineEditFirmware->text();

    //
    // Сначала из пути к прошивке выделяем каталог, где она находится
    // и название файла прошивки
    //
    int slashPos = baseName.lastIndexOf(QChar('/'));
    int extensionBegin = baseName.lastIndexOf(QChar('.'));

    slashPos = slashPos == -1 ? baseName.lastIndexOf(QChar('\\')) : slashPos;
    slashPos = slashPos == -1 ? 0 : slashPos + 1;
    extensionBegin = extensionBegin <= slashPos ? baseName.length() : extensionBegin;

    firmwareDir = baseName.mid(0, slashPos);
    baseName = baseName.mid(slashPos, extensionBegin - slashPos);

    //
    // Если проект собран в Espressif IDE, то CSV-файла таблицы разделов нет
    // и нам необходимо преобразовать таблицу разделов из BIN в CSV
    //
    if(ui->cbxIDE->currentText().toLower() == "espressif" && QFile::exists(Paths::esp32part()))
    {
        partitionFile = firmwareDir + Paths::espressifPartitionBin();
        partitionFileCsv = Paths::tempPartitionTableCsv();
        args << partitionFile << partitionFileCsv;

#if 0
        this->printlog(QString("gen_esp32part.exe\n"));
        this->printlog(QString("\t%1\n").arg(partitionFile));
        this->printlog(QString("\t%1\n\n").arg(partitionFileCsv));
#endif
        this->esp32partProcess->start(Paths::esp32part(), args, QProcess::ReadWrite);

        // Wait for it to start
        if(!this->esp32partProcess->waitForStarted(5000))
        {
            this->esp32partProcess->close();
            showError(tr("Failed to start gen_esp32part.exe"));
        }
        else
        {
            this->esp32partProcess->waitForFinished(5000);
            this->esp32partProcess->close();
        }
    }
    //
    // Arduino IDE уже содержит нужный CSV-файл
    //
    else if(ui->cbxIDE->currentText().toLower() == "arduino")
    {
        partitionFileCsv = firmwareDir + Paths::arduinoPartitionTableCsv();
    }

    QFile partitionTable(partitionFileCsv);

    //
    // Парсим CSV-файл построчно и формируем таблицу разделов
    //
    if(QFile::exists(partitionFileCsv) && partitionTable.open(QFile::ReadOnly))
    {
        QString qLine;
        char line[10240];

        while(partitionTable.readLine(line, sizeof(line)) > 0)
        {
            qLine = QString(line).trimmed();

            if(qLine.length() > 0 && qLine.at(0) == '#')
                continue;
            else
            {
                bool status = false;
                args = qLine.split(",");

                if(args.count() >= 5)
                {
                    PartitionInfo record;
                    record.setLabel(args.at(0).trimmed());
                    record.setType(args.at(1).toLower().trimmed());

                    QString subTypeName = args.at(2).toLower().trimmed();

                    record.setSubType(subTypeName);
                    record.setAddress(args.at(3).toUInt(&status, 16));
                    record.setSize(args.at(4).toUInt(&status, 16));

                    if(!status)
                    {
                        QString sizeStr = args.at(4).toUpper();

                        if(sizeStr.contains("M"))
                        {
                            sizeStr = sizeStr.mid(0, sizeStr.length() - 1);
                            record.setSize(sizeStr.toUInt(&status));

                            if(status)
                            {
                                record.setSize(record.size() * 1024 * 1024);
                            }
                        }
                        else if(sizeStr.contains("K"))
                        {
                            sizeStr = sizeStr.mid(0, sizeStr.length() - 1);
                            record.setSize(sizeStr.toUInt(&status));

                            if(status)
                            {
                                record.setSize(record.size() * 1024);
                            }
                        }
                        else
                        {
                            record.setSize(sizeStr.toUInt(&status));
                        }
                    }

                    partitions.append(record);
                }
            }
        }

        partitionTable.close();
    }

    return partitions;
}

//------------------------------------------------------------------------------
// Открыть/закрыть выбранный порт
//------------------------------------------------------------------------------
void MainWindow::on_pushButtonOpen_clicked()
{
    if(!this->serialPort)
    {
        return;
    }

    if(this->serialPort->isOpen())
    {
        QSerialPortInfo portInfo = SerialPortInfo::info(this->serialPort->portName());

        ui->cbxPort->setEnabled(true);
        ui->cbxSpeed->setEnabled(true);
        ui->pushButtonOpen->setText(tr("Connect"));
        this->serialPort->close();

        emit statusChanged(QString("Disconnected from %1 (%2)").
                           arg(portInfo.description()).
                           arg(portInfo.portName()));
    }
    else
    {
        QString currPort = ui->cbxPort->currentText();
        int32_t currBr = ui->cbxSpeed->currentText().toInt();
        QSerialPortInfo portInfo = SerialPortInfo::info(currPort);

        ui->checkBoxDtr->setCheckState(Qt::Unchecked);
        ui->checkBoxRts->setCheckState(Qt::Unchecked);

        this->serialPort->setPort(portInfo);
        this->serialPort->setBaudRate(currBr);
        this->serialPort->setFlowControl(QSerialPort::NoFlowControl);

        if(this->serialPort->open(QSerialPort::ReadWrite))
        {
            this->serialPort->setRequestToSend(false);
            this->serialPort->setDataTerminalReady(false);
            ui->cbxPort->setEnabled(false);
            ui->cbxSpeed->setEnabled(false);
            ui->pushButtonOpen->setText(tr("Disconnect"));

            this->saveSettings();

            emit statusChanged(QString("Connected to %1 (%2)").
                               arg(portInfo.description()).
                               arg(this->serialPort->portName()));
        }
        else
        {
            this->showError(this->serialPort->errorString());
        }
    }
}

//------------------------------------------------------------------------------
// Вывод диалога ошибок
//------------------------------------------------------------------------------
void MainWindow::showError(QString err)
{
    QMessageBox::critical(this,
                          tr("Error"),
                          err,
                          QMessageBox::Ok);
}

//------------------------------------------------------------------------------
// Вывод принятых из порта данных в окно логирования
//------------------------------------------------------------------------------
void MainWindow::comPortReceive()
{
    QByteArray ba;

    while(this->serialPort->bytesAvailable() > 0)
    {
         ba.append(this->serialPort->readAll());
    }

    printlog(QString(ba));
}

//------------------------------------------------------------------------------
// Управление линией DTR
//------------------------------------------------------------------------------
void MainWindow::on_checkBoxDtr_toggled(bool checked)
{
    if(this->serialPort && !this->serialPort->isOpen())
    {
        ui->checkBoxDtr->setChecked(false);
    }
    else
    {
        this->serialPort->setDataTerminalReady(checked);
    }
}

//------------------------------------------------------------------------------
// Управление линией RTS (она же RESET)
//------------------------------------------------------------------------------
void MainWindow::on_checkBoxRts_toggled(bool checked)
{
    if(this->serialPort && !this->serialPort->isOpen())
    {
        ui->checkBoxRts->setChecked(false);
    }
    else
    {
        this->serialPort->setRequestToSend(checked);
    }
}

//------------------------------------------------------------------------------
// Обнаружение подключенных/отключенных устройств и обновление списка портов
//------------------------------------------------------------------------------
void MainWindow::updatePortList()
{
    QString selected = ui->cbxPort->currentText();
    QList<QSerialPortInfo> availablePorts = QSerialPortInfo::availablePorts();

    if(ui->cbxPort->count() == availablePorts.count())
    {
        return;
    }

    ui->cbxPort->clear();

    foreach(QSerialPortInfo port, availablePorts)
    {
        ui->cbxPort->addItem(port.portName());
    }

    //
    // Если ранее был выбран порт, то необходимо найти его в
    // списке и сделать активным
    //
    if(!selected.isEmpty())
    {
        for(int i = 0; i < ui->cbxPort->count(); i++)
        {
            if(ui->cbxPort->itemText(i) == selected)
            {
                ui->cbxPort->setCurrentIndex(i);
                break;
            }
        }
    }
}

//------------------------------------------------------------------------------
// Отправка данных устройству
//------------------------------------------------------------------------------
void MainWindow::on_pushButtonSend_clicked()
{
    if(this->serialPort->isOpen() == false)
    {
        showError(tr("Port is not open"));
        return;
    }

    //
    // В режиме "HEX" данные должны состоять из цифр 0-9 и букв A-F
    // идти группами по 2 символа и быть разделены пробелами
    //
    if(ui->checkBoxHexMode->isChecked())
    {
        QStringList bytesList = ui->lineEdit->text().trimmed().split(" ", QString::SkipEmptyParts);
        QByteArray ba;
        bool status = true;

        for(int i = 0; i < bytesList.count(); i++)
        {
            QString byte = bytesList.at(i);

            if(byte.length() != 2)
            {
                showError(tr("Invalid byte: \"%1\"").arg(byte));
                status = false;
                break;
            }
            else
            {
                uint hex = byte.toUInt(&status, 16);

                if(!status)
                {
                    showError(tr("Invalid byte: \"%1\"").arg(byte));
                    break;
                }
                else
                {
                    ba.append(static_cast<char>(hex));
                }
            }
        }

        if(status)
        {
            this->serialPort->write(ba);
        }
    }
    else
    {
        //
        // В текстовом режиме данных доступна отправка спец.символов:
        // CR, LF, CTRL+Z, ESC и т.д. Иногда это нужно для обнаружения
        // устройство признака завершения команды. В HEX-режиме ответственность
        // за отправку символов ложится на пользователя
        //
        QString text = ui->lineEdit->text().trimmed();

        if(text.length())
        {
            if(ui->comboBoxSpecialChars->currentText().toUpper() == "CR")
            {
                text += "\r";
            }
            else if(ui->comboBoxSpecialChars->currentText().toUpper() == "LF")
            {
                text += "\n";
            }
            else if(ui->comboBoxSpecialChars->currentText().toUpper() == "CR+LF")
            {
                text += "\r\n";
            }
            else if(ui->comboBoxSpecialChars->currentText().toUpper() == "CTRL+Z")
            {
                text += QChar(26);
            }
            else if(ui->comboBoxSpecialChars->currentText().toUpper() == "ESC")
            {
                text += "\e";
            }

            this->serialPort->write(text.toLatin1().constData(), text.length());
        }
    }
}

//------------------------------------------------------------------------------
// Вызов диалогового окна выбора файла прошивки
//------------------------------------------------------------------------------
void MainWindow::on_pushButtonBrowse_clicked()
{
    QStringList fileNames;
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("BIN (*.bin *.BIN)"));

    if (dialog.exec())
    {
        fileNames = dialog.selectedFiles();

        if(fileNames.count())
        {
            ui->lineEditFirmware->setText(fileNames.first());
        }
    }
}

//------------------------------------------------------------------------------
// Инициация процесса записи Flash
//------------------------------------------------------------------------------
void MainWindow::on_pushButtonBurn_clicked()
{
    if(!QFile::exists(Paths::esptool()))
    {
        showError(tr("esptool.exe programming utility not found\n"));
        return;
    }
    else if(ui->lineEditFirmware->text().isEmpty())
    {
        showError(tr("Firmware file not specified"));
        return;
    }
    else
    {
        QStringList args;
        QString flashMode = "qio";
        QString flashFreq = "80m";
        QString flashSize = "4MB";
        uint32_t bootloaderAddr = 0x1000;
        uint32_t secBootloaderAddr = 0xe000;
        uint32_t partTableAddr = 0x8000;
        uint32_t factoryAddr = 0x10000;
        qint64 bootloaderSize = 0;
        qint64 partTableSize = 0;

        if(ui->cbxChip->currentText().toLower() == "esp32")
        {
            /*
            esptool.exe
                    --chip esp32
                    --port COM3
                    --baud 921600
                    --before default_reset
                    --after hard_reset
                    write_flash
                    -z
                    --flash_mode dio
                    --flash_freq 80m
                    --flash_size 4MB
                    0x1000 TestESP32.bootloader.bin
                    0x8000 TestESP32.partitions.bin
                    0xe000 boot_app0.bin
                    0x10000 TestESP32.bin
            */
            QString baseName = ui->lineEditFirmware->text();
            QString firmwareDir;
            QString bootloaderFile;
            QString partitionFile;
            QString bootApp0File;
            QString firmwareFile = ui->lineEditFirmware->text();

            //
            // Сначала определяем каталог сборки проекта и BIN основной программы
            //
            int slashPos = baseName.lastIndexOf(QChar('/'));
            int extensionBegin = baseName.lastIndexOf(QChar('.'));

            slashPos = slashPos == -1 ? baseName.lastIndexOf(QChar('\\')) : slashPos;
            slashPos = slashPos == -1 ? 0 : slashPos + 1;
            extensionBegin = extensionBegin <= slashPos ? baseName.length() : extensionBegin;

            firmwareDir = baseName.mid(0, slashPos);
            baseName = baseName.mid(slashPos, extensionBegin - slashPos);

            //
            // Далее зная имя файла основной программы формирует имена файлов
            // загрузчиков, таблицы раздела и т.д.
            //
            if(ui->cbxIDE->currentText().toLower() == "espressif")
            {
                bootloaderFile = firmwareDir + Paths::espressifPrimaryBootloader();
                partitionFile = firmwareDir + Paths::espressifPartitionBin();
            }
            else if(ui->cbxIDE->currentText().toLower() == "arduino")
            {
                bootloaderFile = firmwareDir + baseName + ".bootloader.bin";
                partitionFile = firmwareDir + baseName + ".partitions.bin";
                bootApp0File = Paths::arduinoSecondaryBootloader();
            }
            else
            {
                bootloaderFile = firmwareDir + baseName + ".bootloader.bin";
                partitionFile = firmwareDir + baseName + ".partitions.bin";
                bootApp0File = Paths::arduinoSecondaryBootloader();
            }

            //
            // Вычисляем размеры файлов загрузчика и таблицы разделов,
            // которые необходимы исключительно для формирования таблицы разделов
            // и не влияют на процесс программирования
            //
            QFileInfo fi(bootloaderFile);
            bootloaderSize = fi.size();

            fi.setFile(partitionFile);
            partTableSize = fi.size();

            //
            // Начинаем формировани команду для программирования
            //
            args << "--chip" << ui->cbxChip->currentText().toLower();
            args << "--port" << ui->cbxPort->currentText();
            args << "--baud" << ui->cbxFlashSpeed->currentText();
            args << "--before" << "default_reset";
            args << "--after" << "hard_reset";
            args << "write_flash";
            args << "-z";

            //
            // Пробуем найти в каталоге сборки файл flash_args и, если он есть
            // загружаем из него настройки режима программирования, объема флеш,
            // скорости шины и т.д. В противном случае используем параметры
            // по умолчанию, который заданы в начале функции.
            // TODO: в дальнейшем добавить ручное управление данными настройками
            //
            this->parseFlashArgs(flashMode,
                                 flashFreq,
                                 flashSize,
                                 bootloaderAddr,
                                 partTableAddr,
                                 factoryAddr,
                                 secBootloaderAddr);

            args << "--flash_mode" << flashMode;
            args << "--flash_freq" << flashFreq;
            args << "--flash_size" << flashSize;

            //args << "0x1000" << bootloaderFile;
            //args << "0x8000" << partitionFile;

            args << QString("0x%1").arg(bootloaderAddr, 0, 16) << bootloaderFile;
            args << QString("0x%1").arg(partTableAddr, 0, 16) << partitionFile;

            //
            // Если проект собран в Arduino, то добавляем зашивку вторичного загрузчика,
            // который универсален для все проектов на Arduino
            //
            if(ui->cbxIDE->currentText().toLower() == "arduino")
            {
                //args << "0xe000" << bootApp0File;
                args << QString("0x%1").arg(secBootloaderAddr, 0, 16) << bootApp0File;
            }

            //args << "0x10000" << firmwareFile;
            args << QString("0x%1").arg(factoryAddr, 0, 16) << firmwareFile;
        }
        //
        // Если программируем ESP8266, то среда программирование Arduino, Sloeber
        // формируют один единственный BIN-файл и нет необходимости прошивать
        // фрагменты прошивки по отдельности
        //
        else
        {
            args << "--chip" << ui->cbxChip->currentText().toLower();
            args << "--port" << ui->cbxPort->currentText();
            args << "--baud" << "921600";
            args << "--before" << "default_reset";
            args << "--after" << "hard_reset";
            args << "write_flash" << "0x0" << ui->lineEditFirmware->text();
        }

        //
        // Вывод таблицы разделов на печать
        //
        ui->plainTextEdit->clear();
        QList<PartitionInfo> partitionTable = this->readPartitionTable();

        this->printPartitionTable(partitionTable,
                                  bootloaderAddr,
                                  bootloaderSize,
                                  partTableAddr,
                                  partTableSize);

        //
        // Сброс сигналов DTR, RTS
        //
        ui->checkBoxDtr->setChecked(false);
        ui->checkBoxRts->setChecked(false);

        //
        // Запоминаем состояние порта до начала прошивки и закрываем
        // Если порт был открыт, то после программирования памяти сразу же
        // его откроем, чтобы пользователь сразу же мог видеть лог отладочной
        // печати
        //
        this->portStateBeforeBurn = this->serialPort->isOpen();

        if(this->portStateBeforeBurn)
        {
            this->on_pushButtonOpen_clicked();
        }

        //
        // Выводим в окно логирования сформированный текст команды для esptool
        //
        this->printlog(QString("esptool.exe"));

        foreach(QString a, args)
        {
            if(a.contains("--") || a.mid(0, 2).toLower() == "0x")
            {
                this->printlog(QString("\n\t%1 ").arg(a));
            }
            else
            {
                this->printlog(QString("%1 ").arg(a));
            }
        }

        this->printlog(QString("\n\n"));

        //
        // Записываем память
        //
        this->esptoolProcess->start(Paths::esptool(), args, QProcess::ReadWrite);

        if(!this->esptoolProcess->waitForStarted(5000))
        {
            this->esptoolProcess->close();
            showError(tr("Failed to launch esptool.exe"));
            return;
        }

        emit statusChanged("Flash write process started");

        // Подключаем сигнал finished к лямбда-функции
        connect(this->esptoolProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int exitCode, QProcess::ExitStatus exitStatus)
        {
            Q_UNUSED(exitCode)
            Q_UNUSED(exitStatus)

            emit statusChanged("Flash write process completed");
        });
    }
}

//------------------------------------------------------------------------------
// Вывод текстовых данных в окно логирования (stdout, stderr от esptool, печать
// таблицы разделов и т.д.)
//------------------------------------------------------------------------------
void MainWindow::printlog(QString text)
{
    ui->plainTextEdit->insertPlainText(text);
    ui->plainTextEdit->verticalScrollBar()->setValue(ui->plainTextEdit->verticalScrollBar()->maximum());
}

//------------------------------------------------------------------------------
// Вывод в окно логирования ошибок esptool
//------------------------------------------------------------------------------
void MainWindow::showBurnStdErr()
{
    printlog(QString(this->esptoolProcess->readAllStandardError()));
}

//------------------------------------------------------------------------------
// Вывод в окно логирования данных от esptool
//------------------------------------------------------------------------------
void MainWindow::showBurnStdOut()
{
    printlog(QString(this->esptoolProcess->readAllStandardOutput()));
}

//------------------------------------------------------------------------------
// Вывод в окно логирования ошибок get_esp32part
//------------------------------------------------------------------------------
void MainWindow::showEsp32PartStdErr()
{
    printlog(QString(this->esp32partProcess->readAllStandardError()));
}

//------------------------------------------------------------------------------
// Вывод в окно логирования данных от get_esp32part
//------------------------------------------------------------------------------
void MainWindow::showEsp32PartStdOut()
{
    printlog(QString(this->esp32partProcess->readAllStandardOutput()));
}

//------------------------------------------------------------------------------
// Блокировка критичных органов управления UI на момент записи памяти
//------------------------------------------------------------------------------
void MainWindow::burnStart()
{
    ui->pushButtonBrowse->setDisabled(true);
    ui->pushButtonBurn->setDisabled(true);
    ui->pushButtonOpen->setDisabled(true);
    ui->pushButtonErase->setDisabled(true);
}

//------------------------------------------------------------------------------
// Разблокировка критичных органов управления UI по окончании записи памяти
//------------------------------------------------------------------------------
void MainWindow::burnStop()
{
    ui->pushButtonBrowse->setDisabled(false);
    ui->pushButtonBurn->setDisabled(false);
    ui->pushButtonOpen->setDisabled(false);
    ui->pushButtonErase->setDisabled(false);

    if(this->portStateBeforeBurn)
    {
        this->on_pushButtonOpen_clicked();
    }

    this->saveSettings();
}

//------------------------------------------------------------------------------
// Полное стирание Flash
//------------------------------------------------------------------------------
void MainWindow::on_pushButtonErase_clicked()
{
    if(!QFile::exists(Paths::esptool()))
    {
        showError(tr("esptool.exe programming utility not found\n"));
        return;
    }
    else
    {
        QStringList args;

        args << "--chip" << ui->cbxChip->currentText().toLower();
        args << "--port" << ui->cbxPort->currentText();
        args << "--baud" << ui->cbxFlashSpeed->currentText();
        args << "erase_flash";

        ui->plainTextEdit->clear();

        //
        // Сброс сигналов DTR, RTS
        //
        ui->checkBoxDtr->setChecked(false);
        ui->checkBoxRts->setChecked(false);

        //
        // Запоминаем состояние порта до начала прошивки и закрываем.
        // По окончании процесса стирания открываем порт, если он был открыт
        // до запуска процедуры
        //
        this->portStateBeforeBurn = this->serialPort->isOpen();

        if(this->portStateBeforeBurn)
        {
            this->on_pushButtonOpen_clicked();
        }

        this->printlog(QString("esptool.exe %1\n\n").arg(args.join(' ')));
        this->esptoolProcess->start(Paths::esptool(), args, QProcess::ReadWrite);

        if(!this->esptoolProcess->waitForStarted(5000))
        {
            this->esptoolProcess->close();
            showError(tr("Failed to start esptool.exe"));
            return;
        }

        emit statusChanged("Flash erase process started");

        // Подключаем сигнал finished к лямбда-функции
        connect(this->esptoolProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int exitCode, QProcess::ExitStatus exitStatus)
        {
            Q_UNUSED(exitCode)
            Q_UNUSED(exitStatus)

            emit statusChanged("Flash erase process completed");
        });
    }
}
