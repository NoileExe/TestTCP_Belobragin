
#include "server_window.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidget>
#include <QTextEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QThread>
#include <QTimer>
#include <QDateTime>
#include <QJsonObject>

#include "server_worker.h"
#include "config_dialog.h"

// ====================================================================================================

ServerWindow::ServerWindow(QWidget* parent)
    : QMainWindow(parent)
    , send_start_button_(new QPushButton("Send START to Selected"))
    , send_stop_button_(new QPushButton("Send STOP to Selected"))
    , config_button_(new QPushButton("Config Thresholds to Selected"))
    , clients_table_(new QTableWidget())
    , data_table_(new QTableWidget())
    , log_text_edit_(new QTextEdit())
    , server_thread_(new QThread(this))
    , server_worker_(new ServerWorker())
{
    SetupUI();
    SetupConnections();

    // Перемещаем воркер в отдельный поток
    server_worker_->moveToThread(server_thread_);
    connect(server_thread_, &QThread::finished, server_worker_, &QObject::deleteLater);
    server_thread_->start();

    // Отложенный старт сервера (даем потоку время инициализироваться)
    QTimer::singleShot(500, this, [this]()
    {
        // Запускаем прослушивание порта через сигнал
        // Сервер начинает слушать порт автоматически при запуске
        emit StartServer();
        AppendLog("Server initialized and listening on port 12345");
    });
}

ServerWindow::~ServerWindow()
{
    server_thread_->quit();
    server_thread_->wait();
}

void ServerWindow::SetupUI()
{
    setWindowTitle("Server");
    resize(1000, 800);

    QWidget* central_widget = new QWidget(this);
    setCentralWidget(central_widget);

    QVBoxLayout* main_layout = new QVBoxLayout(central_widget);

    clients_table_->setColumnCount(4);
    clients_table_->setHorizontalHeaderLabels({"ID", "IP", "Port", "Status"});
    clients_table_->horizontalHeader()->setStretchLastSection(true);
    clients_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    clients_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    clients_table_->setSelectionMode(QAbstractItemView::SingleSelection);
    main_layout->addWidget(clients_table_);

    QHBoxLayout* client_controls_layout = new QHBoxLayout();
    client_controls_layout->addWidget(send_start_button_);
    client_controls_layout->addWidget(send_stop_button_);
    client_controls_layout->addStretch();
    client_controls_layout->addWidget(config_button_);
    main_layout->addLayout(client_controls_layout);

    data_table_->setColumnCount(4);
    data_table_->setHorizontalHeaderLabels({"Client ID", "Type", "Content", "Time"});
    data_table_->horizontalHeader()->setStretchLastSection(true);
    data_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    data_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    data_table_->setSelectionMode(QAbstractItemView::SingleSelection);
    main_layout->addWidget(data_table_);

    log_text_edit_->setReadOnly(true);
    log_text_edit_->setMaximumHeight(150);
    main_layout->addWidget(log_text_edit_);
}

void ServerWindow::SetupConnections()
{
    connect(send_start_button_, &QPushButton::clicked, this, &ServerWindow::OnSendStartClicked);
    connect(send_stop_button_, &QPushButton::clicked, this, &ServerWindow::OnSendStopClicked);
    connect(config_button_, &QPushButton::clicked, this, &ServerWindow::OnConfigClicked);
    connect(data_table_, &QTableWidget::cellDoubleClicked, this, &ServerWindow::onCellDoubleClicked);

    connect(this, &ServerWindow::StartServer, server_worker_, &ServerWorker::OnStartServer);
    connect(this, &ServerWindow::SendStartCommand, server_worker_, &ServerWorker::OnSendStartCommand);
    connect(this, &ServerWindow::SendStopCommand, server_worker_, &ServerWorker::OnSendStopCommand);
    connect(this, &ServerWindow::RequestClientConfig, server_worker_, &ServerWorker::OnRequestClientConfig);
    connect(this, &ServerWindow::SendConfigToClient, server_worker_, &ServerWorker::OnSendConfigToClient);

    connect(server_worker_, &ServerWorker::ClientConfigReady, this, &ServerWindow::OnClientConfigReady);
    connect(server_worker_, &ServerWorker::ClientConnected, this, &ServerWindow::OnClientConnected);
    connect(server_worker_, &ServerWorker::ClientDisconnected, this, &ServerWindow::OnClientDisconnected);
    connect(server_worker_, &ServerWorker::DataReceived, this, &ServerWindow::OnDataReceived);
    connect(server_worker_, &ServerWorker::LogMessage, this, &ServerWindow::OnLogMessage);
}

// ====================================================================================================

void ServerWindow::OnSendStartClicked()
{
    int row = clients_table_->currentRow();
    if (row < 0)
    {
        AppendLog("No client selected");
        return;
    }


    QString client_id = clients_table_->item(row, 0)->text();
    QString client_state = clients_table_->item(row, 3)->text();

    if (client_state == GetStateString(State::kDisconnected))
    {
        AppendLog(QString("Client %1 is disconnected").arg(client_id));
        return;
    }

    emit SendStartCommand(client_id);

    AppendLog("Sent START command to client: " + client_id);
}

void ServerWindow::OnSendStopClicked()
{
    int row = clients_table_->currentRow();
    if (row < 0)
    {
        AppendLog("No client selected");
        return;
    }


    QString client_id = clients_table_->item(row, 0)->text();
    QString client_state = clients_table_->item(row, 3)->text();

    if (client_state == GetStateString(State::kDisconnected))
    {
        AppendLog(QString("Cannot stop: client %1 is disconnected").arg(client_id));
        return;
    }
    else if (client_state == GetStateString(State::kActive))
    {
        UpdateClientStatus(client_id, State::kConnected);
    }

    emit SendStopCommand(client_id);
    AppendLog("Sent STOP command to client: " + client_id);
}

// ====================================================================================================

void ServerWindow::OnConfigClicked()
{
    int row = clients_table_->currentRow();
    if (row < 0)
    {
        AppendLog("No client selected for configuration");
        return;
    }

    QString client_id = clients_table_->item(row, 0)->text();
    QString client_state = clients_table_->item(row, 3)->text();

    if (client_state == GetStateString(State::kDisconnected))
    {
        AppendLog(QString("Cannot configure: Client %1 is disconnected").arg(client_id));
        return;
    }

    emit RequestClientConfig(client_id);
}

void ServerWindow::OnClientConfigReady(const QString& client_id, const QJsonObject& config)
{
    ConfigDialog dialog(client_id, config, this);

    if (dialog.exec() == QDialog::Accepted)
    {
        int row = client_id_to_row_[client_id];
        if (client_id == clients_table_->item(row, 0)->text())
        {
            QString client_state = clients_table_->item(row, 3)->text();
            if (client_state == GetStateString(State::kDisconnected))
            {
                AppendLog(QString("Cannot configure: Client %1 is disconnected").arg(client_id));
                return;
            }
        }

        QJsonObject new_config;
        new_config["min_bandwidth"] = dialog.GetMinBandwidth();
        new_config["max_latency"] = dialog.GetMaxLatency();
        new_config["max_packet_loss"] = dialog.GetMaxPacketLoss();
        new_config["max_jitter"] = dialog.GetMaxJitter();
        new_config["min_throughput"] = dialog.GetMinThroughput();
        new_config["max_error_rate"] = dialog.GetMaxErrorRate();
        new_config["max_dns_latency"] = dialog.GetMaxDnsLatency();
        new_config["max_tcp_retransmits"] = dialog.GetMaxTcpRetransmits();
        new_config["max_connection_count"] = dialog.GetMaxConnectionCount();
        new_config["max_active_streams"] = dialog.GetMaxActiveStreams();
        new_config["max_buffer_occupancy"] = dialog.GetMaxBufferOccupancy();

        new_config["min_uptime"] = dialog.GetMinUptime();
        new_config["max_cpu_usage"] = dialog.GetMaxCpuUsage();
        new_config["max_memory_usage"] = dialog.GetMaxMemoryUsage();
        new_config["max_disk_usage"] = dialog.GetMaxDiskUsage();
        new_config["max_network_interfaces"] = dialog.GetMaxNetworkInterfaces();
        new_config["max_active_connections"] = dialog.GetMaxActiveConnections();
        new_config["max_temperature"] = dialog.GetMaxTemperature();
        new_config["max_fan_speed"] = dialog.GetMaxFanSpeed();
        new_config["min_battery_level"] = dialog.GetMinBatteryLevel();
        new_config["max_process_count"] = dialog.GetMaxProcessCount();
        new_config["max_thread_count"] = dialog.GetMaxThreadCount();
        new_config["max_io_operations"] = dialog.GetMaxIoOperations();

        emit SendConfigToClient(client_id, new_config);
        AppendLog("Configuration updated for client: " + client_id);
    }
}

void ServerWindow::onCellDoubleClicked(int row, int column)
{
    QTableWidgetItem* item = data_table_->item(row, column);
    if (!item)
    {
        return;
    }

    QString tooltip_text = item->toolTip();
    if (tooltip_text.isEmpty())
    {
        // Если тултип не задан, можно взять текст самой ячейки
        tooltip_text = item->text();
    }


    // Создаем модальный диалог для отображения сообщения
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("View message");
    dialog->setMinimumSize(400, 300);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    QTextEdit* text_edit = new QTextEdit(dialog);
    text_edit->setPlainText(tooltip_text);
    text_edit->setWordWrapMode(QTextOption::WordWrap);
    text_edit->setReadOnly(true);
    text_edit->setFrameStyle(QFrame::NoFrame);
    layout->addWidget(text_edit);

    QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Ok, dialog);
    connect(button_box, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    layout->addWidget(button_box);

    dialog->exec();

    delete dialog;
}

// ====================================================================================================

void ServerWindow::OnClientConnected(const QString& id, const QString& ip, int port)
{
    AddClientRow(id, ip, port);
    AppendLog("Client connected: " + id + " from " + ip + ":" + QString::number(port));
}

void ServerWindow::OnClientDisconnected(const QString& id)
{
    UpdateClientStatus(id, State::kDisconnected);
    AppendLog("Client disconnected: " + id);
}

void ServerWindow::OnDataReceived(const QString& client_id, const QString& type, const QString& content, const QString& time)
{
    AddDataRow(client_id, type, content, time);
    UpdateClientStatus(client_id, State::kActive);
}

void ServerWindow::OnLogMessage(const QString& message)
{
    AppendLog(message);
}

// ====================================================================================================

QString ServerWindow::GetStateString(State state)
{
    static const QStringList states = {
        "Disconnected",
        "Connected",
        "Active"
    };

    size_t idx = static_cast<size_t>(state);

    if (idx > states.size())
    {
        return QString();
    }

    return states[idx];
}

void ServerWindow::UpdateClientStatus(const QString& id, State state)
{
    if (client_id_to_row_.contains(id))
    {
        int row = client_id_to_row_[id];
        QString status_string = GetStateString(state);

        clients_table_->setItem(row, 3, new QTableWidgetItem(status_string));
    }
}

// ====================================================================================================

void ServerWindow::AddClientRow(const QString& id, const QString& ip, int port)
{
    // Быстрая проверка через хеш-таблицу O(1)
    if (client_id_to_row_.contains(id))
    {
        int row = client_id_to_row_[id];

        // Клиент уже есть, обновляем данные
        clients_table_->setItem(row, 1, new QTableWidgetItem(ip));
        clients_table_->setItem(row, 2, new QTableWidgetItem(QString::number(port)));
    }
    else
    {
        int row = clients_table_->rowCount();

        // Добавляем новую строку
        clients_table_->insertRow(row);
        clients_table_->setItem(row, 0, new QTableWidgetItem(id));
        clients_table_->setItem(row, 1, new QTableWidgetItem(ip));
        clients_table_->setItem(row, 2, new QTableWidgetItem(QString::number(port)));

        // Сохраняем индекс строки для быстрого поиска
        client_id_to_row_[id] = row;
    }

    UpdateClientStatus(id, State::kConnected);
}

void ServerWindow::AddDataRow(const QString& clientId, const QString& type, const QString& content, const QString& time)
{
    int row = data_table_->rowCount();
    data_table_->insertRow(row);

    QTableWidgetItem* item = new QTableWidgetItem(clientId);
    item->setToolTip(content);
    data_table_->setItem(row, 0, item);

    item = new QTableWidgetItem(type);
    item->setToolTip(content);
    data_table_->setItem(row, 1, item);

    item = new QTableWidgetItem(content);
    item->setToolTip(content);
    data_table_->setItem(row, 2, item);

    item = new QTableWidgetItem(time);
    item->setToolTip(content);
    data_table_->setItem(row, 3, item);
}

void ServerWindow::AppendLog(const QString& message)
{
    QString time_stamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    log_text_edit_->append("[" + time_stamp + "] " + message);
}
