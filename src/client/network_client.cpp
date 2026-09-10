
#include "network_client.h"

#include <QDebug>
#include <QJsonObject>
#include <QRandomGenerator>

#include "protocol.h"

// ====================================================================================================

NetworkClient::NetworkClient(QObject* parent)
    : QObject(parent)
    , socket_(new QTcpSocket(this))
    , reconnect_timer_(new QTimer(this))
    , send_timer_(new QTimer(this))
    , state_(State::kDisconnected)
{
    connect(socket_, &QTcpSocket::connected, this, &NetworkClient::OnSocketConnected);
    connect(socket_, &QTcpSocket::disconnected, this, &NetworkClient::OnSocketDisconnected);
    connect(socket_, &QTcpSocket::readyRead, this, &NetworkClient::OnSocketReadyRead);
    connect(socket_, &QAbstractSocket::errorOccurred, this, &NetworkClient::OnSocketError);

    reconnect_timer_->setInterval(protocol::kReconnectIntervalMs);
    connect(reconnect_timer_, &QTimer::timeout, this, &NetworkClient::OnReconnectTimerTimeout);

    // PreciseTimer нужен для задержек ~10 мс
    send_timer_->setTimerType(Qt::PreciseTimer);
    connect(send_timer_, &QTimer::timeout, this, &NetworkClient::OnSendTimerTimeout);
}

NetworkClient::~NetworkClient()
{
    StopSendingData();
    socket_->disconnectFromHost();
}

void NetworkClient::Start()
{
    ConnectToServer();
}

// ====================================================================================================

void NetworkClient::ConnectToServer()
{
    // Защита от повторных вызовов: если уже подключаемся или активны - выходим
    if (state_ == State::kConnecting ||
        state_ == State::kConnected ||
        state_ == State::kWaitingForStart ||
        state_ == State::kActive)
    {
        return;
    }

    state_ = State::kConnecting;
    ResetState();

    qDebug() << "[Client] Connecting to" << protocol::kServerHost << ":" << protocol::kServerPort;
    socket_->connectToHost(protocol::kServerHost, protocol::kServerPort);
}

void NetworkClient::OnSocketConnected()
{
    state_ = State::kConnected;
    reconnect_timer_->stop();
    qDebug() << "[Client] TCP connected, waiting for server confirmation";
}

void NetworkClient::OnSocketDisconnected()
{
    qDebug() << "[Client] Disconnected from server";
    state_ = State::kDisconnected;
    StopSendingData();
    ResetState();
    reconnect_timer_->start();
}

void NetworkClient::OnSocketReadyRead()
{
    read_buffer_.append(socket_->readAll());
    ProcessIncomingData();
}

// ====================================================================================================

void NetworkClient::ProcessIncomingData()
{
    QJsonObject message;
    while (protocol::TryReadJsonMessage(&read_buffer_, &message))
    {
        // Обработка подтверждения подключения
        if (state_ == State::kConnected &&
            message.contains(protocol::kKeyStatus) &&
            message[protocol::kKeyStatus].toString() == protocol::kStatusConnected)
        {
            HandleConnectionConfirmation(message);
        }

        // Обработка команды "start" от сервера
        if (state_ == State::kWaitingForStart &&
            message.contains(protocol::kKeyCommand) &&
            message[protocol::kKeyCommand].toString() == protocol::kCommandStart)
        {
            state_ = State::kActive;
            qDebug() << "[Client] Received 'start' command, activating data sending";
            StartSendingData();
        }

        // Обработка команды "stop" от сервера
        if (state_ == State::kActive &&
            message.contains(protocol::kKeyCommand) &&
            message[protocol::kKeyCommand].toString() == protocol::kCommandStop)
        {
            state_ = State::kWaitingForStart;  // Возврат в ожидание
            StopSendingData();
            qDebug() << "[Client] Received 'stop' command, pausing data sending";
        }

        // Обработка команды "set_config" от сервера
        if (message.contains(protocol::kKeyCommand) &&
            message[protocol::kKeyCommand].toString() == protocol::kCommandSetConfig)
        {
            QJsonObject config = message["config"].toObject();
            min_bandwidth_ = config["min_bandwidth"].toDouble();
            max_latency_ = config["max_latency"].toDouble();
            max_packet_loss_ = config["max_packet_loss"].toDouble();
            max_jitter_ = config["max_jitter"].toDouble();
            min_throughput_ = config["min_throughput"].toDouble();
            max_error_rate_ = config["max_error_rate"].toDouble();
            max_dns_latency_ = config["max_dns_latency"].toDouble();
            max_tcp_retransmits_ = config["max_tcp_retransmits"].toInt();
            max_connection_count_ = config["max_connection_count"].toInt();
            max_active_streams_ = config["max_active_streams"].toInt();
            max_buffer_occupancy_ = config["max_buffer_occupancy"].toDouble();

            min_uptime_ = config["min_uptime"].toInt();
            max_cpu_usage_ = config["max_cpu_usage"].toInt();
            max_memory_usage_ = config["max_memory_usage"].toInt();
            max_disk_usage_ = config["max_disk_usage"].toInt();
            max_network_interfaces_ = config["max_network_interfaces"].toInt();
            max_active_connections_ = config["max_active_connections"].toInt();
            max_temperature_ = config["max_temperature"].toDouble();
            max_fan_speed_ = config["max_fan_speed"].toInt();
            min_battery_level_ = config["min_battery_level"].toInt();
            max_process_count_ = config["max_process_count"].toInt();
            max_thread_count_ = config["max_thread_count"].toInt();
            max_io_operations_ = config["max_io_operations"].toInt();

            qDebug() << "[Client] All thresholds updated";
        }
    }
}

void NetworkClient::HandleConnectionConfirmation(const QJsonObject& json)
{
    client_id_ = json[protocol::kKeyClientId].toString();
    state_ = State::kWaitingForStart;
    qDebug() << "[Client] Confirmation received, assigned ID:" << client_id_;
    qDebug() << "[Client] Waiting for 'start' command from server";
}

void NetworkClient::CheckAndSendWarnings(const QJsonObject& message)
{
    QString type = message[protocol::kKeyType].toString();
    QStringList warnings;

    if (type == protocol::kTypeNetworkMetrics)
    {
        if (message.contains("bandwidth") && message["bandwidth"].toDouble() < min_bandwidth_)
            warnings << QString("Low bandwidth: %1 (limit: %2)").arg(message["bandwidth"].toDouble()).arg(min_bandwidth_);
        if (message.contains("latency") && message["latency"].toDouble() > max_latency_)
            warnings << QString("High latency: %1 ms (limit: %2)").arg(message["latency"].toDouble()).arg(max_latency_);
        if (message.contains("packet_loss"))
        {
            double pl = message["packet_loss"].toDouble() * 100.0;
            if (pl > max_packet_loss_)
                warnings << QString("High packet loss: %1% (limit: %2%)").arg(pl, 0, 'f', 2).arg(max_packet_loss_);
        }
        if (message.contains("jitter") && message["jitter"].toDouble() > max_jitter_)
            warnings << QString("High jitter: %1 ms (limit: %2)").arg(message["jitter"].toDouble()).arg(max_jitter_);
        if (message.contains("throughput") && message["throughput"].toDouble() < min_throughput_)
            warnings << QString("Low throughput: %1 (limit: %2)").arg(message["throughput"].toDouble()).arg(min_throughput_);
        if (message.contains("error_rate"))
        {
            double er = message["error_rate"].toDouble() * 100.0;
            if (er > max_error_rate_)
                warnings << QString("High error rate: %1% (limit: %2%)").arg(er, 0, 'f', 2).arg(max_error_rate_);
        }
        if (message.contains("dns_latency") && message["dns_latency"].toDouble() > max_dns_latency_)
            warnings << QString("High DNS latency: %1 ms (limit: %2)").arg(message["dns_latency"].toDouble()).arg(max_dns_latency_);
        if (message.contains("tcp_retransmits") && message["tcp_retransmits"].toInt() > max_tcp_retransmits_)
            warnings << QString("High TCP retransmits: %1 (limit: %2)").arg(message["tcp_retransmits"].toInt()).arg(max_tcp_retransmits_);
        if (message.contains("connection_count") && message["connection_count"].toInt() > max_connection_count_)
            warnings << QString("High connection count: %1 (limit: %2)").arg(message["connection_count"].toInt()).arg(max_connection_count_);
        if (message.contains("active_streams") && message["active_streams"].toInt() > max_active_streams_)
            warnings << QString("High active streams: %1 (limit: %2)").arg(message["active_streams"].toInt()).arg(max_active_streams_);
        if (message.contains("buffer_occupancy") && message["buffer_occupancy"].toDouble() > max_buffer_occupancy_)
            warnings << QString("High buffer occupancy: %1% (limit: %2%)").arg(message["buffer_occupancy"].toDouble()).arg(max_buffer_occupancy_);
    }
    else if (type == protocol::kTypeDeviceStatus)
    {
        if (message.contains("uptime") && message["uptime"].toInt() < min_uptime_)
            warnings << QString("Low uptime: %1 sec (limit: %2)").arg(message["uptime"].toInt()).arg(min_uptime_);
        if (message.contains("cpu_usage") && message["cpu_usage"].toInt() > max_cpu_usage_)
            warnings << QString("High CPU usage: %1% (limit: %2%)").arg(message["cpu_usage"].toInt()).arg(max_cpu_usage_);
        if (message.contains("memory_usage") && message["memory_usage"].toInt() > max_memory_usage_)
            warnings << QString("High memory usage: %1% (limit: %2%)").arg(message["memory_usage"].toInt()).arg(max_memory_usage_);
        if (message.contains("disk_usage") && message["disk_usage"].toInt() > max_disk_usage_)
            warnings << QString("High disk usage: %1% (limit: %2%)").arg(message["disk_usage"].toInt()).arg(max_disk_usage_);
        if (message.contains("network_interfaces") && message["network_interfaces"].toInt() > max_network_interfaces_)
            warnings << QString("High network interfaces: %1 (limit: %2)").arg(message["network_interfaces"].toInt()).arg(max_network_interfaces_);
        if (message.contains("active_connections") && message["active_connections"].toInt() > max_active_connections_)
            warnings << QString("High active connections: %1 (limit: %2)").arg(message["active_connections"].toInt()).arg(max_active_connections_);
        if (message.contains("temperature") && message["temperature"].toDouble() > max_temperature_)
            warnings << QString("High temperature: %1 C (limit: %2 C)").arg(message["temperature"].toDouble()).arg(max_temperature_);
        if (message.contains("fan_speed") && message["fan_speed"].toInt() > max_fan_speed_)
            warnings << QString("High fan speed: %1 RPM (limit: %2)").arg(message["fan_speed"].toInt()).arg(max_fan_speed_);
        if (message.contains("battery_level") && message["battery_level"].toInt() < min_battery_level_)
            warnings << QString("Low battery level: %1% (limit: %2%)").arg(message["battery_level"].toInt()).arg(min_battery_level_);
        if (message.contains("process_count") && message["process_count"].toInt() > max_process_count_)
            warnings << QString("High process count: %1 (limit: %2)").arg(message["process_count"].toInt()).arg(max_process_count_);
        if (message.contains("thread_count") && message["thread_count"].toInt() > max_thread_count_)
            warnings << QString("High thread count: %1 (limit: %2)").arg(message["thread_count"].toInt()).arg(max_thread_count_);
        if (message.contains("io_operations") && message["io_operations"].toInt() > max_io_operations_)
            warnings << QString("High IO operations: %1 (limit: %2)").arg(message["io_operations"].toInt()).arg(max_io_operations_);
    }

    for (const QString& warning : warnings)
    {
        QJsonObject log_message;
        log_message[protocol::kKeyType] = protocol::kTypeLog;
        log_message["message"] = warning;
        log_message["severity"] = "WARNING";

        SendJsonMessage(log_message);
        qDebug() << "[Client] Warning sent:" << warning;
    }
}

// ====================================================================================================

void NetworkClient::OnSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    qDebug() << "[Client] Socket error:" << socket_->errorString();

    // Если сокет подключен или подключается - закрываем (сработает OnSocketDisconnected)
    if (socket_->state() != QAbstractSocket::UnconnectedState)
    {
        socket_->disconnectFromHost();
    }
    // Сокет не подключен (ошибка до подключения) - сразу запускаем реконнект
    else
    {
        state_ = State::kDisconnected;
        StopSendingData();
        ResetState();
        reconnect_timer_->start();
        qDebug() << "[Client] Will retry in" << protocol::kReconnectIntervalMs << "ms";
    }
}

void NetworkClient::OnReconnectTimerTimeout()
{
    qDebug() << "[Client] Reconnect timer triggered, attempting to connect";
    ConnectToServer();
}

void NetworkClient::OnSendTimerTimeout()
{
    // Отправка только в состоянии kActive, следующая задержка выбирается случайно
    if (state_ != State::kActive)
    {
        return;
    }

    QJsonObject message = data_generator_.GenerateRandomMessage();
    SendJsonMessage(message);
    CheckAndSendWarnings(message);

    int next_delay = QRandomGenerator::global()->bounded(
        protocol::kMinSendDelayMs, protocol::kMaxSendDelayMs + 1);
    send_timer_->start(next_delay);
}

// ====================================================================================================

void NetworkClient::StartSendingData()
{
    int initial_delay = QRandomGenerator::global()->bounded(
        protocol::kMinSendDelayMs, protocol::kMaxSendDelayMs + 1);
    send_timer_->start(initial_delay);
    qDebug() << "[Client] Started sending data";
}

void NetworkClient::StopSendingData()
{
    send_timer_->stop();
}

void NetworkClient::SendJsonMessage(const QJsonObject& json)
{
    QByteArray data = protocol::SerializeJsonMessage(json);
    socket_->write(data);
    qDebug() << "[Client] Sent:" << json[protocol::kKeyType].toString();
}

void NetworkClient::ResetState()
{
    client_id_.clear();
    read_buffer_.clear();
}
