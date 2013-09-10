#ifndef PKCS11TEST_H
#define PKCS11TEST_H

// Master header for all PKCS#11 test code.

// Set up the environment for PKCS#11
#include "pkcs11-env.h"
// Include the official PKCS#11 header file.
#include <pkcs11.h>
// Test-wide global variables (specifically g_fns)
#include "globals.h"
// Utilities to convert PKCS#11 types to strings.
#include "pkcs11-describe.h"
// gTest header
#include "gtest/gtest.h"

#include <iostream>
#include <memory>

// Deleter for std::unique_ptr that handles C's malloc'ed memory.
struct freer {
  void operator()(void* p) { free(p); }
};

// Additional macro for checking the return value of a PKCS#11 function.
inline ::testing::AssertionResult IsCKR_OK(CK_RV rv) {
  if (rv == CKR_OK) {
    return testing::AssertionSuccess();
  } else {
    return testing::AssertionFailure() << rv_name(rv);
  }
}
#define EXPECT_CKR_OK(val) EXPECT_TRUE(IsCKR_OK(val))

// Test case that handles Initialize/Finalize
class PKCS11Test : public ::testing::Test {
 public:
  PKCS11Test() {
    // Null argument => only planning to use PKCS#11 from single thread.
    EXPECT_CKR_OK(g_fns->C_Initialize(NULL_PTR));
  }
  virtual ~PKCS11Test() {
    EXPECT_CKR_OK(g_fns->C_Finalize(NULL_PTR));
  }
};

// Test cases that handle session setup/teardown
class SessionTest : public PKCS11Test {
 public:
  SessionTest() {
    CK_SLOT_INFO slot_info;
    EXPECT_CKR_OK(g_fns->C_GetSlotInfo(g_slot_id, &slot_info));
    if (!(slot_info.flags & CKF_TOKEN_PRESENT)) {
      std::cerr << "Need to specify a slot ID that has a token present" << std::endl;
      exit(1);
    }
  }
  virtual ~SessionTest() {
    EXPECT_CKR_OK(g_fns->C_CloseSession(session_));
  }
  void Login(CK_USER_TYPE user_type, const char* pin) {
    CK_RV rv = g_fns->C_Login(session_, user_type, (CK_UTF8CHAR_PTR)pin, strlen(pin));
    if (rv != CKR_OK) {
      std::cerr << "Failed to login as user type " << user_type_name(user_type) << ", error " << rv_name(rv) << std::endl;
      exit(1);
    }
  }
 protected:
  CK_SESSION_HANDLE session_;
};

class ReadOnlySessionTest : public SessionTest {
 public:
  ReadOnlySessionTest() {
    CK_FLAGS flags = CKF_SERIAL_SESSION;
    EXPECT_CKR_OK(g_fns->C_OpenSession(g_slot_id, flags, NULL_PTR, NULL_PTR, &session_));
  }
};

class ReadWriteSessionTest : public SessionTest {
 public:
  ReadWriteSessionTest() {
    CK_FLAGS flags = CKF_SERIAL_SESSION | CKF_RW_SESSION;
    EXPECT_CKR_OK(g_fns->C_OpenSession(g_slot_id, flags, NULL_PTR, NULL_PTR, &session_));
  }
};

class ROUserSessionTest : public ReadOnlySessionTest {
 public:
  ROUserSessionTest() { Login(CKU_USER, g_user_pin); }
  virtual ~ROUserSessionTest() { EXPECT_CKR_OK(g_fns->C_Logout(session_)); }
};

class RWUserSessionTest : public ReadWriteSessionTest {
 public:
  RWUserSessionTest() { Login(CKU_USER, g_user_pin); }
  virtual ~RWUserSessionTest() { EXPECT_CKR_OK(g_fns->C_Logout(session_)); }
};

class RWSOSessionTest : public ReadWriteSessionTest {
 public:
  RWSOSessionTest() { Login(CKU_SO, g_so_pin); }
  virtual ~RWSOSessionTest() { EXPECT_CKR_OK(g_fns->C_Logout(session_)); }
};

#endif  // PKCS11TEST_H
