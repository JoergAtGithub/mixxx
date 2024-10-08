#include <gtest/gtest.h>

#include "controllers/hid/hidreportdescriptor.h"

// Example HID report descriptor data

// clang-format off
uint8_t reportDescriptor[] = {
        0x05, 0x01, // Usage Page (Generic Desktop)
        0x09, 0x02, // Usage (Mouse)
        0xA1, 0x01, // Collection (Application)
        0x09, 0x01,  // Usage (Pointer)
        0xA1, 0x00,  // Collection (Physical)
        0x05, 0x09,   // Usage Page (Button)
        0x19, 0x01,   // Usage Minimum (1)
        0x29, 0x03,   // Usage Maximum (3)
        0x15, 0x00,   // Logical Minimum (0)
        0x25, 0x01,   // Logical Maximum (1)
        0x95, 0x03,   // Report Count (3)
        0x75, 0x01,   // Report Size (1)
        0x81, 0x02,   // Input (Data, Variable, Absolute)
        0x95, 0x01,   // Report Count (1)
        0x75, 0x05,   // Report Size (5)
        0x81, 0x01,   // Input (Constant)
        0x05, 0x01,   // Usage Page (Generic Desktop)
        0x09, 0x30,   // Usage (X)
        0x09, 0x31,   // Usage (Y)
        0x15, 0x81,   // Logical Minimum (-127)
        0x25, 0x7F,   // Logical Maximum (127)
        0x75, 0x08,   // Report Size (8)
        0x95, 0x02,   // Report Count (2)
        0x81, 0x06,   // Input (Data, Variable, Relative)
        0xC0,        // End Collection
        0xC0        // End Collection
};
// clang-format on
TEST(HidReportDescriptorParserTest, ParseReportDescriptor) {
    using namespace hid::reportDescriptor;

    HIDReportDescriptor parser(reportDescriptor, sizeof(reportDescriptor));
    Collection collection = parser.parse();

    // Use getListOfReports to get the list of reports
    auto reportsList = parser.getListOfReports();
    ASSERT_EQ(reportsList.size(), 1);

    auto [collectionIdx, reportType, reportId] = reportsList[0];
    ASSERT_EQ(collectionIdx, 0);

    // Use getReport to get the report
    Report* report = parser.getReport(reportType, reportId);
    ASSERT_NE(report, nullptr);

    // Validate Report fields
    ASSERT_EQ(report->m_reportType, reportType);
    ASSERT_EQ(report->m_reportId, reportId);

    // Validate all Control fields
    const std::vector<Control>& controls = report->getControls();
    ASSERT_EQ(controls.size(), 5);

    // Mouse Button 1
    ASSERT_EQ(controls[0].m_usage, 0x0009'0001);
    ASSERT_EQ(controls[0].m_logicalMinimum, 0);
    ASSERT_EQ(controls[0].m_logicalMaximum, 1);
    ASSERT_EQ(controls[0].m_physicalMinimum, 0);
    ASSERT_EQ(controls[0].m_physicalMaximum, 1);
    ASSERT_EQ(controls[0].m_unitExponent, 0);
    ASSERT_EQ(controls[0].m_unit, 0);
    ASSERT_EQ(controls[0].m_bytePosition, 0);
    ASSERT_EQ(controls[0].m_bitPosition, 0);
    ASSERT_EQ(controls[0].m_bitSize, 1);

    // Mouse Button 2
    ASSERT_EQ(controls[1].m_usage, 0x0009'0002);
    ASSERT_EQ(controls[1].m_logicalMinimum, 0);
    ASSERT_EQ(controls[1].m_logicalMaximum, 1);
    ASSERT_EQ(controls[1].m_physicalMinimum, 0);
    ASSERT_EQ(controls[1].m_physicalMaximum, 1);
    ASSERT_EQ(controls[1].m_unitExponent, 0);
    ASSERT_EQ(controls[1].m_unit, 0);
    ASSERT_EQ(controls[1].m_bytePosition, 0);
    ASSERT_EQ(controls[1].m_bitPosition, 1);
    ASSERT_EQ(controls[1].m_bitSize, 1);

    // Mouse Button 3
    ASSERT_EQ(controls[2].m_usage, 0x0009'0003);
    ASSERT_EQ(controls[2].m_logicalMinimum, 0);
    ASSERT_EQ(controls[2].m_logicalMaximum, 1);
    ASSERT_EQ(controls[2].m_physicalMinimum, 0);
    ASSERT_EQ(controls[2].m_physicalMaximum, 1);
    ASSERT_EQ(controls[2].m_unitExponent, 0);
    ASSERT_EQ(controls[2].m_unit, 0);
    ASSERT_EQ(controls[2].m_bytePosition, 0);
    ASSERT_EQ(controls[2].m_bitPosition, 2);
    ASSERT_EQ(controls[2].m_bitSize, 1);

    // Mouse Movement X
    ASSERT_EQ(controls[3].m_usage, 0x0001'0030);
    ASSERT_EQ(controls[3].m_logicalMinimum, -127);
    ASSERT_EQ(controls[3].m_logicalMaximum, 127);
    ASSERT_EQ(controls[3].m_physicalMinimum, -127);
    ASSERT_EQ(controls[3].m_physicalMaximum, 127);
    ASSERT_EQ(controls[3].m_unitExponent, 0);
    ASSERT_EQ(controls[3].m_unit, 0);
    ASSERT_EQ(controls[3].m_bitSize, 8);
    ASSERT_EQ(controls[3].m_bytePosition, 1);
    ASSERT_EQ(controls[3].m_bitPosition, 0);

    // Mouse Movement Y
    ASSERT_EQ(controls[4].m_usage, 0x0001'0031);
    ASSERT_EQ(controls[4].m_logicalMinimum, -127);
    ASSERT_EQ(controls[4].m_logicalMaximum, 127);
    ASSERT_EQ(controls[4].m_physicalMinimum, -127);
    ASSERT_EQ(controls[4].m_physicalMaximum, 127);
    ASSERT_EQ(controls[4].m_unitExponent, 0);
    ASSERT_EQ(controls[4].m_unit, 0);
    ASSERT_EQ(controls[4].m_bitSize, 8);
    ASSERT_EQ(controls[4].m_bytePosition, 2);
    ASSERT_EQ(controls[4].m_bitPosition, 0);
}
