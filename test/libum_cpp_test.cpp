#include <gtest/gtest.h>
#include <libum.h>
#include <smcp1.h>


namespace {
    class LibumTestCpp : public ::testing::Test {

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

    TEST_F(LibumTestCpp, test_um_init) {
        EXPECT_NE(mUmObj, nullptr);
        EXPECT_FALSE(mUmObj->isOpen());
    }

    TEST_F(LibumTestCpp, test_version) {
        EXPECT_STREQ("v1.400", mUmObj->version());
    }

    TEST_F(LibumTestCpp, test_cmdOptions) {
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

    TEST_F(LibumTestCpp, test_lastError) {
        EXPECT_EQ(LIBUM_NOT_OPEN, mUmObj->lastError());
        EXPECT_TRUE(mUmObj->open());
        EXPECT_EQ(LIBUM_NO_ERROR, mUmObj->lastError());
        EXPECT_FALSE(mUmObj->open());
        EXPECT_EQ(LIBUM_NOT_OPEN, mUmObj->lastError());
    }

    int main(int argc, char **argv) {
        ::testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
    }
}
