/*
 * ESP32IMDB - Persistence Example
 * 
 * Copyright (c) 2026 Xorlent
 * Licensed under the MIT License.
 * https://github.com/Xorlent/ESP32IMDB
 * 
 * This example demonstrates how to save and load the in-memory database
 * to/from SPIFFS for persistent storage across reboots.
 * 
 * Features shown:
 * - Initializing SPIFFS
 * - Loading existing database from file (if available)
 * - Creating a new table if no saved data exists
 * - Inserting records with TTL
 * - Saving database to SPIFFS
 * - Automatic TTL adjustment on load (time "pauses" while powered off)
 * 
 * Usage:
 * 1. First run: Creates table, adds data, saves to SPIFFS
 * 2. Subsequent runs: Loads data from SPIFFS and shows preserved state
 * 3. Reset ESP32 to see data persistence across reboots
 */

#include <ESP32IMDB.h>
#include <SPIFFS.h>

// Create database instance
ESP32IMDB db;

// Filename for database storage
const char* DB_FILENAME = "/sensor_data.imdb";

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32IMDB Persistence Example ===\n");
  
  // Initialize SPIFFS
  Serial.println("1. Initializing SPIFFS...");
  if (!SPIFFS.begin(true)) {
    Serial.println("   ✗ Failed to mount SPIFFS!");
    return;
  }
  Serial.printf("   ✓ SPIFFS mounted (Total: %d bytes, Used: %d bytes)\n", 
                SPIFFS.totalBytes(), SPIFFS.usedBytes());
  
  // Try to load existing database
  Serial.println("\n2. Attempting to load database from file...");
  IMDBResult result = db.loadFromFile(DB_FILENAME);
  
  if (result == IMDB_OK) {
    Serial.println("   ✓ Database loaded from SPIFFS!");
    
    // Show loaded data
    int recordCount = db.getRecordCount();
    Serial.printf("   Records loaded: %d\n", recordCount);
    Serial.printf("   Memory usage: %d bytes\n", db.getMemoryUsage());
    
    // Display all records
    Serial.println("\n   Loaded records:");
    displayAllRecords();
    
  } else if (result == IMDB_ERROR_FILE_OPEN) {
    Serial.println("   ℹ No existing database file found. Creating new table...");
    createAndPopulateTable();
    
  } else if (result == IMDB_ERROR_TABLE_EXISTS) {
    Serial.println("   ℹ Table already exists in memory (cannot load over existing table)");
    
  } else {
    Serial.printf("   ✗ Error loading database: %s\n", ESP32IMDB::resultToString(result));
    Serial.println("   Creating new table instead...");
    createAndPopulateTable();
  }
  
  // Add another record
  Serial.println("\n3. Adding a new sensor reading...");
  delay(2000);
  
  // Add a new record with 30-second TTL
  int32_t newId = millis() / 1000;  // Use timestamp as ID
  const char* newSensor = "Temperature";
  float newValue = 23.5 + (random(0, 100) / 10.0);
  uint32_t newTimestamp = millis() / 1000;
  bool newValid = true;
  const void* newValues[] = {&newId, &newSensor, &newValue, &newTimestamp, &newValid};
  
  result = db.insert(newValues, 30000);  // 30 second TTL
  if (result == IMDB_OK) {
    Serial.printf("   ✓ Added new reading: %.1f°C (expires in 30s)\n", newValue);
  } else {
    Serial.printf("   ✗ Error inserting: %s\n", ESP32IMDB::resultToString(result));
  }
  
  // Display current state
  Serial.println("\n4. Current database state:");
  Serial.printf("   Total records: %d\n", db.count());
  displayAllRecords();
  
  // Save to SPIFFS
  Serial.println("\n5. Saving database to SPIFFS...");
  result = db.saveToFile(DB_FILENAME);
  if (result == IMDB_OK) {
    Serial.println("   ✓ Database saved successfully!");
    Serial.println("   (Reset ESP32 to see data restored from file)");
  } else {
    Serial.printf("   ✗ Error saving: %s\n", ESP32IMDB::resultToString(result));
  }
  
  // Show file info
  if (SPIFFS.exists(DB_FILENAME)) {
    File file = SPIFFS.open(DB_FILENAME, "r");
    Serial.printf("   File size: %d bytes\n", file.size());
    file.close();
  }
  
  Serial.println("\n=== Setup Complete ===");
  Serial.println("The database will auto-save every 60 seconds in loop().");
  Serial.println("Records with TTL will expire and be purged every 5 seconds.");
  Serial.println("Reset the ESP32 to see data persistence!\n");
}

void loop() {
  static unsigned long lastSave = 0;
  static unsigned long lastPurge = 0;
  
  // Purge expired records every 5 seconds
  if (millis() - lastPurge > 5000) {
    lastPurge = millis();
    int expiredCount = db.purgeExpiredRecords();
    
    if (expiredCount > 0) {
      Serial.printf("[%lu] Purged %d expired record(s). Remaining: %d\n", 
                    millis()/1000, expiredCount, db.count());
    }
  }
  
  // Auto-save database snapshot every 60 seconds
  if (millis() - lastSave > 60000) {
    lastSave = millis();
    
    Serial.printf("\n[%lu] === Creating database snapshot ===\n", millis()/1000);
    Serial.printf("[%lu] Current state: %d records, %d bytes\n", 
                  millis()/1000, db.getRecordCount(), db.getMemoryUsage());
    
    IMDBResult result = db.saveToFile(DB_FILENAME);
    if (result == IMDB_OK) {
      Serial.printf("[%lu] ✓ Snapshot saved successfully\n", millis()/1000);
      
      // Show file size
      if (SPIFFS.exists(DB_FILENAME)) {
        File file = SPIFFS.open(DB_FILENAME, "r");
        Serial.printf("[%lu] File size: %d bytes\n", millis()/1000, file.size());
        file.close();
      }
    } else {
      Serial.printf("[%lu] ✗ Snapshot failed: %s\n", 
                    millis()/1000, ESP32IMDB::resultToString(result));
    }
    Serial.printf("[%lu] === Snapshot complete ===\n\n", millis()/1000);
  }
  
  delay(1000);
}

// Create a new sensor data table
void createAndPopulateTable() {
  // Define table structure
  IMDBColumn sensorColumns[] = {
    {"ID", IMDB_TYPE_INT32},
    {"Sensor", IMDB_TYPE_STRING},
    {"Value", IMDB_TYPE_FLOAT},
    {"Timestamp", IMDB_TYPE_EPOCH},
    {"Valid", IMDB_TYPE_BOOL}
  };
  
  IMDBResult result = db.createTable(sensorColumns, 5);
  if (result != IMDB_OK) {
    Serial.printf("   ✗ Failed to create table: %s\n", ESP32IMDB::resultToString(result));
    return;
  }
  Serial.println("   ✓ New table created");
  
  // Insert some sample data
  Serial.println("\n   Inserting sample data...");
  
  // Sensor reading 1 (no expiry)
  int32_t id1 = 1;
  const char* sensor1 = "Temperature";
  float value1 = 22.5;
  uint32_t time1 = millis() / 1000;
  bool valid1 = true;
  const void* values1[] = {&id1, &sensor1, &value1, &time1, &valid1};
  db.insert(values1);
  Serial.println("   - Temperature: 22.5°C");
  
  // Sensor reading 2 (expires in 60 seconds)
  int32_t id2 = 2;
  const char* sensor2 = "Humidity";
  float value2 = 65.3;
  uint32_t time2 = millis() / 1000;
  bool valid2 = true;
  const void* values2[] = {&id2, &sensor2, &value2, &time2, &valid2};
  db.insert(values2, 60000);  // 60 second TTL
  Serial.println("   - Humidity: 65.3% (expires in 60s)");
  
  // Sensor reading 3 (expires in 30 seconds)
  int32_t id3 = 3;
  const char* sensor3 = "Pressure";
  float value3 = 1013.25;
  uint32_t time3 = millis() / 1000;
  bool valid3 = true;
  const void* values3[] = {&id3, &sensor3, &value3, &time3, &valid3};
  db.insert(values3, 30000);  // 30 second TTL
  Serial.println("   - Pressure: 1013.25 hPa (expires in 30s)");
  
  Serial.printf("   ✓ Inserted 3 records\n");
}

// Display all records in the database
void displayAllRecords() {
  int count = db.count();
  if (count == 0) {
    Serial.println("   (No records)");
    return;
  }
  
  // Get all records (no WHERE clause - pass nullptr)
  IMDBSelectResult* results = nullptr;
  int resultCount = 0;
  
  // Query by getting each record's ID
  for (int i = 1; i <= count + 10; i++) {  // Try a range of IDs
    IMDBSelectResult result;
    int32_t id = i;
    
    if (db.select("Sensor", "ID", &id, &result) == IMDB_OK && result.hasValue) {
      // Get other fields for this record
      IMDBSelectResult valueResult, timeResult, validResult;
      db.select("Value", "ID", &id, &valueResult);
      db.select("Timestamp", "ID", &id, &timeResult);
      db.select("Valid", "ID", &id, &validResult);
      
      Serial.printf("   [ID %d] %s: %.2f (time: %u, valid: %s)\n",
                    i,
                    result.stringValue,
                    valueResult.floatValue,
                    timeResult.epochValue,
                    validResult.boolValue ? "yes" : "no");
    }
  }
}
