// #pragma optimize("", off)

#include "controllerhidreporttabsmanager.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "controllers/hid/hidusagetables.h"
#include "moc_controllerhidreporttabsmanager.cpp"

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
            const_cast<hid::reportDescriptor::HIDReportDescriptor&>(m_reportDescriptor);
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
                                            : QStringLiteral("Feature"),
                                    QString::number(reportId, 16)
                                            .rightJustified(2, '0')
                                            .toUpper());

            auto* tabWidget = new QWidget(parentTab);
            auto* layout = new QVBoxLayout(tabWidget);
            auto* topWidgetRow = new QHBoxLayout();

            // Create buttons
            auto* readButton = new QPushButton(QStringLiteral("Read"), tabWidget);
            auto* sendButton = new QPushButton(QStringLiteral("Send"), tabWidget);

            // Adjust visibility/enable state based on the report type
            if (reportType == hid::reportDescriptor::HidReportType::Input) {
                sendButton->hide();
            } else if (reportType == hid::reportDescriptor::HidReportType::Output) {
                readButton->hide();
                sendButton->setDisabled(true);
            }

            topWidgetRow->addWidget(readButton);
            topWidgetRow->addWidget(sendButton);
            layout->addLayout(topWidgetRow);

            auto* table = new QTableWidget(tabWidget);
            layout->addWidget(table);

            auto* report = nonConstReportDescriptor.getReport(reportType, reportId);
            if (report) {
                // Show payload size
                auto* sizeLabel = new QLabel(tabWidget);
                sizeLabel->setText(
                        QStringLiteral("Payload Size: %1 bytes").arg(report->getReportSize()));
                topWidgetRow->insertWidget(0, sizeLabel);

                populateHidReportTable(table, *report, reportType);
            }

            if (reportType != hid::reportDescriptor::HidReportType::Output) {
                connect(readButton,
                        &QPushButton::clicked,
                        this,
                        [this, table, reportId, reportType]() {
                            slotReadButtonClicked(table, reportId, reportType);
                        });
            }

            parentTab->addTab(tabWidget, tabName);
        }
    }
}

void ControllerHidReportTabsManager::slotReadButtonClicked(QTableWidget* table,
        quint8 reportId,
        hid::reportDescriptor::HidReportType reportType) {
    if (!m_hidController->isOpen()) {
        qWarning() << "HID controller is not open.";
        return;
    }

    HidControllerJSProxy* jsProxy = static_cast<HidControllerJSProxy*>(m_hidController->jsProxy());
    if (!jsProxy) {
        qWarning() << "Failed to get JS proxy.";
        return;
    }

    if (reportType == hid::reportDescriptor::HidReportType::Input) {
        auto reportData = jsProxy->getInputReport(reportId);
        if (reportData.isEmpty()) {
            qWarning() << "Failed to get input report.";
            return;
        }
        for (int row = 0; row < table->rowCount(); ++row) {
            auto* item = table->item(row, 5); // Assuming the value column is at index 5
            if (item) {
                item->setText(QString::number(static_cast<quint8>(reportData.at(row))));
            }
        }
        return;
    } else if (reportType == hid::reportDescriptor::HidReportType::Feature) {
        auto reportData = jsProxy->getFeatureReport(reportId);
        if (reportData.isEmpty()) {
            qWarning() << "Failed to get feature report.";
            return;
        }
        for (int row = 0; row < table->rowCount(); ++row) {
            auto* item = table->item(row, 5); // Assuming the value column is at index 5
            if (item) {
                item->setText(QString::number(static_cast<quint8>(reportData.at(row))));
            }
        }
    }
}

void ControllerHidReportTabsManager::populateHidReportTable(
        QTableWidget* table,
        const hid::reportDescriptor::Report& report,
        hid::reportDescriptor::HidReportType reportType) {
    // Temporarily disable updates to speed up populating
    table->setUpdatesEnabled(false);

    // Reserve rows up-front
    const auto& controls = report.getControls();
    table->setRowCount(static_cast<int>(controls.size()));

    bool showVolatileColumn = (reportType == hid::reportDescriptor::HidReportType::Feature ||
            reportType == hid::reportDescriptor::HidReportType::Output);

    // Set the delegate once if needed
    if (showVolatileColumn) {
        table->setItemDelegateForColumn(5, new ValueItemDelegate(table));
    }

    // Set headers
    QStringList headers = {QStringLiteral("Byte Position"),
            QStringLiteral("Bit Position"),
            QStringLiteral("Bit Size"),
            QStringLiteral("Logical Min"),
            QStringLiteral("Logical Max"),
            QStringLiteral("Value"),
            QStringLiteral("Physical Min"),
            QStringLiteral("Physical Max"),
            QStringLiteral("Unit Scaling"),
            QStringLiteral("Unit"),
            QStringLiteral("Abs/Rel"),
            QStringLiteral("Wrap"),
            QStringLiteral("Linear"),
            QStringLiteral("Preferred"),
            QStringLiteral("Null")};
    if (showVolatileColumn) {
        headers << QStringLiteral("Volatile");
    }
    headers << QStringLiteral("Usage Page") << QStringLiteral("Usage");

    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->verticalHeader()->setVisible(false);

    // Helpers
    auto createReadOnlyItem = [](const QString& text, bool rightAlign = false) {
        auto* item = new QTableWidgetItem(text);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        if (rightAlign) {
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        }
        return item;
    };
    auto createValueItem = [reportType](int minVal, int maxVal) {
        auto* item = new QTableWidgetItem;
        QFont font = item->font();
        font.setBold(true);
        item->setFont(font);
        if (reportType == hid::reportDescriptor::HidReportType::Input) {
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        } else {
            item->setFlags(item->flags() | Qt::ItemIsEditable);
            item->setData(Qt::UserRole, QVariant::fromValue(QPair<int, int>(minVal, maxVal)));
        }
        return item;
    };

    int row = 0;
    for (const auto& control : controls) {
        // Column 0 - Byte Position
        table->setItem(row,
                0,
                createReadOnlyItem(QStringLiteral("0x%1").arg(QString::number(
                                           control.m_bytePosition, 16)
                                                   .rightJustified(2, '0')
                                                   .toUpper()),
                        true));
        // Column 1 - Bit Position
        table->setItem(row, 1, createReadOnlyItem(QString::number(control.m_bitPosition), true));
        // Column 2 - Bit Size
        table->setItem(row, 2, createReadOnlyItem(QString::number(control.m_bitSize), true));
        // Column 3 - Logical Min
        table->setItem(row, 3, createReadOnlyItem(QString::number(control.m_logicalMinimum), true));
        // Column 4 - Logical Max
        table->setItem(row, 4, createReadOnlyItem(QString::number(control.m_logicalMaximum), true));
        // Column 5 - Value
        table->setItem(row, 5, createValueItem(control.m_logicalMinimum, control.m_logicalMaximum));
        // Column 6 - Physical Min
        table->setItem(row,
                6,
                createReadOnlyItem(
                        QString::number(control.m_physicalMinimum), true));
        // Column 7 - Physical Max
        table->setItem(row,
                7,
                createReadOnlyItem(
                        QString::number(control.m_physicalMaximum), true));
        // Column 8 - Unit Scaling
        table->setItem(row,
                8,
                createReadOnlyItem(control.m_unitExponent != 0
                                ? QStringLiteral("10^%1").arg(
                                          control.m_unitExponent)
                                : QString(),
                        true));
        // Column 9 - Unit
        table->setItem(row,
                9,
                createReadOnlyItem(hid::reportDescriptor::getScaledUnitString(
                        control.m_unit)));
        // Column 10 - Abs/Rel
        table->setItem(row,
                10,
                createReadOnlyItem(control.m_flags.absolute_relative
                                ? QStringLiteral("Relative")
                                : QStringLiteral("Absolute")));
        // Column 11 - Wrap
        table->setItem(row,
                11,
                createReadOnlyItem(control.m_flags.no_wrap_wrap
                                ? QStringLiteral("Wrap")
                                : QStringLiteral("No Wrap")));
        // Column 12 - Linear
        table->setItem(row,
                12,
                createReadOnlyItem(control.m_flags.linear_non_linear
                                ? QStringLiteral("Non Linear")
                                : QStringLiteral("Linear")));
        // Column 13 - Preferred
        table->setItem(row,
                13,
                createReadOnlyItem(control.m_flags.preferred_no_preferred
                                ? QStringLiteral("No Preferred")
                                : QStringLiteral("Preferred")));
        // Column 14 - Null
        table->setItem(row,
                14,
                createReadOnlyItem(control.m_flags.no_null_null
                                ? QStringLiteral("Null")
                                : QStringLiteral("No Null")));

        // Volatile column (if present)
        int volatileIndex = (showVolatileColumn ? 15 : -1);
        if (volatileIndex != -1) {
            table->setItem(row,
                    volatileIndex,
                    createReadOnlyItem(control.m_flags.non_volatile_volatile
                                    ? QStringLiteral("Volatile")
                                    : QStringLiteral("Non Volatile")));
        }

        // Usage Page / Usage
        int usagePageIdx = showVolatileColumn ? 16 : 15;
        int usageDescIdx = showVolatileColumn ? 17 : 16;
        uint16_t usagePage = static_cast<uint16_t>((control.m_usage & 0xFFFF0000) >> 16);
        uint16_t usage = static_cast<uint16_t>(control.m_usage & 0x0000FFFF);

        table->setItem(row,
                usagePageIdx,
                createReadOnlyItem(
                        mixxx::hid::HidUsageTables::getUsagePageDescription(
                                usagePage)));
        table->setItem(row,
                usageDescIdx,
                createReadOnlyItem(
                        mixxx::hid::HidUsageTables::getUsageDescription(
                                usagePage, usage)));

        ++row;
    }

    // Resize columns to contents
    for (int col = 0; col < table->columnCount(); ++col) {
        table->horizontalHeader()->setSectionResizeMode(col, QHeaderView::ResizeToContents);
    }

    table->setUpdatesEnabled(true);
}

QWidget* ValueItemDelegate::createEditor(QWidget* parent,
        const QStyleOptionViewItem&,
        const QModelIndex& index) const {
    // Create a line edit restricted by (logical min, logical max)
    auto dataRange = index.data(Qt::UserRole).value<QPair<int, int>>();
    auto* editor = new QLineEdit(parent);
    editor->setValidator(new QIntValidator(dataRange.first, dataRange.second, editor));
    return editor;
}

void ValueItemDelegate::setModelData(QWidget* editor,
        QAbstractItemModel* model,
        const QModelIndex& index) const {
    auto* lineEdit = qobject_cast<QLineEdit*>(editor);
    if (!lineEdit) {
        return;
    }

    // Confirm the text is an integer within the expected range
    bool ok = false;
    const int value = lineEdit->text().toInt(&ok);
    if (ok) {
        auto dataRange = index.data(Qt::UserRole).value<QPair<int, int>>();
        if (value >= dataRange.first && value <= dataRange.second) {
            model->setData(index, value, Qt::EditRole);
        }
    }
}
