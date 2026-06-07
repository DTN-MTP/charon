#include <stdio.h>
#define TEST_PASS() printf("  [PASS] %s\n", __func__)
#define TEST_FAIL(msg)                                                                             \
  do {                                                                                             \
    printf("  [FAIL] %s: %s\n", __func__, msg);                                                    \
    return 1;                                                                                      \
  } while (0)

#define ASSERT_EQ(a, b)                                                                            \
  do {                                                                                             \
    if ((a) != (b)) {                                                                              \
      printf("  [FAIL] %s: %s != %s at line %d\n", __func__, #a, #b, __LINE__);                    \
      return 1;                                                                                    \
    }                                                                                              \
  } while (0)

#define ASSERT_NE(a, b)                                                                            \
  do {                                                                                             \
    if ((a) == (b)) {                                                                              \
      printf("  [FAIL] %s: %s == %s at line %d\n", __func__, #a, #b, __LINE__);                    \
      return 1;                                                                                    \
    }                                                                                              \
  } while (0)

#define ASSERT_TRUE(cond)                                                                          \
  do {                                                                                             \
    if (!(cond)) {                                                                                 \
      printf("  [FAIL] %s: %s is false at line %d\n", __func__, #cond, __LINE__);                  \
      return 1;                                                                                    \
    }                                                                                              \
  } while (0)

