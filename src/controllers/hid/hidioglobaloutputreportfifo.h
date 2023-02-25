#pragma once

#include "controllers/controller.h"
#include "controllers/hid/hiddevice.h"
#include "util/compatibility/qmutex.h"
#include "util/duration.h"

namespace {
constexpr int kSizeOfFifoInReports = 32;
}

class HidIoGlobalOutputReportFifo {
  public:
    HidIoGlobalOutputReportFifo();

    /// Caches new report dataset, which will later send by the IO thread
    void addReportDatasetToFifo(const quint8 reportId,
            const QByteArray& data,
            const mixxx::hid::DeviceInfo& deviceInfo,
            const RuntimeLoggingCategory& logOutput);

    /// Purges all report datasets with the given ReportID from the FIFO
    void purgeDatasetsByReportID(const quint8 reportId,
            const mixxx::hid::DeviceInfo& deviceInfo,
            const RuntimeLoggingCategory& logOutput);

    /// Sends the OutputReport to the HID device, when changed data are cached.
    /// Returns true if a time consuming hid_write operation was executed.
    bool sendNextReportDataset(QMutex* pHidDeviceAndPollMutex,
            hid_device* pHidDevice,
            const mixxx::hid::DeviceInfo& deviceInfo,
            const RuntimeLoggingCategory& logOutput);

  private:
    QByteArray m_outputReportFifo[kSizeOfFifoInReports];

    /// Mutex must be locked when reading/writing
    /// m_outputReportFifo and m_lastCachedDataSize
    QMutex m_cachedDataMutex;

    /// Due to swapping of the QbyteArrays, we need to store
    /// this information independent of the QBytearray size
    int m_lastCachedDataSize;
};
