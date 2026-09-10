
#pragma once


#include <QMainWindow>
#include <QString>

// ====================================================================================================

class QTableWidget;
class QTextEdit;
class QPushButton;
class QThread;

class ServerWorker;

/*
 * Графический интерфейс сервера для управления клиентами и отображения данных
 * Содержит таблицы клиентов и данных, лог событий и кнопки управления
*/
class ServerWindow : public QMainWindow
{
    Q_OBJECT

    // Состояния соединения с клиентом
    // Только значения приводимые к size_t (см. GetStateString())
    enum class State
    {
        kDisconnected,
        kConnected,
        kActive
    };

public:
    explicit ServerWindow(QWidget* parent = nullptr);
    ~ServerWindow() override;

private:
    void SetupUI();
    void SetupConnections();

    QString GetStateString(State state);
    void UpdateClientStatus(const QString& id, State state);

    void AddClientRow(const QString& id, const QString& ip, int port);
    void AddDataRow(const QString& client_id, const QString& type, const QString& content, const QString& time);
    void AppendLog(const QString& message);

private slots:
    void onCellDoubleClicked(int row, int column);

    void OnSendStartClicked();
    void OnSendStopClicked();
    void OnConfigClicked();
    void OnClientConfigReady(const QString& client_id, const QJsonObject& config);

    void OnClientConnected(const QString& id, const QString& ip, int port);
    void OnClientDisconnected(const QString& id);
    void OnDataReceived(const QString& client_id, const QString& type, const QString& content, const QString& time);
    void OnLogMessage(const QString& message);

signals:
    void StartServer();
    void SendStartCommand(const QString& client_id);
    void SendStopCommand(const QString& client_id);
    void SendConfigToClient(const QString& client_id, const QJsonObject& config);
    void RequestClientConfig(const QString& client_id);

private:

    // Элементы управления
    QPushButton* send_start_button_;
    QPushButton* send_stop_button_;
    QPushButton* config_button_;

    // Элементы отображения данных
    QTableWidget* clients_table_;
    QTableWidget* data_table_;
    QTextEdit* log_text_edit_;

    // Быстрый поиск строки таблицы по ID клиента (O(1))
    QHash<QString, int> client_id_to_row_;

    // Сетевая логика и многопоточность
    QThread* server_thread_;
    ServerWorker* server_worker_;
};
