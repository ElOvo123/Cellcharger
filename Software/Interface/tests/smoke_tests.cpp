#include <QtTest>

#define protected public
#define private public
#include "coms.h"
#include "console_widget.h"
#include "logger_backend.h"
#include "mainwindow_controller.h"
#include "panel_container.h"
#include "panel_factory.h"
#undef private
#undef protected

#include <QAction>
#include <QPixmap>
#include <QSignalSpy>
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
    void mainWindowSignalsComsRemovalAndDispatchPaths();
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

void SmokeTests::mainWindowSignalsComsRemovalAndDispatchPaths()
{
    MainWindowController window;
    QSignalSpy comsSpy(&window, &MainWindowView::comsClicked);
    QSignalSpy consoleSpy(&window, &MainWindowView::consoleClicked);
    QSignalSpy logSpy(&window, &MainWindowView::logClicked);
    QSignalSpy statusSpy(&window, &MainWindowView::comsStatusClicked);
    QSignalSpy profileSpy(&window, &MainWindowView::profileSetupClicked);

    auto triggerAction = [&window](const QString& name)
    {
        QAction *action = window.findChild<QAction*>(name);
        QVERIFY(action != nullptr);
        action->trigger();
    };

    triggerAction("actionComs");
    QCOMPARE(comsSpy.count(), 1);
    QVERIFY(window.m_comsWindow != nullptr);
    QVERIFY(window.m_comsController != nullptr);
    QVERIFY(window.m_comsWindow->isVisible());

    window.showComsWindow();
    QVERIFY(window.m_comsWindow->isVisible());

    triggerAction("actionConsole");
    triggerAction("actionLog");
    triggerAction("actionComsStatus");
    triggerAction("actionProfileSetup");
    QCOMPARE(consoleSpy.count(), 1);
    QCOMPARE(logSpy.count(), 1);
    QCOMPARE(statusSpy.count(), 1);
    QCOMPARE(profileSpy.count(), 1);
    QCOMPARE(window.m_panelSplitter->count(), 4);

    const int historyBefore = Logger::instance().statusHistory().size();
    window.dispatchChargerCommand(1, 1, true, 4.2);
    QVERIFY(Logger::instance().statusHistory().size() > historyBefore);
    QVERIFY(Logger::instance().statusHistory().last().contains("no connected backend"));

    PanelContainer *panel = qobject_cast<PanelContainer*>(window.m_panelSplitter->widget(0));
    QVERIFY(panel != nullptr);
    window.removePanel(panel);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(window.m_panelSplitter->count() <= 3);

    window.removePanel(nullptr);
    window.centerWindow(nullptr);
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
