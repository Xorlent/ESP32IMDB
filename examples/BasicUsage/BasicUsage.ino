/*
 * ESP32IMDB - Basic Usage Example
 * 
 * Copyright (c) 2026 Xorlent
 * Licensed under the MIT License.
 * https://github.com/Xorlent/ESP32IMDB
 * 
 * This example demonstrates how to use the ESP32IMDB in-memory database.
 * It creates a simple user database and shows:
 * - Creating a table
 * - Inserting records
 * - Updating records (with and without math operations)
 * - Querying data
 * - Using aggregate functions (count, min, max)
 * - Deleting records
 * - Working with TTL (time-to-live)
 * - Float data type usage
 */

#include <ESP32IMDB.h>

// Create database instance
ESP32IMDB db;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32IMDB Basic Usage Example ===\n");
  
  // Example 1: Create a simple users table
  Serial.println("1. Creating 'Users' table...");
  IMDBColumn userColumns[] = {
    {"UserID", IMDB_TYPE_INT32},
    {"Name", IMDB_TYPE_STRING},
    {"Age", IMDB_TYPE_INT32},
    {"Active", IMDB_TYPE_BOOL}
  };
  
  IMDBResult result = db.createTable(userColumns, 4);
  if (result == IMDB_OK) {
    Serial.println("   ✓ Table created successfully");
  } else {
    Serial.printf("   ✗ Error: %s\n", ESP32IMDB::resultToString(result));
    return;
  }
  
  // Example 2: Insert some records
  Serial.println("\n2. Inserting records...");
  
  // User 1
  int32_t id1 = 1;
  const char* name1 = "Alice";
  int32_t age1 = 25;
  bool active1 = true;
  const void* values1[] = {&id1, &name1, &age1, &active1};
  result = db.insert(values1);
  Serial.printf("   Insert User 1: %s\n", ESP32IMDB::resultToString(result));
  
  // User 2
  int32_t id2 = 2;
  const char* name2 = "Bob";
  int32_t age2 = 30;
  bool active2 = true;
  const void* values2[] = {&id2, &name2, &age2, &active2};
  result = db.insert(values2);
  Serial.printf("   Insert User 2: %s\n", ESP32IMDB::resultToString(result));
  
  // User 3 with TTL (expires in 10 seconds)
  int32_t id3 = 3;
  const char* name3 = "Charlie";
  int32_t age3 = 28;
  bool active3 = true;
  const void* values3[] = {&id3, &name3, &age3, &active3};
  result = db.insert(values3, 10000);  // 10 second TTL
  Serial.printf("   Insert User 3 (with 10s TTL): %s\n", ESP32IMDB::resultToString(result));
  
  // Example 3: Count records
  Serial.println("\n3. Counting records...");
  int32_t totalCount = db.count();
  Serial.printf("   Total records: %d\n", totalCount);
  
  // Example 4: Select a record
  Serial.println("\n4. Selecting record where UserID = 1...");
  IMDBSelectResult selectResult;
  int32_t searchId = 1;
  result = db.select("Name", "UserID", &searchId, &selectResult);
  if (result == IMDB_OK && selectResult.hasValue) {
    Serial.printf("   Found: %s\n", selectResult.stringValue);
  } else {
    Serial.printf("   Error: %s\n", ESP32IMDB::resultToString(result));
  }
  
  // Example 5: Update a record
  Serial.println("\n5. Updating Bob's age to 31...");
  int32_t bobId = 2;
  int32_t newAge = 31;
  result = db.update("UserID", &bobId, "Age", &newAge);
  Serial.printf("   Update result: %s\n", ESP32IMDB::resultToString(result));
  
  // Example 6: Update with math operation (increment age)
  Serial.println("\n6. Incrementing Alice's age by 1...");
  int32_t aliceId = 1;
  result = db.updateWithMath("UserID", &aliceId, "Age", IMDB_MATH_ADD, 1);
  Serial.printf("   Update result: %s\n", ESP32IMDB::resultToString(result));
  
  // Verify the update
  result = db.select("Age", "UserID", &aliceId, &selectResult);
  if (result == IMDB_OK && selectResult.hasValue) {
    Serial.printf("   Alice's new age: %d\n", selectResult.int32Value);
  }
  
  // Example 7: Aggregate functions
  Serial.println("\n7. Finding min and max age...");
  IMDBSelectResult minResult, maxResult;
  
  result = db.min("Age", &minResult);
  if (result == IMDB_OK) {
    Serial.printf("   Minimum age: %d\n", minResult.int32Value);
  }
  
  result = db.max("Age", &maxResult);
  if (result == IMDB_OK) {
    Serial.printf("   Maximum age: %d\n", maxResult.int32Value);
  }
  
  // Example 8: Wait for TTL expiration
  Serial.println("\n8. Waiting 11 seconds for Charlie's record to expire...");
  delay(11000);
  db.purgeExpiredRecords();
  
  totalCount = db.count();
  Serial.printf("   Records after purge: %d\n", totalCount);
  
  // Example 9: Top N records
  Serial.println("\n9. Getting top 2 records...");
  IMDBSelectResult* topResults;
  int topCount;
  result = db.top(2, &topResults, &topCount);
  if (result == IMDB_OK) {
    Serial.printf("   Retrieved %d records:\n", topCount);
    for (int i = 0; i < topCount; i++) {
      int baseIndex = i * 4;  // 4 columns per record
      Serial.printf("   - UserID: %d, Name: %s, Age: %d, Active: %s\n",
                    topResults[baseIndex + 0].int32Value,
                    topResults[baseIndex + 1].stringValue,
                    topResults[baseIndex + 2].int32Value,
                    topResults[baseIndex + 3].boolValue ? "true" : "false");
    }
    free(topResults);  // Don't forget to free the results!
  }
  
  // Example 10: Delete a record
  Serial.println("\n10. Deleting user with UserID = 1...");
  result = db.deleteRecords("UserID", &aliceId);
  Serial.printf("   Delete result: %s\n", ESP32IMDB::resultToString(result));
  
  totalCount = db.count();
  Serial.printf("   Records remaining: %d\n", totalCount);
  
  // Example 11: Memory usage
  Serial.println("\n11. Memory statistics...");
  Serial.printf("   Database memory usage: %d bytes\n", db.getMemoryUsage());
  Serial.printf("   Free heap: %d bytes\n", ESP.getFreeHeap());
  
  // Example 12: Drop table
  Serial.println("\n12. Dropping table...");
  result = db.dropTable();
  Serial.printf("   Drop result: %s\n", ESP32IMDB::resultToString(result));
  
  // Example 13: Float data type usage
  Serial.println("\n13. Float data type demonstration...");
  
  // Create table with float columns
  IMDBColumn sensorColumns[] = {
    {"SensorID", IMDB_TYPE_INT32},
    {"Temperature", IMDB_TYPE_FLOAT},
    {"Humidity", IMDB_TYPE_FLOAT}
  };
  
  result = db.createTable(sensorColumns, 3);
  Serial.printf("   Create sensor table: %s\n", ESP32IMDB::resultToString(result));
  
  // Insert sensor readings
  int32_t sensor1 = 1;
  float temp1 = 23.45;
  float humid1 = 65.8;
  const void* sensorVals1[] = {&sensor1, &temp1, &humid1};
  db.insert(sensorVals1);
  Serial.printf("   Inserted: Sensor %d, Temp %.2f°C, Humidity %.1f%%\n", 
                sensor1, temp1, humid1);
  
  int32_t sensor2 = 2;
  float temp2 = 18.92;
  float humid2 = 72.3;
  const void* sensorVals2[] = {&sensor2, &temp2, &humid2};
  db.insert(sensorVals2);
  Serial.printf("   Inserted: Sensor %d, Temp %.2f°C, Humidity %.1f%%\n", 
                sensor2, temp2, humid2);
  
  // Query float values
  IMDBSelectResult floatResult;
  result = db.select("Temperature", "SensorID", &sensor1, &floatResult);
  if (result == IMDB_OK && floatResult.hasValue) {
    Serial.printf("   Sensor 1 temperature: %.2f°C\n", floatResult.floatValue);
  }
  
  // Math operations on floats
  Serial.println("   Increasing all temperatures by 2.5 degrees...");
  db.updateWithMath("SensorID", &sensor1, "Temperature", IMDB_MATH_ADD, 2);  // Adds 2.5 (cast from int)
  db.updateWithMath("SensorID", &sensor2, "Temperature", IMDB_MATH_ADD, 2);
  
  // Find min/max temperatures
  result = db.min("Temperature", &floatResult);
  if (result == IMDB_OK) {
    Serial.printf("   Minimum temperature: %.2f°C\n", floatResult.floatValue);
  }
  
  result = db.max("Temperature", &floatResult);
  if (result == IMDB_OK) {
    Serial.printf("   Maximum temperature: %.2f°C\n", floatResult.floatValue);
  }
  
  // Drop sensor table
  db.dropTable();
  
  Serial.println("\n=== Example Complete ===");
}

void loop() {
  delay(1000);
}
