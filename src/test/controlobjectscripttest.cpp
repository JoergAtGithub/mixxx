#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <QtDebug>

#include "control/controlobject.h"
#include "control/controlobjectscript.h"
#include "test/mixxxtest.h"
#include "util/memory.h"

using ::testing::_;
using ::testing::DoubleEq;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

const RuntimeLoggingCategory k_logger(QString("test").toLocal8Bit());
constexpr int kMaxNumOfRecursions = 128;

class MockControlObjectScript : public ControlObjectScript {
  public:
    MockControlObjectScript(const ConfigKey& key,
            const RuntimeLoggingCategory& logger,
            QObject* pParent)
            : ControlObjectScript(key, logger, pParent) {
    }
    ~MockControlObjectScript() override = default;
    MOCK_METHOD2(slotValueChanged, void(double value, QObject*));
};

class ControlObjectScriptTest : public MixxxTest {
  protected:
    void SetUp() override {
        ck1 = ConfigKey("[Channel1]", "co1");
        ck2 = ConfigKey("[Channel1]", "co2");
        co1 = std::make_unique<ControlObject>(ck1);
        co2 = std::make_unique<ControlObject>(ck2);

        coScript1 = std::make_unique<MockControlObjectScript>(ck1, k_logger, nullptr);
        coScript2 = std::make_unique<MockControlObjectScript>(ck2, k_logger, nullptr);

        conn1.key = ck1;
        conn1.engineJSProxy = nullptr;
        conn1.controllerEngine = nullptr;
        conn1.callback = "mock_callback1";
        conn1.id = QUuid::createUuid();

        conn2.key = ck2;
        conn2.engineJSProxy = nullptr;
        conn2.controllerEngine = nullptr;
        conn2.callback = "mock_callback2";
        conn2.id = QUuid::createUuid();

        conn3.key = ck2;
        conn3.engineJSProxy = nullptr;
        conn3.controllerEngine = nullptr;
        conn3.callback = "mock_callback3";
        conn3.id = QUuid::createUuid();

        coScript1->addScriptConnection(conn1);
        coScript2->addScriptConnection(conn2);
    }

    void TearDown() override {
        coScript1->removeScriptConnection(conn1);
        coScript2->removeScriptConnection(conn2);
    }

    ConfigKey ck1, ck2;
    std::unique_ptr<ControlObject> co1;
    std::unique_ptr<ControlObject> co2;

    std::unique_ptr<MockControlObjectScript> coScript1;
    std::unique_ptr<MockControlObjectScript> coScript2;

    ScriptConnection conn1, conn2, conn3;
};

TEST_F(ControlObjectScriptTest, CompressingProxyCompareCount1) {
    // Check that slotValueChanged callback is never called for conn2
    EXPECT_CALL(*coScript2, slotValueChanged(_, _))
            .Times(0)
            .WillOnce(Return());
    // Check that slotValueChanged callback is called only once (independent of the value)
    EXPECT_CALL(*coScript1, slotValueChanged(_, _))
            .Times(1)
            .WillOnce(Return());
    co1->set(1.0);
    application()->processEvents();
}

TEST_F(ControlObjectScriptTest, CompressingProxyCompareValue1) {
    EXPECT_CALL(*coScript1, slotValueChanged(2.0, _))
            .Times(1)
            .WillOnce(Return());

    co1->set(2.0);
    application()->processEvents();
}

TEST_F(ControlObjectScriptTest, CompressingProxyCompareCount2) {
    // Check that slotValueChanged callback is never called for conn2
    EXPECT_CALL(*coScript2, slotValueChanged(_, _))
            .Times(0)
            .WillOnce(Return());
    // Check that slotValueChanged callback for conn1 is called only once (independent of the value)
    EXPECT_CALL(*coScript1, slotValueChanged(_, _))
            .Times(1)
            .WillOnce(Return());
    co1->set(3.0);
    co1->set(4.0);
    application()->processEvents();
}

TEST_F(ControlObjectScriptTest, CompressingProxyCompareValue2) {
    // Check that slotValueChanged callback is called with the last set value
    EXPECT_CALL(*coScript1, slotValueChanged(6.0, _))
            .Times(1)
            .WillOnce(Return());
    co1->set(5.0);
    co1->set(6.0);
    application()->processEvents();
}

TEST_F(ControlObjectScriptTest, CompressingProxyCompareCountMulti) {
    // Check that slotValueChanged callback for conn1 and conn2 is called only once (independent of the value)
    EXPECT_CALL(*coScript1, slotValueChanged(_, _)).Times(1).WillOnce(Return());
    // Check that slotValueChanged callback for conn1 and conn2 is called only once (independent of the value)
    EXPECT_CALL(*coScript2, slotValueChanged(_, _)).Times(1).WillOnce(Return());
    co1->set(10.0);
    co2->set(11.0);
    co1->set(12.0);
    co2->set(13.0);
    application()->processEvents();
}

TEST_F(ControlObjectScriptTest, CompressingProxyCompareValueMulti) {
    // Check that slotValueChanged callback is called with the last set value
    EXPECT_CALL(*coScript1, slotValueChanged(22.0, _))
            .Times(1)
            .WillOnce(Return());
    EXPECT_CALL(*coScript2, slotValueChanged(23.0, _))
            .Times(1)
            .WillOnce(Return());
    co1->set(20.0);
    co2->set(21.0);
    co1->set(22.0);
    co2->set(23.0);
    application()->processEvents();
}

TEST_F(ControlObjectScriptTest, CompressingProxyMultiConnection) {
    // Check that slotValueChanged callback is called 1 time if multiple connections exist forthe same slot
    EXPECT_CALL(*coScript1, slotValueChanged(32.0, _))
            .Times(1)
            .WillOnce(Return());
    EXPECT_CALL(*coScript2, slotValueChanged(33.0, _))
            .Times(1)
            .WillOnce(Return());

    coScript2->addScriptConnection(conn3);
    co1->set(30.0);
    co2->set(31.0);
    co1->set(32.0);
    co2->set(33.0);
    application()->processEvents();

    coScript2->removeScriptConnection(conn3);
}

TEST_F(ControlObjectScriptTest, CompressingProxyManyEvents) {
    // Check maximum number of recursions
    EXPECT_CALL(*coScript1, slotValueChanged(kMaxNumOfRecursions, _))
            .Times(1)
            .WillOnce(Return());
    EXPECT_CALL(*coScript2, slotValueChanged(42.0, _))
            .Times(1)
            .WillOnce(Return());

    // Add more 3x more events than the recursion limit of the compressing proxy
    for (int i = 1; i <= kMaxNumOfRecursions * 3; i++) {
        co1->set(i);
    }

    // Event queue of second slot
    co2->set(41);
    co2->set(42);

    // Event queue should be cleared
    application()->processEvents();
    EXPECT_CALL(*coScript1, slotValueChanged(_, _))
            .Times(0)
            .WillOnce(Return());
    application()->processEvents();

    // Verify that compressing proxy works again after clearing event queue
    EXPECT_CALL(*coScript1, slotValueChanged(2, _))
            .Times(1)
            .WillOnce(Return());
    co1->set(1);
    co1->set(2);
    application()->processEvents();
}

} // namespace
