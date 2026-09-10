
#pragma once


#include <QJsonObject>

// ====================================================================================================

/*
 * Генератор случайных JSON-данных для отправки на сервер
 * Создает сообщения трех типов: NetworkMetrics, DeviceStatus, Log
*/
class DataGenerator
{
public:
    DataGenerator() = default;
    ~DataGenerator() = default;

    QJsonObject GenerateRandomMessage();
    QJsonObject GenerateNetworkMetrics();
    QJsonObject GenerateDeviceStatus();
    QJsonObject GenerateLog();

private:
    int RandomInt(int min, int max);
    double RandomDouble(double min, double max);
    int RandomIndex(int size);
};
