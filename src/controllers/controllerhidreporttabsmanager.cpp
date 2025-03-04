#pragma optimize("", off)

#include "controllerhidreporttabsmanager.h"

#include <QHeaderView>

#include "controllers/hid/hidusagetables.h"

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
                    QStringLiteral("%1 Report 0x%2")
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
                            .arg(QString::number(reportId, 16).rightJustified(2, '0').toUpper());
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
    table->setColumnCount(17);

    table->setHorizontalHeaderLabels({QStringLiteral("Byte Position"),
            QStringLiteral("Bit Position"),
            QStringLiteral("Bit Size"),
            QStringLiteral("Logical Min"),
            QStringLiteral("Logical Max"),
            QStringLiteral("Physical Min"),
            QStringLiteral("Physical Max"),
            QStringLiteral("Unit Scaling"),
            QStringLiteral("Unit"),
            QStringLiteral("Abs/Rel"),
            QStringLiteral("Wrap"),
            QStringLiteral("Linear"),
            QStringLiteral("Preferred"),
            QStringLiteral("Null"),
            QStringLiteral("Volatile"),
            QStringLiteral("Usage Page"),
            QStringLiteral("Usage")});

    // Hide the vertical header to remove row numbers
    table->verticalHeader()->setVisible(false);

    // Resize columns to fit the header contents for columns 0 to 7
    for (int col = 0; col < table->columnCount() - 8; ++col) {
        table->setColumnWidth(col, table->horizontalHeader()->sectionSizeHint(col));
    }

    int row = 0;
    // Lambda to create a read-only cell with right alignment.
    auto createReadOnlyNumberItem = [](const QString& text) -> QTableWidgetItem* {
        auto item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        // Remove the editable flag.
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        return item;
    };

    // Lambda to create a read-only cell without right alignment.
    auto createReadOnlyStringItem = [](const QString& text) -> QTableWidgetItem* {
        auto item = new QTableWidgetItem(text);
        // Remove the editable flag.
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        return item;
    };

    // Fill table rows.
    for (const auto& control : report.getControls()) {
        table->insertRow(row);
        table->setItem(row,
                0,
                createReadOnlyNumberItem(QStringLiteral("0x%1").arg(
                        QString::number(control.m_bytePosition, 16)
                                .rightJustified(2, '0')
                                .toUpper())));
        table->setItem(row, 1, createReadOnlyNumberItem(QString::number(control.m_bitPosition)));
        table->setItem(row, 2, createReadOnlyNumberItem(QString::number(control.m_bitSize)));
        table->setItem(row, 3, createReadOnlyNumberItem(QString::number(control.m_logicalMinimum)));
        table->setItem(row, 4, createReadOnlyNumberItem(QString::number(control.m_logicalMaximum)));
        table->setItem(row,
                5,
                createReadOnlyNumberItem(
                        QString::number(control.m_physicalMinimum)));
        table->setItem(row,
                6,
                createReadOnlyNumberItem(
                        QString::number(control.m_physicalMaximum)));
        table->setItem(row,
                7,
                createReadOnlyNumberItem(control.m_unitExponent != 0
                                ? QStringLiteral("10^%1").arg(
                                          control.m_unitExponent)
                                : QStringLiteral("")));

        // For columns 8 to 10, use createReadOnlyStringItem.
        table->setItem(row,
                8,
                createReadOnlyStringItem(
                        hid::reportDescriptor::getScaledUnitString(
                                control.m_unitExponent, control.m_unit)));
        table->setItem(row,
                9,
                createReadOnlyStringItem(control.m_flags.absolute_relative
                                ? QStringLiteral("Relative")
                                : QStringLiteral("Absolute")));
        table->setItem(row,
                10,
                createReadOnlyStringItem(control.m_flags.no_wrap_wrap
                                ? QStringLiteral("Wrap")
                                : QStringLiteral("No Wrap")));
        table->setItem(row,
                11,
                createReadOnlyStringItem(control.m_flags.linear_non_linear
                                ? QStringLiteral("Non Linear")
                                : QStringLiteral("Linear")));
        table->setItem(row,
                12,
                createReadOnlyStringItem(control.m_flags.preferred_no_preferred
                                ? QStringLiteral("No Preferred")
                                : QStringLiteral("Preferred")));
        table->setItem(row,
                13,
                createReadOnlyStringItem(control.m_flags.no_null_null
                                ? QStringLiteral("Null")
                                : QStringLiteral("No Null")));
        table->setItem(row,
                14,
                createReadOnlyStringItem(control.m_flags.non_volatile_volatile
                                ? QStringLiteral("Volatile")
                                : QStringLiteral("Non Volatile")));

        uint16_t usagePage = static_cast<uint16_t>((control.m_usage & 0xFFFF0000) >> 16);
        uint16_t usage = static_cast<uint16_t>(control.m_usage & 0x0000FFFF);
        table->setItem(row,
                15,
                createReadOnlyStringItem(
                        mixxx::hid::HidUsageTables::getUsagePageDescription(
                                usagePage)));
        table->setItem(row,
                16,
                createReadOnlyStringItem(
                        mixxx::hid::HidUsageTables::getUsageDescription(
                                usagePage, usage)));
        row++;
    }

    // Set the last three columns to resize to contents (this uses the computed maximum size).
    for (int col = table->columnCount() - 3; col < table->columnCount(); ++col) {
        table->horizontalHeader()->setSectionResizeMode(col, QHeaderView::ResizeToContents);
    }
}
