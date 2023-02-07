#include <gtest/gtest.h>
#include <libum.h>

TEST(libumTest, test_um_get_version) {
  const char * version = um_get_version();
  // Expect two strings not to be equal.
  EXPECT_STREQ("v1.400", version);
}

TEST(libumTest, um_open) {
  um_state * result = um_open("INVALID-IP", 100, 0);
  EXPECT_EQ(NULL, result);
  // TODO. Add more tests here
}

TEST(libumTest, test_um_get_timestamp_us) {
  struct timespec tms;

  timespec_get(&tms, TIME_UTC);

  unsigned long long sysEpoch = tms.tv_sec * 1000;
  sysEpoch += tms.tv_nsec/1000000;
  EXPECT_NE(sysEpoch, 0);

  unsigned long long result = um_get_timestamp_ms();
  // Note! We might expect occasional 1-2 us gap sometimes.
  // TODO add some tolerance here
  EXPECT_EQ(sysEpoch, result);
}
