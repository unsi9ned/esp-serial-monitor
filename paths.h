#ifndef PATHS_H
#define PATHS_H

#include <QObject>
#include <QApplication>
#include <QDebug>

class Paths : public QObject
{
    Q_OBJECT

private:
    static constexpr const char* SETTINGS = "settings.sav";
    static constexpr const char* ESP_TOOL_PATH = "utils/esptool.exe";
    static constexpr const char* ARDUINO_SECONDARY_BL = "utils/boot_app0.bin";
    static constexpr const char* ARDUINO_FLASH_ARGS = "flash_args";
    static constexpr const char* ARDUINO_PARTITION_TABLE_CSV = "partitions.csv";
    static constexpr const char* ESP32_PART = "utils/gen_esp32part.exe";
    static constexpr const char* PARTITION_TABLE_CSV = "partition-table.csv";
    static constexpr const char* ESPRESSIF_PRIMARY_BL = "bootloader/bootloader.bin";
    static constexpr const char* ESPRESSIF_PARTITION_BIN = "partition_table/partition-table.bin";
    static constexpr const char* ESPRESSIF_FLASH_ARGS = "flash_args";
    static constexpr const char* PLATFORMIO_BOOTLOADER = "bootloader.bin";
    static constexpr const char* PLATFORMIO_PARTITION_BIN = "partitions.bin";

public:
    explicit Paths(QObject *parent = nullptr);

    inline static QString appSettings()
    {
        return QApplication::applicationDirPath() + "/" + SETTINGS;
    }

    inline static QString esptool()
    {
        return QApplication::applicationDirPath() + "/" + ESP_TOOL_PATH;
    }

    inline static QString esp32part()
    {
        return QApplication::applicationDirPath() + "/" + ESP32_PART;
    }

    inline static QString tempPartitionTableCsv()
    {
        return QApplication::applicationDirPath() + "/" + PARTITION_TABLE_CSV;
    }

    inline static QString espressifPrimaryBootloader()
    {
        return ESPRESSIF_PRIMARY_BL;
    }

    inline static QString espressifPartitionBin()
    {
        return ESPRESSIF_PARTITION_BIN;
    }

    inline static QString espressifFlashArgs()
    {
        return ESPRESSIF_FLASH_ARGS;
    }

    inline static QString arduinoSecondaryBootloader()
    {
        return QApplication::applicationDirPath() + "/" + ARDUINO_SECONDARY_BL;
    }

    static QString arduinoSecondaryBootloaderName();

    inline static QString arduinoFlashArgs()
    {
        return ARDUINO_FLASH_ARGS;
    }

    inline static QString arduinoPartitionTableCsv()
    {
        return ARDUINO_PARTITION_TABLE_CSV;
    }

    inline static QString platformioBootloader()
    {
        return PLATFORMIO_BOOTLOADER;
    }

    inline static QString platformioPartitionBin()
    {
        return PLATFORMIO_PARTITION_BIN;
    }

signals:

public slots:
};

#endif // PATHS_H
