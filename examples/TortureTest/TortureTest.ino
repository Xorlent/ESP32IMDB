/*
 * ESP32IMDB - Comprehensive Torture Test
 * 
 * Copyright (c) 2026 Xorlent
 * Licensed under the MIT License.
 * https://github.com/Xorlent/ESP32IMDB
 * 
 * This test suite covers all library features:
 * - All data types (INT32, FLOAT, MAC, STRING, EPOCH, BOOL)
 * - All CRUD operations
 * - All aggregate functions
 * - Edge cases and boundary conditions
 * - Error handling
 * - Memory management
 * - TTL functionality
 * - Math operations
 * - Optionally, persistence (save/load to SPIFFS)
 * 
 */

// To enable persistence testing (save/load from SPIFFS), uncomment the line below:
//#define ENABLE_PERSISTENCE_TEST

#include <ESP32IMDB.h>
#ifdef ENABLE_PERSISTENCE_TEST
#include <SPIFFS.h>
#endif

ESP32IMDB db;
int testsPassed = 0;
int testsFailed = 0;

// Helper to print database state for debugging
void printDbState() {
  Serial.printf("      [DEBUG] DB State: %d records, %d bytes used, %u heap free\n", 
                db.getRecordCount(), db.getMemoryUsage(), ESP.getFreeHeap());
}

// Test result macros with enhanced debugging
#define TEST_ASSERT(condition, testName) \
  if (condition) { \
    Serial.printf("✓ PASS: %s\n", testName); \
    testsPassed++; \
  } else { \
    Serial.printf("✗ FAIL: %s\n", testName); \
    Serial.printf("      [Line %d] Condition failed: %s\n", __LINE__, #condition); \
    printDbState(); \
    testsFailed++; \
  }

#define TEST_ASSERT_EQUAL(expected, actual, testName) \
  if ((expected) == (actual)) { \
    Serial.printf("✓ PASS: %s\n", testName); \
    testsPassed++; \
  } else { \
    Serial.printf("✗ FAIL: %s\n", testName); \
    Serial.printf("      [Line %d] Expected: %d, Got: %d\n", __LINE__, (int)(expected), (int)(actual)); \
    printDbState(); \
    testsFailed++; \
  }

#define TEST_ASSERT_STR_EQUAL(expected, actual, testName) \
  if (strcmp(expected, actual) == 0) { \
    Serial.printf("✓ PASS: %s\n", testName); \
    testsPassed++; \
  } else { \
    Serial.printf("✗ FAIL: %s\n", testName); \
    Serial.printf("      [Line %d] Expected: \"%s\", Got: \"%s\"\n", __LINE__, expected, actual); \
    printDbState(); \
    testsFailed++; \
  }

void printSeparator() {
  Serial.println("\n" + String('-', 60));
}

void printTestResults() {
  printSeparator();
  Serial.printf("TOTAL: %d tests, %d passed, %d failed\n", 
                testsPassed + testsFailed, testsPassed, testsFailed);
  if (testsFailed == 0) {
    Serial.println("✓✓✓ ALL TESTS PASSED! ✓✓✓");
  } else {
    Serial.printf("✗✗✗ %d TEST(S) FAILED ✗✗✗\n", testsFailed);
  }
  printSeparator();
}

// Test 1: Basic table operations
void testTableOperations() {
  Serial.println("\n=== TEST 1: Table Operations ===");
  
  // Test creating a table
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}};
  TEST_ASSERT(db.createTable(cols, 1) == IMDB_OK, "Create table");
  
  // Test duplicate table creation
  TEST_ASSERT(db.createTable(cols, 1) == IMDB_ERROR_TABLE_EXISTS, "Reject duplicate table");
  
  // Test drop table
  TEST_ASSERT(db.dropTable() == IMDB_OK, "Drop table");
  
  // Test operations on non-existent table
  int32_t val = 1;
  const void* values[] = {&val};
  TEST_ASSERT(db.insert(values) == IMDB_ERROR_NO_TABLE, "Insert fails without table");
  TEST_ASSERT(db.count() == 0, "Count returns 0 without table");
  
  // Recreate for subsequent tests
  db.createTable(cols, 1);
  db.dropTable();
}

// Test 2: INT32 data type
void testInt32Type() {
  Serial.println("\n=== TEST 2: INT32 Data Type ===");
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"Value", IMDB_TYPE_INT32}};
  db.createTable(cols, 2);
  
  // Test positive, negative, zero, min, max values
  int32_t id1 = 1, val1 = 12345;
  const void* vals1[] = {&id1, &val1};
  TEST_ASSERT(db.insert(vals1) == IMDB_OK, "Insert positive INT32");
  
  int32_t id2 = 2, val2 = -54321;
  const void* vals2[] = {&id2, &val2};
  TEST_ASSERT(db.insert(vals2) == IMDB_OK, "Insert negative INT32");
  
  int32_t id3 = 3, val3 = 0;
  const void* vals3[] = {&id3, &val3};
  TEST_ASSERT(db.insert(vals3) == IMDB_OK, "Insert zero INT32");
  
  int32_t id4 = 4, val4 = 0x7FFFFFFF;  // Max int32
  const void* vals4[] = {&id4, &val4};
  TEST_ASSERT(db.insert(vals4) == IMDB_OK, "Insert max INT32");
  
  int32_t id5 = 5, val5 = (int32_t)0x80000000;  // Min int32
  const void* vals5[] = {&id5, &val5};
  TEST_ASSERT(db.insert(vals5) == IMDB_OK, "Insert min INT32");
  
  // Test select and verify values
  IMDBSelectResult result;
  db.select("Value", "ID", &id1, &result);
  TEST_ASSERT(result.hasValue && result.int32Value == 12345, "Select positive INT32");
  
  db.select("Value", "ID", &id2, &result);
  TEST_ASSERT(result.hasValue && result.int32Value == -54321, "Select negative INT32");
  
  db.select("Value", "ID", &id3, &result);
  TEST_ASSERT(result.hasValue && result.int32Value == 0, "Select zero INT32");
  
  db.select("Value", "ID", &id4, &result);
  TEST_ASSERT(result.hasValue && result.int32Value == 0x7FFFFFFF, "Select max INT32");
  
  db.select("Value", "ID", &id5, &result);
  TEST_ASSERT(result.hasValue && result.int32Value == (int32_t)0x80000000, "Select min INT32");
  
  // Test min/max functions
  db.min("Value", &result);
  TEST_ASSERT(result.int32Value == (int32_t)0x80000000, "Min INT32 value");
  
  db.max("Value", &result);
  TEST_ASSERT(result.int32Value == 0x7FFFFFFF, "Max INT32 value");
  
  TEST_ASSERT(db.count() == 5, "Count all INT32 records");
  
  db.dropTable();
}

// Test 3: STRING data type
void testStringType() {
  Serial.println("\n=== TEST 3: STRING Data Type ===");
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"Name", IMDB_TYPE_STRING}};
  db.createTable(cols, 2);
  
  // Test normal strings
  int32_t id1 = 1;
  const char* name1 = "Alice";
  const void* vals1[] = {&id1, &name1};
  TEST_ASSERT(db.insert(vals1) == IMDB_OK, "Insert normal string");
  
  // Test empty string
  int32_t id2 = 2;
  const char* name2 = "";
  const void* vals2[] = {&id2, &name2};
  TEST_ASSERT(db.insert(vals2) == IMDB_OK, "Insert empty string");
  
  // Test string with spaces
  int32_t id3 = 3;
  const char* name3 = "John Doe Jr.";
  const void* vals3[] = {&id3, &name3};
  TEST_ASSERT(db.insert(vals3) == IMDB_OK, "Insert string with spaces");
  
  // Test string with special characters
  int32_t id4 = 4;
  const char* name4 = "Test@#$%^&*()_+-=[]{}|;':\",./<>?";
  const void* vals4[] = {&id4, &name4};
  TEST_ASSERT(db.insert(vals4) == IMDB_OK, "Insert special characters");
  
  // Test maximum length string (255 chars)
  int32_t id5 = 5;
  char maxStr[256];
  memset(maxStr, 'X', 255);
  maxStr[255] = '\0';
  const char* name5 = maxStr;
  const void* vals5[] = {&id5, &name5};
  TEST_ASSERT(db.insert(vals5) == IMDB_OK, "Insert max length string (255)");
  
  // Test over-length string (should truncate to 255)
  int32_t id6 = 6;
  char overStr[300];
  memset(overStr, 'Y', 299);
  overStr[299] = '\0';
  const char* name6 = overStr;
  const void* vals6[] = {&id6, &name6};
  TEST_ASSERT(db.insert(vals6) == IMDB_OK, "Insert over-length string (truncates)");
  
  // Verify string retrieval
  IMDBSelectResult result;
  IMDBResult res = db.select("Name", "ID", &id1, &result);
  if (res != IMDB_OK) Serial.printf("      [DEBUG] Select failed with: %s\n", ESP32IMDB::resultToString(res));
  TEST_ASSERT_STR_EQUAL("Alice", result.stringValue, "Select normal string");
  
  res = db.select("Name", "ID", &id2, &result);
  if (res != IMDB_OK) Serial.printf("      [DEBUG] Select failed with: %s\n", ESP32IMDB::resultToString(res));
  TEST_ASSERT_STR_EQUAL("", result.stringValue, "Select empty string");
  
  res = db.select("Name", "ID", &id3, &result);
  if (res != IMDB_OK) Serial.printf("      [DEBUG] Select failed with: %s\n", ESP32IMDB::resultToString(res));
  TEST_ASSERT_STR_EQUAL("John Doe Jr.", result.stringValue, "Select string with spaces");
  
  res = db.select("Name", "ID", &id4, &result);
  if (res != IMDB_OK) Serial.printf("      [DEBUG] Select failed with: %s\n", ESP32IMDB::resultToString(res));
  TEST_ASSERT_STR_EQUAL("Test@#$%^&*()_+-=[]{}|;':\",./<>?", result.stringValue, "Select special characters");
  
  db.select("Name", "ID", &id5, &result);
  TEST_ASSERT(result.hasValue && strlen(result.stringValue) == 255, "Select max length string");
  
  db.select("Name", "ID", &id6, &result);
  TEST_ASSERT(result.hasValue && strlen(result.stringValue) == 255, "Verify truncated string is 255");
  
  // Test string update
  const char* newName = "Bob";
  res = db.update("ID", &id1, "Name", &newName);
  if (res != IMDB_OK) Serial.printf("      [DEBUG] Update failed with: %s\n", ESP32IMDB::resultToString(res));
  TEST_ASSERT(res == IMDB_OK, "Update string value");
  res = db.select("Name", "ID", &id1, &result);
  if (res != IMDB_OK) Serial.printf("      [DEBUG] Select after update failed with: %s\n", ESP32IMDB::resultToString(res));
  TEST_ASSERT_STR_EQUAL("Bob", result.stringValue, "Verify updated string");
  
  db.dropTable();
}

// Test 4: MAC address data type
void testMacType() {
  Serial.println("\n=== TEST 4: MAC Address Data Type ===");
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"MAC", IMDB_TYPE_MAC}};
  db.createTable(cols, 2);
  
  // Test normal MAC address
  int32_t id1 = 1;
  uint8_t mac1[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
  const void* vals1[] = {&id1, mac1};
  TEST_ASSERT(db.insert(vals1) == IMDB_OK, "Insert MAC address");
  
  // Test all zeros
  int32_t id2 = 2;
  uint8_t mac2[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  const void* vals2[] = {&id2, mac2};
  TEST_ASSERT(db.insert(vals2) == IMDB_OK, "Insert zero MAC");
  
  // Test all ones
  int32_t id3 = 3;
  uint8_t mac3[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  const void* vals3[] = {&id3, mac3};
  TEST_ASSERT(db.insert(vals3) == IMDB_OK, "Insert broadcast MAC");
  
  // Verify MAC retrieval
  IMDBSelectResult result;
  db.select("MAC", "ID", &id1, &result);
  bool macMatch = result.hasValue && 
                  memcmp(result.macAddress, mac1, 6) == 0;
  TEST_ASSERT(macMatch, "Select MAC address");
  
  // Test MAC address parsing (12-char format)
  uint8_t parsedMac[6];
  TEST_ASSERT(ESP32IMDB::parseMacAddress("aabbccddeeff", parsedMac), "Parse MAC (12 chars)");
  TEST_ASSERT(memcmp(parsedMac, mac1, 6) == 0, "Verify parsed MAC");
  
  // Test MAC address parsing (colon format)
  TEST_ASSERT(ESP32IMDB::parseMacAddress("AA:BB:CC:DD:EE:FF", parsedMac), "Parse MAC (colon)");
  TEST_ASSERT(memcmp(parsedMac, mac1, 6) == 0, "Verify colon MAC");
  
  // Test MAC address parsing (dash format)
  TEST_ASSERT(ESP32IMDB::parseMacAddress("AA-BB-CC-DD-EE-FF", parsedMac), "Parse MAC (dash)");
  TEST_ASSERT(memcmp(parsedMac, mac1, 6) == 0, "Verify dash MAC");
  
  // Test MAC address formatting
  char macStr[18];
  ESP32IMDB::formatMacAddress(mac1, macStr);
  TEST_ASSERT(strcmp(macStr, "aa:bb:cc:dd:ee:ff") == 0, "Format MAC address");
  
  // Test invalid MAC parsing
  TEST_ASSERT(!ESP32IMDB::parseMacAddress("invalid", parsedMac), "Reject invalid MAC");
  TEST_ASSERT(!ESP32IMDB::parseMacAddress("GG:HH:II:JJ:KK:LL", parsedMac), "Reject non-hex MAC");
  
  db.dropTable();
}

// Test 5: EPOCH data type
void testEpochType() {
  Serial.println("\n=== TEST 5: EPOCH Data Type ===");
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"Timestamp", IMDB_TYPE_EPOCH}};
  db.createTable(cols, 2);
  
  // Test various timestamps
  int32_t id1 = 1;
  uint32_t epoch1 = 0;  // Unix epoch start
  const void* vals1[] = {&id1, &epoch1};
  TEST_ASSERT(db.insert(vals1) == IMDB_OK, "Insert epoch 0");
  
  int32_t id2 = 2;
  uint32_t epoch2 = 1609459200;  // 2021-01-01 00:00:00
  const void* vals2[] = {&id2, &epoch2};
  TEST_ASSERT(db.insert(vals2) == IMDB_OK, "Insert normal epoch");
  
  int32_t id3 = 3;
  uint32_t epoch3 = 2147483647;  // Max 32-bit epoch (2038-01-19)
  const void* vals3[] = {&id3, &epoch3};
  TEST_ASSERT(db.insert(vals3) == IMDB_OK, "Insert max epoch");
  
  // Verify epoch retrieval
  IMDBSelectResult result;
  db.select("Timestamp", "ID", &id1, &result);
  TEST_ASSERT(result.hasValue && result.epochValue == 0, "Select epoch 0");
  
  db.select("Timestamp", "ID", &id2, &result);
  TEST_ASSERT(result.hasValue && result.epochValue == 1609459200, "Select normal epoch");
  
  db.select("Timestamp", "ID", &id3, &result);
  TEST_ASSERT(result.hasValue && result.epochValue == 2147483647, "Select max epoch");
  
  // Test min/max on epochs
  db.min("Timestamp", &result);
  TEST_ASSERT(result.epochValue == 0, "Min epoch value");
  
  db.max("Timestamp", &result);
  TEST_ASSERT(result.epochValue == 2147483647, "Max epoch value");
  
  db.dropTable();
}

// Test 6: BOOL data type
void testBoolType() {
  Serial.println("\n=== TEST 6: BOOL Data Type ===");
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"Active", IMDB_TYPE_BOOL}};
  db.createTable(cols, 2);
  
  // Test true value
  int32_t id1 = 1;
  bool active1 = true;
  const void* vals1[] = {&id1, &active1};
  TEST_ASSERT(db.insert(vals1) == IMDB_OK, "Insert bool true");
  
  // Test false value
  int32_t id2 = 2;
  bool active2 = false;
  const void* vals2[] = {&id2, &active2};
  TEST_ASSERT(db.insert(vals2) == IMDB_OK, "Insert bool false");
  
  // Verify bool retrieval
  IMDBSelectResult result;
  db.select("Active", "ID", &id1, &result);
  TEST_ASSERT(result.hasValue && result.boolValue == true, "Select bool true");
  
  db.select("Active", "ID", &id2, &result);
  TEST_ASSERT(result.hasValue && result.boolValue == false, "Select bool false");
  
  // Test bool update
  bool newActive = false;
  TEST_ASSERT(db.update("ID", &id1, "Active", &newActive) == IMDB_OK, "Update bool");
  db.select("Active", "ID", &id1, &result);
  TEST_ASSERT(result.boolValue == false, "Verify updated bool");
  
  db.dropTable();
}

// Test 7: FLOAT data type
void testFloatType() {
  Serial.println("\n=== TEST 7: FLOAT Data Type ===");
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"Value", IMDB_TYPE_FLOAT}};
  db.createTable(cols, 2);
  
  // Test positive, negative, zero values
  int32_t id1 = 1;
  float val1 = 3.14159;
  const void* vals1[] = {&id1, &val1};
  TEST_ASSERT(db.insert(vals1) == IMDB_OK, "Insert positive float");
  
  int32_t id2 = 2;
  float val2 = -273.15;
  const void* vals2[] = {&id2, &val2};
  TEST_ASSERT(db.insert(vals2) == IMDB_OK, "Insert negative float");
  
  int32_t id3 = 3;
  float val3 = 0.0;
  const void* vals3[] = {&id3, &val3};
  TEST_ASSERT(db.insert(vals3) == IMDB_OK, "Insert zero float");
  
  // Test very small and very large values
  int32_t id4 = 4;
  float val4 = 1.23e-10;  // Very small
  const void* vals4[] = {&id4, &val4};
  TEST_ASSERT(db.insert(vals4) == IMDB_OK, "Insert very small float");
  
  int32_t id5 = 5;
  float val5 = 1.23e10;  // Very large
  const void* vals5[] = {&id5, &val5};
  TEST_ASSERT(db.insert(vals5) == IMDB_OK, "Insert very large float");
  
  int32_t id6 = 6;
  float val6 = 3.4028235e38;  // Near max float
  const void* vals6[] = {&id6, &val6};
  TEST_ASSERT(db.insert(vals6) == IMDB_OK, "Insert max float");
  
  int32_t id7 = 7;
  float val7 = -3.4028235e38;  // Near min float
  const void* vals7[] = {&id7, &val7};
  TEST_ASSERT(db.insert(vals7) == IMDB_OK, "Insert min float");
  
  // Test select and verify values (with small tolerance for floating point comparison)
  IMDBSelectResult result;
  db.select("Value", "ID", &id1, &result);
  TEST_ASSERT(result.hasValue && fabs(result.floatValue - 3.14159) < 0.00001, "Select positive float");
  
  db.select("Value", "ID", &id2, &result);
  TEST_ASSERT(result.hasValue && fabs(result.floatValue - (-273.15)) < 0.01, "Select negative float");
  
  db.select("Value", "ID", &id3, &result);
  TEST_ASSERT(result.hasValue && result.floatValue == 0.0, "Select zero float");
  
  db.select("Value", "ID", &id4, &result);
  TEST_ASSERT(result.hasValue && result.floatValue > 0.0 && result.floatValue < 1.0e-9, "Select very small float");
  
  db.select("Value", "ID", &id5, &result);
  TEST_ASSERT(result.hasValue && result.floatValue > 1.0e9, "Select very large float");
  
  // Test min/max functions on floats
  db.min("Value", &result);
  TEST_ASSERT(result.floatValue < -1.0e37, "Min float value");
  
  db.max("Value", &result);
  TEST_ASSERT(result.floatValue > 1.0e37, "Max float value");
  
  // Test float update
  float newVal = 2.71828;
  TEST_ASSERT(db.update("ID", &id1, "Value", &newVal) == IMDB_OK, "Update float value");
  db.select("Value", "ID", &id1, &result);
  TEST_ASSERT(fabs(result.floatValue - 2.71828) < 0.00001, "Verify updated float");
  
  TEST_ASSERT(db.count() == 7, "Count all float records");
  
  db.dropTable();
}

// Test 8: Float math operations
void testFloatMathOperations() {
  Serial.println("\n=== TEST 8: Float Math Operations ===");
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"Value", IMDB_TYPE_FLOAT}};
  db.createTable(cols, 2);
  
  int32_t id = 1;
  float val = 100.5;
  const void* vals[] = {&id, &val};
  db.insert(vals);
  
  IMDBSelectResult result;
  
  // Test addition
  TEST_ASSERT(db.updateWithMath("ID", &id, "Value", IMDB_MATH_ADD, 50) == IMDB_OK, "Float Math ADD");
  db.select("Value", "ID", &id, &result);
  TEST_ASSERT(fabs(result.floatValue - 150.5) < 0.1, "Verify float ADD result");
  
  // Test subtraction
  TEST_ASSERT(db.updateWithMath("ID", &id, "Value", IMDB_MATH_SUBTRACT, 30) == IMDB_OK, "Float Math SUBTRACT");
  db.select("Value", "ID", &id, &result);
  TEST_ASSERT(fabs(result.floatValue - 120.5) < 0.1, "Verify float SUBTRACT result");
  
  // Test multiplication
  TEST_ASSERT(db.updateWithMath("ID", &id, "Value", IMDB_MATH_MULTIPLY, 2) == IMDB_OK, "Float Math MULTIPLY");
  db.select("Value", "ID", &id, &result);
  TEST_ASSERT(fabs(result.floatValue - 241.0) < 0.1, "Verify float MULTIPLY result");
  
  // Test division
  TEST_ASSERT(db.updateWithMath("ID", &id, "Value", IMDB_MATH_DIVIDE, 4) == IMDB_OK, "Float Math DIVIDE");
  db.select("Value", "ID", &id, &result);
  TEST_ASSERT(fabs(result.floatValue - 60.25) < 0.1, "Verify float DIVIDE result");
  
  // Test modulo on float
  TEST_ASSERT(db.updateWithMath("ID", &id, "Value", IMDB_MATH_MODULO, 7) == IMDB_OK, "Float Math MODULO");
  db.select("Value", "ID", &id, &result);
  // Result should be around 4.25 (60.25 % 7)
  TEST_ASSERT(result.floatValue >= 0.0 && result.floatValue < 7.0, "Verify float MODULO result");
  
  // Test divide by zero on float
  TEST_ASSERT(db.updateWithMath("ID", &id, "Value", IMDB_MATH_DIVIDE, 0) == IMDB_ERROR_INVALID_OPERATION, 
              "Float reject divide by zero");
  
  // Test modulo by zero on float
  TEST_ASSERT(db.updateWithMath("ID", &id, "Value", IMDB_MATH_MODULO, 0) == IMDB_ERROR_INVALID_OPERATION, 
              "Float reject modulo by zero");
  
  db.dropTable();
}

// Test 9: Math operations
void testMathOperations() {
  Serial.println("\n=== TEST 9: Math Operations ===");
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"Value", IMDB_TYPE_INT32}};
  db.createTable(cols, 2);
  
  int32_t id = 1, val = 100;
  const void* vals[] = {&id, &val};
  db.insert(vals);
  
  IMDBSelectResult result;
  
  // Test addition
  TEST_ASSERT(db.updateWithMath("ID", &id, "Value", IMDB_MATH_ADD, 50) == IMDB_OK, "Math ADD");
  db.select("Value", "ID", &id, &result);
  TEST_ASSERT(result.int32Value == 150, "Verify ADD result");
  
  // Test subtraction
  TEST_ASSERT(db.updateWithMath("ID", &id, "Value", IMDB_MATH_SUBTRACT, 30) == IMDB_OK, "Math SUBTRACT");
  db.select("Value", "ID", &id, &result);
  TEST_ASSERT(result.int32Value == 120, "Verify SUBTRACT result");
  
  // Test multiplication
  TEST_ASSERT(db.updateWithMath("ID", &id, "Value", IMDB_MATH_MULTIPLY, 2) == IMDB_OK, "Math MULTIPLY");
  db.select("Value", "ID", &id, &result);
  TEST_ASSERT(result.int32Value == 240, "Verify MULTIPLY result");
  
  // Test division
  TEST_ASSERT(db.updateWithMath("ID", &id, "Value", IMDB_MATH_DIVIDE, 4) == IMDB_OK, "Math DIVIDE");
  db.select("Value", "ID", &id, &result);
  TEST_ASSERT(result.int32Value == 60, "Verify DIVIDE result");
  
  // Test modulo
  TEST_ASSERT(db.updateWithMath("ID", &id, "Value", IMDB_MATH_MODULO, 7) == IMDB_OK, "Math MODULO");
  db.select("Value", "ID", &id, &result);
  TEST_ASSERT(result.int32Value == 4, "Verify MODULO result");
  
  // Test divide by zero
  TEST_ASSERT(db.updateWithMath("ID", &id, "Value", IMDB_MATH_DIVIDE, 0) == IMDB_ERROR_INVALID_OPERATION, 
              "Reject divide by zero");
  
  // Test modulo by zero
  TEST_ASSERT(db.updateWithMath("ID", &id, "Value", IMDB_MATH_MODULO, 0) == IMDB_ERROR_INVALID_OPERATION, 
              "Reject modulo by zero");
  
  db.dropTable();
}

// Test 10: TTL (Time-To-Live)
void testTTL() {
  Serial.println("\n=== TEST 10: TTL Functionality ===");;
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}};
  db.createTable(cols, 1);
  
  // Insert record with 2 second TTL
  int32_t id1 = 1;
  const void* vals1[] = {&id1};
  TEST_ASSERT(db.insert(vals1, 2000) == IMDB_OK, "Insert with 2s TTL");
  
  // Insert permanent record
  int32_t id2 = 2;
  const void* vals2[] = {&id2};
  TEST_ASSERT(db.insert(vals2) == IMDB_OK, "Insert without TTL");
  
  // Verify both exist
  TEST_ASSERT(db.count() == 2, "Count before expiry");
  
  // Wait for expiry
  Serial.println("   Waiting 3 seconds for TTL expiry...");
  delay(3000);
  db.purgeExpiredRecords();
  
  TEST_ASSERT(db.count() == 1, "Count after expiry");
  
  // Verify correct record expired
  IMDBSelectResult result;
  TEST_ASSERT(db.select("ID", "ID", &id1, &result) == IMDB_ERROR_NO_RECORDS, "TTL record expired");
  TEST_ASSERT(db.select("ID", "ID", &id2, &result) == IMDB_OK, "Permanent record exists");
  
  // Test max TTL validation
  int32_t id3 = 3;
  const void* vals3[] = {&id3};
  uint32_t maxTTL = 30UL * 24UL * 60UL * 60UL * 1000UL + 1;  // Exceed 30 days
  TEST_ASSERT(db.insert(vals3, maxTTL) == IMDB_ERROR_INVALID_VALUE, "Reject excessive TTL");
  
  db.dropTable();
}

// Test 11: Update and Delete operations
void testUpdateDelete() {
  Serial.println("\n=== TEST 11: Update and Delete Operations ===");;
  
  IMDBColumn cols[] = {
    {"ID", IMDB_TYPE_INT32}, 
    {"Name", IMDB_TYPE_STRING}, 
    {"Age", IMDB_TYPE_INT32}
  };
  db.createTable(cols, 3);
  
  // Insert test records
  int32_t id1 = 1;
  const char* name1 = "Alice";
  int32_t age1 = 25;
  const void* vals1[] = {&id1, &name1, &age1};
  db.insert(vals1);
  
  int32_t id2 = 2;
  const char* name2 = "Bob";
  int32_t age2 = 30;
  const void* vals2[] = {&id2, &name2, &age2};
  db.insert(vals2);
  
  // Test update
  int32_t newAge = 26;
  TEST_ASSERT(db.update("ID", &id1, "Age", &newAge) == IMDB_OK, "Update existing record");
  
  IMDBSelectResult result;
  db.select("Age", "ID", &id1, &result);
  TEST_ASSERT(result.int32Value == 26, "Verify update");
  
  // Test update non-existent record
  int32_t id3 = 999;
  TEST_ASSERT(db.update("ID", &id3, "Age", &newAge) == IMDB_ERROR_NO_RECORDS, 
              "Update non-existent record");
  
  // Test delete
  TEST_ASSERT(db.deleteRecords("ID", &id1) == IMDB_OK, "Delete existing record");
  TEST_ASSERT(db.count() == 1, "Count after delete");
  TEST_ASSERT(db.select("Name", "ID", &id1, &result) == IMDB_ERROR_NO_RECORDS, 
              "Verify record deleted");
  
  // Test delete non-existent
  TEST_ASSERT(db.deleteRecords("ID", &id3) == IMDB_ERROR_NO_RECORDS, 
              "Delete non-existent record");
  
  db.dropTable();
}

// Test 12: Query operations
void testQueryOperations() {
  Serial.println("\n=== TEST 12: Query Operations ===");;
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"Value", IMDB_TYPE_INT32}};
  db.createTable(cols, 2);
  
  // Insert test data
  for (int i = 1; i <= 10; i++) {
    int32_t id = i;
    int32_t val = i * 10;
    const void* vals[] = {&id, &val};
    db.insert(vals);
  }
  
  // Test count
  TEST_ASSERT(db.count() == 10, "Count all records");
  
  // Test countWhere
  int32_t searchId = 5;
  TEST_ASSERT(db.countWhere("ID", &searchId) == 1, "CountWhere existing");
  
  int32_t badId = 999;
  TEST_ASSERT(db.countWhere("ID", &badId) == 0, "CountWhere non-existent");
  
  // Test min/max
  IMDBSelectResult result;
  db.min("Value", &result);
  TEST_ASSERT(result.int32Value == 10, "Min value");
  
  db.max("Value", &result);
  TEST_ASSERT(result.int32Value == 100, "Max value");
  
  // Test top N
  IMDBSelectResult* topResults;
  int topCount;
  TEST_ASSERT(db.top(5, &topResults, &topCount) == IMDB_OK, "Get top 5");
  TEST_ASSERT(topCount == 5, "Top returns correct count");
  free(topResults);
  
  // Test top N exceeding record count
  TEST_ASSERT(db.top(20, &topResults, &topCount) == IMDB_OK, "Get top 20 (only 10 exist)");
  TEST_ASSERT(topCount == 10, "Top returns available records");
  free(topResults);
  
  // Test selectAll
  int32_t findId = 3;
  TEST_ASSERT(db.selectAll("ID", &findId, &topResults, &topCount) == IMDB_OK, "SelectAll existing");
  TEST_ASSERT(topCount == 1, "SelectAll count");
  TEST_ASSERT(topResults[0].int32Value == 3, "SelectAll value");
  free(topResults);
  
  // Test selectAll non-existent
  TEST_ASSERT(db.selectAll("ID", &badId, &topResults, &topCount) == IMDB_ERROR_NO_RECORDS, 
              "SelectAll non-existent");
  
  db.dropTable();
}

// Test 13: Error conditions
void testErrorConditions() {
  Serial.println("\n=== TEST 13: Error Conditions ===");;
  
  // Test operations without table
  IMDBSelectResult result;
  int32_t val = 1;
  TEST_ASSERT(db.select("ID", "ID", &val, &result) == IMDB_ERROR_NO_TABLE, 
              "Select without table");
  TEST_ASSERT(db.min("ID", &result) == IMDB_ERROR_NO_TABLE, "Min without table");
  TEST_ASSERT(db.max("ID", &result) == IMDB_ERROR_NO_TABLE, "Max without table");
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"Name", IMDB_TYPE_STRING}};
  db.createTable(cols, 2);
  
  // Test invalid column names
  TEST_ASSERT(db.select("InvalidCol", "ID", &val, &result) == IMDB_ERROR_COLUMN_NOT_FOUND, 
              "Select invalid column");
  
  int32_t newVal = 5;
  TEST_ASSERT(db.update("BadCol", &val, "ID", &newVal) == IMDB_ERROR_COLUMN_NOT_FOUND, 
              "Update invalid where column");
  TEST_ASSERT(db.update("ID", &val, "BadCol", &newVal) == IMDB_ERROR_COLUMN_NOT_FOUND, 
              "Update invalid set column");
  
  TEST_ASSERT(db.deleteRecords("BadCol", &val) == IMDB_ERROR_COLUMN_NOT_FOUND, 
              "Delete invalid column");
  
  // Test math on non-numeric column
  TEST_ASSERT(db.updateWithMath("ID", &val, "Name", IMDB_MATH_ADD, 1) == IMDB_ERROR_INVALID_TYPE, 
              "Math on string column");
  
  // Test min/max on non-numeric column
  TEST_ASSERT(db.min("Name", &result) == IMDB_ERROR_INVALID_TYPE, "Min on string column");
  TEST_ASSERT(db.max("Name", &result) == IMDB_ERROR_INVALID_TYPE, "Max on string column");
  
  // Test operations on empty table
  TEST_ASSERT(db.select("ID", "ID", &val, &result) == IMDB_ERROR_NO_RECORDS, 
              "Select from empty table");
  TEST_ASSERT(db.update("ID", &val, "ID", &newVal) == IMDB_ERROR_NO_RECORDS, 
              "Update empty table");
  TEST_ASSERT(db.deleteRecords("ID", &val) == IMDB_ERROR_NO_RECORDS, 
              "Delete from empty table");
  TEST_ASSERT(db.min("ID", &result) == IMDB_ERROR_NO_RECORDS, "Min on empty table");
  TEST_ASSERT(db.max("ID", &result) == IMDB_ERROR_NO_RECORDS, "Max on empty table");
  
  db.dropTable();
}

// Test 14: Multi-column operations
void testMultiColumn() {
  Serial.println("\n=== TEST 14: Multi-Column Operations ===");
  
  IMDBColumn cols[] = {
    {"ID", IMDB_TYPE_INT32},
    {"Name", IMDB_TYPE_STRING},
    {"MAC", IMDB_TYPE_MAC},
    {"Timestamp", IMDB_TYPE_EPOCH},
    {"Active", IMDB_TYPE_BOOL}
  };
  db.createTable(cols, 5);
  
  // Insert complex record
  int32_t id = 1;
  const char* name = "Device1";
  uint8_t mac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  uint32_t timestamp = 1609459200;
  bool active = true;
  const void* vals[] = {&id, &name, mac, &timestamp, &active};
  
  TEST_ASSERT(db.insert(vals) == IMDB_OK, "Insert multi-type record");
  
  // Verify all columns
  IMDBSelectResult result;
  db.select("ID", "ID", &id, &result);
  TEST_ASSERT(result.int32Value == 1, "Verify ID column");
  
  db.select("Name", "ID", &id, &result);
  TEST_ASSERT(strcmp(result.stringValue, "Device1") == 0, "Verify Name column");
  
  db.select("MAC", "ID", &id, &result);
  TEST_ASSERT(memcmp(result.macAddress, mac, 6) == 0, "Verify MAC column");
  
  db.select("Timestamp", "ID", &id, &result);
  TEST_ASSERT(result.epochValue == 1609459200, "Verify Timestamp column");
  
  db.select("Active", "ID", &id, &result);
  TEST_ASSERT(result.boolValue == true, "Verify Active column");
  
  // Test selectAll with all columns
  IMDBSelectResult* allResults;
  int count;
  db.selectAll("ID", &id, &allResults, &count);
  TEST_ASSERT(count == 1, "SelectAll multi-column count");
  TEST_ASSERT(allResults[0].int32Value == 1, "SelectAll column 0");
  TEST_ASSERT(strcmp(allResults[1].stringValue, "Device1") == 0, "SelectAll column 1");
  TEST_ASSERT(memcmp(allResults[2].macAddress, mac, 6) == 0, "SelectAll column 2");
  TEST_ASSERT(allResults[3].epochValue == 1609459200, "SelectAll column 3");
  TEST_ASSERT(allResults[4].boolValue == true, "SelectAll column 4");
  free(allResults);
  
  db.dropTable();
}

// Test 16: Memory management
void testMemoryManagement() {
  Serial.println("\n=== TEST 16: Memory Management ===");;
  
  uint32_t heapBefore = ESP.getFreeHeap();
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"Name", IMDB_TYPE_STRING}};
  db.createTable(cols, 2);
  
  // Insert many records
  for (int i = 0; i < 100; i++) {
    int32_t id = i;
    const char* name = "TestUser";
    const void* vals[] = {&id, &name};
    db.insert(vals);
  }
  
  size_t dbMemory = db.getMemoryUsage();
  Serial.printf("   Database using %d bytes for 100 records\n", dbMemory);
  TEST_ASSERT(dbMemory > 0, "Memory usage reported");
  TEST_ASSERT(db.count() == 100, "All records inserted");
  
  // Delete half the records
  for (int i = 0; i < 50; i++) {
    int32_t id = i;
    db.deleteRecords("ID", &id);
  }
  
  TEST_ASSERT(db.count() == 50, "Half records deleted");
  
  // Drop table and check heap recovery
  db.dropTable();
  
  uint32_t heapAfter = ESP.getFreeHeap();
  int32_t heapDiff = heapAfter - heapBefore;
  Serial.printf("   Heap before: %u, after: %u, diff: %d\n", heapBefore, heapAfter, heapDiff);
  
  // Heap should be close to original (within 1KB tolerance for measurement variations)
  TEST_ASSERT(abs(heapDiff) < 1024, "Heap recovered after dropTable");
}

// Test 15: Stress test
void testStressTest() {
  Serial.println("\n=== TEST 15: Stress Test ===");;
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"Data", IMDB_TYPE_STRING}};
  db.createTable(cols, 2);
  
  // Rapid insertions
  Serial.println("   Inserting 500 records...");
  int insertFailures = 0;
  for (int i = 0; i < 500; i++) {
    int32_t id = i;
    char data[50];
    sprintf(data, "Record_%d", i);
    const char* dataPtr = data;
    const void* vals[] = {&id, &dataPtr};
    IMDBResult res = db.insert(vals);
    if (res != IMDB_OK) {
      Serial.printf("      [DEBUG] Insert failed at record %d: %s (heap: %u)\n", 
                    i, ESP32IMDB::resultToString(res), ESP.getFreeHeap());
      insertFailures++;
      if (insertFailures >= 5) {
        Serial.println("      [DEBUG] Too many failures, stopping insertions");
        break;
      }
    }
  }
  
  int32_t finalCount = db.count();
  Serial.printf("   Successfully inserted %d records\n", finalCount);
  TEST_ASSERT(finalCount > 0, "Stress test insertions");
  
  // Rapid queries
  Serial.println("   Performing 1000 queries...");
  int successfulQueries = 0;
  int queryFailures = 0;
  IMDBSelectResult result;
  for (int i = 0; i < 1000; i++) {
    int32_t searchId = i % finalCount;
    IMDBResult res = db.select("Data", "ID", &searchId, &result);
    if (res == IMDB_OK) {
      successfulQueries++;
    } else if (queryFailures < 5) {
      Serial.printf("      [DEBUG] Query %d failed (searchId=%d): %s\n", 
                    i, searchId, ESP32IMDB::resultToString(res));
      queryFailures++;
    }
  }
  Serial.printf("   Successful queries: %d/1000\n", successfulQueries);
  if (successfulQueries != 1000) {
    Serial.printf("      [DEBUG] %d queries failed\n", 1000 - successfulQueries);
  }
  TEST_ASSERT(successfulQueries == 1000, "Stress test queries");
  
  db.dropTable();
}

#ifdef ENABLE_PERSISTENCE_TEST
// Test 17: Persistence (save/load to SPIFFS)
void testPersistence() {
  Serial.println("\n=== TEST 17: Persistence (SPIFFS Save/Load) ===");
  Serial.println("   Note: 'SPIFFS: mount failed' is normal on first run, as we are formatting the partition. Subsequent runs should show a successful mount.");
  
  // Initialize SPIFFS
  // Note: SPIFFS.begin(true) will format on first run if needed
  // Any "mount failed" error logged is normal and expected
  if (!SPIFFS.begin(true)) {
    Serial.println("   ✗ SPIFFS mount failed - skipping persistence tests");
    testsFailed++;
    return;
  }
  Serial.println("   ✓ SPIFFS mounted (auto-formatted if needed)");
  
  const char* testFile = "/test_torture.imdb";
  
  // Clean up any existing test file
  if (SPIFFS.exists(testFile)) {
    SPIFFS.remove(testFile);
  }
  
  // Test 1: Save without table
  TEST_ASSERT(db.saveToFile(testFile) == IMDB_ERROR_NO_TABLE, "Save without table");
  
  // Test 2: Create table and save empty
  IMDBColumn cols[] = {
    {"ID", IMDB_TYPE_INT32},
    {"Name", IMDB_TYPE_STRING},
    {"Value", IMDB_TYPE_FLOAT},
    {"MAC", IMDB_TYPE_MAC},
    {"Time", IMDB_TYPE_EPOCH},
    {"Active", IMDB_TYPE_BOOL}
  };
  db.createTable(cols, 6);
  TEST_ASSERT(db.saveToFile(testFile) == IMDB_OK, "Save empty table");
  
  // Test 3: Load empty table (should fail - table already exists)
  TEST_ASSERT(db.loadFromFile(testFile) == IMDB_ERROR_TABLE_EXISTS, "Load over existing table");
  
  db.dropTable();
  
  // Test 4: Load empty table into clean database
  TEST_ASSERT(db.loadFromFile(testFile) == IMDB_OK, "Load empty table");
  TEST_ASSERT(db.count() == 0, "Loaded empty table has 0 records");
  
  // Test 5: Insert data and save
  int32_t id1 = 1;
  const char* name1 = "TestDevice";
  float value1 = 123.45;
  uint8_t mac1[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
  uint32_t time1 = 1609459200;
  bool active1 = true;
  const void* vals1[] = {&id1, &name1, &value1, mac1, &time1, &active1};
  db.insert(vals1);
  
  int32_t id2 = 2;
  const char* name2 = "Device2";
  float value2 = 67.89;
  uint8_t mac2[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  uint32_t time2 = 1609459300;
  bool active2 = false;
  const void* vals2[] = {&id2, &name2, &value2, mac2, &time2, &active2};
  db.insert(vals2);
  
  TEST_ASSERT(db.count() == 2, "Inserted 2 records");
  TEST_ASSERT(db.saveToFile(testFile) == IMDB_OK, "Save table with data");
  
  // Verify file exists and has reasonable size
  TEST_ASSERT(SPIFFS.exists(testFile), "Save file exists");
  File file = SPIFFS.open(testFile, "r");
  size_t fileSize = file.size();
  file.close();
  Serial.printf("   File size: %d bytes\n", fileSize);
  TEST_ASSERT(fileSize > 100, "Save file has reasonable size");
  
  // Test 6: Load data and verify
  db.dropTable();
  TEST_ASSERT(db.loadFromFile(testFile) == IMDB_OK, "Load table with data");
  TEST_ASSERT(db.count() == 2, "Loaded 2 records");
  
  // Verify data integrity
  IMDBSelectResult result;
  db.select("Name", "ID", &id1, &result);
  TEST_ASSERT_STR_EQUAL("TestDevice", result.stringValue, "Loaded Name matches");
  
  db.select("Value", "ID", &id1, &result);
  TEST_ASSERT(fabs(result.floatValue - 123.45) < 0.01, "Loaded Value matches");
  
  db.select("MAC", "ID", &id1, &result);
  TEST_ASSERT(memcmp(result.macAddress, mac1, 6) == 0, "Loaded MAC matches");
  
  db.select("Time", "ID", &id1, &result);
  TEST_ASSERT(result.epochValue == 1609459200, "Loaded Time matches");
  
  db.select("Active", "ID", &id1, &result);
  TEST_ASSERT(result.boolValue == true, "Loaded Active matches");
  
  db.select("Active", "ID", &id2, &result);
  TEST_ASSERT(result.boolValue == false, "Loaded second Active matches");
  
  // Test 7: Save/load with TTL records
  db.dropTable();
  db.createTable(cols, 6);
  
  int32_t id3 = 3;
  const char* name3 = "TTL_Device";
  float value3 = 99.99;
  uint8_t mac3[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
  uint32_t time3 = millis() / 1000;
  bool active3 = true;
  const void* vals3[] = {&id3, &name3, &value3, mac3, &time3, &active3};
  db.insert(vals3, 60000);  // 60 second TTL
  
  int32_t id4 = 4;
  const char* name4 = "Permanent";
  float value4 = 11.11;
  uint8_t mac4[6] = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA};
  uint32_t time4 = millis() / 1000;
  bool active4 = true;
  const void* vals4[] = {&id4, &name4, &value4, mac4, &time4, &active4};
  db.insert(vals4);  // No TTL
  
  TEST_ASSERT(db.count() == 2, "Inserted 1 TTL + 1 permanent");
  TEST_ASSERT(db.saveToFile(testFile) == IMDB_OK, "Save with TTL records");
  
  db.dropTable();
  TEST_ASSERT(db.loadFromFile(testFile) == IMDB_OK, "Load with TTL records");
  TEST_ASSERT(db.count() == 2, "Loaded both records");
  
  // TTL should be preserved
  delay(1000);
  db.purgeExpiredRecords();
  TEST_ASSERT(db.count() == 2, "TTL records not expired yet");
  
  // Test 8: Multiple save/load cycles
  db.dropTable();
  db.createTable(cols, 6);
  
  for (int i = 0; i < 5; i++) {
    Serial.printf("   Save/load cycle %d...\n", i + 1);
    
    // Insert data
    int32_t id = i;
    char name[50];
    sprintf(name, "Cycle%d", i);
    const char* namePtr = name;
    float val = (float)i * 10.5;
    uint8_t mac[6] = {(uint8_t)i, (uint8_t)i, (uint8_t)i, (uint8_t)i, (uint8_t)i, (uint8_t)i};
    uint32_t tm = 1609459200 + i;
    bool act = (i % 2 == 0);
    const void* vals[] = {&id, &namePtr, &val, mac, &tm, &act};
    db.insert(vals);
    
    // Save
    IMDBResult saveRes = db.saveToFile(testFile);
    if (saveRes != IMDB_OK) {
      Serial.printf("      [DEBUG] Save failed on cycle %d: %s\n", i, ESP32IMDB::resultToString(saveRes));
      TEST_ASSERT(false, "Multiple cycle save");
      break;
    }
    
    // Load (need to drop first)
    db.dropTable();
    IMDBResult loadRes = db.loadFromFile(testFile);
    if (loadRes != IMDB_OK) {
      Serial.printf("      [DEBUG] Load failed on cycle %d: %s\n", i, ESP32IMDB::resultToString(loadRes));
      TEST_ASSERT(false, "Multiple cycle load");
      break;
    }
    
    // Verify count
    int expectedCount = i + 1;
    int actualCount = db.count();
    if (actualCount != expectedCount) {
      Serial.printf("      [DEBUG] Cycle %d: expected %d records, got %d\n", i, expectedCount, actualCount);
      TEST_ASSERT(false, "Multiple cycle count");
      break;
    }
  }
  TEST_ASSERT(db.count() == 5, "All cycles completed");
  
  // Test 9: Large dataset save/load
  db.dropTable();
  IMDBColumn largeCols[] = {{"ID", IMDB_TYPE_INT32}, {"Data", IMDB_TYPE_STRING}};
  db.createTable(largeCols, 2);
  
  Serial.println("   Inserting 200 records for large dataset test...");
  for (int i = 0; i < 200; i++) {
    int32_t id = i;
    char data[100];
    sprintf(data, "LargeDataRecord_%d_with_extra_padding_to_increase_size", i);
    const char* dataPtr = data;
    const void* vals[] = {&id, &dataPtr};
    db.insert(vals);
  }
  
  TEST_ASSERT(db.count() == 200, "Large dataset inserted");
  TEST_ASSERT(db.saveToFile(testFile) == IMDB_OK, "Save large dataset");
  
  file = SPIFFS.open(testFile, "r");
  fileSize = file.size();
  file.close();
  Serial.printf("   Large dataset file size: %d bytes\n", fileSize);
  TEST_ASSERT(fileSize > 1000, "Large file has appropriate size");
  
  db.dropTable();
  TEST_ASSERT(db.loadFromFile(testFile) == IMDB_OK, "Load large dataset");
  TEST_ASSERT(db.count() == 200, "Large dataset loaded completely");
  
  // Spot check a few records
  IMDBSelectResult spotCheck;
  int32_t checkId = 50;
  db.select("Data", "ID", &checkId, &spotCheck);
  TEST_ASSERT(strstr(spotCheck.stringValue, "LargeDataRecord_50") != nullptr, "Large dataset spot check 1");
  
  checkId = 150;
  db.select("Data", "ID", &checkId, &spotCheck);
  TEST_ASSERT(strstr(spotCheck.stringValue, "LargeDataRecord_150") != nullptr, "Large dataset spot check 2");
  
  // Test 10: Load non-existent file
  db.dropTable();
  // Ensure the file truly doesn't exist
  if (SPIFFS.exists("/nonexistent.imdb")) {
    SPIFFS.remove("/nonexistent.imdb");
  }
  //Serial.print(db.loadFromFile("/nonexistent.imdb"));
  TEST_ASSERT(db.loadFromFile("/nonexistent.imdb") == IMDB_ERROR_FILE_OPEN, "Load non-existent file");
  
  // Test 11: Save with invalid path
  db.createTable(cols, 6);
  TEST_ASSERT(db.saveToFile("") == IMDB_ERROR_FILE_OPEN, "Save with empty path");
  
  // Test 12: Corrupted file handling
  db.dropTable();
  File corruptFile = SPIFFS.open(testFile, "w");
  corruptFile.print("This is not a valid IMDB file!");
  corruptFile.close();
  
  IMDBResult corruptResult = db.loadFromFile(testFile);
  TEST_ASSERT(corruptResult != IMDB_OK, "Reject corrupted file");
  Serial.printf("   Corrupted file returned: %s\n", ESP32IMDB::resultToString(corruptResult));
  
  // Cleanup
  SPIFFS.remove(testFile);
  db.dropTable();
  
  Serial.println("   ✓ Persistence tests complete");
}
#endif

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔════════════════════════════════════════════════════════════╗");
  Serial.println("║                ESP32IMDB TORTURE TEST SUITE                ║");
  Serial.println("╚════════════════════════════════════════════════════════════╝");
  
#ifdef ENABLE_PERSISTENCE_TEST
  Serial.println("ℹ Persistence test ENABLED (SPIFFS save/load)");
#else
  Serial.println("ℹ Persistence test DISABLED (uncomment ENABLE_PERSISTENCE_TEST to enable)");
#endif
  Serial.println();
  
  uint32_t startTime = millis();
  
  // Run all tests
  testTableOperations();
  testInt32Type();
  testStringType();
  testMacType();
  testEpochType();
  testBoolType();
  testFloatType();
  testFloatMathOperations();
  testMathOperations();
  testTTL();
  testUpdateDelete();
  testQueryOperations();
  testErrorConditions();
  testMultiColumn();
  testStressTest();
  testMemoryManagement();
  
#ifdef ENABLE_PERSISTENCE_TEST
  testPersistence();
#endif
  
  uint32_t elapsed = millis() - startTime;
  
  // Print final results
  Serial.println();
  printTestResults();
  Serial.printf("Total execution time: %u ms\n", elapsed);
  Serial.printf("Final free heap: %u bytes\n", ESP.getFreeHeap());
}

void loop() {
  delay(1000);
}
