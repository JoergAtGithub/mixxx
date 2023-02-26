#include "controllers/hid/hidioglobaloutputreportfifo.h"

#include <hidapi.h>

#include "controllers/defs_controllers.h"
#include "controllers/hid/legacyhidcontrollermappingfilehandler.h"
#include "util/compatibility/qbytearray.h"
#include "util/string.h"
#include "util/time.h"
#include "util/trace.h"
#pragma optimize("", off)

namespace {
constexpr int kReportIdSize = 1;
constexpr int kMaxHidErrorMessageSize = 512;
} // namespace

HidIoGlobalOutputReportFifo::HidIoGlobalOutputReportFifo()
        : m_indexOfLastSentReport(0),
          m_indexOfLastCachedReport(0),
          m_maxCachedDataSize(0) {
}

void HidIoGlobalOutputReportFifo::addReportDatasetToFifo(const quint8 reportId,
        const QByteArray& data,
        const mixxx::hid::DeviceInfo& deviceInfo,
        const RuntimeLoggingCategory& logOutput) {
    auto cacheLock = lockMutex(&m_fifoMutex);

    if (m_maxCachedDataSize < data.size()) {
        // If we use the max size of all reports ever sent in non-skipping mode,
        // we prevent unnecessary resize of the QByteArray m_outputReportFifo
        m_maxCachedDataSize = data.size();
    }

    unsigned int indexOfReportToCache;

    if (m_indexOfLastCachedReport + 1 < kSizeOfFifoInReports) {
        indexOfReportToCache = m_indexOfLastCachedReport + 1;
    } else {
        indexOfReportToCache = 0;
    }

    // If the FIFO is full we have no other chance than skipping the dataset
    if (m_indexOfLastSentReport == indexOfReportToCache) {
        qCWarning(logOutput)
                << "FIFO overflow: Unable to add OutputReport " << reportId
                << " to the global cache for non-skipping sending of "
                   "OututReports for "
                << deviceInfo.formatName();
        return;
    }

    // First byte must always contain the ReportID - also after swapping,
    // therefore initialize both arrays
    QByteArray cachedData;
    cachedData.reserve(kReportIdSize + m_maxCachedDataSize);
    cachedData.append(reportId);
    cachedData.append(data);

    // Deep copy with reusing the already allocated heap memory
    qByteArrayReplaceWithPositionAndSize(&m_outputReportFifo[indexOfReportToCache],
            0,
            m_outputReportFifo[indexOfReportToCache].size(),
            cachedData.constData(),
            cachedData.size());

    m_indexOfLastCachedReport = indexOfReportToCache;
}

bool HidIoGlobalOutputReportFifo::sendNextReportDataset(QMutex* pHidDeviceAndPollMutex,
        hid_device* pHidDevice,
        const mixxx::hid::DeviceInfo& deviceInfo,
        const RuntimeLoggingCategory& logOutput) {
    auto startOfHidWrite = mixxx::Time::elapsed();

    auto fifoLock = lockMutex(&m_fifoMutex);

    if (m_indexOfLastSentReport == m_indexOfLastCachedReport) {
        // No data in FIFO to be send
        // Return with false, to signal the caller, that no time consuming IO
        // operation was necessary
        return false;
    }

    if (m_indexOfLastSentReport + 1 < kSizeOfFifoInReports) {
        m_indexOfLastSentReport++;
    } else {
        m_indexOfLastSentReport = 0;
    }

    // Preemptively set m_lastSentData and m_possiblyUnsentDataCached,
    // to release the mutex during the time consuming hid_write operation.
    // In the unlikely case that hid_write fails, they will be invalidated afterwards
    // This is safe, because these members are only reset in this scope of this method,
    // and concurrent execution of this method is prevented by locking pHidDeviceMutex

    QByteArray dataToSend;
    dataToSend.reserve(kReportIdSize + m_maxCachedDataSize);
    dataToSend.swap(m_outputReportFifo[m_indexOfLastSentReport]);

    fifoLock.unlock();

    auto hidDeviceLock = lockMutex(pHidDeviceAndPollMutex);

    // hid_write can take several milliseconds, because hidapi synchronizes
    // the asyncron HID communication from the OS
    int result = hid_write(pHidDevice,
            reinterpret_cast<const unsigned char*>(dataToSend.constData()),
            dataToSend.size());
    if (result == -1) {
        qCWarning(logOutput) << "Unable to send data to" <<
    deviceInfo.formatName() << ":"
                             << mixxx::convertWCStringToQString(
                                        hid_error(pHidDevice),
                                        kMaxHidErrorMessageSize);
    }

    hidDeviceLock.unlock();

    fifoLock.relock();

    if (result == 0) {
        qCDebug(logOutput) << "t:" << startOfHidWrite.formatMillisWithUnit()
                           << " " << result << "bytes sent to"
                           << deviceInfo.formatName() << "serial #"
                           << deviceInfo.serialNumber()
                           << "(including report ID of"
                           << dataToSend.constData()[0] << ") - Needed: "
                           << (mixxx::Time::elapsed() - startOfHidWrite)
                                      .formatMicrosWithUnit();
    }

    // Return with true, to signal the caller, that the time consuming hid_write
    // operation was executed
    return true;
}
