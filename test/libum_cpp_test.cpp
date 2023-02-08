#include <gtest/gtest.h>
#include <libum.h>
#include <smcp1.h>


class LibUmTest : public ::testing::Test {

    protected:
        void SetUp() override {
            umObj = new LibUm();
        }
        void TearDown() override {
            if (umObj->isOpen()) {
                umObj->close();
            }
        }
    protected:
        LibUm * umObj;
};

TEST_F(LibUmTest, test_um_init) {
    EXPECT_NE(umObj, nullptr);
    EXPECT_FALSE(umObj->isOpen());
}

TEST_F(LibUmTest, test_version) {
    EXPECT_STREQ("v1.400", umObj->version());
}

TEST_F(LibUmTest, test_cmdOptions) {
    int options = (
        SMCP1_OPT_WAIT_TRIGGER_1 |
        SMCP1_OPT_PRIORITY |
        SMCP1_OPT_REQ_BCAST |
        SMCP1_OPT_REQ_NOTIFY |
        SMCP1_OPT_REQ_RESP |
        SMCP1_OPT_REQ_ACK
    );

    // Successful cases
    EXPECT_TRUE(umObj->open());
    EXPECT_EQ(0, umObj->cmdOptions(0));
    EXPECT_EQ(options, umObj->cmdOptions(options));
    // Failure cases
    umObj->close();
    EXPECT_EQ(um_error::LIBUM_NOT_OPEN, umObj->cmdOptions(0));
    EXPECT_EQ(um_error::LIBUM_NOT_OPEN, umObj->lastError());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
