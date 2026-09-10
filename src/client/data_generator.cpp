
#include "data_generator.h"

#include <QRandomGenerator>
#include <QStringList>

#include "protocol.h"

// ====================================================================================================

QJsonObject DataGenerator::GenerateRandomMessage()
{
    int type_index = RandomIndex(3);
    switch (type_index)
    {
    case 0:
        return GenerateNetworkMetrics();
    case 1:
        return GenerateDeviceStatus();
    case 2:
    default:
        return GenerateLog();
    }
}

QJsonObject DataGenerator::GenerateNetworkMetrics()
{
    QJsonObject json;
    json[protocol::kKeyType] = protocol::kTypeNetworkMetrics;

    // Выбираем длину с равной вероятностью
    int length_type = RandomIndex(3);

    if (length_type == 0)
    {
        // Короткое
        json["bandwidth"] = RandomDouble(10.0, 1000.0);
        json["latency"] = RandomDouble(1.0, 100.0);
        json["packet_loss"] = RandomDouble(0.0, 1.0);
    }
    else if (length_type == 1)
    {
        // Среднее
        json["bandwidth"] = RandomDouble(10.0, 1000.0);
        json["latency"] = RandomDouble(1.0, 100.0);
        json["packet_loss"] = RandomDouble(0.0, 1.0);
        json["jitter"] = RandomDouble(0.0, 50.0);
        json["throughput"] = RandomDouble(1.0, 500.0);
        json["error_rate"] = RandomDouble(0.0, 0.1);
    }
    else
    {
        // Длинное
        json["bandwidth"] = RandomDouble(10.0, 1000.0);
        json["latency"] = RandomDouble(1.0, 100.0);
        json["packet_loss"] = RandomDouble(0.0, 1.0);
        json["jitter"] = RandomDouble(0.0, 50.0);
        json["throughput"] = RandomDouble(1.0, 500.0);
        json["error_rate"] = RandomDouble(0.0, 0.1);
        json["dns_latency"] = RandomDouble(1.0, 200.0);
        json["tcp_retransmits"] = RandomInt(0, 100);
        json["connection_count"] = RandomInt(1, 1000);
        json["active_streams"] = RandomInt(1, 100);
        json["buffer_occupancy"] = RandomDouble(0.0, 100.0);
    }

    return json;
}

QJsonObject DataGenerator::GenerateDeviceStatus()
{
    QJsonObject json;
    json[protocol::kKeyType] = protocol::kTypeDeviceStatus;

    int length_type = RandomIndex(3);

    if (length_type == 0)
    {
        // Короткое
        json["uptime"] = RandomInt(0, 86400);
        json["cpu_usage"] = RandomInt(0, 100);
        json["memory_usage"] = RandomInt(0, 100);
    }
    else if (length_type == 1)
    {
        // Среднее
        json["uptime"] = RandomInt(0, 86400);
        json["cpu_usage"] = RandomInt(0, 100);
        json["memory_usage"] = RandomInt(0, 100);
        json["disk_usage"] = RandomInt(0, 100);
        json["network_interfaces"] = RandomInt(1, 10);
        json["active_connections"] = RandomInt(0, 500);
    }
    else
    {
        // Длинное
        json["uptime"] = RandomInt(0, 86400);
        json["cpu_usage"] = RandomInt(0, 100);
        json["memory_usage"] = RandomInt(0, 100);
        json["disk_usage"] = RandomInt(0, 100);
        json["network_interfaces"] = RandomInt(1, 10);
        json["active_connections"] = RandomInt(0, 500);
        json["temperature"] = RandomDouble(20.0, 80.0);
        json["fan_speed"] = RandomInt(1000, 5000);
        json["battery_level"] = RandomInt(0, 100);
        json["process_count"] = RandomInt(50, 500);
        json["thread_count"] = RandomInt(100, 1000);
        json["io_operations"] = RandomInt(0, 10000);
    }

    return json;
}

QJsonObject DataGenerator::GenerateLog()
{
    QJsonObject json;
    json[protocol::kKeyType] = protocol::kTypeLog;

    static const QStringList short_messages = {
        "Interface eth0 restarted",
        "Connection established",
        "High latency detected"
    };

    static const QStringList medium_messages = {
        "High latency detected on primary link, switching to backup interface",
        "Packet loss threshold exceeded, initiating automatic recovery procedure"
    };

    static const QStringList long_messages = {
        "Critical error detected in network subsystem: persistent high packet loss observed on primary"
        " interface eth0 over the last 300 seconds, automatic failover to backup interface eth1 initia"
        "ted, system monitoring continues for additional anomalies in network traffic patterns and dev"
        "ice performance metrics across all active connections"
    };

    int length_type = RandomIndex(3);
    QString message;

    if (length_type == 0)
    {
        message = short_messages[RandomIndex(short_messages.size())];
    }
    else if (length_type == 1)
    {
        message = medium_messages[RandomIndex(medium_messages.size())];
    }
    else
    {
        message = long_messages[RandomIndex(long_messages.size())];
    }

    json["message"] = message;

    static const QStringList kLogLevels = {"INFO", "WARNING", "ERROR"};
    json["severity"] = kLogLevels[RandomIndex(kLogLevels.size())];

    return json;
}

// ====================================================================================================

int DataGenerator::RandomInt(int min, int max)
{
    return QRandomGenerator::global()->bounded(min, max + 1);
}

double DataGenerator::RandomDouble(double min, double max)
{
    return QRandomGenerator::global()->generateDouble() * (max - min) + min;
}

int DataGenerator::RandomIndex(int size)
{
    return QRandomGenerator::global()->bounded(size);
}
