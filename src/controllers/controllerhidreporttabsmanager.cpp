#include "controllerhidreporttabsmanager.h"

ControllerHidReportTabsManager::ControllerHidReportTabsManager(
        QTabWidget* parentTabWidget, HidController* hidController)
        : m_pParentTabWidget(parentTabWidget),
          m_hidController(hidController),
          m_reportDescriptor(hidController->getReportDescriptor()) {
}

void ControllerHidReportTabsManager::createHidReportTabs() {
    auto hidReportTabs = std::make_unique<QTabWidget>(m_pParentTabWidget);

    // Create tabs for InputReports, OutputReports, and FeatureReports
    auto inputReportsTab = std::make_unique<QTabWidget>(hidReportTabs.get());
    auto outputReportsTab = std::make_unique<QTabWidget>(hidReportTabs.get());
    auto featureReportsTab = std::make_unique<QTabWidget>(hidReportTabs.get());

    createReportTabs(inputReportsTab.get(), hid::reportDescriptor::HidReportType::Input);
    createReportTabs(outputReportsTab.get(), hid::reportDescriptor::HidReportType::Output);
    createReportTabs(featureReportsTab.get(), hid::reportDescriptor::HidReportType::Feature);

    if (inputReportsTab->count() > 0) {
        m_pParentTabWidget->addTab(inputReportsTab.release(), QStringLiteral("Input Reports"));
    }
    if (outputReportsTab->count() > 0) {
        m_pParentTabWidget->addTab(outputReportsTab.release(), QStringLiteral("Output Reports"));
    }
    if (featureReportsTab->count() > 0) {
        m_pParentTabWidget->addTab(featureReportsTab.release(), QStringLiteral("Feature Reports"));
    }
}

void ControllerHidReportTabsManager::createReportTabs(QTabWidget* parentTab,
        hid::reportDescriptor::HidReportType reportType) {
    auto& nonConstReportDescriptor =
            const_cast<hid::reportDescriptor::HIDReportDescriptor&>(
                    m_reportDescriptor);
    nonConstReportDescriptor.parse();

    for (const auto& reportInfo : nonConstReportDescriptor.getListOfReports()) {
        auto [index, type, reportId] = reportInfo;
        if (type == reportType) {
            QString tabName =
                    QStringLiteral("%1Report 0x%2")
                            .arg(reportType ==
                                                    hid::reportDescriptor::
                                                            HidReportType::Input
                                            ? QStringLiteral("Input")
                                            : reportType ==
                                                    hid::reportDescriptor::
                                                            HidReportType::
                                                                    Output
                                            ? QStringLiteral("Output")
                                            : QStringLiteral("Feature"))
                            .arg(reportId, 2, 16, QChar('0'))
                            .toUpper();
            auto table = std::make_unique<QTableWidget>(parentTab);
            auto report = nonConstReportDescriptor.getReport(reportType, reportId);
            if (report) {
                populateHidReportTable(table.get(), *report);
            }
            parentTab->addTab(table.release(), tabName);
        }
    }
}

void ControllerHidReportTabsManager::populateHidReportTable(
        QTableWidget* table, const hid::reportDescriptor::Report& report) {
    table->setColumnCount(10); // Adjust the number of columns based on the
                               // members of hid::reportDescriptor::Control
    table->setHorizontalHeaderLabels({QStringLiteral("Usage"),
            QStringLiteral("Logical Min"),
            QStringLiteral("Logical Max"),
            QStringLiteral("Physical Min"),
            QStringLiteral("Physical Max"),
            QStringLiteral("Unit Exponent"),
            QStringLiteral("Unit"),
            QStringLiteral("Byte Position"),
            QStringLiteral("Bit Position"),
            QStringLiteral("Bit Size")}); // Adjust headers as needed

    int row = 0;
    for (const auto& control : report.getControls()) {
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(QString::number(control.m_usage)));
        table->setItem(row, 1, new QTableWidgetItem(QString::number(control.m_logicalMinimum)));
        table->setItem(row, 2, new QTableWidgetItem(QString::number(control.m_logicalMaximum)));
        table->setItem(row, 3, new QTableWidgetItem(QString::number(control.m_physicalMinimum)));
        table->setItem(row, 4, new QTableWidgetItem(QString::number(control.m_physicalMaximum)));
        table->setItem(row, 5, new QTableWidgetItem(QString::number(control.m_unitExponent)));
        table->setItem(row, 6, new QTableWidgetItem(QString::number(control.m_unit)));
        table->setItem(row, 7, new QTableWidgetItem(QString::number(control.m_bytePosition)));
        table->setItem(row, 8, new QTableWidgetItem(QString::number(control.m_bitPosition)));
        table->setItem(row, 9, new QTableWidgetItem(QString::number(control.m_bitSize)));
        row++;
    }
}
