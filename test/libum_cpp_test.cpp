#include <gtest/gtest.h>
#include <libum.h>
#include <smcp1.h>

namespace {

#define UNDEFINED_UMX_INDEX     5

    // Basic Cpp tests
    class LibumTestBasicCpp : public ::testing::Test {

        protected:
            void SetUp() override {
                mUmObj = new LibUm();
            }
            void TearDown() override {
                delete mUmObj;
                mUmObj = NULL;
            }
        protected:
            LibUm * mUmObj;
    };

    TEST_F(LibumTestBasicCpp, test_um_init) {
        EXPECT_NE(mUmObj, nullptr);
        EXPECT_FALSE(mUmObj->isOpen());
    }

    TEST_F(LibumTestBasicCpp, test_version) {
        EXPECT_STREQ("v1.400", mUmObj->version());
    }

    TEST_F(LibumTestBasicCpp, test_open_isOpen_close) {
        EXPECT_TRUE(mUmObj->open());
        EXPECT_TRUE(mUmObj->isOpen());
        mUmObj->close();
        EXPECT_FALSE(mUmObj->isOpen());

        // Multiple open
        EXPECT_TRUE(mUmObj->open());
        EXPECT_TRUE(mUmObj->isOpen());
        // Seconds open fails...
        EXPECT_FALSE(mUmObj->open());
        // ... but existing connection remains open.
        EXPECT_TRUE(mUmObj->isOpen());
        mUmObj->close();
        EXPECT_FALSE(mUmObj->isOpen());

        // Multiple close
        EXPECT_TRUE(mUmObj->open());
        mUmObj->close();
        EXPECT_FALSE(mUmObj->isOpen());
        mUmObj->close();
        EXPECT_FALSE(mUmObj->isOpen());
    }

    TEST_F(LibumTestBasicCpp, test_cmdOptions) {
        int options = (
            SMCP1_OPT_WAIT_TRIGGER_1 |
            SMCP1_OPT_PRIORITY |
            SMCP1_OPT_REQ_BCAST |
            SMCP1_OPT_REQ_NOTIFY |
            SMCP1_OPT_REQ_RESP |
            SMCP1_OPT_REQ_ACK
        );

        // Successful cases
        EXPECT_TRUE(mUmObj->open());
        EXPECT_EQ(0, mUmObj->cmdOptions(0));
        EXPECT_EQ(options, mUmObj->cmdOptions(options));
        // Failure cases
        mUmObj->close();
        EXPECT_EQ(um_error::LIBUM_NOT_OPEN, mUmObj->cmdOptions(0));
        EXPECT_EQ(um_error::LIBUM_NOT_OPEN, mUmObj->lastError());
    }

    TEST_F(LibumTestBasicCpp, test_lastError) {
        EXPECT_EQ(LIBUM_NOT_OPEN, mUmObj->lastError());
        EXPECT_TRUE(mUmObj->open());
        EXPECT_EQ(LIBUM_NO_ERROR, mUmObj->lastError());
        EXPECT_FALSE(mUmObj->open());
        EXPECT_EQ(LIBUM_NOT_OPEN, mUmObj->lastError());
    }

    TEST_F(LibumTestBasicCpp, test_getDeviceList) {
        EXPECT_TRUE(mUmObj->open());
        EXPECT_EQ(1, mUmObj->getDeviceList());
    }

    TEST_F(LibumTestBasicCpp, test_clearDeviceList) {
        EXPECT_TRUE(mUmObj->open());
        EXPECT_TRUE(mUmObj->clearDeviceList());
    }

    // uMp spesific tests
    class LibumTestUmpCpp : public LibumTestBasicCpp {
        protected:
            int umId = 1;
    };

    TEST_F(LibumTestUmpCpp, test_ping) {
        EXPECT_TRUE(mUmObj->open());
        EXPECT_TRUE(mUmObj->ping(umId));
        EXPECT_FALSE(mUmObj->ping(umId+UNDEFINED_UMX_INDEX));
    }

    TEST_F(LibumTestUmpCpp, test_getAxisCount) {
        EXPECT_TRUE(mUmObj->open());
        EXPECT_EQ(3, mUmObj->getAxisCount(umId));
        EXPECT_LT(mUmObj->getAxisCount(umId+UNDEFINED_UMX_INDEX), 0);
    }

    TEST_F(LibumTestUmpCpp, test_umpLEDcontrol) {
        EXPECT_TRUE(mUmObj->open());
        EXPECT_EQ(LIBUM_NO_ERROR, mUmObj->lastError());
        EXPECT_TRUE(mUmObj->umpLEDcontrol(true, umId));     // Disable all LEDs / sleep
        // We use a broadcast address so we might see out own packages also (LIBUM_INVALID_DEV)
        EXPECT_TRUE(LIBUM_NO_ERROR == mUmObj->lastError() ||
                  LIBUM_INVALID_DEV == mUmObj->lastError());

        EXPECT_TRUE(mUmObj->umpLEDcontrol(false, umId));    // Back to normal / wakeup
        // We use a broadcast address so we might see out own packages also (LIBUM_INVALID_DEV)
        EXPECT_TRUE(LIBUM_NO_ERROR == mUmObj->lastError() ||
                  LIBUM_INVALID_DEV == mUmObj->lastError());
        // Wait a second to get the device online again.
        usleep(100000); // 100 ms

        EXPECT_FALSE(mUmObj->umpLEDcontrol(false, umId+UNDEFINED_UMX_INDEX));    // Back to normal / wakeup
    }

    TEST_F(LibumTestUmpCpp, test_readVersion) {
        const int versionBufSize = 5;
        EXPECT_TRUE(mUmObj->open());
        int versionBuf[versionBufSize] = {-1};
        EXPECT_TRUE(mUmObj->readVersion(versionBuf, versionBufSize, umId));

        for (int i = 0; i < versionBufSize; i++) {
            EXPECT_NE(-1, versionBuf[i]) << "i=" << i;
        }
    }

    TEST_F(LibumTestUmpCpp, test_getParam) {
        EXPECT_TRUE(mUmObj->open());
        // device id
        int paramDevId = -1;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_DEV_ID, &paramDevId, umId));
        EXPECT_EQ(umId, paramDevId);
        // MemSpeed
        int paramMemSpeed = -1;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_MEM_SPEED, &paramMemSpeed, umId));
        EXPECT_GE(paramMemSpeed, 0);
        // VirtualX_Angle
        int paramVirtualXAngle = -1;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_VIRTUALX_ANGLE, &paramVirtualXAngle, umId));
        EXPECT_LT(paramVirtualXAngle, 900);
        EXPECT_GE(paramVirtualXAngle, 0);
        // Axis config
        int paramAxisConf = -1;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_AXIS_HEAD_CONFIGURATION, &paramAxisConf, umId));
        EXPECT_GE(paramAxisConf, 0);
        EXPECT_LE(paramAxisConf, 0b1111);
        // VirtualX_Angle
        int paramHwId = -1;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_HW_ID, &paramHwId, umId));
        EXPECT_LT(paramHwId, 5);
        EXPECT_GE(paramHwId, 1);
        // Serial number
        int paramSerialNumber = -2;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_SN, &paramSerialNumber, umId));
        EXPECT_GE(paramSerialNumber, -1);
        // EOW
        int paramEOW = -2;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_EOW, &paramEOW, umId));
        EXPECT_GE(paramEOW, -1);
        // VirtualX Detected Angle
        int paramVaDetectedAngle = -2;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_VIRTUALX_DETECTED_ANGLE, &paramVaDetectedAngle, umId));
        EXPECT_LT(paramVaDetectedAngle, 900);
        EXPECT_GE(paramVaDetectedAngle, 0);
        // Axis count
        int paramAxisCount = -2;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_AXIS_COUNT, &paramAxisCount, umId));
        EXPECT_GE(paramAxisCount, -1);

        // device id but from unfound device => will result an error
        int paramDevIdNotFound = -2;
        EXPECT_FALSE(mUmObj->getParam(SMCP1_PARAM_DEV_ID, &paramDevIdNotFound, umId+UNDEFINED_UMX_INDEX));
        EXPECT_EQ(-2, paramDevIdNotFound);
        // Invalid param
        int paramNoAccess = -2;
        EXPECT_FALSE(mUmObj->getParam(0x201, &paramNoAccess, umId));
        EXPECT_EQ(-2, paramNoAccess);
    }

    // Main
    int main(int argc, char **argv) {
        ::testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
    }
}
