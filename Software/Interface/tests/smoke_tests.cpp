#include <QtTest>

#define protected public
#define private public
#include "console_widget.h"
#include "logger_backend.h"
#include "mainwindow_controller.h"
#include "panel_container.h"
#include "panel_factory.h"
#undef private
#undef protected

#include <QAction>
#include <QPixmap>
#include <QSplitter>

namespace
{
void renderSmoke(QWidget& widget)
{
    widget.resize(qMax(widget.sizeHint().width(), 640),
                  qMax(widget.sizeHint().height(), 360));
    QPixmap pixmap(widget.size());
    pixmap.fill(Qt::transparent);
    widget.render(&pixmap);
    QVERIFY(!pixmap.isNull());
}
}

class SmokeTests : public QObject
{
    Q_OBJECT

private slots:
    void mainWindowConstructsAndOpensPrimaryPanels();
    void panelFactoryCreatesEveryKnownPanel();
};

void SmokeTests::mainWindowConstructsAndOpensPrimaryPanels()
{
    const int statusHistoryBefore = Logger::instance().statusHistory().size();
    MainWindowController window;
    QVERIFY(window.m_panelSplitter != nullptr);
    QCOMPARE(window.m_panelSplitter->count(), 0);
    QVERIFY(Logger::instance().statusHistory().size() >= statusHistoryBefore + 2);

    renderSmoke(window);

    window.onConsole();
    QCOMPARE(window.m_panelSplitter->count(), 1);
    window.onLog();
    QCOMPARE(window.m_panelSplitter->count(), 2);
    window.onComsStatus();
    QCOMPARE(window.m_panelSplitter->count(), 3);
    window.onProfileSetup();
    QCOMPARE(window.m_panelSplitter->count(), 4);

    renderSmoke(window);
    QVERIFY(window.findChildren<PanelContainer*>().size() >= 4);
}

void SmokeTests::panelFactoryCreatesEveryKnownPanel()
{
    PCPDatabase database;
    QVERIFY(database.loadFromFile("pcp.yaml"));
    ConsoleWidget::setSharedPCPDatabase(&database);

    const QList<PanelType> panelTypes = {
        PanelType::Console,
        PanelType::Log,
        PanelType::ComsStatus,
        PanelType::ProfileSetup
    };

    for (PanelType type : panelTypes)
    {
        std::unique_ptr<QWidget> panel(PanelFactory::createPanelWidget(type));
        QVERIFY(panel != nullptr);
        QVERIFY(!PanelFactory::panelTitle(type).isEmpty());
        renderSmoke(*panel);
    }
}

QTEST_MAIN(SmokeTests)
#include "smoke_tests.moc"
