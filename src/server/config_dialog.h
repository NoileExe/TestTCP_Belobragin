
#pragma once


#include <QDialog>
#include <QDoubleSpinBox>
#include <QSpinBox>

// ====================================================================================================

class QVBoxLayout;
class QTabWidget;
class QPushButton;

/*
 * Диалог для настройки пороговых значений метрик сети (11) / состояний устройства (12) клиента
*/
class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(const QString& client_id, const QJsonObject& config, QWidget* parent = nullptr);
    ~ConfigDialog() override = default;

    // Геттеры для сетевых метрик (11)
    double GetMinBandwidth() const { return min_bw_spin_->value(); }
    double GetMaxLatency() const { return max_lat_spin_->value(); }
    double GetMaxPacketLoss() const { return max_loss_spin_->value(); }
    double GetMaxJitter() const { return max_jitter_spin_->value(); }
    double GetMinThroughput() const { return min_thr_spin_->value(); }
    double GetMaxErrorRate() const { return max_err_spin_->value(); }
    double GetMaxDnsLatency() const { return max_dns_spin_->value(); }
    int GetMaxTcpRetransmits() const { return max_tcp_spin_->value(); }
    int GetMaxConnectionCount() const { return max_conn_spin_->value(); }
    int GetMaxActiveStreams() const { return max_stream_spin_->value(); }
    double GetMaxBufferOccupancy() const { return max_buf_spin_->value(); }

    // Геттеры для состояния устройства (12)
    int GetMinUptime() const { return min_up_spin_->value(); }
    int GetMaxCpuUsage() const { return max_cpu_spin_->value(); }
    int GetMaxMemoryUsage() const { return max_mem_spin_->value(); }
    int GetMaxDiskUsage() const { return max_disk_spin_->value(); }
    int GetMaxNetworkInterfaces() const { return max_if_spin_->value(); }
    int GetMaxActiveConnections() const { return max_act_conn_spin_->value(); }
    double GetMaxTemperature() const { return max_temp_spin_->value(); }
    int GetMaxFanSpeed() const { return max_fan_spin_->value(); }
    int GetMinBatteryLevel() const { return min_bat_spin_->value(); }
    int GetMaxProcessCount() const { return max_proc_spin_->value(); }
    int GetMaxThreadCount() const { return max_thr_cnt_spin_->value(); }
    int GetMaxIoOperations() const { return max_io_spin_->value(); }

private:
    void SetupUI(const QString& client_id = "");

    // Загрузка текущих настроек клиента в диалог
    void SetConfig(const QJsonObject& config);

    QSpinBox* AddIntRow(QVBoxLayout* layout, const QString& lbl, int min, int max, int val);
    QDoubleSpinBox* AddDoubleRow(QVBoxLayout* layout, const QString& lbl, double min, double max, double val, int dec);


private slots:
    // Активирует кнопку OK при любом изменении
    void OnValueChanged();

private:
    QDoubleSpinBox* min_bw_spin_;       // min_bandwidth
    QDoubleSpinBox* max_lat_spin_;      // max_latency
    QDoubleSpinBox* max_loss_spin_;     // max_packet_loss
    QDoubleSpinBox* max_jitter_spin_;   // max_jitter
    QDoubleSpinBox* min_thr_spin_;      // min_throughput
    QDoubleSpinBox* max_err_spin_;      // max_error_rate
    QDoubleSpinBox* max_dns_spin_;      // max_dns_latency
    QSpinBox* max_tcp_spin_;            // max_tcp_retransmits
    QSpinBox* max_conn_spin_;           // max_connection_count
    QSpinBox* max_stream_spin_;         // max_active_streams
    QDoubleSpinBox* max_buf_spin_;      // max_buffer_occupancy

    QSpinBox* min_up_spin_;             // min_uptime
    QSpinBox* max_cpu_spin_;            // max_cpu_usage
    QSpinBox* max_mem_spin_;            // max_memory_usage
    QSpinBox* max_disk_spin_;           // max_disk_usage
    QSpinBox* max_if_spin_;             // max_network_interfaces
    QSpinBox* max_act_conn_spin_;       // max_active_connections
    QDoubleSpinBox* max_temp_spin_;     // max_temperature
    QSpinBox* max_fan_spin_;            // max_fan_speed
    QSpinBox* min_bat_spin_;            // min_battery_level
    QSpinBox* max_proc_spin_;           // max_process_count
    QSpinBox* max_thr_cnt_spin_;        // max_thread_count
    QSpinBox* max_io_spin_;             // max_io_operations

    QTabWidget* tab_widget_;
    QPushButton* ok_button_;
    QPushButton* cancel_button_;
};