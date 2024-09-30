#include "controllers/hid/hidreportdescriptor.h"

#include <cstdint>

#include "util/assert.h"
#pragma optimize("", off)

namespace hid::reportDescriptor {

// Class for Controls

Control::Control(const ControlFlags flags,
        const uint32_t usage,
        const int32_t logicalMinimum,
        const int32_t logicalMaximum,
        const int32_t physicalMinimum,
        const int32_t physicalMaximum,
        const int32_t unitExponent,
        const uint32_t unit,
        const uint16_t bytePosition,
        const uint8_t bitPosition,
        const uint8_t bitSize)
        : m_flags(flags),
          m_usage(usage),
          m_logicalMinimum(logicalMinimum),
          m_logicalMaximum(logicalMaximum),
          m_physicalMinimum(physicalMinimum),
          m_physicalMaximum(physicalMaximum),
          m_unitExponent(unitExponent),
          m_unit(unit),
          m_bytePosition(bytePosition),
          m_bitPosition(bitPosition),
          m_bitSize(bitSize) {
}

Report::Report(const HidReportType& reportType, const uint8_t& reportId)
        : m_reportType(reportType),
          m_reportId(reportId),
          m_lastBytePosition(0),
          m_lastBitPosition(0) {
}

void Report::addControl(const Control& item) {
    m_controls.push_back(item);
}

void Report::increasePosition(unsigned int bitSize) {
    // Calculate the new bit position
    m_lastBitPosition += bitSize;

    // If the bit position exceeds 8 bits, adjust the byte position
    m_lastBytePosition += m_lastBitPosition / 8;
    m_lastBitPosition %= 8;
}

// Class for Collections
void Collection::addReport(const Report& report) {
    m_reports.push_back(report);
}
Report* Collection::getReport(const HidReportType& reportType, const uint8_t& reportId) {
    for (auto& report : m_reports) {
        if (report.m_reportType == reportType && report.m_reportId == reportId) {
            return &report;
        }
    }
    return nullptr;
}

// HID Report Descriptor Parser
HIDReportDescriptor::HIDReportDescriptor(const uint8_t* data, size_t length)
        : m_data(data),
          m_length(length),
          m_pos(0),
          m_collectionLevel(0),
          m_deviceHasReportIds(kNotSet) {
}

std::pair<HidItemTag, HidItemSize> HIDReportDescriptor::readTag() {
    uint8_t byte = m_data[m_pos++];

    VERIFY_OR_DEBUG_ASSERT(byte !=
            static_cast<uint8_t>(HidItemSize::LongItemKeyword)){
            // Long items are only reserved for future use, they can't be used
            // according to HID class definition 1.11
    };

    HidItemTag tag = static_cast<HidItemTag>(
            byte & static_cast<uint8_t>(HidItemTag::AllTagBitsMask));
    HidItemSize size = static_cast<HidItemSize>(
            byte & static_cast<uint8_t>(HidItemSize::AllSizeBitsMask));

    return {tag, size};
}

uint32_t HIDReportDescriptor::readPayload(HidItemSize payloadSize) {
    uint32_t payload;

    switch (payloadSize) {
    case HidItemSize::ZeroBytePayload:
        return 0;
    case HidItemSize::OneBytePayload:
        VERIFY_OR_DEBUG_ASSERT(m_pos + 1 <= m_length) {
            return 0;
        }
        return m_data[m_pos++];
    case HidItemSize::TwoBytePayload:
        VERIFY_OR_DEBUG_ASSERT(m_pos + 2 <= m_length) {
            return 0;
        }
        payload = m_data[m_pos++];
        payload |= m_data[m_pos++] << 8;
        return payload;
    case HidItemSize::FourBytePayload:
        VERIFY_OR_DEBUG_ASSERT(m_pos + 4 <= m_length) {
            return 0;
        }
        payload = m_data[m_pos++];
        payload |= m_data[m_pos++] << 8;
        payload |= m_data[m_pos++] << 16;
        payload |= m_data[m_pos++] << 24;
        return payload;
    default:
        DEBUG_ASSERT(true);
        return 0;
    }
}

int32_t HIDReportDescriptor::getSignedValue(uint32_t payload, HidItemSize payloadSize) {
    switch (payloadSize) {
    case HidItemSize::ZeroBytePayload:
        return 0;
    case HidItemSize::OneBytePayload:
        if (payload & 0x80) {                                  // Check if the sign bit is set
            return static_cast<int32_t>(payload | 0xFFFFFF00); // Sign extend to 32 bits
        }
        return static_cast<int32_t>(payload);
    case HidItemSize::TwoBytePayload:
        if (payload & 0x8000) {                                // Check if the sign bit is set
            return static_cast<int32_t>(payload | 0xFFFF0000); // Sign extend to 32 bits
        }
        return static_cast<int32_t>(payload);
    case HidItemSize::FourBytePayload:
        return static_cast<int32_t>(payload); // Already 32 bits, no need to sign extend
    default:
        DEBUG_ASSERT(true);
        return 0;
    }
}

uint32_t HIDReportDescriptor::getDecodedUsage(
        uint16_t usagePage, uint32_t usage, HidItemSize usageSize) {
    switch (usageSize) {
    case HidItemSize::ZeroBytePayload:
        return usagePage << 16;
    case HidItemSize::OneBytePayload:
    case HidItemSize::TwoBytePayload:
        return (usagePage << 16) + usage;
    case HidItemSize::FourBytePayload:
        return usage; // Full 32bit usage superseds Usage Page
    default:
        DEBUG_ASSERT(true);
        return usagePage << 16;
    }
}

HidReportType HIDReportDescriptor::getReportType(HidItemTag tag) {
    switch (tag) {
    case HidItemTag::Input:
        return HidReportType::Input;
    case HidItemTag::Output:
        return HidReportType::Output;
        break;
    case HidItemTag::Feature:
        return HidReportType::Feature;
    default:
        DEBUG_ASSERT(true);
        return HidReportType::Input; // Dummy value for error case
    }
}

Collection HIDReportDescriptor::parse() {
    Collection collection;                           // Top level collection
    std::unique_ptr<Report> currentReport = nullptr; // Use a unique_ptr for currentReport

    // Global item values
    GlobalItems globalItems;

    // Local item values
    LocalItems localItems;

    while (m_pos < m_length) {
        auto [tag, size] = readTag();
        auto payload = readPayload(size);

        switch (tag) {
        // Global Items
        case HidItemTag::UsagePage:
            globalItems.usagePage = payload;
            break;
        case HidItemTag::LogicalMinimum:
            globalItems.logicalMinimum = getSignedValue(payload, size);
            break;
        case HidItemTag::LogicalMaximum:
            globalItems.logicalMaximum = getSignedValue(payload, size);
            break;
        case HidItemTag::PhysicalMinimum:
            globalItems.physicalMinimum = getSignedValue(payload, size);
            break;
        case HidItemTag::PhysicalMaximum:
            globalItems.physicalMaximum = getSignedValue(payload, size);
            break;
        case HidItemTag::UnitExponent:
            globalItems.unitExponent = getSignedValue(payload, size);
            break;
        case HidItemTag::Unit:
            globalItems.unit = payload;
            break;
        case HidItemTag::ReportSize:
            globalItems.reportSize = payload;
            break;
        case HidItemTag::ReportId:
            globalItems.reportId = static_cast<uint8_t>(payload);
            break;
        case HidItemTag::ReportCount:
            globalItems.reportCount = payload;
            break;
        case HidItemTag::Push:
            // Places a copy of the global item state table on the stack
            globalItemsStack.push_back(globalItems);
            break;
        case HidItemTag::Pop:
            // Replaces the item state table with the top structure from the stack
            VERIFY_OR_DEBUG_ASSERT(!globalItemsStack.empty()) {
                globalItems = globalItemsStack.back();
                globalItemsStack.pop_back();
            }
            break;

        // Local Items
        case HidItemTag::Usage:
            localItems.Usage.push_back(getDecodedUsage(globalItems.usagePage, payload, size));
            break;
        case HidItemTag::UsageMinimum:
            localItems.UsageMinimum = getDecodedUsage(globalItems.usagePage, payload, size);
            break;
        case HidItemTag::UsageMaximum:
            localItems.UsageMaximum = getDecodedUsage(globalItems.usagePage, payload, size);
            break;
        case HidItemTag::DesignatorIndex:
            localItems.DesignatorIndex = payload;
            break;
        case HidItemTag::DesignatorMinimum:
            localItems.DesignatorMinimum = payload;
            break;
        case HidItemTag::DesignatorMaximum:
            localItems.DesignatorMaximum = payload;
            break;
        case HidItemTag::StringIndex:
            localItems.StringIndex = payload;
            break;
        case HidItemTag::StringMinimum:
            localItems.StringMinimum = payload;
            break;
        case HidItemTag::StringMaximum:
            localItems.StringMaximum = payload;
            break;
        case HidItemTag::Delimiter:
            localItems.Delimiter = payload;
            break;

        // Main Items
        case HidItemTag::Input:
        case HidItemTag::Output:
        case HidItemTag::Feature: {
            if (currentReport == nullptr) {
                // First control of this device
                if (globalItems.reportId == kNotSet) {
                    m_deviceHasReportIds = false;
                } else {
                    m_deviceHasReportIds = true;
                }
                currentReport = std::make_unique<Report>(getReportType(tag), globalItems.reportId);
            } else if (currentReport->m_reportType != getReportType(tag) ||
                    globalItems.reportId != currentReport->m_reportId) {
                // First control of a new report
                collection.addReport(*currentReport);
                currentReport = std::make_unique<Report>(getReportType(tag), globalItems.reportId);
            }

            int32_t physicalMinimum, physicalMaximum;
            if (globalItems.physicalMinimum == 0 && globalItems.physicalMaximum == 0) {
                // According remark in chapter 6.2.2.7 of HID class definition 1.11
                physicalMinimum = globalItems.logicalMinimum;
                physicalMaximum = globalItems.logicalMaximum;
            } else {
                physicalMinimum = globalItems.physicalMinimum;
                physicalMaximum = globalItems.physicalMaximum;
            }

            ControlFlags flags = *reinterpret_cast<const ControlFlags*>(&payload);

            if (flags.data_constant == 1) {
                // Constant value padding - Usually for byte alignment
                currentReport->increasePosition(globalItems.reportSize * globalItems.reportCount);
            } else if (flags.array_variable == 0) {
                // Array (e.g. list of pressed keys of a computer keyboard)
                // TODO: Not relevant for mapping wizard, but could be
                // implemented by overloaded Control class
                currentReport->increasePosition(globalItems.reportSize * globalItems.reportCount);
            } else {
                // Normal variable control
                uint32_t usage = 0;
                for (unsigned int controlIdx = 0;
                        controlIdx < globalItems.reportCount;
                        controlIdx++) {
                    if (localItems.UsageMinimum != kNotSet && localItems.UsageMaximum != kNotSet) {
                        if (controlIdx == 0) {
                            usage = localItems.UsageMinimum;
                        } else if (usage < localItems.UsageMaximum) {
                            usage++;
                        }
                    } else if (!localItems.Usage.empty()) {
                        // If there are less usages than reportCount,
                        // the last usage value is valid for the remaining
                        usage = localItems.Usage.front();
                        localItems.Usage.erase(localItems.Usage.begin());
                    }
                    auto [lastBytePos, lastBitPos] = currentReport->getLastPosition();

                    Control control(flags,
                            usage,
                            globalItems.logicalMinimum,
                            globalItems.logicalMaximum,
                            physicalMinimum,
                            physicalMaximum,
                            globalItems.unitExponent,
                            globalItems.unit,
                            lastBytePos,
                            lastBitPos,
                            globalItems.reportSize);
                    currentReport->addControl(control);
                    currentReport->increasePosition(globalItems.reportSize);
                }
            }

            localItems = LocalItems();
            break;
        }

        case HidItemTag::Collection:
            m_collectionLevel++;

            // We only handle top-level-collections
            // according to chapter 8.4 "Report Constraints" HID class definition 1.11
            if (m_collectionLevel == 1) {
                DEBUG_ASSERT(payload == static_cast<uint32_t>(CollectionType::Application));
            }
            // Local items are only valid for the actual control definition, reset them
            localItems = LocalItems();
            break;
        case HidItemTag::EndCollection:
            if (m_collectionLevel == 1) {
                if (currentReport) {
                    collection.addReport(*currentReport);
                    currentReport.reset();
                }
                m_topLevelCollections.push_back(collection);
                collection = Collection();
            }
            if (m_collectionLevel > 0) {
                m_collectionLevel--;
            }
            break;

        default:
            DEBUG_ASSERT(true);
            break;
        }
    }

    if (currentReport) {
        collection.addReport(*currentReport);
    }

    return collection;
}

Report* HIDReportDescriptor::getReport(const HidReportType& reportType, const uint8_t& reportId) {
    for (auto& collection : m_topLevelCollections) {
        Report* report = collection.getReport(reportType, reportId);
        if (report != nullptr) {
            return report;
        }
    }
    return nullptr;
}

std::vector<std::tuple<size_t, HidReportType, uint8_t>> HIDReportDescriptor::getListOfReports() {
    std::vector<std::tuple<size_t, HidReportType, uint8_t>> orderedList;
    for (size_t i = 0; i < m_topLevelCollections.size(); ++i) {
        for (const auto& report : m_topLevelCollections[i].getReports()) {
            orderedList.emplace_back(i, report.m_reportType, report.m_reportId);
        }
    }
    return orderedList;
}

} // namespace hid::reportDescriptor
