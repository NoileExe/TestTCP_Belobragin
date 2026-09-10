
#include "config_dialog.h"

#include <QTabWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QJsonObject>
#include <QSignalBlocker>

// ====================================================================================================

ConfigDialog::ConfigDialog(const QString& client_id, const QJsonObject& config, QWidget* parent)
    : QDialog(parent)
    , min_bw_spin_(nullptr), max_lat_spin_(nullptr), max_loss_spin_(nullptr), max_jitter_spin_(nullptr)
    , min_thr_spin_(nullptr), max_err_spin_(nullptr), max_dns_spin_(nullptr), max_tcp_spin_(nullptr)
    , max_conn_spin_(nullptr), max_stream_spin_(nullptr), max_buf_spin_(nullptr)
    , min_up_spin_(nullptr), max_cpu_spin_(nullptr), max_mem_spin_(nullptr), max_disk_spin_(nullptr)
    , max_if_spin_(nullptr), max_act_conn_spin_(nullptr), max_temp_spin_(nullptr), max_fan_spin_(nullptr)
    , min_bat_spin_(nullptr), max_proc_spin_(nullptr), max_thr_cnt_spin_(nullptr), max_io_spin_(nullptr)
    , tab_widget_(new QTabWidget())
    , ok_button_(new QPushButton("OK"))
    , cancel_button_(new QPushButton("Cancel"))
{
    SetupUI(client_id);
    SetConfig(config);

    // Кнопка OK неактивна до изменения хотя бы одного значения
    ok_button_->setEnabled(false);

    connect(ok_button_, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel_button_, &QPushButton::clicked, this, &QDialog::reject);
}

void ConfigDialog::SetupUI(const QString& client_id)
{
    setWindowTitle(QString("Client %1 Warning Thresholds").arg(client_id));
    resize(500, 550);

    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->addWidget(tab_widget_);

    // ========================================================================
    // Вкладка 1: Network Metrics (11 параметров)
    // ========================================================================
    QWidget* tab_metrics = new QWidget();
    QVBoxLayout* layout_metrics = new QVBoxLayout(tab_metrics);
    QScrollArea* scroll_metrics = new QScrollArea();
    scroll_metrics->setWidgetResizable(true);
    QWidget* content_metrics = new QWidget();
    QVBoxLayout* vbox_metrics = new QVBoxLayout(content_metrics);

    min_bw_spin_ = AddDoubleRow(vbox_metrics, "Min Bandwidth:", 0.0, 2000.0, 50.0, 1);
    max_lat_spin_ = AddDoubleRow(vbox_metrics, "Max Latency (ms):", 0.0, 1000.0, 100.0, 1);
    max_loss_spin_ = AddDoubleRow(vbox_metrics, "Max Packet Loss (%):", 0.0, 100.0, 5.0, 2);
    max_jitter_spin_ = AddDoubleRow(vbox_metrics, "Max Jitter (ms):",0.0, 500.0, 30.0, 1);
    min_thr_spin_ = AddDoubleRow(vbox_metrics, "Min Throughput:", 0.0, 1000.0, 10.0, 1);
    max_err_spin_ = AddDoubleRow(vbox_metrics, "Max Error Rate (%):", 0.0, 100.0, 5.0, 2);
    max_dns_spin_ = AddDoubleRow(vbox_metrics, "Max DNS Latency (ms):", 0.0, 1000.0, 150.0, 1);
    max_tcp_spin_ = AddIntRow(vbox_metrics, "Max TCP Retransmits:", 0, 1000, 50);
    max_conn_spin_ = AddIntRow(vbox_metrics, "Max Connection Count:", 0, 5000, 500);
    max_stream_spin_ = AddIntRow(vbox_metrics, "Max Active Streams:", 0, 1000, 50);
    max_buf_spin_ = AddDoubleRow(vbox_metrics, "Max Buffer Occupancy (%):", 0.0, 100.0, 80.0, 1);

    vbox_metrics->addStretch();
    scroll_metrics->setWidget(content_metrics);
    layout_metrics->addWidget(scroll_metrics);
    tab_widget_->addTab(tab_metrics, "Network Metrics");

    // ========================================================================
    // Вкладка 2: Device Status (12 параметров)
    // ========================================================================
    QWidget* tab_status = new QWidget();
    QVBoxLayout* layout_status = new QVBoxLayout(tab_status);
    QScrollArea* scroll_status = new QScrollArea();
    scroll_status->setWidgetResizable(true);
    QWidget* content_status = new QWidget();
    QVBoxLayout* vbox_status = new QVBoxLayout(content_status);

    min_up_spin_ = AddIntRow(vbox_status, "Min Uptime (sec):", 0, 86400, 3600);
    max_cpu_spin_ = AddIntRow(vbox_status, "Max CPU Usage (%):", 0, 100, 80);
    max_mem_spin_ = AddIntRow(vbox_status, "Max Memory Usage (%):", 0, 100, 90);
    max_disk_spin_ = AddIntRow(vbox_status, "Max Disk Usage (%):", 0, 100, 90);
    max_if_spin_ = AddIntRow(vbox_status, "Max Network Interfaces:", 0, 20, 5);
    max_act_conn_spin_ = AddIntRow(vbox_status, "Max Active Connections:", 0, 2000, 200);
    max_temp_spin_ = AddDoubleRow(vbox_status, "Max Temperature (°C):", 0.0, 120.0, 75.0, 1);
    max_fan_spin_ = AddIntRow(vbox_status, "Max Fan Speed (RPM):", 0, 10000, 4000);
    min_bat_spin_ = AddIntRow(vbox_status, "Min Battery Level (%):", 0, 100, 20);
    max_proc_spin_ = AddIntRow(vbox_status, "Max Process Count:", 0, 2000, 300);
    max_thr_cnt_spin_ = AddIntRow(vbox_status, "Max Thread Count:", 0, 5000, 500);
    max_io_spin_ = AddIntRow(vbox_status, "Max IO Operations:", 0, 50000, 5000);

    vbox_status->addStretch();
    scroll_status->setWidget(content_status);
    layout_status->addWidget(scroll_status);
    tab_widget_->addTab(tab_status, "Device Status");

    // ========================================================================
    // Кнопки OK / Cancel
    // ========================================================================
    QHBoxLayout* btn_layout = new QHBoxLayout();
    btn_layout->addStretch();
    btn_layout->addWidget(ok_button_);
    btn_layout->addWidget(cancel_button_);
    main_layout->addLayout(btn_layout);
}

QSpinBox* ConfigDialog::AddIntRow(QVBoxLayout* layout, const QString& lbl, int min, int max, int val)
{
    auto *row_layout = new QHBoxLayout();
    row_layout->addWidget(new QLabel(lbl));
    QSpinBox* s = new QSpinBox();
    s->setRange(min, max);
    s->setValue(val);
    row_layout->addWidget(s);
    layout->addLayout(row_layout);

    // Автоматически активируем кнопку OK при изменении значения
    connect(s, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigDialog::OnValueChanged);

    return s;
}

QDoubleSpinBox* ConfigDialog::AddDoubleRow(QVBoxLayout* layout, const QString& lbl, double min, double max, double val, int dec)
{
    auto *row_layout = new QHBoxLayout();
    row_layout->addWidget(new QLabel(lbl));

    QDoubleSpinBox* s = new QDoubleSpinBox();
    s->setRange(min, max);
    s->setValue(val);
    s->setDecimals(dec);
    row_layout->addWidget(s);
    layout->addLayout(row_layout);

    // Автоматически активируем кнопку OK при изменении значения
    connect(s, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ConfigDialog::OnValueChanged);

    return s;
}

void ConfigDialog::OnValueChanged()
{
    ok_button_->setEnabled(true);
}

void ConfigDialog::SetConfig(const QJsonObject& config)
{
    if (config.isEmpty())
        return;

    // Network Metrics (11)
    QSignalBlocker min_bw_spin_blocker(min_bw_spin_);
    QSignalBlocker max_lat_spin_blocker(max_lat_spin_);
    QSignalBlocker max_loss_spin_blocker(max_loss_spin_);
    QSignalBlocker max_jitter_spin_blocker(max_jitter_spin_);
    QSignalBlocker min_thr_spin_blocker(min_thr_spin_);
    QSignalBlocker max_err_spin_blocker(max_err_spin_);
    QSignalBlocker max_dns_spin_blocker(max_dns_spin_);
    QSignalBlocker max_tcp_spin_blocker(max_tcp_spin_);
    QSignalBlocker max_conn_spin_blocker(max_conn_spin_);
    QSignalBlocker max_stream_spin_blocker(max_stream_spin_);
    QSignalBlocker max_buf_spin_blocker(max_buf_spin_);

    min_bw_spin_->setValue(config["min_bandwidth"].toDouble());
    max_lat_spin_->setValue(config["max_latency"].toDouble());
    max_loss_spin_->setValue(config["max_packet_loss"].toDouble());
    max_jitter_spin_->setValue(config["max_jitter"].toDouble());
    min_thr_spin_->setValue(config["min_throughput"].toDouble());
    max_err_spin_->setValue(config["max_error_rate"].toDouble());
    max_dns_spin_->setValue(config["max_dns_latency"].toDouble());
    max_tcp_spin_->setValue(config["max_tcp_retransmits"].toInt());
    max_conn_spin_->setValue(config["max_connection_count"].toInt());
    max_stream_spin_->setValue(config["max_active_streams"].toInt());
    max_buf_spin_->setValue(config["max_buffer_occupancy"].toDouble());

    // Device Status (12)
    QSignalBlocker min_up_spin_blocker(min_up_spin_);
    QSignalBlocker max_cpu_spin_blocker(max_cpu_spin_);
    QSignalBlocker max_mem_spin_blocker(max_mem_spin_);
    QSignalBlocker max_disk_spin_blocker(max_disk_spin_);
    QSignalBlocker max_if_spin_blocker(max_if_spin_);
    QSignalBlocker max_act_conn_spin_blocker(max_act_conn_spin_);
    QSignalBlocker max_temp_spin_blocker(max_temp_spin_);
    QSignalBlocker max_fan_spin_blocker(max_fan_spin_);
    QSignalBlocker min_bat_spin_blocker(min_bat_spin_);
    QSignalBlocker max_proc_spin_blocker(max_proc_spin_);
    QSignalBlocker max_thr_cnt_spin_blocker(max_thr_cnt_spin_);
    QSignalBlocker max_io_spin_blocker(max_io_spin_);

    min_up_spin_->setValue(config["min_uptime"].toInt());
    max_cpu_spin_->setValue(config["max_cpu_usage"].toInt());
    max_mem_spin_->setValue(config["max_memory_usage"].toInt());
    max_disk_spin_->setValue(config["max_disk_usage"].toInt());
    max_if_spin_->setValue(config["max_network_interfaces"].toInt());
    max_act_conn_spin_->setValue(config["max_active_connections"].toInt());
    max_temp_spin_->setValue(config["max_temperature"].toDouble());
    max_fan_spin_->setValue(config["max_fan_speed"].toInt());
    min_bat_spin_->setValue(config["min_battery_level"].toInt());
    max_proc_spin_->setValue(config["max_process_count"].toInt());
    max_thr_cnt_spin_->setValue(config["max_thread_count"].toInt());
    max_io_spin_->setValue(config["max_io_operations"].toInt());
}
