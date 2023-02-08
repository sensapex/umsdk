#include <gtest/gtest.h>
#include <libum.h>

TEST(libumTest, test_um_get_version)
{
    const char *version = um_get_version();
    // Expect two strings not to be equal.
    EXPECT_STREQ("v1.400", version);
}

TEST(libumTest, um_open)
{
    um_state *umHandle = um_open("INVALID-IP", 100, 0);
    EXPECT_EQ(NULL, umHandle);
    umHandle = um_open(LIBUM_DEF_BCAST_ADDRESS, 100, 0);
    EXPECT_NE(nullptr, umHandle);
    if (umHandle) {
        um_close(umHandle);
        umHandle = NULL;
    }
}

TEST(libumTest, test_um_get_timestamp_us)
{
    struct timespec tms;

    timespec_get(&tms, TIME_UTC);

    unsigned long long sys_epoch = tms.tv_sec * 1000;
    sys_epoch += tms.tv_nsec / 1000000;
    EXPECT_NE(sys_epoch, 0);

    unsigned long long result = um_get_timestamp_ms();
    // Note! We might expect occasional 1-2 us gap sometimes.
    // TODO add some tolerance here
    EXPECT_EQ(sys_epoch, result);
}

TEST(libumTest, test_um_errorstr)
{
    for (int error_code = -10; error_code <= 0; error_code++)
    {
        const char *error_str = um_errorstr((const um_error)error_code);

        switch (error_code)
        {
        case LIBUM_NO_ERROR:
            EXPECT_STREQ("No error", error_str);
            break;
        case LIBUM_OS_ERROR:
            EXPECT_STREQ("Operation system error", error_str);
            break;
        case LIBUM_NOT_OPEN:
            EXPECT_STREQ("Not opened", error_str);
            break;
        case LIBUM_TIMEOUT:
            EXPECT_STREQ("Timeout", error_str);
            break;
        case LIBUM_INVALID_ARG:
            EXPECT_STREQ("Invalid argument", error_str);
            break;
        case LIBUM_INVALID_DEV:
            EXPECT_STREQ("Invalid device id", error_str);
            break;
        case LIBUM_INVALID_RESP:
            EXPECT_STREQ("Invalid response", error_str);
            break;
        default:
            EXPECT_STREQ("Unknown error", error_str);
            break;
        }
    }
}
