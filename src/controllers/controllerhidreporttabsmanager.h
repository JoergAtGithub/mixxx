#pragma once

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
    void populateHidReportTable(QTableWidget* table, const hid::reportDescriptor::Report& report);
    void createReportTabs(QTabWidget* parentTab, hid::reportDescriptor::HidReportType reportType);

    QTabWidget* m_pParentTabWidget;
    HidController* m_hidController;
    const hid::reportDescriptor::HIDReportDescriptor m_reportDescriptor;
};
