#include <gtest/gtest.h>
#include <libum.h>

// Demonstrate some basic assertions.
TEST(libumTest, LibUmVersion) {
  const char * version = um_get_version();
  // Expect two strings not to be equal.
  EXPECT_STREQ("v1.400", version);
}
