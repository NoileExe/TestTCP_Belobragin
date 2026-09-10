
#pragma once


#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QString>

// ====================================================================================================

// Общие константы и утилиты протокола обмена данными
// Используются клиентом и сервером для обеспечения совместимости
namespace protocol
{

// Порт, на котором сервер слушает подключения
inline constexpr int kServerPort = 12345;

// Хост, к которому подключается клиент
inline constexpr char kServerHost[] = "127.0.0.1"; //"localhost"; "127.0.0.1";

// Интервал между попытками переподключения (мс)
inline constexpr int kReconnectIntervalMs = 5000;

// Диапазон задержек между отправками данных (мс)
inline constexpr int kMinSendDelayMs = 10;
inline constexpr int kMaxSendDelayMs = 100;

// Ключи JSON-сообщений
inline constexpr char kKeyType[] = "type";
inline constexpr char kKeyStatus[] = "status";
inline constexpr char kKeyClientId[] = "clientId";
inline constexpr char kKeyMessage[] = "message";
inline constexpr char kKeyCommand[] = "command";

// Команды от сервера
inline constexpr char kCommandStart[] = "start";
inline constexpr char kCommandStop[] = "stop";
inline constexpr char kCommandSetConfig[] = "set_config";

// Типы данных
inline constexpr char kTypeNetworkMetrics[] = "NetworkMetrics";
inline constexpr char kTypeDeviceStatus[] = "DeviceStatus";
inline constexpr char kTypeLog[] = "Log";

// Статусы ответов сервера
inline constexpr char kStatusConnected[] = "connected";

// Разделитель сообщений в TCP-потоке
// TCP - потоковый протокол, поэтому границы сообщений обозначаются явно
constexpr char kMessageDelimiter = '\n';


/*inline QString qs(std::string_view sv)
{
    return QString::fromUtf8(sv.data(), sv.size());
}*/

// Извлекает одно полное JSON-сообщение из буфера
// Возвращает true, если сообщение собрано и распарсено
// Модифицирует buffer, удаляя прочитанные байты
inline bool TryReadJsonMessage(QByteArray* buffer, QJsonObject* out_json)
{
    int delimiter_pos = buffer->indexOf(kMessageDelimiter);
    if (delimiter_pos < 0)
    {
        return false;
    }

    QByteArray line = buffer->left(delimiter_pos).trimmed();
    buffer->remove(0, delimiter_pos + 1);

    if (line.isEmpty())
    {
        return false;
    }

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(line, &parse_error);
    if (parse_error.error != QJsonParseError::NoError || !doc.isObject())
    {
        return false;
    }

    *out_json = doc.object();
    return true;
}

// Сериализует JSON в байты с добавлением разделителя
inline QByteArray SerializeJsonMessage(const QJsonObject& json)
{
    return QJsonDocument(json).toJson(QJsonDocument::Compact) + kMessageDelimiter;
}

}  // namespace protocol
