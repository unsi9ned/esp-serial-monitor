#include "partitioninfo.h"

const QMap<PartitionInfo::EspPartitionSubtype, QString> PartitionInfo::appSubtypeMap =
{
    {ESP_PARTITION_SUBTYPE_APP_FACTORY, "factory"},
    {ESP_PARTITION_SUBTYPE_APP_OTA_MIN, "ota_min"},
    {ESP_PARTITION_SUBTYPE_APP_OTA_0,   "ota_0"},
    {ESP_PARTITION_SUBTYPE_APP_OTA_1,   "ota_1"},
    {ESP_PARTITION_SUBTYPE_APP_OTA_2,   "ota_2"},
    {ESP_PARTITION_SUBTYPE_APP_OTA_3,   "ota_3"},
    {ESP_PARTITION_SUBTYPE_APP_OTA_4,   "ota_4"},
    {ESP_PARTITION_SUBTYPE_APP_OTA_5,   "ota_5"},
    {ESP_PARTITION_SUBTYPE_APP_OTA_6,   "ota_6"},
    {ESP_PARTITION_SUBTYPE_APP_OTA_7,   "ota_7"},
    {ESP_PARTITION_SUBTYPE_APP_OTA_8,   "ota_8"},
    {ESP_PARTITION_SUBTYPE_APP_OTA_9,   "ota_9"},
    {ESP_PARTITION_SUBTYPE_APP_OTA_10,  "ota_10"},
    {ESP_PARTITION_SUBTYPE_APP_OTA_11,  "ota_11"},
    {ESP_PARTITION_SUBTYPE_APP_OTA_12,  "ota_12"},
    {ESP_PARTITION_SUBTYPE_APP_OTA_13,  "ota_13"},
    {ESP_PARTITION_SUBTYPE_APP_OTA_14,  "ota_14"},
    {ESP_PARTITION_SUBTYPE_APP_OTA_15,  "ota_15"},
    {ESP_PARTITION_SUBTYPE_APP_OTA_MAX, "ota_max"},
    {ESP_PARTITION_SUBTYPE_APP_TEST,    "test"},
};

const QMap<PartitionInfo::EspPartitionSubtype, QString> PartitionInfo::dataSubtypeMap =
{
    {ESP_PARTITION_SUBTYPE_DATA_OTA,       "ota"},
    {ESP_PARTITION_SUBTYPE_DATA_PHY,       "phy"},
    {ESP_PARTITION_SUBTYPE_DATA_NVS,       "nvs"},
    {ESP_PARTITION_SUBTYPE_DATA_COREDUMP,  "coredump"},
    {ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS,  "nvs_keys"},
    {ESP_PARTITION_SUBTYPE_DATA_EFUSE_EM,  "efuse_em"},
    {ESP_PARTITION_SUBTYPE_DATA_UNDEFINED, "undefined"},
    {ESP_PARTITION_SUBTYPE_DATA_ESPHTTPD,  "esphttpd"},
    {ESP_PARTITION_SUBTYPE_DATA_FAT,       "fat"},
    {ESP_PARTITION_SUBTYPE_DATA_SPIFFS,    "spiffs"},
};

PartitionInfo::PartitionInfo()
{
    _type = PartitionInfo::ESP_PARTITION_TYPE_ANY;
    _subtype = PartitionInfo::ESP_PARTITION_SUBTYPE_ANY;
    _address = 0;
    _size = 0;
    _label = "Unknown";
}

void PartitionInfo::setType(const QString &type)
{
    this->_type = PartitionInfo::ESP_PARTITION_TYPE_ANY;

    if(type == "app")
    {
        this->_type = PartitionInfo::ESP_PARTITION_TYPE_APP;
    }
    else if(type == "data")
    {
        this->_type = PartitionInfo::ESP_PARTITION_TYPE_DATA;
    }
}

void PartitionInfo::setSubType(const QString &sTypeName)
{
    this->_subtype = PartitionInfo::ESP_PARTITION_SUBTYPE_ANY;

    for (auto it = PartitionInfo::appSubtypeMap.keyBegin(); it != PartitionInfo::appSubtypeMap.keyEnd(); ++it)
    {
        const  PartitionInfo::EspPartitionSubtype& key = *it;
        QString currName = PartitionInfo::appSubtypeMap.value(key, "");

        if(sTypeName == currName)
        {
            this->_subtype = key;
            return;
        }
    }

    for (auto it = PartitionInfo::dataSubtypeMap.keyBegin(); it != PartitionInfo::dataSubtypeMap.keyEnd(); ++it)
    {
        const  PartitionInfo::EspPartitionSubtype& key = *it;
        QString currName = PartitionInfo::dataSubtypeMap.value(key, "");

        if(sTypeName == currName)
        {
            this->_subtype = key;
            return;
        }
    }
}

QString PartitionInfo::subTypeName()
{
    QString typeName = "any";

    if(this->_type == PartitionInfo::ESP_PARTITION_TYPE_APP)
    {
        typeName = PartitionInfo::appSubtypeMap.value(this->_subtype, "any");
    }
    else if(this->_type == PartitionInfo::ESP_PARTITION_TYPE_DATA)
    {
        typeName = PartitionInfo::dataSubtypeMap.value(this->_subtype, "any");
    }
    else
    {
        typeName = "any";
    }

    return typeName;
}
