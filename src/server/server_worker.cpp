
#include "server_worker.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QUuid>
#include <QJsonDocument>
#include <QDateTime>

#include "protocol.h"

// ====================================================================================================

ServerWorker::ServerWorker(QObject* parent)
    : QObject(parent)
    , server_(new QTcpServer(this))
{
    connect(server_, &QTcpServer::newConnection, this, &ServerWorker::OnNewConnection);
}

ServerWorker::~ServerWorker()
{
    // Гарантируем чистую остановку и освобождение порта при уничтожении объекта
    StopServer();
}

// ====================================================================================================

void ServerWorker::StopServer()
{
    if (!server_->isListening())
    {
        return;
    }

    for (QTcpSocket* socket : clients_.keys())
    {
        socket->disconnectFromHost();
    }
    clients_.clear();

    server_->close();
    emit LogMessage("Server stopped");
}

void ServerWorker::OnStartServer()
{
    if (server_->isListening())
    {
        return;
    }

    if (!server_->listen(QHostAddress::Any, protocol::kServerPort))
    {
        emit LogMessage("Failed to start server: " + server_->errorString());
        return;
    }

    emit LogMessage("Server started on port " + QString::number(protocol::kServerPort));
}

// ====================================================================================================

void ServerWorker::OnRequestClientConfig(const QString& client_id)
{
    for (const auto& info : clients_)
    {
        if (info.id_ == client_id)
        {
            emit ClientConfigReady(client_id, info.current_config_);
            return;
        }
    }
}

void ServerWorker::OnSendConfigToClient(const QString& client_id, const QJsonObject& config)
{
    for (auto it = clients_.begin(), itEnd = clients_.end(); it != itEnd; ++it)
    {
        if (it.value().id_ == client_id)
        {
            it.value().current_config_ = config;

            QJsonObject command;
            command[QString(protocol::kKeyCommand)] = QString(protocol::kCommandSetConfig);
            command["config"] = config;

            QByteArray data = protocol::SerializeJsonMessage(command);
            it.key()->write(data);  // Отправка настроек клиенту
            break;
        }
    }
}

// ====================================================================================================

void ServerWorker::OnNewConnection()
{
    while (server_->hasPendingConnections())
    {
        QTcpSocket* socket = server_->nextPendingConnection();

        QString client_id = GenerateClientId();

        ClientInfo info;
        info.id_ = client_id;

        //info.ip_ = socket->peerAddress().toString();

        // Нормализуем адрес до IPv4-адресов
        QHostAddress peer_addr = socket->peerAddress();
        if (peer_addr.protocol() == QAbstractSocket::IPv6Protocol)
        {
            bool ok = false;
            quint32 ipv4 = peer_addr.toIPv4Address(&ok);
            if (ok)
                peer_addr = QHostAddress(ipv4);
        }
        info.ip_ = peer_addr.toString();

        info.port_ = socket->peerPort();

        // TODO перенести значения по умолчанию в protocol.h
        info.current_config_ = QJsonObject{
            {"min_bandwidth", 50.0}, {"max_latency", 100.0}, {"max_packet_loss", 5.0},
            {"max_jitter", 30.0}, {"min_throughput", 10.0}, {"max_error_rate", 5.0},
            {"max_dns_latency", 150.0}, {"max_tcp_retransmits", 50}, {"max_connection_count", 500},
            {"max_active_streams", 50}, {"max_buffer_occupancy", 80.0},
            {"min_uptime", 3600}, {"max_cpu_usage", 80}, {"max_memory_usage", 90},
            {"max_disk_usage", 90}, {"max_network_interfaces", 5}, {"max_active_connections", 200},
            {"max_temperature", 75.0}, {"max_fan_speed", 4000}, {"min_battery_level", 20},
            {"max_process_count", 300}, {"max_thread_count", 500}, {"max_io_operations", 5000}
        };

        clients_[socket] = info;

        connect(socket, &QTcpSocket::readyRead, this, &ServerWorker::OnClientReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &ServerWorker::OnClientDisconnected);

        SendConfirmation(socket, client_id);

        emit ClientConnected(client_id, info.ip_, info.port_);
        emit LogMessage("New connection from " + info.ip_ + ":" + QString::number(info.port_));
    }
}

void ServerWorker::OnClientReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || !clients_.contains(socket))
    {
        return;
    }

    clients_[socket].read_buffer_.append(socket->readAll());
    ProcessClientData(socket);
}

void ServerWorker::ProcessClientData(QTcpSocket* socket)
{
    ClientInfo &info = clients_[socket];
    QJsonObject json;

    while (protocol::TryReadJsonMessage(&info.read_buffer_, &json))
    {
        QString type = json[protocol::kKeyType].toString();
        QString content = ExtractContent(json);
        QString time = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

        emit DataReceived(info.id_, type, content, time);
    }
}

void ServerWorker::OnClientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || !clients_.contains(socket))
    {
        return;
    }

    QString client_id = clients_[socket].id_;
    clients_.remove(socket);
    socket->deleteLater();

    emit ClientDisconnected(client_id);
    emit LogMessage("Client disconnected: " + client_id);
}

// ====================================================================================================

void ServerWorker::OnSendStartCommand(const QString& client_id)
{
    for (auto it = clients_.begin(), itEnd = clients_.end(); it != itEnd; ++it)
    {
        if (it.value().id_ == client_id)
        {
            SendCommand(it.key(), protocol::kCommandStart);
            break;
        }
    }
}

void ServerWorker::OnSendStopCommand(const QString& client_id)
{
    for (auto it = clients_.begin(), itEnd = clients_.end(); it != itEnd; ++it)
    {
        if (it.value().id_ == client_id)
        {
            SendCommand(it.key(), protocol::kCommandStop);
            break;
        }
    }
}

// ====================================================================================================

void ServerWorker::SendConfirmation(QTcpSocket* socket, const QString& client_id)
{
    QJsonObject json;
    json[protocol::kKeyStatus] = protocol::kStatusConnected;
    json[protocol::kKeyClientId] = client_id;

    QByteArray data = protocol::SerializeJsonMessage(json);
    socket->write(data);
}

void ServerWorker::SendCommand(QTcpSocket* socket, const QString& command)
{
    QJsonObject json;
    json[protocol::kKeyCommand] = command;

    QByteArray data = protocol::SerializeJsonMessage(json);
    socket->write(data);
}

QString ServerWorker::GenerateClientId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

QString ServerWorker::ExtractContent(const QJsonObject& json)
{
    QString type = json[protocol::kKeyType].toString();

    if (type == protocol::kTypeNetworkMetrics)
    {
        QString msg = QString("bandwidth=%1, latency=%2, loss=%3")
            .arg(json["bandwidth"].toDouble())
            .arg(json["latency"].toDouble())
            .arg(json["packet_loss"].toDouble());

        // Поля присутствующие в сообщениях средней длины
        if (json.contains("jitter"))
            msg += QString(", jitter=%1").arg(json["jitter"].toDouble());
        if (json.contains("throughput"))
            msg += QString(", throughput=%1").arg(json["throughput"].toDouble());
        if (json.contains("error_rate"))
            msg += QString(", error_rate=%1").arg(json["error_rate"].toDouble());

        // Поля присутствующие в сообщениях считающихся дилнными
        if (json.contains("dns_latency"))
            msg += QString(", dns_latency=%1").arg(json["dns_latency"].toDouble());
        if (json.contains("tcp_retransmits"))
            msg += QString(", tcp_retransmits=%1").arg(json["tcp_retransmits"].toInt());
        if (json.contains("connection_count"))
            msg += QString(", connection_count=%1").arg(json["connection_count"].toInt());
        if (json.contains("active_streams"))
            msg += QString(", active_streams=%1").arg(json["active_streams"].toInt());
        if (json.contains("buffer_occupancy"))
            msg += QString(", buffer_occupancy=%1").arg(json["buffer_occupancy"].toDouble());

        return msg;
    }
    else if (type == protocol::kTypeDeviceStatus)
    {
        QString msg = QString("uptime=%1, cpu=%2%, mem=%3%")
            .arg(json["uptime"].toInt())
            .arg(json["cpu_usage"].toInt())
            .arg(json["memory_usage"].toInt());

        // Поля присутствующие в сообщениях средней длины
        if (json.contains("disk_usage"))
            msg += QString(", disk_usage=%1").arg(json["disk_usage"].toInt());
        if (json.contains("network_interfaces"))
            msg += QString(", network_interfaces=%1").arg(json["network_interfaces"].toInt());
        if (json.contains("active_connections"))
            msg += QString(", active_connections=%1").arg(json["active_connections"].toInt());

        // Поля присутствующие в сообщениях считающихся дилнными
        if (json.contains("temperature"))
            msg += QString(", temperature=%1").arg(json["temperature"].toDouble());
        if (json.contains("fan_speed"))
            msg += QString(", fan_speed=%1").arg(json["fan_speed"].toInt());
        if (json.contains("battery_level"))
            msg += QString(", battery_level=%1").arg(json["battery_level"].toInt());
        if (json.contains("process_count"))
            msg += QString(", process_count=%1").arg(json["process_count"].toInt());
        if (json.contains("thread_count"))
            msg += QString(", thread_count=%1").arg(json["thread_count"].toInt());
        if (json.contains("io_operations"))
            msg += QString(", io_operations=%1").arg(json["io_operations"].toInt());

        return msg;
    }
    else if (type == protocol::kTypeLog)
    {
        return QString("[%1] %2")
            .arg(json["severity"].toString())
            .arg(json["message"].toString());
    }

    return json[protocol::kKeyMessage].toString();
}
