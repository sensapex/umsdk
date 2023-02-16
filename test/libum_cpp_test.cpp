#include <gtest/gtest.h>
#include <libum.h>
#include <smcp1.h>

namespace {

#define UNDEFINED_UMX_INDEX     5
#define UMX_DEFAULT_DEV_ID      1

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

    TEST_F(LibumTestBasicCpp, test_getHandle) {
        EXPECT_EQ(nullptr, mUmObj->getHandle());
        EXPECT_TRUE(mUmObj->open());
        EXPECT_NE(nullptr, mUmObj->getHandle());
    }

    TEST_F(LibumTestBasicCpp, test_setLogCallback) {
        EXPECT_FALSE(mUmObj->setLogCallback(3, NULL, NULL));
        EXPECT_TRUE(mUmObj->open());
        EXPECT_TRUE(mUmObj->setLogCallback(3, NULL, NULL));
        EXPECT_FALSE(mUmObj->setLogCallback(-1, NULL, NULL));
    }

    // uMp spesific tests
    class LibumTestUmpCpp : public LibumTestBasicCpp {
        protected:
            int mUmId = UMX_DEFAULT_DEV_ID;
    };

    TEST_F(LibumTestUmpCpp, test_ping) {
        EXPECT_TRUE(mUmObj->open());
        EXPECT_TRUE(mUmObj->ping(mUmId));
        EXPECT_FALSE(mUmObj->ping(mUmId+UNDEFINED_UMX_INDEX));
    }

    TEST_F(LibumTestUmpCpp, test_getAxisCount) {
        EXPECT_TRUE(mUmObj->open());
        EXPECT_EQ(3, mUmObj->getAxisCount(mUmId));
        EXPECT_LT(mUmObj->getAxisCount(mUmId+UNDEFINED_UMX_INDEX), 0);
    }

    TEST_F(LibumTestUmpCpp, test_umpLEDcontrol) {
        EXPECT_TRUE(mUmObj->open());
        EXPECT_EQ(LIBUM_NO_ERROR, mUmObj->lastError());
        EXPECT_TRUE(mUmObj->umpLEDcontrol(true, mUmId));     // Disable all LEDs / sleep
        // We use a broadcast address so we might see out own packages also (LIBUM_INVALID_DEV)
        EXPECT_TRUE(LIBUM_NO_ERROR == mUmObj->lastError() ||
                  LIBUM_INVALID_DEV == mUmObj->lastError());

        EXPECT_TRUE(mUmObj->umpLEDcontrol(false, mUmId));    // Back to normal / wakeup
        // We use a broadcast address so we might see out own packages also (LIBUM_INVALID_DEV)
        EXPECT_TRUE(LIBUM_NO_ERROR == mUmObj->lastError() ||
                  LIBUM_INVALID_DEV == mUmObj->lastError());
        // Wait a second to get the device online again.
        usleep(100000); // 100 ms

        EXPECT_FALSE(mUmObj->umpLEDcontrol(false, mUmId+UNDEFINED_UMX_INDEX));    // Back to normal / wakeup
    }

    TEST_F(LibumTestUmpCpp, test_readVersion) {
        const int versionBufSize = 5;
        EXPECT_TRUE(mUmObj->open());
        int versionBuf[versionBufSize] = {-1};
        EXPECT_TRUE(mUmObj->readVersion(versionBuf, versionBufSize, mUmId));

        for (int i = 0; i < versionBufSize; i++) {
            EXPECT_NE(-1, versionBuf[i]) << "i=" << i;
        }
    }

    TEST_F(LibumTestUmpCpp, test_getParam) {
        EXPECT_TRUE(mUmObj->open());
        // device id
        int paramDevId = -1;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_DEV_ID, &paramDevId, mUmId));
        EXPECT_EQ(mUmId, paramDevId);
        // MemSpeed
        int paramMemSpeed = -1;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_MEM_SPEED, &paramMemSpeed, mUmId));
        EXPECT_GE(paramMemSpeed, 0);
        // VirtualX_Angle
        int paramVirtualXAngle = -1;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_VIRTUALX_ANGLE, &paramVirtualXAngle, mUmId));
        EXPECT_LT(paramVirtualXAngle, 900);
        EXPECT_GE(paramVirtualXAngle, 0);
        // Axis config
        int paramAxisConf = -1;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_AXIS_HEAD_CONFIGURATION, &paramAxisConf, mUmId));
        EXPECT_GE(paramAxisConf, 0);
        EXPECT_LE(paramAxisConf, 0b1111);
        // VirtualX_Angle
        int paramHwId = -1;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_HW_ID, &paramHwId, mUmId));
        EXPECT_LT(paramHwId, 5);
        EXPECT_GE(paramHwId, 1);
        // Serial number
        int paramSerialNumber = -2;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_SN, &paramSerialNumber, mUmId));
        EXPECT_GE(paramSerialNumber, -1);
        // EOW
        int paramEOW = -2;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_EOW, &paramEOW, mUmId));
        EXPECT_GE(paramEOW, -1);
        // VirtualX Detected Angle
        int paramVaDetectedAngle = -2;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_VIRTUALX_DETECTED_ANGLE, &paramVaDetectedAngle, mUmId));
        EXPECT_LT(paramVaDetectedAngle, 900);
        EXPECT_GE(paramVaDetectedAngle, 0);
        // Axis count
        int paramAxisCount = -2;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_AXIS_COUNT, &paramAxisCount, mUmId));
        EXPECT_GE(paramAxisCount, -1);

        // device id but from unfound device => will result an error
        int paramDevIdNotFound = -2;
        EXPECT_FALSE(mUmObj->getParam(SMCP1_PARAM_DEV_ID, &paramDevIdNotFound, mUmId+UNDEFINED_UMX_INDEX));
        EXPECT_EQ(-2, paramDevIdNotFound);
        // Invalid param
        int paramNoAccess = -2;
        EXPECT_FALSE(mUmObj->getParam(0x201, &paramNoAccess, mUmId));
        EXPECT_EQ(-2, paramNoAccess);
    }

    TEST_F(LibumTestUmpCpp, test_setParam) {
        EXPECT_TRUE(mUmObj->open());
        // Virtual axis angle
        int paramOrigVirtualXAngle = -1;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_VIRTUALX_ANGLE, &paramOrigVirtualXAngle, mUmId));
        EXPECT_LT(paramOrigVirtualXAngle, 900);
        EXPECT_GE(paramOrigVirtualXAngle, 0);

        int paramNewVirtualXAngle = 450;
        EXPECT_TRUE(mUmObj->setParam(SMCP1_PARAM_VIRTUALX_ANGLE, paramNewVirtualXAngle, mUmId));

        int paramVirtualXAngle = -1;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_VIRTUALX_ANGLE, &paramVirtualXAngle, mUmId));
        EXPECT_EQ(paramVirtualXAngle, paramNewVirtualXAngle);

        EXPECT_TRUE(mUmObj->setParam(SMCP1_PARAM_VIRTUALX_ANGLE, paramOrigVirtualXAngle, mUmId));

        // Read only params. We need to set REQ_RESP to get any response. Like errors.
        EXPECT_TRUE(mUmObj->cmdOptions(SMCP1_OPT_REQ_ACK | SMCP1_OPT_REQ_RESP));
        EXPECT_FALSE(mUmObj->setParam(SMCP1_PARAM_HW_ID, 99, mUmId));
        int myHwId;
        EXPECT_TRUE(mUmObj->getParam(SMCP1_PARAM_HW_ID, &myHwId, mUmId));
        EXPECT_NE(99, myHwId);
    }

    TEST_F(LibumTestUmpCpp, test_hasUnicastAddress) {
        EXPECT_TRUE(mUmObj->open());
        EXPECT_TRUE(mUmObj->clearDeviceList());
        EXPECT_FALSE(mUmObj->hasUnicastAddress(mUmId)); // In theory this might fail sometimes due timings (that we cannot avoid)
        EXPECT_TRUE(mUmObj->ping(mUmId));
        EXPECT_TRUE(mUmObj->hasUnicastAddress(mUmId));
    }

    // Main
    int main(int argc, char **argv) {
        ::testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
    }
}
