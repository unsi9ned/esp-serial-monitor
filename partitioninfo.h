#ifndef PARTITIONINFO_H
#define PARTITIONINFO_H

#include <QMap>

class PartitionInfo
{
    enum EspPartitionType
    {
        ESP_PARTITION_TYPE_APP = 0x00,       //!< Application partition type
        ESP_PARTITION_TYPE_DATA = 0x01,      //!< Data partition type

        ESP_PARTITION_TYPE_ANY = 0xff,       //!< Used to search for partitions with any type
    };

    enum EspPartitionSubtype
    {
        ESP_PARTITION_SUBTYPE_APP_FACTORY = 0x00,                                 //!< Factory application partition
        ESP_PARTITION_SUBTYPE_APP_OTA_MIN = 0x10,                                 //!< Base for OTA partition subtypes
        ESP_PARTITION_SUBTYPE_APP_OTA_0 = ESP_PARTITION_SUBTYPE_APP_OTA_MIN + 0,  //!< OTA partition 0
        ESP_PARTITION_SUBTYPE_APP_OTA_1 = ESP_PARTITION_SUBTYPE_APP_OTA_MIN + 1,  //!< OTA partition 1
        ESP_PARTITION_SUBTYPE_APP_OTA_2 = ESP_PARTITION_SUBTYPE_APP_OTA_MIN + 2,  //!< OTA partition 2
        ESP_PARTITION_SUBTYPE_APP_OTA_3 = ESP_PARTITION_SUBTYPE_APP_OTA_MIN + 3,  //!< OTA partition 3
        ESP_PARTITION_SUBTYPE_APP_OTA_4 = ESP_PARTITION_SUBTYPE_APP_OTA_MIN + 4,  //!< OTA partition 4
        ESP_PARTITION_SUBTYPE_APP_OTA_5 = ESP_PARTITION_SUBTYPE_APP_OTA_MIN + 5,  //!< OTA partition 5
        ESP_PARTITION_SUBTYPE_APP_OTA_6 = ESP_PARTITION_SUBTYPE_APP_OTA_MIN + 6,  //!< OTA partition 6
        ESP_PARTITION_SUBTYPE_APP_OTA_7 = ESP_PARTITION_SUBTYPE_APP_OTA_MIN + 7,  //!< OTA partition 7
        ESP_PARTITION_SUBTYPE_APP_OTA_8 = ESP_PARTITION_SUBTYPE_APP_OTA_MIN + 8,  //!< OTA partition 8
        ESP_PARTITION_SUBTYPE_APP_OTA_9 = ESP_PARTITION_SUBTYPE_APP_OTA_MIN + 9,  //!< OTA partition 9
        ESP_PARTITION_SUBTYPE_APP_OTA_10 = ESP_PARTITION_SUBTYPE_APP_OTA_MIN + 10,//!< OTA partition 10
        ESP_PARTITION_SUBTYPE_APP_OTA_11 = ESP_PARTITION_SUBTYPE_APP_OTA_MIN + 11,//!< OTA partition 11
        ESP_PARTITION_SUBTYPE_APP_OTA_12 = ESP_PARTITION_SUBTYPE_APP_OTA_MIN + 12,//!< OTA partition 12
        ESP_PARTITION_SUBTYPE_APP_OTA_13 = ESP_PARTITION_SUBTYPE_APP_OTA_MIN + 13,//!< OTA partition 13
        ESP_PARTITION_SUBTYPE_APP_OTA_14 = ESP_PARTITION_SUBTYPE_APP_OTA_MIN + 14,//!< OTA partition 14
        ESP_PARTITION_SUBTYPE_APP_OTA_15 = ESP_PARTITION_SUBTYPE_APP_OTA_MIN + 15,//!< OTA partition 15
        ESP_PARTITION_SUBTYPE_APP_OTA_MAX = ESP_PARTITION_SUBTYPE_APP_OTA_MIN + 16,//!< Max subtype of OTA partition
        ESP_PARTITION_SUBTYPE_APP_TEST = 0x20,                                    //!< Test application partition

        ESP_PARTITION_SUBTYPE_DATA_OTA = 0x00,                                    //!< OTA selection partition
        ESP_PARTITION_SUBTYPE_DATA_PHY = 0x01,                                    //!< PHY init data partition
        ESP_PARTITION_SUBTYPE_DATA_NVS = 0x02,                                    //!< NVS partition
        ESP_PARTITION_SUBTYPE_DATA_COREDUMP = 0x03,                               //!< COREDUMP partition
        ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS = 0x04,                               //!< Partition for NVS keys
        ESP_PARTITION_SUBTYPE_DATA_EFUSE_EM = 0x05,                               //!< Partition for emulate eFuse bits
        ESP_PARTITION_SUBTYPE_DATA_UNDEFINED = 0x06,                              //!< Undefined (or unspecified) data partition

        ESP_PARTITION_SUBTYPE_DATA_ESPHTTPD = 0x80,                               //!< ESPHTTPD partition
        ESP_PARTITION_SUBTYPE_DATA_FAT = 0x81,                                    //!< FAT partition
        ESP_PARTITION_SUBTYPE_DATA_SPIFFS = 0x82,                                 //!< SPIFFS partition

        ESP_PARTITION_SUBTYPE_ANY = 0xff,                                         //!< Used to search for partitions with any subtype
    };

private:
    EspPartitionType    _type;           /*!< partition type (app/data) */
    EspPartitionSubtype _subtype;        /*!< partition subtype */
    uint32_t            _address;        /*!< starting address of the partition in flash */
    uint32_t            _size;           /*!< size of the partition, in bytes */
    QString             _label;          /*!< partition label, zero-terminated ASCII string */

    static const QMap<EspPartitionSubtype, QString> appSubtypeMap;
    static const QMap<EspPartitionSubtype, QString> dataSubtypeMap;

public:
    explicit PartitionInfo();

    inline void setType(PartitionInfo::EspPartitionType type)
    {
        this->_type = type;
    }

    void setType(const QString &type);

    inline void setSubType(PartitionInfo::EspPartitionSubtype subtype)
    {
        this->_subtype = subtype;
    }

    void setSubType(const QString &sTypeName);

    inline void setAddress(uint32_t addr)
    {
        this->_address = addr;
    }

    inline void setSize(uint32_t size)
    {
        this->_size = size;
    }

    inline void setLabel(const QString &label)
    {
        this->_label = label;
    }

    inline PartitionInfo::EspPartitionType type()
    {
        return this->_type;
    }

    inline PartitionInfo::EspPartitionSubtype subtype()
    {
        return this->_subtype;
    }

    inline uint32_t address()
    {
        return this->_address;
    }

    inline uint32_t size()
    {
        return this->_size;
    }

    inline QString label()
    {
        return this->_label;
    }

    QString subTypeName();

    inline QString typeName()
    {
        QString name = "any";

        switch(this->_type)
        {
            case PartitionInfo::ESP_PARTITION_TYPE_APP:
                name = "app";
            break;

            case PartitionInfo::ESP_PARTITION_TYPE_DATA:
                name = "data";
            break;

            default:
                name = "any";
            break;
        }

        return name;
    }

    inline QString addrToHex()
    {
        return QString("0x%1").arg(this->_address, 0, 16);
    }

    inline QString sizeToHex()
    {
        return QString("0x%1").arg(this->_size, 0, 16);
    }
};

#endif // PARTITIONINFO_H
