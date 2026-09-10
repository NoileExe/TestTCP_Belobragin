
#pragma once


#include <QObject>
#include <QMap>
#include <QByteArray>
#include <QString>
#include <QJsonObject>

// ====================================================================================================

class QTcpServer;
class QTcpSocket;

/*
 * Сетевой воркер для приема подключений клиентов и парсинга JSON-данных
 * Работает в отдельном потоке, общается с GUI через сигналы и слоты
 */
class ServerWorker : public QObject
{
    Q_OBJECT

public:
    explicit ServerWorker(QObject* parent = nullptr);
    ~ServerWorker() override;

private:
    void StopServer();

    void ProcessClientData(QTcpSocket* socket);
    void SendConfirmation(QTcpSocket* socket, const QString& client_id);
    void SendCommand(QTcpSocket* socket, const QString& command);
    QString GenerateClientId();
    QString ExtractContent(const QJsonObject& json);

public slots:
    void OnStartServer();
    void OnSendStartCommand(const QString& client_id);
    void OnSendStopCommand(const QString& client_id);

    void OnRequestClientConfig(const QString& client_id);
    void OnSendConfigToClient(const QString& client_id, const QJsonObject& config);

private slots:
    void OnNewConnection();
    void OnClientReadyRead();
    void OnClientDisconnected();

signals:
    void ClientConnected(const QString& id, const QString& ip, int port);
    void ClientDisconnected(const QString& id);
    void DataReceived(const QString& client_id, const QString& type, const QString& content, const QString& time);
    void LogMessage(const QString& message);

    void ClientConfigReady(const QString& client_id, const QJsonObject& config);

private:
    // Структура для хранения состояния каждого подключенного клиента
    struct ClientInfo
    {
        QString id_;
        QString ip_;
        int port_;
        QByteArray read_buffer_;
        QJsonObject current_config_;
    };


    // Сетевой компонент
    QTcpServer* server_;

    // Хранилище активных клиентов и их буферов
    QMap<QTcpSocket*, ClientInfo> clients_;
};
