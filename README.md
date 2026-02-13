# ESP32IMDB - Simple In-Memory Database for ESP32

A lightweight, thread-safe in-memory database engine designed specifically for ESP32 Arduino projects. Perfect for beginners and experienced developers who need simple, small footprint, fast data storage without external dependencies.

## Table of Contents

- [Features](#features)
- [Supported Data Types](#supported-data-types)
- [Installation](#installation)
- [Requirements](#requirements)
- [Quick Start](#quick-start)
- [API Reference](#api-reference)
  - [Table Operations](#table-operations)
  - [Data Operations](#data-operations)
  - [Query Operations](#query-operations)
  - [Utility Functions](#utility-functions)
  - [Persistence Functions](#persistence-functions)
- [Migrating from SQL to IMDB](#migrating-from-sql-to-imdb)
- [Configuration](#configuration)
- [Examples](#examples)
  - [Basic Usage](#basic-usage)
  - [Persistence Example](#persistence-example)
  - [Working with Float Data](#working-with-float-data)
- [Memory Management](#memory-management)
- [Time-To-Live (TTL)](#time-to-live-ttl)
- [Thread Safety](#thread-safety)
- [Error Handling](#error-handling)
- [Limitations](#limitations)
- [Best Practices](#best-practices)
- [Optimizations](#optimizations)
- [Troubleshooting](#troubleshooting)
- [Version History](#version-history)

## Features

- **Simple & Beginner-Friendly**: Easy-to-understand API inspired by SQL
- **Thread-Safe**: Built-in mutex protection for multi-threaded applications
- **Multiple Data Types**: Support for integers, floats, strings, MAC addresses, timestamps, and booleans
- **Automatic Memory Management**: String compaction and configurable heap limit
- **Time-To-Live (TTL)**: Automatic expiration and purging of old records
- **Optional Persistent Storage**: Save/load database to SPIFFS for data preservation across reboots
- **No External Dependencies**: Uses only ESP32-Arduino built-in libraries
- **SQL-Like Operations**: Familiar patterns for INSERT, UPDATE, DELETE, and SELECT
- **Aggregate Functions**: COUNT, MIN, MAX, and TOP
- **Math Operations**: Perform +, -, *, /, % directly on fields

## Supported Data Types

| Type | Description | Example |
|------|-------------|---------|
| `IMDB_TYPE_INT32` | Signed 32-bit integer | -2147483648 to 2147483647 |
| `IMDB_TYPE_FLOAT` | 32-bit floating point | 3.14159, -273.15, 1.23e-4 |
| `IMDB_TYPE_STRING` | Text up to 255 characters | "Hello World" |
| `IMDB_TYPE_MAC` | MAC address (6 bytes) | aa:bb:cc:dd:ee:ff |
| `IMDB_TYPE_EPOCH` | Unix timestamp | 1234567890 |
| `IMDB_TYPE_BOOL` | Boolean value | true or false |

## Installation

1. Download this library as a ZIP file
2. In Arduino IDE: Sketch → Include Library → Add .ZIP Library
3. Select the downloaded ZIP file
4. Restart Arduino IDE

Or manually copy the `ESP32IMDB` folder to your Arduino libraries directory.

## Requirements

- **Arduino IDE**: 2.3.6 or later
- **ESP32 Board Package**: esp32 by Espressif Systems 3.3.5 or later

This library has been tested and verified on the above versions.

## Quick Start

```cpp
#include <ESP32IMDB.h>

ESP32IMDB db;

void setup() {
  Serial.begin(115200);
  
  // 1. Define table structure
  IMDBColumn columns[] = {
    {"ID", IMDB_TYPE_INT32},
    {"Name", IMDB_TYPE_STRING},
    {"Active", IMDB_TYPE_BOOL}
  };
  
  // 2. Create table
  db.createTable(columns, 3);
  
  // 3. Insert data
  int32_t id = 1;
  const char* name = "Arduino";
  bool active = true;
  const void* values[] = {&id, &name, &active};
  db.insert(values);
  
  // 4. Query data
  IMDBSelectResult result;
  db.select("Name", "ID", &id, &result);
  Serial.println(result.stringValue);  // Prints: Arduino
}

void loop() {
  delay(1000);
}
```

## API Reference

### Table Operations

#### createTable()
Creates a new table with specified columns.

```cpp
IMDBColumn columns[] = {
  {"ColumnName", IMDB_TYPE_INT32},
  // ... more columns
};
IMDBResult result = db.createTable(columns, columnCount);
```

#### dropTable()
Drops the current table and frees all memory.

```cpp
db.dropTable();
```

### Data Operations

#### insert()
Inserts a new record. Values must be provided for all columns in order.

```cpp
// Without TTL
const void* values[] = {&value1, &value2, &value3};
db.insert(values);

// With TTL (expires after 60 seconds)
db.insert(values, 60000);
```

#### update()
Updates records matching a WHERE condition.

```cpp
// UPDATE table SET Name = "NewName" WHERE ID = 1
int32_t id = 1;
const char* newName = "NewName";
db.update("ID", &id, "Name", &newName);
```

#### updateWithMath()
Updates numeric fields using math operations. Works with INT32, EPOCH, and FLOAT types.

```cpp
// UPDATE table SET Counter = Counter + 1 WHERE ID = 1
int32_t id = 1;
db.updateWithMath("ID", &id, "Counter", IMDB_MATH_ADD, 1);

// Float: Convert Celsius to Fahrenheit (C * 9/5 + 32)
int32_t sensorId = 1;
db.updateWithMath("SensorID", &sensorId, "Temperature", IMDB_MATH_MULTIPLY, 9);
db.updateWithMath("SensorID", &sensorId, "Temperature", IMDB_MATH_DIVIDE, 5);
db.updateWithMath("SensorID", &sensorId, "Temperature", IMDB_MATH_ADD, 32);
```

Math operations: `IMDB_MATH_ADD`, `IMDB_MATH_SUBTRACT`, `IMDB_MATH_MULTIPLY`, `IMDB_MATH_DIVIDE`, `IMDB_MATH_MODULO`

#### deleteRecords()
Deletes records matching a WHERE condition.

```cpp
// DELETE FROM table WHERE ID = 1
int32_t id = 1;
db.deleteRecords("ID", &id);
```

### Query Operations

#### select()
Retrieves a single column value from the first matching record.

```cpp
IMDBSelectResult result;
int32_t searchId = 1;
db.select("Name", "ID", &searchId, &result);

if (result.hasValue) {
  Serial.println(result.stringValue);
}
```

#### selectAll()
Retrieves all columns from all records matching a WHERE condition.

```cpp
IMDBSelectResult* results;
int resultCount;
bool activeValue = true;
db.selectAll("Active", &activeValue, &results, &resultCount);

// Process results - each record contains all columns
for (int i = 0; i < resultCount; i++) {
  int baseIndex = i * columnCount;
  // Access: results[baseIndex + columnIndex]
}

free(results);  // Don't forget to free!
```

#### count()
Returns the number of valid, non-expired records.

```cpp
int32_t totalRecords = db.count();
```

#### getRecordCount()
Returns the total number of record slots currently in use.  
Example: Insert 1000 records, delete 900  
- getRecordCount(): 100 (compacted, decreases after deletes)  
- count(): 100 or less (excludes expired records)  

```cpp
int usedSlots = db.getRecordCount();   // Used record slots (after compaction)
int32_t validRecords = db.count();     // Only non-expired, valid records
```

#### countWhere()
Counts records matching a WHERE condition.

```cpp
bool activeValue = true;
int32_t activeCount = db.countWhere("Active", &activeValue);
```

#### min() / max()
Finds minimum or maximum value in a numeric column (INT32, EPOCH, or FLOAT).

```cpp
IMDBSelectResult result;
db.min("Age", &result);  // or db.max("Age", &result);
Serial.println(result.int32Value);

// For float columns:
db.min("Temperature", &result);
Serial.println(result.floatValue);  // Access via floatValue
```

#### top()
Retrieves the first N records.

```cpp
IMDBSelectResult* results;
int resultCount;
db.top(10, &results, &resultCount);

// Process results...
// Each record has all columns: results[recordIndex * columnCount + columnIndex]

free(results);  // Don't forget to free!
```

### Utility Functions

#### purgeExpiredRecords()
Manually removes all expired records based on TTL.

```cpp
db.purgeExpiredRecords();
```

#### getMemoryUsage()
Returns estimated memory usage in bytes.

```cpp
size_t bytes = db.getMemoryUsage();
```

#### parseMacAddress()
Parses MAC address strings in multiple formats. Validates hexadecimal characters.  
Supported formats:  
- "aabbccddeeff"        (12 hex chars, no delimiters)  
- "aa:bb:cc:dd:ee:ff"   (colon-separated)  
- "aa-bb-cc-dd-ee-ff"   (hyphen-separated)  
Returns false for invalid hex characters (e.g., "gg:hh:ii:jj:kk:ll")  

```cpp
uint8_t mac[6];
bool success = ESP32IMDB::parseMacAddress("aa:bb:cc:dd:ee:ff", mac);
```

#### formatMacAddress()
Formats a 6-byte MAC address as a colon-separated string.

```cpp
uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
char macStr[18];  // Buffer must be at least 18 bytes
ESP32IMDB::formatMacAddress(mac, macStr);
Serial.println(macStr);  // Prints: "aa:bb:cc:dd:ee:ff"

// Use with query results
IMDBSelectResult result;
db.select("DeviceMAC", "ID", &id, &result);
char formattedMac[18];
ESP32IMDB::formatMacAddress(result.macAddress, formattedMac);
```

#### resultToString()
Converts result codes to human-readable strings.

```cpp
IMDBResult result = db.insert(values);
Serial.println(ESP32IMDB::resultToString(result));
```

### Persistence Functions

#### saveToFile()
Saves the entire database to a SPIFFS file. Records with expired TTL are automatically purged before saving.

```cpp
#include <SPIFFS.h>

void setup() {
  SPIFFS.begin(true);  // Initialize SPIFFS
  
  // ... create table and insert data ...
  
  // Save to file
  IMDBResult result = db.saveToFile("/mydata.imdb");
  if (result == IMDB_OK) {
    Serial.println("Database saved!");
  } else {
    Serial.printf("Save failed: %s\n", ESP32IMDB::resultToString(result));
  }
}
```

**Features:**
- Atomic writes (uses temporary file and rename)
- TTL timestamps are preserved (time "pauses" while micro is powered off)
- Automatically removes expired records before saving

#### loadFromFile()
Loads a database from a SPIFFS file. Recreates the table schema and all records.

```cpp
void setup() {
  SPIFFS.begin(true);
  
  // Load existing database
  IMDBResult result = db.loadFromFile("/mydata.imdb");
  
  if (result == IMDB_OK) {
    Serial.println("Database loaded!");
    Serial.printf("Records: %d\n", db.count());
  } else if (result == IMDB_ERROR_FILE_OPEN) {
    Serial.println("No saved file, creating new table");
    // Create new table...
  } else if (result == IMDB_ERROR_TABLE_EXISTS) {
    Serial.println("Table already exists in memory");
    // Must call db.dropTable() first to reload
  } else {
    Serial.printf("Load failed: %s\n", ESP32IMDB::resultToString(result));
  }
}
```

**Features:**
- Returns `IMDB_ERROR_TABLE_EXISTS` if a table is already in memory to prevent accidental overwrite
- TTL values are automatically adjusted based on elapsed time
- File format and structure validation
- Full error recovery on corrupt/invalid files

**Important Notes:**
- SPIFFS must first be initialized with `SPIFFS.begin()`
- TTL timing: remaining time is preserved, but TTL is effectively paused while the device is powered off
- To reload: call `db.dropTable()` first, then `db.loadFromFile()`

## Migrating from SQL to IMDB

### When to Use IMDB vs File-Based Databases

**Choose ESP32IMDB when:**
- **Speed is critical** - All data is in RAM for microsecond access times
- **Frequent updates** - Prevents premature failure of on-chip flash
- **Small-to-medium datasets** - Sensor readings, device states, temporary caches (< 100KB typical)
- **Time-sensitive data** - Built-in TTL for automatic expiration
- **Multi-threaded apps** - Thread-safe by design with FreeRTOS mutexes
- **No persistence needed** - With ESP32IMDB's persistence mode, however, you can periodically write consistent table state to flash
- **Power-efficient** - Minimizing or eliminating flash writes improves power efficiency

**Choose SQLite3 or file-based DB when:**
- **Large datasets** - Dataset won't comfortably fit in RAM
- **Persistence is primary** - Zero data loss tolerance
- **Complex queries** - Need JOINs, nested queries, tables with relationships
- **Growing data** - Unknown data size that may exceed available RAM

**Key advantages of IMDB for embedded:**
- **No flash wear** - Flash memory tolerates limited write cycles (~10,000-100,000)
- **10-1000x faster** - RAM access vs flash I/O
- **Smaller codebase** - Significantly smaller than full-featured databases (up to 40x)
- **Predictable performance** - No blocking

### SQL to IMDB Code Translation

| SQL | ESP32IMDB |
|-----|-----------|
| `CREATE TABLE Users (ID INT, Name VARCHAR(255))` | `IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"Name", IMDB_TYPE_STRING}}; db.createTable(cols, 2);` |
| `INSERT INTO Users VALUES(1, "John")` | `const void* vals[] = {&id, &name}; db.insert(vals);` |
| `UPDATE Users SET Name = "Jane" WHERE ID = 1` | `db.update("ID", &id, "Name", &newName);` |
| `UPDATE Users SET Counter = Counter + 1 WHERE ID = 1` | `db.updateWithMath("ID", &id, "Counter", IMDB_MATH_ADD, 1);` |
| `SELECT Name FROM Users WHERE ID = 1` | `db.select("Name", "ID", &id, &result);` |
| `DELETE FROM Users WHERE ID = 1` | `db.deleteRecords("ID", &id);` |
| `SELECT COUNT(*) FROM Users` | `int32_t cnt = db.count();` |
| `SELECT MIN(Age) FROM Users` | `db.min("Age", &result);` |
| `DROP TABLE Users` | `db.dropTable();` |

## Configuration

Edit these constants in `ESP32IMDB.h`:

```cpp
// Feature flags - set to 0 to disable and reduce binary size
#define IMDB_ENABLE_PERSISTENCE 1  // Enable saveToFile/loadFromFile (requires SPIFFS)

// Minimum free heap required (operations fail below this)
#define IMDB_MIN_HEAP_BYTES 30000

// Maximum string length
#define IMDB_MAX_STRING_LENGTH 255

// Maximum TTL (30 days)
#define IMDB_MAX_TTL_MS (30UL * 24UL * 60UL * 60UL * 1000UL)
```

**To disable persistence** (saves about 30kB NVRAM space):
```cpp
#define IMDB_ENABLE_PERSISTENCE 0  // Excludes SPIFFS.h and persistence functions
```

## Examples

### Basic Usage
Demonstrates all core features including creating tables, inserting data, querying, updates with math operations, aggregate functions, and TTL. Includes examples for all data types including floats.

**File**: `examples/BasicUsage/BasicUsage.ino`

### Persistence Example
Shows how to save and load databases to/from SPIFFS for persistent storage across reboots. Demonstrates automatic TTL adjustment, file handling, and recovery from missing files.

**File**: `examples/PersistenceExample/PersistenceExample.ino`

### Working with Float Data

Floats can be useful for sensor readings, temperatures, GPS coordinates, etc:

```cpp
// Create table with float column
IMDBColumn columns[] = {
  {"SensorID", IMDB_TYPE_INT32},
  {"Temperature", IMDB_TYPE_FLOAT},
  {"Humidity", IMDB_TYPE_FLOAT}
};
db.createTable(columns, 3);

// Insert sensor data
int32_t id = 1;
float temp = 23.45;
float humidity = 65.2;
const void* values[] = {&id, &temp, &humidity};
db.insert(values);

// Query float values
IMDBSelectResult result;
db.select("Temperature", "SensorID", &id, &result);
Serial.printf("Temperature: %.2f°C\n", result.floatValue);

// Math operations work with floats
db.updateWithMath("SensorID", &id, "Temperature", IMDB_MATH_ADD, 5);  // Add 5 degrees

// Find min/max temperatures
db.min("Temperature", &result);
Serial.printf("Min temp: %.2f°C\n", result.floatValue);
```

## Memory Management

ESP32IMDB is designed to be memory-efficient:

1. **String Compaction**: Strings are stored with only their actual length, not the full 255-byte maximum
2. **Heap Limit Protection**: Operations fail gracefully if free heap drops below `IMDB_MIN_HEAP_BYTES`
3. **Efficient Search**: Linear search optimized for small to medium datasets
4. **Record Compaction**: Deleted records are removed to recover memory

Monitor memory usage:
```cpp
Serial.printf("DB uses %d bytes\n", db.getMemoryUsage());
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
```

## Time-To-Live (TTL)

Records can automatically expire after a specified time:

```cpp
// Record expires after 5 minutes (300000 milliseconds)
db.insert(values, 300000);

// Manually purge expired records
db.purgeExpiredRecords();

// Or set up automatic purging in loop()
void loop() {
  static unsigned long lastPurge = 0;
  if (millis() - lastPurge > 60000) {  // Every 60 seconds
    db.purgeExpiredRecords();
    lastPurge = millis();
  }
}
```

## Thread Safety

ESP32IMDB uses FreeRTOS mutexes for thread-safe operations:

```cpp
// Safe to call from multiple tasks
void task1(void* param) {
  db.insert(values1);
}

void task2(void* param) {
  db.insert(values2);
}
```

## Error Handling

Always check return values:

```cpp
IMDBResult result = db.insert(values);
if (result != IMDB_OK) {
  Serial.printf("Error: %s\n", ESP32IMDB::resultToString(result));
}
```

Common error codes:
- `IMDB_OK`: Success
- `IMDB_ERROR_OUT_OF_MEMORY`: Malloc failed
- `IMDB_ERROR_HEAP_LIMIT`: Insufficient free heap
- `IMDB_ERROR_TABLE_EXISTS`: Table already exists (call dropTable first)
- `IMDB_ERROR_NO_TABLE`: Table doesn't exist
- `IMDB_ERROR_INVALID_TYPE`: Invalid data type
- `IMDB_ERROR_INVALID_VALUE`: Invalid value provided
- `IMDB_ERROR_COLUMN_COUNT_MISMATCH`: Wrong number of values for columns
- `IMDB_ERROR_COLUMN_NOT_FOUND`: Invalid column name
- `IMDB_ERROR_INVALID_OPERATION`: Operation not supported for this data type
- `IMDB_ERROR_NO_RECORDS`: No matching records found
- `IMDB_ERROR_INVALID_MAC_FORMAT`: MAC address format invalid  
Persistence mode additional error codes:  
- `IMDB_ERROR_FILE_OPEN`: Cannot open file
- `IMDB_ERROR_FILE_WRITE`: Cannot write to file
- `IMDB_ERROR_FILE_READ`: Cannot read from file
- `IMDB_ERROR_CORRUPT_FILE`: File is corrupted or invalid format

## Limitations

- **Single Table**: One table per database instance (create multiple instances for multiple tables)
- **No Indexes**: Uses linear search (optimized for small-medium datasets)
- **No Joins**: Single table operations only
- **Simple WHERE**: Single column comparison only
- **TTL Timing**: TTL countdown pauses while device is powered off (preserved, not real-time)

For complex queries, retrieve data and process in your code:
```cpp
IMDBSelectResult* results;
int count;
db.top(100, &results, &count);
// Filter/process results as needed
free(results);
```

## Best Practices

1. **Check return values**: Always verify `IMDBResult` codes
2. **Free results**: When using `top()` or `selectAll()`, always `free()` the results
3. **Monitor memory**: Regularly check `getMemoryUsage()` and `ESP.getFreeHeap()` for intensive use cases
4. **Use TTL wisely**: Set appropriate expiration times to prevent memory bloat
5. **Purge regularly**: Call `purgeExpiredRecords()` periodically if using record TTL values
6. **Compact strings**: Shorter strings = less memory usage
7. **Provide all values**: `insert()` requires values for every column

## Optimizations

- **Memory Footprint**: Not using persistent storage? 30kB can be saved by adding "#define IMDB_ENABLE_PERSISTENCE 0" before your #include statements
- **Memory Fragmentation**: Strategically calling compactRecords() may help deal with memory congestion on especially data intensive or complex projects

## Troubleshooting

**"Table already exists" error**
- Call `dropTable()` before creating a new table
- Or use a different database instance

**"Out of memory" errors**
- Reduce `IMDB_MAX_STRING_LENGTH` if you don't need 255 characters
- Increase `IMDB_MIN_HEAP_BYTES` threshold
- Use shorter strings when possible
- Implement TTL on records
- Purge old data more frequently using `purgeExpiredRecords()`  

**Records not found after waiting**
- Check if TTL has expired
- Call `purgeExpiredRecords()` to see if count changes
- Verify column names match exactly (case-sensitive)

## Version History

- **1.1.2** (2026-02-13): Expose all user-controllable settings
  - Add support for configuring all user-controllable settings from a sketch
    - IMDB_ENABLE_PERSISTENCE (0 to disable)
    - IMDB_MIN_HEAP_BYTES (default 30000)
    - IMDB_MAX_STRING_LENGTH (default 255)

- **1.1.1** (2026-02-13): Persistence feature control
  - Add support for enabling/disabling IMDB_ENABLE_PERSISTENCE flag from a sketch  

- **1.1.0** (2026-02-12): Persistence support
  - `saveToFile()` and `loadFromFile()` for SPIFFS persistence
  - Atomic file writes with temporary file and rename
  - TTL preservation across reboots
  - Binary file format with validation
  - New error codes for file operations  

- **1.0.0** : Initial release
  - Full CRUD operations
  - Thread-safe with FreeRTOS mutexes
  - TTL support with wraparound handling
  - 6 data types supported (INT32, FLOAT, STRING, MAC, EPOCH, BOOL)
  - Math operations on numeric fields (INT32, EPOCH, FLOAT)
  - Aggregate functions
  - Memory efficient string compaction

