#pragma once

#include <QStyledItemDelegate>
#include <QTabWidget>
#include <QTableWidget>
#include <memory>

#include "controllers/hid/hidcontroller.h"
#include "controllers/hid/hidreportdescriptor.h"

class ControllerHidReportTabsManager {
  public:
    ControllerHidReportTabsManager(QTabWidget* parentTabWidget, HidController* hidController);
    void createHidReportTabs();

  private:
    void createReportTabs(QTabWidget* parentTab, hid::reportDescriptor::HidReportType reportType);
    void populateHidReportTable(QTableWidget* table,
            const hid::reportDescriptor::Report& report,
            hid::reportDescriptor::HidReportType reportType);

    QTabWidget* m_pParentTabWidget;
    HidController* m_hidController;
    const hid::reportDescriptor::HIDReportDescriptor m_reportDescriptor;
};

class ValueItemDelegate : public QStyledItemDelegate {
  public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget* createEditor(QWidget* parent,
            const QStyleOptionViewItem& option,
            const QModelIndex& index) const override;

    void setModelData(QWidget* editor,
            QAbstractItemModel* model,
            const QModelIndex& index) const override;
};
