#include <gtest/gtest.h>
#include <libum.h>
#include <smcp1.h>
#include "libum_internal.h"
#include <cerrno>
#include <climits>
#include <cstdlib>

#if defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64)
# ifndef _WINDOWS
#  define _WINDOWS
# endif
# include <windows.h>

#endif

namespace {

#define UNDEFINED_UMX_INDEX     5
#define UMX_DEFAULT_DEV_ID      1
#define UMSDK_TEST_DEVICE_ID    "UMSDK_TEST_DEVICE_ID"

    static int test_device_id() {
        const char *value = std::getenv(UMSDK_TEST_DEVICE_ID);
        if (!value || !*value) {
            return UMX_DEFAULT_DEV_ID;
        }

        char *end = nullptr;
        errno = 0;
        const long parsed = std::strtol(value, &end, 10);
        if (errno || end == value || *end != '\0' || parsed <= 0 || parsed > INT_MAX) {
            return 0;
        }
        return static_cast<int>(parsed);
    }

    // Basic Cpp tests
    class LibumTestBasicCpp : public ::testing::Test {

    protected:
        void SetUp() override {
            mUmObj = new LibUm ();
        }

        void TearDown() override {
            delete mUmObj;
            mUmObj = NULL;
        }

        static void sleep_ms(int ms) {
#ifdef _WINDOWS
            Sleep(ms);
#else
            usleep (ms * 1000);
#endif
        }
    protected:
        LibUm *mUmObj = nullptr;
    };

    TEST_F(LibumTestBasicCpp, test_um_init) {
        EXPECT_NE(mUmObj, nullptr);
        EXPECT_FALSE(mUmObj->isOpen ());
    }

    TEST_F(LibumTestBasicCpp, test_version) {
        EXPECT_STREQ("v1.602", mUmObj->version ());
    }

    TEST_F(LibumTestBasicCpp, test_open_isOpen_close) {
        EXPECT_TRUE(mUmObj->open ());
        EXPECT_TRUE(mUmObj->isOpen ());
        mUmObj->close ();
        EXPECT_FALSE(mUmObj->isOpen ());

        // Multiple open
        EXPECT_TRUE(mUmObj->open ());
        EXPECT_TRUE(mUmObj->isOpen ());
        // Seconds open fails...
        EXPECT_FALSE(mUmObj->open ());
        // ... but existing connection remains open.
        EXPECT_TRUE(mUmObj->isOpen ());
        mUmObj->close ();
        EXPECT_FALSE(mUmObj->isOpen ());

        // Multiple close
        EXPECT_TRUE(mUmObj->open ());
        mUmObj->close ();
        EXPECT_FALSE(mUmObj->isOpen ());
        mUmObj->close ();
        EXPECT_FALSE(mUmObj->isOpen ());
    }

    TEST_F(LibumTestBasicCpp, test_openOnInterface) {
        EXPECT_TRUE(mUmObj->openOnInterface ("127.0.0.1", "127.0.0.1", 100, 0));
        EXPECT_TRUE(mUmObj->isOpen ());
        EXPECT_FALSE(mUmObj->openOnInterface ("INVALID-IP", "127.0.0.1", 100, 0));
        EXPECT_TRUE(mUmObj->isOpen ());
        mUmObj->close ();
        EXPECT_FALSE(mUmObj->isOpen ());
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
        EXPECT_TRUE(mUmObj->open ());
        EXPECT_EQ(0, mUmObj->cmdOptions (0));
        EXPECT_EQ(options, mUmObj->cmdOptions (options));
        // Failure cases
        mUmObj->close ();
        EXPECT_EQ(um_error::LIBUM_NOT_OPEN, mUmObj->cmdOptions (0));
        EXPECT_EQ(um_error::LIBUM_NOT_OPEN, mUmObj->lastError ());
    }

    TEST_F(LibumTestBasicCpp, test_lastError) {
        EXPECT_EQ(LIBUM_NOT_OPEN, mUmObj->lastError ());
        EXPECT_TRUE(mUmObj->open ());
        EXPECT_EQ(LIBUM_NO_ERROR, mUmObj->lastError ());
        EXPECT_FALSE(mUmObj->open ());
        EXPECT_EQ(LIBUM_NOT_OPEN, mUmObj->lastError ());
    }

    TEST_F(LibumTestBasicCpp, test_getDeviceList) {
        EXPECT_LE(mUmObj->getDeviceList (), 0);

        //EXPECT_TRUE(mUmObj->open ());
        //EXPECT_EQ(1, mUmObj->getDeviceList ());
    }

    TEST_F(LibumTestBasicCpp, test_clearDeviceList) {
        EXPECT_FALSE(mUmObj->clearDeviceList ());
        EXPECT_TRUE(mUmObj->open ());
        EXPECT_TRUE(mUmObj->clearDeviceList ());
    }

    TEST_F(LibumTestBasicCpp, test_getHandle) {
        EXPECT_EQ(nullptr, mUmObj->getHandle ());
        EXPECT_TRUE(mUmObj->open ());
        EXPECT_NE(nullptr, mUmObj->getHandle ());
    }

    static void localLogCallBack(int level, const void *arg, const char *func, const char *message) {
        std::cout << "localLogCallBack called - " << (char *) arg << " - " << func << " - " << message << std::endl;
    }

    TEST_F(LibumTestBasicCpp, test_setLogCallback) {
        EXPECT_FALSE(mUmObj->setLogCallback (3, NULL, NULL));
        EXPECT_TRUE(mUmObj->open ());
        EXPECT_TRUE(mUmObj->setLogCallback (3, NULL, NULL));
        EXPECT_FALSE(mUmObj->setLogCallback (-1, NULL, NULL));

        um_log_print_func tmpCallbackFuncPtr = &localLogCallBack;
        EXPECT_TRUE(mUmObj->setLogCallback (3, tmpCallbackFuncPtr, "Callback argument"));
        // EXPECT_TRUE(mUmObj->ping (1));
    }

    // Callback test helpers
    static void localCalibrationCallback(int dev, int status, const void *arg) {
        (void)dev; (void)status; (void)arg;
    }

    static void localPositionDriveCallback(int dev, int status, const void *arg) {
        (void)dev; (void)status; (void)arg;
    }

    static void localInitZeroCallback(int dev, int status, const void *arg) {
        (void)dev; (void)status; (void)arg;
    }

    static void localStatusChangedCallback(int dev, int state_mask, const void *arg) {
        (void)dev; (void)state_mask; (void)arg;
    }

    TEST_F(LibumTestBasicCpp, test_setCalibrationCallback) {
        // Expect failure when not open
        EXPECT_FALSE(mUmObj->setCalibrationCallback (NULL, NULL));

        EXPECT_TRUE(mUmObj->open ());

        // Set callback with NULL (disable)
        EXPECT_TRUE(mUmObj->setCalibrationCallback (NULL, NULL));

        // Set callback with function pointer
        EXPECT_TRUE(mUmObj->setCalibrationCallback (&localCalibrationCallback, NULL));

        // Set callback with function pointer and argument
        EXPECT_TRUE(mUmObj->setCalibrationCallback (&localCalibrationCallback, "test arg"));

        // Disable callback
        EXPECT_TRUE(mUmObj->setCalibrationCallback (NULL, NULL));
    }

    TEST_F(LibumTestBasicCpp, test_setPositionDriveCallback) {
        // Expect failure when not open
        EXPECT_FALSE(mUmObj->setPositionDriveCallback (NULL, NULL));

        EXPECT_TRUE(mUmObj->open ());

        // Set callback with NULL (disable)
        EXPECT_TRUE(mUmObj->setPositionDriveCallback (NULL, NULL));

        // Set callback with function pointer
        EXPECT_TRUE(mUmObj->setPositionDriveCallback (&localPositionDriveCallback, NULL));

        // Set callback with function pointer and argument
        EXPECT_TRUE(mUmObj->setPositionDriveCallback (&localPositionDriveCallback, "test arg"));

        // Disable callback
        EXPECT_TRUE(mUmObj->setPositionDriveCallback (NULL, NULL));
    }

    TEST_F(LibumTestBasicCpp, test_setInitZeroCallback) {
        // Expect failure when not open
        EXPECT_FALSE(mUmObj->setInitZeroCallback (NULL, NULL));

        EXPECT_TRUE(mUmObj->open ());

        // Set callback with NULL (disable)
        EXPECT_TRUE(mUmObj->setInitZeroCallback (NULL, NULL));

        // Set callback with function pointer
        EXPECT_TRUE(mUmObj->setInitZeroCallback (&localInitZeroCallback, NULL));

        // Set callback with function pointer and argument
        EXPECT_TRUE(mUmObj->setInitZeroCallback (&localInitZeroCallback, "test arg"));

        // Disable callback
        EXPECT_TRUE(mUmObj->setInitZeroCallback (NULL, NULL));
    }

    TEST_F(LibumTestBasicCpp, test_setStatusChangedCallback) {
        // Expect failure when not open
        EXPECT_FALSE(mUmObj->setStatusChangedCallback (NULL, NULL));

        EXPECT_TRUE(mUmObj->open ());

        // Set callback with NULL (disable)
        EXPECT_TRUE(mUmObj->setStatusChangedCallback (NULL, NULL));

        // Set callback with function pointer
        EXPECT_TRUE(mUmObj->setStatusChangedCallback (&localStatusChangedCallback, NULL));

        // Set callback with function pointer and argument
        EXPECT_TRUE(mUmObj->setStatusChangedCallback (&localStatusChangedCallback, "test arg"));

        // Disable callback
        EXPECT_TRUE(mUmObj->setStatusChangedCallback (NULL, NULL));
    }

    // uMp spesific tests
    class LibumTestUmpCpp : public LibumTestBasicCpp {
    protected:
        int mUmId = test_device_id();
    };

    TEST_F(LibumTestUmpCpp, test_ping) {
        EXPECT_TRUE(mUmObj->open ());
        EXPECT_TRUE(mUmObj->ping (mUmId));
        EXPECT_FALSE(mUmObj->ping (mUmId + UNDEFINED_UMX_INDEX));
    }

    TEST_F(LibumTestUmpCpp, test_isMoving) {
        // Expect error when not open
        EXPECT_LT(mUmObj->isMoving (mUmId), 0);

        EXPECT_TRUE(mUmObj->open ());

        // When device is idle, isMoving should return 0
        EXPECT_EQ(0, mUmObj->isMoving (mUmId));

        // Invalid device returns 0 (no cached status available)
        EXPECT_EQ(0, mUmObj->isMoving (mUmId + UNDEFINED_UMX_INDEX));
    }

    TEST_F(LibumTestUmpCpp, test_getAxisCount) {
        EXPECT_LE(mUmObj->getAxisCount (mUmId), 0);
        EXPECT_TRUE(mUmObj->open ());
        EXPECT_EQ(3, mUmObj->getAxisCount (mUmId));
        EXPECT_LT(mUmObj->getAxisCount (mUmId + UNDEFINED_UMX_INDEX), 0);
        EXPECT_LE(mUmObj->getAxisCount (-1), 0);
    }

    TEST_F(LibumTestUmpCpp, test_umpLEDcontrol) {
        EXPECT_TRUE(mUmObj->open ());

        // Disable all LEDs / sleep
        EXPECT_TRUE(mUmObj->umpLEDcontrol (true, mUmId));
        sleep_ms (100); // Wait a short time to get the device stabilized

        // Back to normal / wakeup
        EXPECT_TRUE(mUmObj->umpLEDcontrol (false, mUmId));
        sleep_ms (100); // Wait a short time to get the device stabilized

        EXPECT_FALSE(mUmObj->umpLEDcontrol (false, mUmId + UNDEFINED_UMX_INDEX));    // Back to normal / wakeup
    }

    TEST_F(LibumTestUmpCpp, test_readVersion) {
        const int versionBufSize = 5;
        int versionBuf[versionBufSize] = {-1};
        EXPECT_FALSE(mUmObj->readVersion (versionBuf, versionBufSize, mUmId));

        EXPECT_TRUE(mUmObj->open ());
        EXPECT_TRUE(mUmObj->readVersion (versionBuf, versionBufSize, mUmId));

        for (int i = 0; i < versionBufSize; i++) {
            EXPECT_NE(-1, versionBuf[i]) << "i=" << i;
        }

        EXPECT_FALSE(mUmObj->readVersion (versionBuf, versionBufSize, -1));
    }

    TEST_F(LibumTestUmpCpp, test_getParam) {

        int paramTmp = -2;
        EXPECT_FALSE(mUmObj->getParam (SMCP1_PARAM_VIRTUALX_ANGLE, &paramTmp, mUmId));
        EXPECT_EQ(-2, paramTmp);

        EXPECT_TRUE(mUmObj->open ());
        // device id
        int paramDevId = -1;
        EXPECT_TRUE(mUmObj->getParam (SMCP1_PARAM_DEV_ID, &paramDevId, mUmId));
        EXPECT_EQ(mUmId, paramDevId);
        // MemSpeed
        int paramMemSpeed = -1;
        EXPECT_TRUE(mUmObj->getParam (SMCP1_PARAM_MEM_SPEED, &paramMemSpeed, mUmId));
        EXPECT_GE(paramMemSpeed, 0);
        // VirtualX_Angle
        int paramVirtualXAngle = -1;
        EXPECT_TRUE(mUmObj->getParam (SMCP1_PARAM_VIRTUALX_ANGLE, &paramVirtualXAngle, mUmId));
        EXPECT_LT(paramVirtualXAngle, 900);
        EXPECT_GE(paramVirtualXAngle, 0);
        // Axis config
        int paramAxisConf = -1;
        EXPECT_TRUE(mUmObj->getParam (SMCP1_PARAM_AXIS_HEAD_CONFIGURATION, &paramAxisConf, mUmId));
        EXPECT_GE(paramAxisConf, 0);
        EXPECT_LE(paramAxisConf, 0b1111);
        // VirtualX_Angle
        int paramHwId = -1;
        EXPECT_TRUE(mUmObj->getParam (SMCP1_PARAM_HW_ID, &paramHwId, mUmId));
        EXPECT_LT(paramHwId, 5);
        EXPECT_GE(paramHwId, 1);
        // Serial number
        int paramSerialNumber = -2;
        EXPECT_TRUE(mUmObj->getParam (SMCP1_PARAM_SN, &paramSerialNumber, mUmId));
        EXPECT_GE(paramSerialNumber, -1);
        // EOW
        int paramEOW = -2;
        EXPECT_TRUE(mUmObj->getParam (SMCP1_PARAM_EOW, &paramEOW, mUmId));
        EXPECT_GE(paramEOW, -1);
        // VirtualX Detected Angle
        int paramVaDetectedAngle = -2;
        EXPECT_TRUE(mUmObj->getParam (SMCP1_PARAM_VIRTUALX_DETECTED_ANGLE, &paramVaDetectedAngle, mUmId));
        EXPECT_LT(paramVaDetectedAngle, 900);
        EXPECT_GE(paramVaDetectedAngle, 0);
        // Axis count
        int paramAxisCount = -2;
        EXPECT_TRUE(mUmObj->getParam (SMCP1_PARAM_AXIS_COUNT, &paramAxisCount, mUmId));
        EXPECT_GE(paramAxisCount, -1);

        // device id but from unfound device => will result an error
        int paramDevIdNotFound = -2;
        EXPECT_FALSE(mUmObj->getParam (SMCP1_PARAM_DEV_ID, &paramDevIdNotFound, mUmId + UNDEFINED_UMX_INDEX));
        EXPECT_EQ(-2, paramDevIdNotFound);
        // Invalid param
        int paramNoAccess = -2;
        EXPECT_FALSE(mUmObj->getParam (0x201, &paramNoAccess, mUmId));
        EXPECT_EQ(-2, paramNoAccess);
    }

    TEST_F(LibumTestUmpCpp, test_setParam) {
        EXPECT_FALSE(mUmObj->setParam (SMCP1_PARAM_VIRTUALX_ANGLE, 1000, mUmId));

        EXPECT_TRUE(mUmObj->open ());
        // Virtual axis angle
        int paramOrigVirtualXAngle = -1;
        EXPECT_TRUE(mUmObj->getParam (SMCP1_PARAM_VIRTUALX_ANGLE, &paramOrigVirtualXAngle, mUmId));
        EXPECT_LT(paramOrigVirtualXAngle, 900);
        EXPECT_GE(paramOrigVirtualXAngle, 0);

        int paramNewVirtualXAngle = 450;
        EXPECT_TRUE(mUmObj->setParam (SMCP1_PARAM_VIRTUALX_ANGLE, paramNewVirtualXAngle, mUmId));

        int paramVirtualXAngle = -1;
        EXPECT_TRUE(mUmObj->getParam (SMCP1_PARAM_VIRTUALX_ANGLE, &paramVirtualXAngle, mUmId));
        EXPECT_EQ(paramVirtualXAngle, paramNewVirtualXAngle);

        EXPECT_TRUE(mUmObj->setParam (SMCP1_PARAM_VIRTUALX_ANGLE, paramOrigVirtualXAngle, mUmId));

        // Read only params. We need to set REQ_RESP to get any response. Like errors.
        EXPECT_TRUE(mUmObj->cmdOptions (SMCP1_OPT_REQ_ACK | SMCP1_OPT_REQ_RESP));
        EXPECT_FALSE(mUmObj->setParam (SMCP1_PARAM_HW_ID, 99, mUmId));
        EXPECT_EQ(LIBUM_PEER_ERROR, mUmObj->lastError ());
        int myHwId;
        EXPECT_TRUE(mUmObj->getParam (SMCP1_PARAM_HW_ID, &myHwId, mUmId));
        EXPECT_NE(99, myHwId);
    }

    TEST_F(LibumTestUmpCpp, test_hasUnicastAddress) {
        EXPECT_TRUE(mUmObj->open ());
        EXPECT_TRUE(mUmObj->clearDeviceList ());
        EXPECT_FALSE(mUmObj->hasUnicastAddress (
                mUmId)); // In theory this might fail sometimes due timings (that we cannot avoid)
        EXPECT_TRUE(mUmObj->ping (mUmId));
        EXPECT_TRUE(mUmObj->hasUnicastAddress (mUmId));
    }

    TEST_F(LibumTestUmpCpp, test_getPositions) {
        const float unInitPosition = -123456.7890f;
        float x1 = unInitPosition;
        float y1 = unInitPosition;
        float z1 = unInitPosition;
        float w1 = unInitPosition;

        // Expect failure. Not open.
        EXPECT_FALSE(mUmObj->getPositions (&x1, &y1, &z1, &w1, mUmId));
        EXPECT_EQ(x1, unInitPosition);
        EXPECT_EQ(y1, unInitPosition);
        EXPECT_EQ(z1, unInitPosition);
        EXPECT_EQ(w1, unInitPosition);

        // Expect failure. Timeout.
        EXPECT_TRUE(mUmObj->open ());
        EXPECT_FALSE(mUmObj->getPositions (&x1, &y1, &z1, &w1, mUmId + UNDEFINED_UMX_INDEX));
        EXPECT_EQ(x1, unInitPosition);
        EXPECT_EQ(y1, unInitPosition);
        EXPECT_EQ(z1, unInitPosition);
        EXPECT_EQ(w1, unInitPosition);

        // Expect failure. Invalid device.
        EXPECT_FALSE(mUmObj->getPositions (&x1, &y1, &z1, &w1, -1));
        EXPECT_EQ(x1, unInitPosition);
        EXPECT_EQ(y1, unInitPosition);
        EXPECT_EQ(z1, unInitPosition);
        EXPECT_EQ(w1, unInitPosition);

        // Expect pass. Normal use case.
        int axisCnt = mUmObj->getAxisCount (mUmId);
        EXPECT_GE(axisCnt, 3);
        EXPECT_LE(axisCnt, 4);

        EXPECT_TRUE(mUmObj->getPositions (&x1, &y1, &z1, &w1, mUmId));

        EXPECT_NE(unInitPosition, x1);
        EXPECT_NE(unInitPosition, y1);
        EXPECT_NE(unInitPosition, z1);
        if (axisCnt == 4) {
            EXPECT_NE(unInitPosition, w1);
        } else {
            EXPECT_EQ(unInitPosition, w1);
        }

        float x2 = unInitPosition;
        float y2 = unInitPosition;
        float z2 = unInitPosition;
        float w2 = unInitPosition;

        // Force read using cached values
        EXPECT_TRUE(mUmObj->getPositions (&x2, &y2, &z2, &w2, mUmId, LIBUM_TIMELIMIT_CACHE_ONLY));
        EXPECT_EQ(x1, x2);
        EXPECT_EQ(y1, y2);
        EXPECT_EQ(z1, z2);
        EXPECT_EQ(w1, w2);

        // Force read coordinates from device
        float x3 = unInitPosition;
        float y3 = unInitPosition;
        float z3 = unInitPosition;
        float w3 = unInitPosition;

        EXPECT_TRUE(mUmObj->getPositions (&x3, &y3, &z3, &w3, mUmId, LIBUM_TIMELIMIT_DISABLED));
        EXPECT_NE(unInitPosition, x3);
        EXPECT_NE(unInitPosition, y3);
        EXPECT_NE(unInitPosition, z3);
        if (axisCnt == 4) {
            EXPECT_NE(unInitPosition, w3);
        } else {
            EXPECT_EQ(unInitPosition, w3);
        }
    }

    TEST_F(LibumTestUmpCpp, test_getSetFeature) {
        bool value;
        EXPECT_FALSE(mUmObj->getFeature (SMCP10_FEAT_DISABLE_LEDS, &value, mUmId));
        EXPECT_FALSE(mUmObj->setFeature (SMCP10_FEAT_DISABLE_LEDS, false, mUmId));

        EXPECT_TRUE(mUmObj->open ());

        // SMCP10_FEAT_DISABLE_LEDS
        EXPECT_TRUE(mUmObj->getFeature (SMCP10_FEAT_DISABLE_LEDS, &value, mUmId));

        bool newValue = !value;
        EXPECT_NE(newValue, value);
        EXPECT_TRUE(mUmObj->setFeature (SMCP10_FEAT_DISABLE_LEDS, newValue, mUmId));
        EXPECT_TRUE(mUmObj->getFeature (SMCP10_FEAT_DISABLE_LEDS, &value, mUmId));
        EXPECT_EQ(newValue, value);

        newValue = !value;
        EXPECT_NE(newValue, value);
        EXPECT_TRUE(mUmObj->setFeature (SMCP10_FEAT_DISABLE_LEDS, newValue, mUmId));
        EXPECT_TRUE(mUmObj->getFeature (SMCP10_FEAT_DISABLE_LEDS, &value, mUmId));
        EXPECT_EQ(newValue, value);

        // SMCP10_FEAT_PREVENT_MOVEMENT
        EXPECT_TRUE(mUmObj->getFeature (SMCP10_FEAT_PREVENT_MOVEMENT, &value, mUmId));

        newValue = !value;
        EXPECT_NE(newValue, value);
        EXPECT_TRUE(mUmObj->setFeature (SMCP10_FEAT_PREVENT_MOVEMENT, newValue, mUmId));
        EXPECT_TRUE(mUmObj->getFeature (SMCP10_FEAT_PREVENT_MOVEMENT, &value, mUmId));
        EXPECT_EQ(newValue, value);

        newValue = !value;
        EXPECT_NE(newValue, value);
        EXPECT_TRUE(mUmObj->setFeature (SMCP10_FEAT_PREVENT_MOVEMENT, newValue, mUmId));
        EXPECT_TRUE(mUmObj->getFeature (SMCP10_FEAT_PREVENT_MOVEMENT, &value, mUmId));
        EXPECT_EQ(newValue, value);
    }

    TEST_F(LibumTestUmpCpp, test_getSetExtFeature) {
        bool value;
        EXPECT_FALSE(mUmObj->getExtFeature (SMCP10_EXT_FEAT_SOFT_START, &value, mUmId));
        EXPECT_FALSE(mUmObj->setExtFeature (SMCP10_EXT_FEAT_SOFT_START, false, mUmId));

        EXPECT_TRUE(mUmObj->open ());
        EXPECT_TRUE(mUmObj->getExtFeature (SMCP10_EXT_FEAT_SOFT_START, &value, mUmId));

        bool newValue = !value;
        EXPECT_NE(newValue, value);
        EXPECT_TRUE(mUmObj->setExtFeature (SMCP10_EXT_FEAT_SOFT_START, newValue, mUmId));
        EXPECT_TRUE(mUmObj->getExtFeature (SMCP10_EXT_FEAT_SOFT_START, &value, mUmId));
        EXPECT_EQ(newValue, value);

        newValue = !value;
        EXPECT_NE(newValue, value);
        EXPECT_TRUE(mUmObj->setExtFeature (SMCP10_EXT_FEAT_SOFT_START, newValue, mUmId));
        EXPECT_TRUE(mUmObj->getExtFeature (SMCP10_EXT_FEAT_SOFT_START, &value, mUmId));
        EXPECT_EQ(newValue, value);
    }


    TEST_F(LibumTestUmpCpp, test_gotoPos) {
        EXPECT_FALSE(mUmObj->gotoPos (0, 0, 0, 0, 1000, mUmId));
        EXPECT_TRUE(mUmObj->open ());

        int axisCnt = mUmObj->getAxisCount (mUmId);
        EXPECT_GE(axisCnt, 3);
        EXPECT_LE(axisCnt, 4);

        float x1, y1, z1, w1;
        const float KDeltaUm = 200;
        EXPECT_TRUE(mUmObj->getPositions (&x1, &y1, &z1, &w1, mUmId));

        float x2 = x1 + KDeltaUm;
        float y2 = y1 + KDeltaUm;
        float z2 = z1 + KDeltaUm;
        float w2 = w1 + KDeltaUm;
        EXPECT_TRUE(mUmObj->gotoPos (x2, y2, z2, w2, KDeltaUm, mUmId, true));

        sleep_ms (axisCnt == 3 ? 2000 : 3000);

        float x3, y3, z3, w3;
        const float KTargetToleranceUm = 1.00;
        EXPECT_TRUE(mUmObj->getPositions (&x3, &y3, &z3, &w3, mUmId, LIBUM_TIMELIMIT_DISABLED));

        EXPECT_TRUE((x3 >= x2 - KTargetToleranceUm) && (x3 <= x2 + KTargetToleranceUm));
        EXPECT_TRUE((y3 >= y2 - KTargetToleranceUm) && (y3 <= y2 + KTargetToleranceUm));
        EXPECT_TRUE((z3 >= z2 - KTargetToleranceUm) && (z3 <= z2 + KTargetToleranceUm));
        if (axisCnt == 4) {
            EXPECT_TRUE((w3 >= w2 - KTargetToleranceUm) && (w3 <= w2 + KTargetToleranceUm));
        } else {
            EXPECT_TRUE((w3 >= w1 - KTargetToleranceUm) && (w3 <= w1 + KTargetToleranceUm));
        }

        // Move actuators back to their original positions
        x2 = x1;
        y2 = y1;
        z2 = z1;
        w2 = w1;
        EXPECT_TRUE(mUmObj->gotoPos (x2, y2, z2, w2, KDeltaUm, mUmId, true));
        sleep_ms (axisCnt == 3 ? 2000 : 3000);

        EXPECT_TRUE(mUmObj->getPositions (&x3, &y3, &z3, &w3, mUmId));

        EXPECT_TRUE((x3 >= x2 - KTargetToleranceUm) && (x3 <= x2 + KTargetToleranceUm));
        EXPECT_TRUE((y3 >= y2 - KTargetToleranceUm) && (y3 <= y2 + KTargetToleranceUm));
        EXPECT_TRUE((z3 >= z2 - KTargetToleranceUm) && (z3 <= z2 + KTargetToleranceUm));
        if (axisCnt == 4) {
            EXPECT_TRUE((w3 >= w2 - KTargetToleranceUm) && (w3 <= w2 + KTargetToleranceUm));
        } else {
            EXPECT_TRUE((w3 >= w1 - KTargetToleranceUm) && (w3 <= w1 + KTargetToleranceUm));
        }
        // Invalid deviceId (-1)
        EXPECT_FALSE(mUmObj->gotoPos (1000, 1000, 1000, 1000, 500, -1, true));
        // Invalid position (LIBUM_MAX_POSITION+1)
        EXPECT_FALSE(mUmObj->gotoPos (LIBUM_MAX_POSITION + 1, 1000, 1000, 1000, 500, mUmId, true));
        // Invalid position (-1001)
        EXPECT_FALSE(mUmObj->gotoPos (1000, -1001, 1000, 1000, 500, mUmId, true));
        // Invalid speed (-1.0)
        EXPECT_FALSE(mUmObj->gotoPos (1000, 1000, 1000, 1000, -1.0, mUmId, true));

        // Verify that prevent_movement filter works
        EXPECT_TRUE(mUmObj->setFeature (SMCP10_FEAT_PREVENT_MOVEMENT, true, mUmId));
        EXPECT_FALSE(mUmObj->gotoPos (1000, 1000, 1000, 1000, 500, mUmId, true));
        EXPECT_TRUE(mUmObj->setFeature (SMCP10_FEAT_PREVENT_MOVEMENT, false, mUmId));
        sleep_ms (1000);

    }

    TEST_F(LibumTestUmpCpp, test_takeStep) {
        EXPECT_FALSE(mUmObj->takeStep (10, 10, 10, 10, 200, mUmId));
        EXPECT_TRUE(mUmObj->open ());

        int axisCnt = mUmObj->getAxisCount (mUmId);
        EXPECT_GE(axisCnt, 3);
        EXPECT_LE(axisCnt, 4);

        // Phase 1. Set reference point
        float x1, y1, z1, w1;
        const float KDeltaUm = 200;
        const int KSpeedUms = static_cast<int>(2.0f * KDeltaUm);
        EXPECT_TRUE(mUmObj->getPositions (&x1, &y1, &z1, &w1, mUmId, LIBUM_TIMELIMIT_DISABLED));

        // Phase 2. Move to target point
        EXPECT_TRUE(mUmObj->takeStep (KDeltaUm, KDeltaUm, KDeltaUm, KDeltaUm, KSpeedUms, mUmId));
        sleep_ms (axisCnt == 3 ? 2000 : 3000);

        float x2, y2, z2, w2;
        const float KTargetToleranceUm = 1.00;
        EXPECT_TRUE(mUmObj->getPositions (&x2, &y2, &z2, &w2, mUmId, LIBUM_TIMELIMIT_DISABLED));

        EXPECT_TRUE((x2 >= x1 + KDeltaUm - KTargetToleranceUm) &&
                    (x2 <= x1 + KDeltaUm + KTargetToleranceUm));
        EXPECT_TRUE((y2 >= y1 + KDeltaUm - KTargetToleranceUm) &&
                    (y2 <= y1 + KDeltaUm + KTargetToleranceUm));
        EXPECT_TRUE((z2 >= z1 + KDeltaUm - KTargetToleranceUm) &&
                    (z2 <= z1 + KDeltaUm + KTargetToleranceUm));
        if (axisCnt == 4) {
            EXPECT_TRUE((w2 >= w1 + KDeltaUm - KTargetToleranceUm) &&
                        (w2 <= w1 + KDeltaUm + KTargetToleranceUm));
        } else {
            EXPECT_TRUE((w2 >= w1 - KTargetToleranceUm) &&
                        (w2 <= w1 + KTargetToleranceUm));
        }

        // Phase 3. Move back to reference point
        EXPECT_TRUE(mUmObj->takeStep (-KDeltaUm, -KDeltaUm, -KDeltaUm, -KDeltaUm, KSpeedUms, mUmId));
        sleep_ms (axisCnt == 3 ? 2000 : 3000);

        float x3, y3, z3, w3;
        EXPECT_TRUE(mUmObj->getPositions (&x3, &y3, &z3, &w3, mUmId, LIBUM_TIMELIMIT_DISABLED));

        EXPECT_TRUE((x3 >= x1 - KTargetToleranceUm) &&
                    (x3 <= x1 + KTargetToleranceUm));
        EXPECT_TRUE((y3 >= y1 - KTargetToleranceUm) &&
                    (y3 <= y1 + KTargetToleranceUm));
        EXPECT_TRUE((z3 >= z1 - KTargetToleranceUm) &&
                    (z3 <= z1 + KTargetToleranceUm));
        EXPECT_TRUE((w3 >= w1 - KTargetToleranceUm) &&
                    (w3 <= w1 + KTargetToleranceUm));

        // Phase 4. Verify with the prevent_movement feature
        EXPECT_TRUE(mUmObj->setFeature (SMCP10_FEAT_PREVENT_MOVEMENT, true, mUmId));
        EXPECT_FALSE(mUmObj->takeStep (KDeltaUm, KDeltaUm, KDeltaUm, KDeltaUm, KSpeedUms, mUmId));
        EXPECT_TRUE(mUmObj->setFeature (SMCP10_FEAT_PREVENT_MOVEMENT, false, mUmId));
    }

    TEST_F(LibumTestUmpCpp, test_umpHandednessConfiguration) {
        EXPECT_LT(mUmObj->umpHandednessConfiguration (mUmId), 0);
        EXPECT_TRUE(mUmObj->open ());
        int handednessConfig = mUmObj->umpHandednessConfiguration (mUmId);
        EXPECT_TRUE(handednessConfig == 0 || handednessConfig == 1);
    }

    TEST_F(LibumTestBasicCpp, test_isValidAxisDriveOrder) {
        // Valid axis drive orders - all permutations must have each axis 0,1,2,3 exactly once
        EXPECT_TRUE(is_valid_axis_drive_order(0x00010203)); // X->Y->Z->D
        EXPECT_TRUE(is_valid_axis_drive_order(0x03020100)); // D->Z->Y->X
        EXPECT_TRUE(is_valid_axis_drive_order(0x03000102)); // D->X->Y->Z (default for uMs)
        EXPECT_TRUE(is_valid_axis_drive_order(0x01020003)); // Y->Z->X->D (default for uMp)
        EXPECT_TRUE(is_valid_axis_drive_order(0x02010300)); // Z->Y->D->X
        EXPECT_TRUE(is_valid_axis_drive_order(0x00030201)); // X->D->Z->Y

        // Invalid: axis value > 3
        EXPECT_FALSE(is_valid_axis_drive_order(0x04010203)); // 4 is not valid
        EXPECT_FALSE(is_valid_axis_drive_order(0x00010204)); // 4 is not valid
        EXPECT_FALSE(is_valid_axis_drive_order(0xFF010203)); // 0xFF is not valid

        // Invalid: duplicate axis values
        EXPECT_FALSE(is_valid_axis_drive_order(0x00000102)); // X appears twice
        EXPECT_FALSE(is_valid_axis_drive_order(0x00010101)); // Y appears three times
        EXPECT_FALSE(is_valid_axis_drive_order(0x03030201)); // D appears twice
        EXPECT_FALSE(is_valid_axis_drive_order(0x00000000)); // X appears four times
        EXPECT_FALSE(is_valid_axis_drive_order(0x01010101)); // Y appears four times
        EXPECT_FALSE(is_valid_axis_drive_order(0x02020202)); // Z appears four times
        EXPECT_FALSE(is_valid_axis_drive_order(0x03030303)); // D appears four times
    }

    TEST_F(LibumTestUmpCpp, test_driveOrderCustom) {
        EXPECT_FALSE(mUmObj->setAxisDriveOrder (0x00010203, mUmId));
        EXPECT_TRUE(mUmObj->open ());

        // Get current drive order
        int currentDriveOrder = mUmObj->getAxisDriveOrder (mUmId);
        EXPECT_GE(currentDriveOrder, 0);

        // Set new drive order
        int newDriveOrder = 0x03020100; // D->Z->Y->X
        EXPECT_TRUE(mUmObj->setAxisDriveOrder (newDriveOrder, mUmId));

        // Verify new drive order
        int verifyDriveOrder = mUmObj->getAxisDriveOrder (mUmId);
        EXPECT_EQ(newDriveOrder, verifyDriveOrder);

        // Restore original drive order
        EXPECT_TRUE(mUmObj->setAxisDriveOrder (currentDriveOrder, mUmId));

        // Verify restored drive order
        int restoredDriveOrder = mUmObj->getAxisDriveOrder (mUmId);
        EXPECT_EQ(currentDriveOrder, restoredDriveOrder);
    }

    // Main
    int main(int argc, char **argv) {
        ::testing::InitGoogleTest (&argc, argv);
        return RUN_ALL_TESTS ();
    }
}
