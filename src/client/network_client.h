
#pragma once


#include <QObject>
#include <QTimer>
#include <QTcpSocket>
#include <QString>
#include <QByteArray>

#include "data_generator.h"

// ====================================================================================================

/*
 * Сетевой клиент устройства
 * Отвечает за подключение к серверу, ожидание подтверждения
 * и периодическую отправку JSON-данных (метрики, статус, логи)
 * Гарантирует отправку данных только после получения подтверждения от сервера
*/
class NetworkClient : public QObject
{
    Q_OBJECT

public:
    // Инициализирует сокет, таймеры и связывает сигналы со слотами
    // Начальное состояние - kDisconnected
    explicit NetworkClient(QObject* parent = nullptr);

    ~NetworkClient() override;

    // Запускает клиент: инициирует первое подключение
    void Start();

private:
    // Состояния конечного автомата клиента
    enum class State
    {
        kDisconnected,      // Соединение отсутствует, идёт ожидание реконнекта
        kConnecting,        // Идёт попытка установки соединения
        kConnected,         // Соединение установлено, но подтверждение от сервера ещё не получено
        kWaitingForStart,   // Соединено, подтверждение получено, ждем команду "start"
        kActive             // Соединено, подтверждено, команда "start" получена - отправляем данные
    };

    void ConnectToServer();
    void StartSendingData();
    void StopSendingData();
    void SendJsonMessage(const QJsonObject& json);

    // Отправка отдельных логов в случае если значения Network Metrics или Device Status
    // вышли за установленные предельные значения
    void CheckAndSendWarnings(const QJsonObject& message);

    // Извлекает полные JSON-сообщения и обрабатывает их.
    // Модифицирует read_buffer_, удаляя прочитанные байты.
    void ProcessIncomingData();

    // Сохраняет client_id из ответа сервера и переводит клиент в kActive
    void HandleConnectionConfirmation(const QJsonObject& json);

    // Очищает client_id и read_buffer_ при разрыве соединения
    void ResetState();

private slots:
    void OnSocketConnected();
    void OnSocketDisconnected();
    void OnSocketReadyRead();
    void OnSocketError(QAbstractSocket::SocketError error);
    void OnReconnectTimerTimeout();
    void OnSendTimerTimeout();

private:
    QTcpSocket* socket_;                  // TCP-сокет для связи с сервером

    QTimer* reconnect_timer_;             // Таймер попыток переподключения
    QTimer* send_timer_;                  // Таймер отправки данных

    DataGenerator data_generator_;        // Генератор случайных данных
    QByteArray read_buffer_;              // Буфер для накопления входящих данных
    State state_;                         // Текущее состояние клиента
    QString client_id_;                   // ID, назначенный сервером

    // TODO перенести значения по умолчанию в protocol.h
    // Пороги для Network Metrics (11)
    double min_bandwidth_ = 50.0;
    double max_latency_ = 100.0;
    double max_packet_loss_ = 5.0;
    double max_jitter_ = 30.0;
    double min_throughput_ = 10.0;
    double max_error_rate_ = 5.0;
    double max_dns_latency_ = 150.0;
    int max_tcp_retransmits_ = 50;
    int max_connection_count_ = 500;
    int max_active_streams_ = 50;
    double max_buffer_occupancy_ = 80.0;

    // Пороги для Device Status (12)
    int min_uptime_ = 3600;
    int max_cpu_usage_ = 80;
    int max_memory_usage_ = 90;
    int max_disk_usage_ = 90;
    int max_network_interfaces_ = 5;
    int max_active_connections_ = 200;
    double max_temperature_ = 75.0;
    int max_fan_speed_ = 4000;
    int min_battery_level_ = 20;
    int max_process_count_ = 300;
    int max_thread_count_ = 500;
    int max_io_operations_ = 5000;
};
