#include <QtTest>

#include "console_message_filter_proxy_model.h"
#include "console_message_model.h"
#include "logger_backend.h"
#include "pcp_database.h"
#include "pcp_decode_formatter.h"
#include "pcp_decoder.h"
#include "pcp_encoder.h"
#include "pcp_formatter.h"

#include <QTemporaryDir>
#include <QDateTime>
#include <QFile>
#include <QSignalSpy>
#include <QTextStream>

#include <yaml-cpp/yaml.h>

#include <thread>
#include <vector>

class CoreTests : public QObject
{
    Q_OBJECT

private slots:
    void pcpDatabase_loadsExpectedDevicesAndMessages();
    void pcpDatabase_returnsFallbacksForUnknownItems();
    void pcpDatabase_throwsForMissingFile();
    void pcpDatabase_throwsForInvalidYaml();
    void pcpDatabase_handlesYamlWithoutDevices();
    void pcpDatabase_handlesDevicesWithoutMessagesOrSignals();
    void pcpEncoderAndDecoder_roundTripConfiguredSignals();
    void pcpEncoder_throwsWhenSignalMissingFromDefinition();
    void pcpEncoder_throwsOnInvalidConfiguration();
    void pcpEncoder_throwsWhenDeviceIdIsOutOfRange();
    void pcpEncoder_throwsWhenMessageIdIsOutOfRange();
    void pcpEncoder_acceptsSignedAndUnsignedBoundaryValues();
    void pcpEncoder_roundsScaledSignalValues();
    void pcpDecoder_withoutDatabaseReturnsNoMessage();
    void pcpDecoder_returnsNulloptForUnknownMessageId();
    void pcpDecoder_setDatabaseUpdatesPointer();
    void pcpDecoder_decodesPositiveSignedSignals();
    void pcpDecoder_handlesBoundaryIdsAndNegativeSignals();
    void pcpFormatter_formatsKnownFrames();
    void pcpFormatter_formatsUnknownFrames();
    void pcpDecodeFormatter_formatsDecodedMessages();
    void pcpDecodeFormatter_formatsEmptyDecodedMessages();
    void consoleMessageModel_exposesAndSortsRecords();
    void consoleMessageModel_appendAndClearWork();
    void consoleMessageModel_handlesInvalidIndexesAndRoles();
    void consoleMessageModel_coversAllDisplayColumnsAndFallbacks();
    void consoleMessageFilterProxyModel_filtersByConfiguredCriteria();
    void consoleMessageFilterProxyModel_handlesNullSourceModelAndCaseInsensitiveText();
    void logger_recordsAndEmitsMessages();
    void logger_trimsHistoryToMaximumSize();
    void logger_handlesConcurrentWrites();
};

void CoreTests::pcpDatabase_loadsExpectedDevicesAndMessages()
{
    PCPDatabase database;

    QVERIFY(database.loadFromFile("pcp.yaml"));
    QCOMPARE(database.idLayout().deviceIdBits, 4);
    QCOMPARE(database.idLayout().messageIdBits, 7);

    const auto deviceIds = database.deviceIds();
    QVERIFY(!deviceIds.empty());
    QVERIFY(std::find(deviceIds.begin(), deviceIds.end(), 1u) != deviceIds.end());
    QCOMPARE(deviceIds.size(), static_cast<size_t>(1));

    const auto device = database.device(1);
    QVERIFY(device.has_value());
    QCOMPARE(QString::fromStdString(device->name), QString("Charger Bus"));

    const PCPMessageDefinition* message =
        database.messageByName(1, "status");
    QVERIFY(message != nullptr);
    QCOMPARE(message->messageId, 18u);
    QVERIFY(message->signalDefinitions.find("voltage") !=
            message->signalDefinitions.end());

    const auto signalNames = database.signalNames(1, "status");
    QVERIFY(std::find(signalNames.begin(), signalNames.end(), "voltage") != signalNames.end());
    QVERIFY(std::find(signalNames.begin(), signalNames.end(), "charger_id") != signalNames.end());
    QVERIFY(database.messageByName(1, "cell_info") != nullptr);
    QVERIFY(database.messageByName(1, "command") != nullptr);
}

void CoreTests::pcpDatabase_returnsFallbacksForUnknownItems()
{
    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));

    QVERIFY(!database.device(99).has_value());
    QCOMPARE(QString::fromStdString(database.deviceName(99)), QString("Device 99"));
    QVERIFY(database.messageByName(1, "missing") == nullptr);
    QVERIFY(database.messageById(1, 999) == nullptr);
    QVERIFY(database.signalNames(1, "missing").empty());
    QVERIFY(database.messageNames(99).empty());

    const auto knownMessageNames = database.messageNames(1);
    QVERIFY(std::find(knownMessageNames.begin(), knownMessageNames.end(), "status") !=
            knownMessageNames.end());
}

void CoreTests::pcpDatabase_throwsForMissingFile()
{
    PCPDatabase database;

    QVERIFY_EXCEPTION_THROWN(
        database.loadFromFile("/definitely/missing/pcp.yaml"),
        YAML::BadFile);
}

void CoreTests::pcpDatabase_throwsForInvalidYaml()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QFile file(dir.filePath("pcp-invalid.yaml"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << "id_layout:\n";
    stream << "  total_bits: [broken\n";
    file.close();

    PCPDatabase database;
    QVERIFY_EXCEPTION_THROWN(
        database.loadFromFile(file.fileName().toStdString()),
        YAML::ParserException);
}

void CoreTests::pcpDatabase_handlesYamlWithoutDevices()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QFile file(dir.filePath("pcp-empty.yaml"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << "id_layout:\n";
    stream << "  total_bits: 11\n";
    stream << "  device_id_bits: 4\n";
    stream << "  message_id_bits: 7\n";
    file.close();

    PCPDatabase database;
    QVERIFY(database.loadFromFile(file.fileName().toStdString()));
    QVERIFY(database.deviceIds().empty());
}

void CoreTests::pcpDatabase_handlesDevicesWithoutMessagesOrSignals()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QFile file(dir.filePath("pcp-partial.yaml"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << "id_layout:\n";
    stream << "  total_bits: 11\n";
    stream << "  device_id_bits: 4\n";
    stream << "  message_id_bits: 7\n";
    stream << "devices:\n";
    stream << "  4:\n";
    stream << "    name: Bare Node\n";
    stream << "  5:\n";
    stream << "    name: Message Node\n";
    stream << "    messages:\n";
    stream << "      ping:\n";
    stream << "        message_id: 3\n";
    stream << "        dlc: 1\n";
    file.close();

    PCPDatabase database;
    QVERIFY(database.loadFromFile(file.fileName().toStdString()));

    QCOMPARE(QString::fromStdString(database.deviceName(4)), QString("Bare Node"));
    QVERIFY(database.messageNames(4).empty());
    QVERIFY(database.signalNames(5, "ping").empty());

    const PCPMessageDefinition* ping = database.messageByName(5, "ping");
    QVERIFY(ping != nullptr);
    QCOMPARE(ping->dlc, static_cast<uint8_t>(1));
    QVERIFY(ping->signalDefinitions.empty());
}

void CoreTests::pcpEncoderAndDecoder_roundTripConfiguredSignals()
{
    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));

    PCPEncoder encoder(&database);
    PCPDecoder decoder(&database);

    const std::map<std::string, double> signalValues = {
        {"charger_id", 1.0},
        {"status", 1.0},
        {"fault_code", 0.0},
        {"voltage", 12.5},
        {"current", -1.0},
        {"temperature", 30.0}
    };

    const PCPFrame frame = encoder.encode(1, "status", signalValues);
    QCOMPARE(frame.dlc, 8);

    const auto decoded = decoder.decode(frame.id, frame.dlc, frame.data);
    QVERIFY(decoded.has_value());
    QCOMPARE(QString::fromStdString(decoded->messageName), QString("status"));
    QCOMPARE(decoded->deviceId, 1u);
    QCOMPARE(QString::fromStdString(decoded->deviceName), QString("Charger Bus"));
    QCOMPARE(decoded->decodedSignals.at("charger_id").rawValue, 1);
    QCOMPARE(decoded->decodedSignals.at("status").rawValue, 1);
    QCOMPARE(decoded->decodedSignals.at("fault_code").rawValue, 0);
    QCOMPARE(decoded->decodedSignals.at("voltage").physicalValue, 12.5);
    QCOMPARE(decoded->decodedSignals.at("current").physicalValue, -1.0);
    QCOMPARE(decoded->decodedSignals.at("temperature").physicalValue, 30.0);
}

void CoreTests::pcpEncoder_throwsWhenSignalMissingFromDefinition()
{
    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));

    PCPEncoder encoder(&database);

    QVERIFY_EXCEPTION_THROWN(
        encoder.encode(1, "status", {{"not_real", 1.0}}),
        std::runtime_error);
}

void CoreTests::pcpEncoder_throwsOnInvalidConfiguration()
{
    PCPEncoder encoder;
    QVERIFY_EXCEPTION_THROWN(
        encoder.encode(1, "status", {{"voltage", 1.0}}),
        std::runtime_error);

    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));
    encoder.setDatabase(&database);
    QCOMPARE(encoder.database(), &database);

    QVERIFY_EXCEPTION_THROWN(
        encoder.encode(99, "status", {{"voltage", 1.0}}),
        std::runtime_error);

    QVERIFY_EXCEPTION_THROWN(
        encoder.encode(1, "status", {{"current", -100.0}}),
        std::runtime_error);

    QVERIFY_EXCEPTION_THROWN(
        encoder.encode(1, "status", {{"temperature", -1000.0}}),
        std::runtime_error);

    QVERIFY_EXCEPTION_THROWN(
        encoder.encode(1, "status", {{"voltage", 100.0}}),
        std::runtime_error);
}

void CoreTests::pcpEncoder_throwsWhenDeviceIdIsOutOfRange()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QFile file(dir.filePath("pcp-bad-device-id.yaml"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << "id_layout:\n";
    stream << "  total_bits: 3\n";
    stream << "  device_id_bits: 1\n";
    stream << "  message_id_bits: 2\n";
    stream << "devices:\n";
    stream << "  2:\n";
    stream << "    name: Bad Device Node\n";
    stream << "    messages:\n";
    stream << "      ok:\n";
    stream << "        message_id: 1\n";
    stream << "        dlc: 1\n";
    stream << "        signals:\n";
    stream << "          state:\n";
    stream << "            start_bit: 0\n";
    stream << "            bit_length: 2\n";
    stream << "            signed: false\n";
    stream << "            scale: 1.0\n";
    stream << "            offset: 0.0\n";
    file.close();

    PCPDatabase database;
    QVERIFY(database.loadFromFile(file.fileName().toStdString()));

    PCPEncoder encoder(&database);
    QVERIFY_EXCEPTION_THROWN(
        encoder.encode(2, "ok", {{"state", 1.0}}),
        std::runtime_error);
}

void CoreTests::pcpEncoder_throwsWhenMessageIdIsOutOfRange()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QFile file(dir.filePath("pcp-bad-id.yaml"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << "id_layout:\n";
    stream << "  total_bits: 11\n";
    stream << "  device_id_bits: 4\n";
    stream << "  message_id_bits: 7\n";
    stream << "devices:\n";
    stream << "  1:\n";
    stream << "    name: Bad Id Node\n";
    stream << "    messages:\n";
    stream << "      bad:\n";
    stream << "        message_id: 200\n";
    stream << "        dlc: 1\n";
    stream << "        signals:\n";
    stream << "          state:\n";
    stream << "            start_bit: 0\n";
    stream << "            bit_length: 2\n";
    stream << "            signed: false\n";
    stream << "            scale: 1.0\n";
    stream << "            offset: 0.0\n";
    file.close();

    PCPDatabase database;
    QVERIFY(database.loadFromFile(file.fileName().toStdString()));

    PCPEncoder encoder(&database);
    QVERIFY_EXCEPTION_THROWN(
        encoder.encode(1, "bad", {{"state", 1.0}}),
        std::runtime_error);
}

void CoreTests::pcpEncoder_acceptsSignedAndUnsignedBoundaryValues()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QFile file(dir.filePath("pcp-boundary.yaml"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << "id_layout:\n";
    stream << "  total_bits: 11\n";
    stream << "  device_id_bits: 4\n";
    stream << "  message_id_bits: 7\n";
    stream << "devices:\n";
    stream << "  1:\n";
    stream << "    name: Boundary Node\n";
    stream << "    messages:\n";
    stream << "      edge:\n";
    stream << "        message_id: 5\n";
    stream << "        dlc: 2\n";
    stream << "        signals:\n";
    stream << "          unsigned_value:\n";
    stream << "            start_bit: 0\n";
    stream << "            bit_length: 8\n";
    stream << "            signed: false\n";
    stream << "            scale: 1.0\n";
    stream << "            offset: 0.0\n";
    stream << "          signed_value:\n";
    stream << "            start_bit: 8\n";
    stream << "            bit_length: 8\n";
    stream << "            signed: true\n";
    stream << "            scale: 1.0\n";
    stream << "            offset: 0.0\n";
    file.close();

    PCPDatabase database;
    QVERIFY(database.loadFromFile(file.fileName().toStdString()));

    PCPEncoder encoder(&database);
    PCPDecoder decoder(&database);

    const PCPFrame minFrame = encoder.encode(1, "edge", {
        {"unsigned_value", 0.0},
        {"signed_value", -128.0}
    });
    QCOMPARE(minFrame.data[0], static_cast<uint8_t>(0x00));
    QCOMPARE(minFrame.data[1], static_cast<uint8_t>(0x80));

    const auto minDecoded = decoder.decode(minFrame.id, minFrame.dlc, minFrame.data);
    QVERIFY(minDecoded.has_value());
    QCOMPARE(minDecoded->decodedSignals.at("unsigned_value").rawValue, 0);
    QCOMPARE(minDecoded->decodedSignals.at("signed_value").rawValue, -128);

    const PCPFrame maxFrame = encoder.encode(1, "edge", {
        {"unsigned_value", 255.0},
        {"signed_value", 127.0}
    });
    QCOMPARE(maxFrame.data[0], static_cast<uint8_t>(0xFF));
    QCOMPARE(maxFrame.data[1], static_cast<uint8_t>(0x7F));

    const auto maxDecoded = decoder.decode(maxFrame.id, maxFrame.dlc, maxFrame.data);
    QVERIFY(maxDecoded.has_value());
    QCOMPARE(maxDecoded->decodedSignals.at("unsigned_value").rawValue, 255);
    QCOMPARE(maxDecoded->decodedSignals.at("signed_value").rawValue, 127);
}

void CoreTests::pcpEncoder_roundsScaledSignalValues()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QFile file(dir.filePath("pcp-rounded.yaml"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << "id_layout:\n";
    stream << "  total_bits: 11\n";
    stream << "  device_id_bits: 4\n";
    stream << "  message_id_bits: 7\n";
    stream << "devices:\n";
    stream << "  1:\n";
    stream << "    name: Rounded Node\n";
    stream << "    messages:\n";
    stream << "      scaled:\n";
    stream << "        message_id: 9\n";
    stream << "        dlc: 1\n";
    stream << "        signals:\n";
    stream << "          value:\n";
    stream << "            start_bit: 0\n";
    stream << "            bit_length: 8\n";
    stream << "            signed: false\n";
    stream << "            scale: 0.5\n";
    stream << "            offset: 0.0\n";
    file.close();

    PCPDatabase database;
    QVERIFY(database.loadFromFile(file.fileName().toStdString()));

    PCPEncoder encoder(&database);
    PCPDecoder decoder(&database);

    const PCPFrame frame = encoder.encode(1, "scaled", {{"value", 2.4}});
    QCOMPARE(frame.data[0], static_cast<uint8_t>(5));

    const auto decoded = decoder.decode(frame.id, frame.dlc, frame.data);
    QVERIFY(decoded.has_value());
    QCOMPARE(decoded->decodedSignals.at("value").rawValue, 5);
    QCOMPARE(decoded->decodedSignals.at("value").physicalValue, 2.5);
}

void CoreTests::pcpDecoder_withoutDatabaseReturnsNoMessage()
{
    PCPDecoder decoder;
    const std::array<uint8_t, 8> data{};

    QVERIFY(!decoder.decode(0x12, 8, data).has_value());
}

void CoreTests::pcpDecoder_returnsNulloptForUnknownMessageId()
{
    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));

    PCPDecoder decoder(&database);
    QCOMPARE(decoder.database(), &database);

    const std::array<uint8_t, 8> data{};
    QVERIFY(!decoder.decode((1u << 7) | 99u, 8, data).has_value());
}

void CoreTests::pcpDecoder_setDatabaseUpdatesPointer()
{
    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));

    PCPDecoder decoder;
    QCOMPARE(decoder.database(), nullptr);
    decoder.setDatabase(&database);
    QCOMPARE(decoder.database(), &database);
}

void CoreTests::pcpDecoder_decodesPositiveSignedSignals()
{
    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));

    PCPEncoder encoder(&database);
    PCPDecoder decoder(&database);

    const PCPFrame frame = encoder.encode(1, "status", {
        {"charger_id", 1.0},
        {"status", 1.0},
        {"fault_code", 0.0},
        {"voltage", 12.5},
        {"current", 1.0},
        {"temperature", 30.0}
    });

    const auto decoded = decoder.decode(frame.id, frame.dlc, frame.data);
    QVERIFY(decoded.has_value());
    QCOMPARE(decoded->decodedSignals.at("current").rawValue, 1000);
    QCOMPARE(decoded->decodedSignals.at("current").physicalValue, 1.0);
}

void CoreTests::pcpDecoder_handlesBoundaryIdsAndNegativeSignals()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QFile file(dir.filePath("pcp-max-device.yaml"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << "id_layout:\n";
    stream << "  total_bits: 11\n";
    stream << "  device_id_bits: 4\n";
    stream << "  message_id_bits: 7\n";
    stream << "devices:\n";
    stream << "  15:\n";
    stream << "    name: Edge Device\n";
    stream << "    messages:\n";
    stream << "      status:\n";
    stream << "        message_id: 127\n";
    stream << "        dlc: 8\n";
    stream << "        signals:\n";
    stream << "          voltage:\n";
    stream << "            start_bit: 0\n";
    stream << "            bit_length: 12\n";
    stream << "            signed: false\n";
    stream << "            scale: 0.01\n";
    stream << "            offset: 0.0\n";
    stream << "          current:\n";
    stream << "            start_bit: 12\n";
    stream << "            bit_length: 10\n";
    stream << "            signed: true\n";
    stream << "            scale: 0.1\n";
    stream << "            offset: 0.0\n";
    stream << "          temperature:\n";
    stream << "            start_bit: 22\n";
    stream << "            bit_length: 8\n";
    stream << "            signed: false\n";
    stream << "            scale: 1.0\n";
    stream << "            offset: -40.0\n";
    stream << "          state:\n";
    stream << "            start_bit: 30\n";
    stream << "            bit_length: 3\n";
    stream << "            signed: false\n";
    stream << "            scale: 1.0\n";
    stream << "            offset: 0.0\n";
    file.close();

    PCPDatabase database;
    QVERIFY(database.loadFromFile(file.fileName().toStdString()));

    PCPEncoder encoder(&database);
    PCPDecoder decoder(&database);

    const PCPFrame frame = encoder.encode(15, "status", {
        {"voltage", 14.0},
        {"current", -2.0},
        {"temperature", 60.0},
        {"state", 4.0}
    });

    const auto decoded = decoder.decode(frame.id, frame.dlc, frame.data);
    QVERIFY(decoded.has_value());
    QCOMPARE(decoded->deviceId, 15u);
    QCOMPARE(decoded->messageId, 127u);
    QCOMPARE(QString::fromStdString(decoded->deviceName), QString("Edge Device"));
    QCOMPARE(decoded->decodedSignals.at("current").physicalValue, -2.0);
    QCOMPARE(decoded->decodedSignals.at("current").rawValue, -20);
}

void CoreTests::pcpFormatter_formatsKnownFrames()
{
    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));

    PCPEncoder encoder(&database);
    const PCPFrame frame = encoder.encode(1, "status", {
        {"charger_id", 1.0},
        {"status", 2.0},
        {"fault_code", 7.0},
        {"voltage", 12.0},
        {"current", 0.0},
        {"temperature", 25.0}
    });

    const QString text = PCPFormatter::toConsoleString(frame, "RX", database);

    QVERIFY(text.contains("RX"));
    QVERIFY(text.contains("Charger Bus (1)"));
    QVERIFY(text.contains("status"));
    QVERIFY(text.contains("DLC 8"));
}

void CoreTests::pcpFormatter_formatsUnknownFrames()
{
    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));

    PCPFrame frame;
    frame.id = (4u << database.idLayout().messageIdBits) | 99u;
    frame.dlc = 0;

    const QString text = PCPFormatter::toConsoleString(frame, "TX", database);

    QVERIFY(text.contains("TX"));
    QVERIFY(text.contains("Device 4 (4)"));
    QVERIFY(text.contains("unknown"));
    QVERIFY(text.contains("Message 99"));
    QVERIFY(text.contains("DLC 0"));
}

void CoreTests::pcpDecodeFormatter_formatsDecodedMessages()
{
    PCPDecodedMessage message;
    message.messageName = "status";
    message.deviceId = 1;
    message.deviceName = "Main Controller";
    message.messageId = 18;
    message.decodedSignals["voltage"] = {12.5, 125};
    message.decodedSignals["state"] = {1.0, 1};

    const QString text = PCPDecodeFormatter::toText(message);

    QVERIFY(text.contains("PCP status"));
    QVERIFY(text.contains("DEV=1"));
    QVERIFY(text.contains("NAME=Main Controller"));
    QVERIFY(text.contains("voltage = 12.5"));
    QVERIFY(text.contains("state = 1"));
}

void CoreTests::pcpDecodeFormatter_formatsEmptyDecodedMessages()
{
    PCPDecodedMessage message;
    message.messageName = "heartbeat";
    message.deviceId = 0;
    message.deviceName = "Node 0";
    message.messageId = 0;

    const QString text = PCPDecodeFormatter::toText(message);

    QCOMPARE(text, QString("PCP heartbeat | DEV=0 | NAME=Node 0 | MSG=0"));
}

void CoreTests::consoleMessageModel_exposesAndSortsRecords()
{
    ConsoleMessageModel model;

    const QList<ConsoleMessageRecord> records = {
        {"10:00:01", "RX", "charger", "status", "1", "8", "AA", "raw1"},
        {"10:00:00", "TX", "motor", "speed", "2", "4", "BB", "raw2"}
    };

    model.setRecords(records);
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.columnCount(), 7);
    QCOMPARE(model.data(model.index(0, 2), Qt::DisplayRole).toString(), QString("charger"));
    QCOMPARE(model.headerData(3, Qt::Horizontal, Qt::DisplayRole).toString(), QString("Name"));

    model.sort(0, Qt::AscendingOrder);
    QCOMPARE(model.recordAt(0).timestamp, QString("10:00:00"));

    model.sort(2, Qt::DescendingOrder);
    QCOMPARE(model.recordAt(0).deviceName, QString("motor"));
}

void CoreTests::consoleMessageModel_appendAndClearWork()
{
    ConsoleMessageModel model;

    model.appendRecord({"10:00:03", "RX", "node", "status", "18", "8", "11 22", "raw"});
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.recordAt(0).messageId, QString("18"));

    model.clear();
    QCOMPARE(model.rowCount(), 0);
}

void CoreTests::consoleMessageModel_handlesInvalidIndexesAndRoles()
{
    ConsoleMessageModel model;
    model.appendRecord({"10:00:03", "RX", "node", "status", "18", "8", "11 22", "raw"});

    const QModelIndex validParent = model.index(0, 0);

    QCOMPARE(model.rowCount(validParent), 0);
    QCOMPARE(model.columnCount(validParent), 0);
    QVERIFY(!model.data(QModelIndex(), Qt::DisplayRole).isValid());
    QVERIFY(!model.data(model.index(5, 0), Qt::DisplayRole).isValid());
    QVERIFY(!model.headerData(0, Qt::Vertical, Qt::DisplayRole).isValid());
    QVERIFY(!model.headerData(0, Qt::Horizontal, Qt::ToolTipRole).isValid());
    QCOMPARE(model.flags(QModelIndex()), Qt::NoItemFlags);

    QCOMPARE(model.data(model.index(0, 1), Qt::TextAlignmentRole).toInt(),
             static_cast<int>(Qt::AlignCenter));
    QCOMPARE(model.data(model.index(0, 0), Qt::TextAlignmentRole).toInt(),
             static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter));
    QVERIFY(!model.data(model.index(0, 0), Qt::ToolTipRole).isValid());
    QCOMPARE(model.flags(model.index(0, 0)),
             Qt::ItemIsEnabled | Qt::ItemIsSelectable);
}

void CoreTests::consoleMessageModel_coversAllDisplayColumnsAndFallbacks()
{
    ConsoleMessageModel model;
    model.setRecords({
        {"10:00:03", "RX", "node", "status", "18", "8", "11 22", "raw"},
        {"10:00:01", "TX", "alpha", "alarm", "02", "4", "AA", "raw2"}
    });

    QCOMPARE(model.data(model.index(0, 0), Qt::DisplayRole).toString(), QString("10:00:03"));
    QCOMPARE(model.data(model.index(0, 1), Qt::DisplayRole).toString(), QString("RX"));
    QCOMPARE(model.data(model.index(0, 2), Qt::DisplayRole).toString(), QString("node"));
    QCOMPARE(model.data(model.index(0, 3), Qt::DisplayRole).toString(), QString("status"));
    QCOMPARE(model.data(model.index(0, 4), Qt::DisplayRole).toString(), QString("18"));
    QCOMPARE(model.data(model.index(0, 5), Qt::DisplayRole).toString(), QString("8"));
    QCOMPARE(model.data(model.index(0, 6), Qt::DisplayRole).toString(), QString("11 22"));
    QVERIFY(!model.data(model.index(0, 7), Qt::DisplayRole).isValid());

    QCOMPARE(model.headerData(0, Qt::Horizontal, Qt::DisplayRole).toString(), QString("Time"));
    QCOMPARE(model.headerData(1, Qt::Horizontal, Qt::DisplayRole).toString(), QString("Dir"));
    QCOMPARE(model.headerData(2, Qt::Horizontal, Qt::DisplayRole).toString(), QString("Device"));
    QCOMPARE(model.headerData(4, Qt::Horizontal, Qt::DisplayRole).toString(), QString("Msg"));
    QCOMPARE(model.headerData(5, Qt::Horizontal, Qt::DisplayRole).toString(), QString("DLC"));
    QCOMPARE(model.headerData(6, Qt::Horizontal, Qt::DisplayRole).toString(), QString("Data"));
    QVERIFY(!model.headerData(7, Qt::Horizontal, Qt::DisplayRole).isValid());

    model.sort(1, Qt::AscendingOrder);
    QCOMPARE(model.recordAt(0).direction, QString("RX"));
    model.sort(3, Qt::AscendingOrder);
    QCOMPARE(model.recordAt(0).messageName, QString("alarm"));
    model.sort(4, Qt::AscendingOrder);
    QCOMPARE(model.recordAt(0).messageId, QString("02"));
    model.sort(5, Qt::AscendingOrder);
    QCOMPARE(model.recordAt(0).dlc, QString("4"));
    model.sort(6, Qt::AscendingOrder);
    QCOMPARE(model.recordAt(0).data, QString("11 22"));
    model.sort(99, Qt::AscendingOrder);
    QCOMPARE(model.recordAt(0).data, QString("11 22"));
}

void CoreTests::consoleMessageFilterProxyModel_filtersByConfiguredCriteria()
{
    auto* model = new ConsoleMessageModel(this);
    model->setRecords({
        {"10:00:01", "RX", "charger", "status", "1", "8", "AA BB", "raw1"},
        {"10:00:02", "TX", "motor", "speed", "2", "8", "CC DD", "raw2"}
    });

    ConsoleMessageFilterProxyModel proxy;
    proxy.setSourceModel(model);

    QCOMPARE(proxy.rowCount(), 2);

    proxy.setDeviceFilters({"charger"});
    QCOMPARE(proxy.rowCount(), 1);
    QCOMPARE(proxy.index(0, 2).data().toString(), QString("charger"));

    proxy.setTextFilters({"AA"});
    QCOMPARE(proxy.rowCount(), 1);

    proxy.setMessageIdFilters({"1"});
    QCOMPARE(proxy.rowCount(), 1);

    proxy.setMessageNameFilters({"speed"});
    QCOMPARE(proxy.rowCount(), 0);

    proxy.setDeviceFilters({});
    proxy.setTextFilters({});
    proxy.setMessageIdFilters({});
    proxy.setMessageNameFilters({"speed"});
    QCOMPARE(proxy.rowCount(), 1);
    QCOMPARE(proxy.index(0, 3).data().toString(), QString("speed"));
}

void CoreTests::consoleMessageFilterProxyModel_handlesNullSourceModelAndCaseInsensitiveText()
{
    ConsoleMessageFilterProxyModel proxy;
    QCOMPARE(proxy.rowCount(), 0);

    auto* model = new ConsoleMessageModel(this);
    model->setRecords({
        {"10:00:01", "RX", "charger", "status", "18", "8", "AA BB", "raw1"},
        {"10:00:02", "TX", "motor", "speed", "2", "8", "cc dd", "raw2"}
    });

    proxy.setSourceModel(model);
    proxy.setTextFilters({"aa bb"});
    QCOMPARE(proxy.rowCount(), 1);
    QCOMPARE(proxy.index(0, 6).data().toString(), QString("AA BB"));

    proxy.setTextFilters({"10:00:02"});
    QCOMPARE(proxy.rowCount(), 1);
    QCOMPARE(proxy.index(0, 2).data().toString(), QString("motor"));

    proxy.setTextFilters({"missing"});
    QCOMPARE(proxy.rowCount(), 0);
}

void CoreTests::logger_recordsAndEmitsMessages()
{
    Logger& logger = Logger::instance();

    const int statusBefore = logger.statusHistory().size();
    const int comsBefore = logger.comsHistory().size();
    const int decodedBefore = logger.decodedHistory().size();

    QSignalSpy statusSpy(&logger, &Logger::newStatusMessage);
    QSignalSpy comsSpy(&logger, &Logger::newComsMessage);
    QSignalSpy decodedSpy(&logger, &Logger::newDecodedMessage);

    logger.logStatus("unit status");
    logger.logComs("unit coms");
    logger.logDecoded("unit decoded");

    QCOMPARE(statusSpy.count(), 1);
    QCOMPARE(comsSpy.count(), 1);
    QCOMPARE(decodedSpy.count(), 1);

    QCOMPARE(logger.statusHistory().size(), statusBefore + 1);
    QCOMPARE(logger.comsHistory().size(), comsBefore + 1);
    QCOMPARE(logger.decodedHistory().size(), decodedBefore + 1);

    QVERIFY(logger.statusHistory().last().contains("unit status"));
    QVERIFY(logger.comsHistory().last().contains("unit coms"));
    QVERIFY(logger.decodedHistory().last().contains("unit decoded"));
}

void CoreTests::logger_trimsHistoryToMaximumSize()
{
    Logger& logger = Logger::instance();

    for (int i = 0; i < 5100; ++i)
        logger.logStatus(QString("trim-status-%1").arg(i));

    const QStringList history = logger.statusHistory();
    QCOMPARE(history.size(), 5000);
    QVERIFY(history.last().contains("trim-status-5099"));
    QVERIFY(!history.first().contains("trim-status-0"));

    for (int i = 0; i < 5100; ++i)
    {
        logger.logComs(QString("trim-coms-%1").arg(i));
        logger.logDecoded(QString("trim-decoded-%1").arg(i));
    }

    QCOMPARE(logger.comsHistory().size(), 5000);
    QCOMPARE(logger.decodedHistory().size(), 5000);
    QVERIFY(logger.comsHistory().last().contains("trim-coms-5099"));
    QVERIFY(logger.decodedHistory().last().contains("trim-decoded-5099"));
}

void CoreTests::logger_handlesConcurrentWrites()
{
    Logger& logger = Logger::instance();

    const int threadCount = 4;
    const int messagesPerThread = 25;
    const QString marker = QString("concurrent-batch-%1").arg(QDateTime::currentMSecsSinceEpoch());

    std::vector<std::thread> workers;
    workers.reserve(threadCount);

    for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex)
    {
        workers.emplace_back([threadIndex, messagesPerThread, marker]()
        {
            for (int messageIndex = 0; messageIndex < messagesPerThread; ++messageIndex)
            {
                Logger::instance().logStatus(
                    QString("%1-status-%2-%3").arg(marker).arg(threadIndex).arg(messageIndex));
                Logger::instance().logComs(
                    QString("%1-coms-%2-%3").arg(marker).arg(threadIndex).arg(messageIndex));
                Logger::instance().logDecoded(
                    QString("%1-decoded-%2-%3").arg(marker).arg(threadIndex).arg(messageIndex));
            }
        });
    }

    for (std::thread& worker : workers)
        worker.join();

    const QStringList statusHistory = logger.statusHistory();
    const QStringList comsHistory = logger.comsHistory();
    const QStringList decodedHistory = logger.decodedHistory();

    int concurrentStatusMatches = 0;
    int concurrentComsMatches = 0;
    int concurrentDecodedMatches = 0;
    for (const QString& entry : statusHistory)
        concurrentStatusMatches += entry.contains(marker + "-status-") ? 1 : 0;
    for (const QString& entry : comsHistory)
        concurrentComsMatches += entry.contains(marker + "-coms-") ? 1 : 0;
    for (const QString& entry : decodedHistory)
        concurrentDecodedMatches += entry.contains(marker + "-decoded-") ? 1 : 0;

    QCOMPARE(concurrentStatusMatches, threadCount * messagesPerThread);
    QCOMPARE(concurrentComsMatches, threadCount * messagesPerThread);
    QCOMPARE(concurrentDecodedMatches, threadCount * messagesPerThread);
}
QTEST_APPLESS_MAIN(CoreTests)

#include "core_tests.moc"
