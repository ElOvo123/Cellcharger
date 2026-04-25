#include <QtTest>

#include "coms.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QToolButton>

class ComsTests : public QObject
{
    Q_OBJECT

private slots:
    void canInterfaces_areExposedInExpectedOrder();
    void canInterface_normalizesUnsupportedValues();
    void configPageIndex_matchesTransportType();
    void invalidRows_returnDefaultValues();
    void addConnectionButton_emitsSignal();
    void setConnections_roundTripsCanConfiguration();
    void setConnections_roundTripsTcpConfiguration();
    void invalidCanInterface_fallsBackToDefaultOption();
    void setConnections_updatesRowCountAndRemovesExtraRows();
    void rowButtons_emitExpectedSignals();
    void changingType_updatesReturnedConfiguration();
    void secondRow_buttonsEmitSecondRowIndex();
    void setConnections_roundTripsSerialConfigurationAndStatusText();
    void setConnections_supportsUdpAndInvalidTypeFallback();
    void setConnections_handlesEmptyList();
};

void ComsTests::canInterfaces_areExposedInExpectedOrder()
{
    const QStringList expected = {"vcan0", "vcan1", "vcan2", "can0", "can1", "can2"};

    QCOMPARE(availableCanInterfaces(), expected);
}

void ComsTests::canInterface_normalizesUnsupportedValues()
{
    QCOMPARE(normalizedCanInterface("can2"), QString("can2"));
    QCOMPARE(normalizedCanInterface("invalid0"), QString("vcan0"));
    QCOMPARE(normalizedCanInterface(QString()), QString("vcan0"));
}

void ComsTests::configPageIndex_matchesTransportType()
{
    QCOMPARE(configPageIndexForType(ComsType::Serial), 0);
    QCOMPARE(configPageIndexForType(ComsType::Socket_vcan), 1);
    QCOMPARE(configPageIndexForType(ComsType::Socket_UDP), 2);
    QCOMPARE(configPageIndexForType(ComsType::Socket_TCP), 2);
}

void ComsTests::invalidRows_returnDefaultValues()
{
    Coms view;

    QCOMPARE(view.connectionType(-1), ComsType::Serial);
    QCOMPARE(view.connectionType(99), ComsType::Serial);

    const ComsConfig negativeConfig = view.connectionConfig(-1);
    QCOMPARE(negativeConfig.baudrate, 115200);
    QCOMPARE(negativeConfig.canInterface, QString("vcan0"));
    QVERIFY(negativeConfig.ip.isEmpty());
    QCOMPARE(negativeConfig.port, 0);

    const ComsConfig missingConfig = view.connectionConfig(99);
    QCOMPARE(missingConfig.baudrate, 115200);
    QCOMPARE(missingConfig.canInterface, QString("vcan0"));
    QVERIFY(missingConfig.ip.isEmpty());
    QCOMPARE(missingConfig.port, 0);
}

void ComsTests::addConnectionButton_emitsSignal()
{
    Coms view;
    QSignalSpy addSpy(&view, &Coms::addConnectionRequested);

    auto* addButton = view.findChild<QToolButton*>("addConnectionButton");
    QVERIFY(addButton != nullptr);

    addButton->click();
    QCOMPARE(addSpy.count(), 1);
}

void ComsTests::setConnections_roundTripsCanConfiguration()
{
    Coms view;

    ComsConnectionInfo connection;
    connection.type = ComsType::Socket_vcan;
    connection.config.canInterface = "can2";

    view.setConnections({connection});

    QCOMPARE(view.connectionType(0), ComsType::Socket_vcan);

    const ComsConfig config = view.connectionConfig(0);
    QCOMPARE(config.canInterface, QString("can2"));
    QVERIFY(config.ip.isEmpty());
    QCOMPARE(config.port, 0);
}

void ComsTests::setConnections_roundTripsTcpConfiguration()
{
    Coms view;

    ComsConnectionInfo connection;
    connection.type = ComsType::Socket_TCP;
    connection.config.ip = "192.168.0.10";
    connection.config.port = 1200;

    view.setConnections({connection});

    QCOMPARE(view.connectionType(0), ComsType::Socket_TCP);

    const ComsConfig config = view.connectionConfig(0);
    QCOMPARE(config.ip, QString("192.168.0.10"));
    QCOMPARE(config.port, 1200);
    QCOMPARE(config.canInterface, QString("vcan0"));
}

void ComsTests::invalidCanInterface_fallsBackToDefaultOption()
{
    Coms view;

    ComsConnectionInfo connection;
    connection.type = ComsType::Socket_vcan;
    connection.config.canInterface = "vcan42";

    view.setConnections({connection});

    const ComsConfig config = view.connectionConfig(0);
    QCOMPARE(config.canInterface, QString("vcan0"));
}

void ComsTests::setConnections_updatesRowCountAndRemovesExtraRows()
{
    Coms view;

    view.setConnections({ComsConnectionInfo{}, ComsConnectionInfo{}});
    QCOMPARE(view.findChildren<QPushButton*>("connectButton").size(), 2);

    view.setConnections({ComsConnectionInfo{}});
    QCOMPARE(view.findChildren<QPushButton*>("connectButton").size(), 1);
}

void ComsTests::rowButtons_emitExpectedSignals()
{
    Coms view;
    view.setConnections({ComsConnectionInfo{}});

    QSignalSpy connectSpy(&view, &Coms::connectConnectionRequested);
    QSignalSpy disconnectSpy(&view, &Coms::disconnectConnectionRequested);
    QSignalSpy removeSpy(&view, &Coms::removeConnectionRequested);

    auto* connectButton = view.findChild<QPushButton*>("connectButton");
    auto* removeButton = view.findChild<QPushButton*>("removeButton");
    QVERIFY(connectButton != nullptr);
    QVERIFY(removeButton != nullptr);

    connectButton->click();
    QCOMPARE(connectSpy.count(), 1);
    QCOMPARE(connectSpy.at(0).at(0).toInt(), 0);

    ComsConnectionInfo connectedRow;
    connectedRow.connected = true;
    view.setConnections({connectedRow});

    connectButton = view.findChild<QPushButton*>("connectButton");
    QVERIFY(connectButton != nullptr);
    connectButton->click();
    QCOMPARE(disconnectSpy.count(), 1);
    QCOMPARE(disconnectSpy.at(0).at(0).toInt(), 0);

    removeButton->click();
    QCOMPARE(removeSpy.count(), 1);
    QCOMPARE(removeSpy.at(0).at(0).toInt(), 0);
}

void ComsTests::changingType_updatesReturnedConfiguration()
{
    Coms view;
    view.setConnections({ComsConnectionInfo{}});

    auto* typeCombo = view.findChild<QComboBox*>("typeCombo");
    auto* canCombo = view.findChild<QComboBox*>("canInterfaceCombo");
    auto* ipEdit = view.findChild<QLineEdit*>("ipEdit");
    auto* portEdit = view.findChild<QLineEdit*>("portEdit");
    QVERIFY(typeCombo != nullptr);
    QVERIFY(canCombo != nullptr);
    QVERIFY(ipEdit != nullptr);
    QVERIFY(portEdit != nullptr);

    typeCombo->setCurrentIndex(typeCombo->findData(static_cast<int>(ComsType::Socket_vcan)));
    canCombo->setCurrentText("can1");
    QCOMPARE(view.connectionType(0), ComsType::Socket_vcan);
    QCOMPARE(view.connectionConfig(0).canInterface, QString("can1"));

    typeCombo->setCurrentIndex(typeCombo->findData(static_cast<int>(ComsType::Socket_TCP)));
    ipEdit->setText("10.0.0.5");
    portEdit->setText("9000");
    QCOMPARE(view.connectionType(0), ComsType::Socket_TCP);
    QCOMPARE(view.connectionConfig(0).ip, QString("10.0.0.5"));
    QCOMPARE(view.connectionConfig(0).port, 9000);
}

void ComsTests::secondRow_buttonsEmitSecondRowIndex()
{
    Coms view;
    view.setConnections({ComsConnectionInfo{}, ComsConnectionInfo{}});

    QSignalSpy connectSpy(&view, &Coms::connectConnectionRequested);
    QSignalSpy removeSpy(&view, &Coms::removeConnectionRequested);

    const auto connectButtons = view.findChildren<QPushButton*>("connectButton");
    const auto removeButtons = view.findChildren<QPushButton*>("removeButton");
    QCOMPARE(connectButtons.size(), 2);
    QCOMPARE(removeButtons.size(), 2);

    connectButtons.at(1)->click();
    removeButtons.at(1)->click();

    QCOMPARE(connectSpy.count(), 1);
    QCOMPARE(removeSpy.count(), 1);
    QCOMPARE(connectSpy.at(0).at(0).toInt(), 1);
    QCOMPARE(removeSpy.at(0).at(0).toInt(), 1);
}

void ComsTests::setConnections_roundTripsSerialConfigurationAndStatusText()
{
    Coms view;

    ComsConnectionInfo connection;
    connection.type = ComsType::Serial;
    connection.config.serialPort = "ttyUSB9";
    connection.config.baudrate = 57600;
    connection.indicatorColor = QColor("#00ff00");

    view.setConnections({connection});
    view.setStatusText("Connected to test serial");
    view.setOverallConnected(true);
    view.pulseReceiveActivity();

    QCOMPARE(view.connectionType(0), ComsType::Serial);
    const ComsConfig config = view.connectionConfig(0);
    QCOMPARE(config.baudrate, 57600);
    QCOMPARE(view.toolTip(), QString("Connected to test serial"));

    auto* serialPortCombo = view.findChild<QComboBox*>("serialPortCombo");
    QVERIFY(serialPortCombo != nullptr);
    QCOMPARE(serialPortCombo->currentText(), config.serialPort);

    auto* wifiLabel = view.findChild<QLabel*>("wifiLabel");
    QVERIFY(wifiLabel != nullptr);
    QVERIFY(!wifiLabel->pixmap().isNull());
}

void ComsTests::setConnections_supportsUdpAndInvalidTypeFallback()
{
    Coms view;

    ComsConnectionInfo udpConnection;
    udpConnection.type = ComsType::Socket_UDP;
    udpConnection.config.ip = "127.0.0.1";
    udpConnection.config.port = 4555;

    ComsConnectionInfo invalidConnection;
    invalidConnection.type = static_cast<ComsType>(999);
    invalidConnection.config.canInterface = "can1";

    view.setConnections({udpConnection, invalidConnection});

    QCOMPARE(view.connectionType(0), ComsType::Socket_UDP);
    QCOMPARE(view.connectionConfig(0).ip, QString("127.0.0.1"));
    QCOMPARE(view.connectionConfig(0).port, 4555);

    QCOMPARE(view.connectionType(1), ComsType::Serial);
    QCOMPARE(view.connectionConfig(1).canInterface, QString("can1"));

    const auto stacks = view.findChildren<QStackedWidget*>();
    QCOMPARE(stacks.size(), 2);
    QCOMPARE(stacks.at(0)->currentIndex(), configPageIndexForType(ComsType::Socket_UDP));
    QCOMPARE(stacks.at(1)->currentIndex(), configPageIndexForType(ComsType::Serial));
}

void ComsTests::setConnections_handlesEmptyList()
{
    auto* view = new Coms;
    view->setConnections({});

    QCOMPARE(view->findChildren<QPushButton*>("connectButton").size(), 0);
    QCOMPARE(view->findChildren<QPushButton*>("removeButton").size(), 0);

    delete view;
}

QTEST_MAIN(ComsTests)

#include "coms_tests.moc"
