#pragma once

#include <QThread>
#include <map>
#include <QMutexLocker>

#include "controllers/controller.h"
#include "controllers/hid/hiddevice.h"
#include "controllers/hid/legacyhidcontrollermapping.h"
#include "util/duration.h"


namespace {
   static constexpr int kBufferSize = 255;
} // namespace

class HidIoReport : public QObject {
    Q_OBJECT
  public:
    HidIoReport(const unsigned char& reportId, hid_device* device, const QString device_name, const wchar_t* device_serial_number, const RuntimeLoggingCategory& logOutput);
    ~HidIoReport();
    void sendOutputReport(QByteArray data);

  private:
    QByteArray m_lastSentOutputreport;
    const unsigned char m_reportId;
    hid_device* m_pHidDevice;
    const QString m_pHidDeviceName;
    const wchar_t* m_pHidDeviceSerialNumber;
    const RuntimeLoggingCategory m_logOutput;
    const QObject* m_pParent;
};


class HidIo : public QThread {
    Q_OBJECT
  public:
    HidIo(hid_device* device, const QString, const wchar_t*, const RuntimeLoggingCategory& logBase, const RuntimeLoggingCategory& logInput, const RuntimeLoggingCategory& logOutput);
    ~HidIo();

    void stop() {
        m_stop = 1;
    }
    mutable QMutex m_HidIoMutex;

    static constexpr int kNumBuffers = 2;
    unsigned char m_pPollData[kNumBuffers][kBufferSize];
    int m_lastPollSize;
    int m_pollingBufferIndex;
    const RuntimeLoggingCategory m_logBase;
    const RuntimeLoggingCategory m_logInput;
    const RuntimeLoggingCategory m_logOutput;

  signals:
    /// Signals that a HID InputReport received by Interupt triggered from HID device
    void receive(const QByteArray& data, mixxx::Duration timestamp);

  public slots:
    QByteArray getInputReport(unsigned int reportID);
    void sendOutputReport(QByteArray data, unsigned int reportID);
    void sendFeatureReport(const QByteArray& reportData, unsigned int reportID);
    QByteArray getFeatureReport(unsigned int reportID);

  protected:
    void run();

  private:
    void poll();
    void processInputReport(int bytesRead);
    hid_device* m_pHidDevice;
    const QString m_pHidDeviceName;
    const wchar_t* m_pHidDeviceSerialNumber;
    QAtomicInt m_stop;
    std::map<unsigned char, std::unique_ptr<HidIoReport>> m_outputReports;
};
