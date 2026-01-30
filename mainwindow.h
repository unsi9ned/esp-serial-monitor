#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QProcess>
#include <QDialog>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include "aboutdialog.h"
#include "partitioninfo.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    QTimer       portScanTimer;
    QSerialPort* serialPort;
    QFile        settings;
    QProcess*    esptoolProcess;
    QProcess*    esp32partProcess;
    bool         portStateBeforeBurn;
    AboutDialog* aboutDlg;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:

    void loadSettings();
    void saveSettings();
    void parseFlashArgs(QString &flashMode,
                        QString &flashFreq,
                        QString &flashSize,
                        uint32_t &bootloaderAddr,
                        uint32_t &partTableAddr,
                        uint32_t &factoryAddr,
                        uint32_t &secondBootloderAddr);

    void updateUiFlashMode(const QString &flashMode,
                           const QString &flashFreq,
                           const QString &flashSize);

    void getFlashMode(QString &flashMode,
                      QString &flashFreq,
                      QString &flashSize);

    void getFirmwareLocation(QString &firmwareDir,
                             QString &baseName);

    void printPartitionTable(const QList<PartitionInfo> &partitionTable,
                             uint32_t bootloaderAddr,
                             qint64 bootloaderSize,
                             uint32_t partTableAddr,
                             qint64 partTableSize);

    QList<PartitionInfo> readPartitionTable();


private slots:
    void on_pushButtonOpen_clicked();
    void on_checkBoxDtr_toggled(bool checked);
    void on_checkBoxRts_toggled(bool checked);
    void updatePortList();

    void showError(QString err);
    void comPortReceive();
    void showBurnStdErr();
    void showBurnStdOut();
    void showEsp32PartStdErr();
    void showEsp32PartStdOut();
    void burnStart();
    void burnStop();

    void on_pushButtonSend_clicked();
    void on_pushButtonBrowse_clicked();
    void on_pushButtonBurn_clicked();

    void printlog(QString text);

    void on_pushButtonErase_clicked();

signals:
    void statusChanged(QString status);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
