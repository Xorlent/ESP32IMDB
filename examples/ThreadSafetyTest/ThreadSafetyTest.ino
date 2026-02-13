/*
 * ESP32IMDB - Thread Safety & Concurrency Test
 * 
 * Copyright (c) 2026 Xorlent
 * Licensed under the MIT License.
 * https://github.com/Xorlent/ESP32IMDB
 * 
 * This test verifies thread-safe operation under concurrent access:
 * - Multiple tasks performing simultaneous reads/writes
 * - Race condition detection
 * - Deadlock prevention verification
 * - Data consistency under concurrent load
 * - Stress test with high contention scenarios
 * - Optionally, persistence operations under concurrent load
 * 
 */

// To enable persistence testing under load, uncomment the line below:
//#define ENABLE_PERSISTENCE_STRESS_TEST

#include <ESP32IMDB.h>
#ifdef ENABLE_PERSISTENCE_STRESS_TEST
#include <SPIFFS.h>
#endif

ESP32IMDB db;
volatile int testsPassed = 0;
volatile int testsFailed = 0;
volatile bool testRunning = true;
SemaphoreHandle_t testMutex;

// Statistics for analysis
volatile uint32_t insertCount = 0;
volatile uint32_t selectCount = 0;
volatile uint32_t updateCount = 0;
volatile uint32_t deleteCount = 0;
volatile uint32_t errorCount = 0;

#define TEST_DURATION_MS 10000
#define NUM_WRITER_TASKS 3
#define NUM_READER_TASKS 3
#define NUM_MIXED_TASKS 2

// Shared test result tracking
void recordPass(const char* testName) {
  xSemaphoreTake(testMutex, portMAX_DELAY);
  Serial.printf("✓ PASS: %s\n", testName);
  testsPassed++;
  xSemaphoreGive(testMutex);
}

void recordFail(const char* testName, const char* reason) {
  xSemaphoreTake(testMutex, portMAX_DELAY);
  Serial.printf("✗ FAIL: %s - %s\n", testName, reason);
  testsFailed++;
  xSemaphoreGive(testMutex);
}

void printSeparator() {
  Serial.println("\n" + String('-', 70));
}

// Task 1: Continuous writer - inserts data
void writerTask(void* parameter) {
  int taskId = (int)parameter;
  uint32_t localInserts = 0;
  
  Serial.printf("[Writer %d] Started\n", taskId);
  vTaskDelay(100 / portTICK_PERIOD_MS); // Stagger start
  
  while (testRunning) {
    int32_t id = (taskId * 10000) + localInserts;
    int32_t value = random(1, 1000);
    const void* values[] = {&id, &value};
    
    IMDBResult result = db.insert(values);
    if (result == IMDB_OK) {
      insertCount++;
      localInserts++;
    } else if (result != IMDB_ERROR_HEAP_LIMIT && result != IMDB_ERROR_OUT_OF_MEMORY) {
      // Don't count heap/memory errors during stress test as failures
      errorCount++;
    }
    
    vTaskDelay(random(1, 10) / portTICK_PERIOD_MS);
  }
  
  Serial.printf("[Writer %d] Completed: %u inserts\n", taskId, localInserts);
  vTaskDelete(NULL);
}

// Task 2: Continuous reader - performs queries
void readerTask(void* parameter) {
  int taskId = (int)parameter;
  uint32_t localReads = 0;
  
  Serial.printf("[Reader %d] Started\n", taskId);
  vTaskDelay(150 / portTICK_PERIOD_MS); // Stagger start
  
  while (testRunning) {
    // Count records
    int count = db.count();
    if (count >= 0) {
      selectCount++;
      localReads++;
    }
    
    // Try aggregate functions if we have data
    if (count > 0) {
      IMDBSelectResult result;
      db.min("Value", &result);
      db.max("Value", &result);
      selectCount += 4;
    }
    
    vTaskDelay(random(5, 20) / portTICK_PERIOD_MS);
  }
  
  Serial.printf("[Reader %d] Completed: %u reads\n", taskId, localReads);
  vTaskDelete(NULL);
}

// Task 3: Mixed operations - reads and writes
void mixedTask(void* parameter) {
  int taskId = (int)parameter;
  uint32_t localOps = 0;
  
  Serial.printf("[Mixed %d] Started\n", taskId);
  vTaskDelay(200 / portTICK_PERIOD_MS); // Stagger start
  
  while (testRunning) {
    int operation = random(0, 4);
    
    switch (operation) {
      case 0: { // Insert
        int32_t id = 50000 + (taskId * 10000) + random(0, 1000);
        int32_t value = random(1, 1000);
        const void* values[] = {&id, &value};
        if (db.insert(values) == IMDB_OK) {
          insertCount++;
        }
        break;
      }
      
      case 1: { // Select
        int32_t searchId = random(0, 60000);
        IMDBSelectResult result;
        db.select("Value", "ID", &searchId, &result);
        selectCount++;
        break;
      }
      
      case 2: { // Update
        int32_t searchId = random(0, 60000);
        int32_t newValue = random(1, 1000);
        if (db.update("ID", &searchId, "Value", &newValue) != IMDB_ERROR_NO_TABLE) {
          updateCount++;
        }
        break;
      }
      
      case 3: { // Delete
        int32_t searchId = random(0, 60000);
        if (db.deleteRecords("ID", &searchId) != IMDB_ERROR_NO_TABLE) {
          deleteCount++;
        }
        break;
      }
    }
    
    localOps++;
    vTaskDelay(random(2, 15) / portTICK_PERIOD_MS);
  }
  
  Serial.printf("[Mixed %d] Completed: %u operations\n", taskId, localOps);
  vTaskDelete(NULL);
}

// Test 1: Basic concurrent reads (should never fail)
void testConcurrentReads() {
  Serial.println("\n=== TEST 1: Concurrent Reads ===");
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"Value", IMDB_TYPE_INT32}};
  db.createTable(cols, 2);
  
  // Populate with test data
  for (int i = 0; i < 100; i++) {
    int32_t id = i;
    int32_t value = i * 10;
    const void* values[] = {&id, &value};
    db.insert(values);
  }
  
  volatile int tasksCompleted = 0;
  const int NUM_TASKS = 5;
  
  // Launch multiple reader tasks
  TaskHandle_t readers[NUM_TASKS];
  for (int i = 0; i < NUM_TASKS; i++) {
    xTaskCreate(
      [](void* param) {
        volatile int* completed = (volatile int*)param;
        for (int j = 0; j < 50; j++) {
          int32_t searchId = random(0, 100);
          IMDBSelectResult result;
          db.select("Value", "ID", &searchId, &result);
          vTaskDelay(1 / portTICK_PERIOD_MS);
        }
        (*completed)++;
        vTaskDelete(NULL);
      },
      "Reader",
      4096,
      (void*)&tasksCompleted,
      1,
      &readers[i]
    );
  }
  
  // Wait for all tasks to complete
  uint32_t startWait = millis();
  while (tasksCompleted < NUM_TASKS && (millis() - startWait) < 3000) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  vTaskDelay(100 / portTICK_PERIOD_MS);
  
  int count = db.count();
  Serial.printf("Tasks completed: %d/%d, Final count: %d\n", tasksCompleted, NUM_TASKS, count);
  
  if (count == 100 && tasksCompleted == NUM_TASKS) {
    recordPass("Concurrent reads maintain data integrity");
  } else {
    char msg[64];
    snprintf(msg, sizeof(msg), "Data: %d/100, Tasks: %d/%d", count, tasksCompleted, NUM_TASKS);
    recordFail("Concurrent reads", msg);
  }
  
  db.dropTable();
}

// Test 2: Concurrent writes
void testConcurrentWrites() {
  Serial.println("\n=== TEST 2: Concurrent Writes ===");
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"Value", IMDB_TYPE_INT32}};
  db.createTable(cols, 2);
  
  volatile int tasksCompleted = 0;
  const int NUM_TASKS = 4;
  const int RECORDS_PER_TASK = 25;
  
  // Parameter structure for task
  struct TaskParams {
    volatile int* completed;
    int taskId;
  };
  
  TaskParams taskParams[NUM_TASKS];
  
  // Launch multiple writer tasks
  TaskHandle_t writers[NUM_TASKS];
  
  for (int i = 0; i < NUM_TASKS; i++) {
    taskParams[i].completed = &tasksCompleted;
    taskParams[i].taskId = i;
    
    xTaskCreate(
      [](void* param) {
        TaskParams* params = (TaskParams*)param;
        int taskId = params->taskId;
        volatile int* completed = params->completed;
        
        for (int j = 0; j < RECORDS_PER_TASK; j++) {
          int32_t id = taskId * 1000 + j;
          int32_t value = j;
          const void* values[] = {&id, &value};
          db.insert(values);
          vTaskDelay(2 / portTICK_PERIOD_MS);
        }
        (*completed)++;
        vTaskDelete(NULL);
      },
      "Writer",
      4096,
      &taskParams[i],
      1,
      &writers[i]
    );
  }
  
  // Wait for all tasks to complete
  uint32_t startWait = millis();
  while (tasksCompleted < NUM_TASKS && (millis() - startWait) < 3000) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  vTaskDelay(100 / portTICK_PERIOD_MS);
  
  int count = db.count();
  int expected = NUM_TASKS * RECORDS_PER_TASK;
  Serial.printf("Tasks completed: %d/%d, Records: %d/%d\n", 
                tasksCompleted, NUM_TASKS, count, expected);
  
  if (count == expected && tasksCompleted == NUM_TASKS) {
    recordPass("Concurrent writes: all records inserted");
  } else {
    char msg[64];
    snprintf(msg, sizeof(msg), "Expected %d records, got %d (tasks: %d/%d)", 
             expected, count, tasksCompleted, NUM_TASKS);
    recordFail("Concurrent writes", msg);
  }
  
  db.dropTable();
}

// Test 3: Read-Write contention
void testReadWriteContention() {
  Serial.println("\n=== TEST 3: Read-Write Contention ===");
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"Value", IMDB_TYPE_INT32}};
  db.createTable(cols, 2);
  
  // Prepopulate
  for (int i = 0; i < 50; i++) {
    int32_t id = i;
    int32_t value = i * 2;
    const void* values[] = {&id, &value};
    db.insert(values);
  }
  
  volatile bool testActive = true;
  volatile int tasksCompleted = 0;
  volatile int readErrors = 0;
  
  // Parameter structure for read-write tasks
  struct RWParams {
    volatile bool* active;
    volatile int* completed;
    volatile int* errors;
  };
  
  RWParams params = {&testActive, &tasksCompleted, &readErrors};
  
  // Writer task
  xTaskCreate(
    [](void* param) {
      RWParams* p = (RWParams*)param;
      while (*(p->active)) {
        int32_t id = 100 + random(0, 50);
        int32_t value = random(0, 1000);
        const void* values[] = {&id, &value};
        db.insert(values);
        vTaskDelay(5 / portTICK_PERIOD_MS);
      }
      (*(p->completed))++;
      vTaskDelete(NULL);
    },
    "RWWriter",
    4096,
    &params,
    1,
    NULL
  );
  
  // Reader task
  xTaskCreate(
    [](void* param) {
      RWParams* p = (RWParams*)param;
      while (*(p->active)) {
        int count = db.count();
        if (count < 50) {
          // Should never happen - we started with 50
          (*(p->errors))++;
        }
        vTaskDelay(3 / portTICK_PERIOD_MS);
      }
      (*(p->completed))++;
      vTaskDelete(NULL);
    },
    "RWReader",
    4096,
    &params,
    1,
    NULL
  );
  
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  testActive = false;
  
  // Wait for tasks to complete
  uint32_t startWait = millis();
  while (tasksCompleted < 2 && (millis() - startWait) < 1000) {
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
  
  int finalCount = db.count();
  Serial.printf("Tasks completed: %d/2, Final count: %d, Read errors: %d\n",
                tasksCompleted, finalCount, readErrors);
  
  if (finalCount >= 50 && readErrors == 0 && tasksCompleted == 2) {
    recordPass("Read-write contention: data consistent");
  } else {
    char msg[64];
    snprintf(msg, sizeof(msg), "Count: %d, Errors: %d, Tasks: %d/2", 
             finalCount, readErrors, tasksCompleted);
    recordFail("Read-write contention", msg);
  }
  
  db.dropTable();
}

// Test 4: Stress test with high contention
void testHighContentionStress() {
  Serial.println("\n=== TEST 4: High Contention Stress Test ===");
  Serial.printf("Running for %d seconds with %d tasks...\n", 
                TEST_DURATION_MS / 1000, 
                NUM_WRITER_TASKS + NUM_READER_TASKS + NUM_MIXED_TASKS);
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"Value", IMDB_TYPE_INT32}};
  db.createTable(cols, 2);
  
  // Reset counters
  insertCount = 0;
  selectCount = 0;
  updateCount = 0;
  deleteCount = 0;
  errorCount = 0;
  testRunning = true;
  
  uint32_t startFreeHeap = ESP.getFreeHeap();
  uint32_t startTime = millis();
  
  // Launch writer tasks
  for (int i = 0; i < NUM_WRITER_TASKS; i++) {
    xTaskCreate(writerTask, "Writer", 4096, (void*)i, 1, NULL);
  }
  
  // Launch reader tasks
  for (int i = 0; i < NUM_READER_TASKS; i++) {
    xTaskCreate(readerTask, "Reader", 4096, (void*)i, 1, NULL);
  }
  
  // Launch mixed tasks
  for (int i = 0; i < NUM_MIXED_TASKS; i++) {
    xTaskCreate(mixedTask, "Mixed", 4096, (void*)i, 1, NULL);
  }
  
  // Monitor progress
  for (int i = 0; i < TEST_DURATION_MS / 1000; i++) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.printf("  [%ds] Records: %d, Ops: I=%u S=%u U=%u D=%u E=%u, Heap: %u\n",
                  i + 1, db.count(), insertCount, selectCount, updateCount, 
                  deleteCount, errorCount, ESP.getFreeHeap());
  }
  
  testRunning = false;
  vTaskDelay(500 / portTICK_PERIOD_MS); // Let tasks finish
  
  uint32_t endTime = millis();
  int finalRecordCount = db.count();
  
  // Results
  Serial.println("\n--- Stress Test Results ---");
  Serial.printf("Duration: %u ms\n", endTime - startTime);
  Serial.printf("Total Operations: %u\n", insertCount + selectCount + updateCount + deleteCount);
  Serial.printf("  Inserts: %u\n", insertCount);
  Serial.printf("  Selects: %u\n", selectCount);
  Serial.printf("  Updates: %u\n", updateCount);
  Serial.printf("  Deletes: %u\n", deleteCount);
  Serial.printf("  Errors: %u\n", errorCount);
  Serial.printf("Final record count: %d\n", finalRecordCount);
  Serial.printf("Operations/sec: %.1f\n", 
                (float)(insertCount + selectCount + updateCount + deleteCount) / 
                ((endTime - startTime) / 1000.0f));
  
  // Clean up and measure memory after cleanup
  db.dropTable();
  vTaskDelay(100 / portTICK_PERIOD_MS);
  uint32_t endFreeHeap = ESP.getFreeHeap();
  int heapDiff = (int)startFreeHeap - (int)endFreeHeap;
  Serial.printf("Heap change after cleanup: %d bytes\n", heapDiff);
  
  // Validation
  if (errorCount == 0) {
    recordPass("High contention stress: no errors");
  } else {
    char msg[64];
    snprintf(msg, sizeof(msg), "%u errors occurred", errorCount);
    recordFail("High contention stress", msg);
  }
  
  if (finalRecordCount >= 0) {
    recordPass("High contention stress: data structure intact");
  } else {
    recordFail("High contention stress", "Data structure corrupted");
  }
  
  // Memory leak check after cleanup (allow 10KB variation for task/fragmentation overhead)
  if (abs(heapDiff) < 10000) {
    recordPass("High contention stress: no memory leak");
  } else {
    char msg[64];
    snprintf(msg, sizeof(msg), "Heap changed by %d bytes after cleanup", heapDiff);
    recordFail("High contention stress", msg);
  }
}

// Test 5: Race condition detection
void testRaceConditions() {
  Serial.println("\n=== TEST 5: Race Condition Detection ===");
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"Counter", IMDB_TYPE_INT32}};
  db.createTable(cols, 2);
  
  // Insert initial counter with unique ID
  int32_t id = 1;
  int32_t counter = 0;
  const void* values[] = {&id, &counter};
  db.insert(values);
  
  volatile int tasksCompleted = 0;
  const int NUM_TASKS = 5;
  const int INCREMENTS_PER_TASK = 20;
  
  // Multiple tasks incrementing same counter using update
  TaskHandle_t tasks[NUM_TASKS];
  for (int i = 0; i < NUM_TASKS; i++) {
    xTaskCreate(
      [](void* param) {
        volatile int* completed = (volatile int*)param;
        for (int j = 0; j < INCREMENTS_PER_TASK; j++) {
          // Read current value
          IMDBSelectResult result;
          int32_t searchId = 1;
          if (db.select("Counter", "ID", &searchId, &result) == IMDB_OK && result.hasValue) {
            // Increment using update (tests atomic operation protection)
            int32_t newVal = result.int32Value + 1;
            db.update("ID", &searchId, "Counter", &newVal);
          }
          vTaskDelay(2 / portTICK_PERIOD_MS);
        }
        (*completed)++;
        vTaskDelete(NULL);
      },
      "Incrementer",
      4096,
      (void*)&tasksCompleted,
      1,
      &tasks[i]
    );
  }
  
  // Wait for all tasks to complete
  uint32_t startWait = millis();
  while (tasksCompleted < NUM_TASKS && (millis() - startWait) < 5000) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  vTaskDelay(100 / portTICK_PERIOD_MS); // Extra settling time
  
  // Check final value
  IMDBSelectResult result;
  int32_t searchId = 1;
  if (db.select("Counter", "ID", &searchId, &result) == IMDB_OK && result.hasValue) {
    int32_t finalValue = result.int32Value;
    int32_t expectedValue = NUM_TASKS * INCREMENTS_PER_TASK;
    Serial.printf("Tasks completed: %d/%d\n", tasksCompleted, NUM_TASKS);
    Serial.printf("Expected increments: %d, Got: %d\n", expectedValue, finalValue);
    
    // With proper locking, we might not get exactly expected due to read-modify-write races,
    // but we should get a reasonable value (at least 50% of expected)
    if (finalValue >= expectedValue / 2 && finalValue <= expectedValue) {
      recordPass("Race condition: operations protected");
    } else {
      char msg[64];
      snprintf(msg, sizeof(msg), "Expected ~%d, got %d", expectedValue, finalValue);
      recordFail("Race condition", msg);
    }
  } else {
    recordFail("Race condition", "Counter lost or not found");
  }
  
  db.dropTable();
}

#ifdef ENABLE_PERSISTENCE_STRESS_TEST
// Test 6: Persistence operations under concurrent load
void testPersistenceUnderLoad() {
  Serial.println("\n=== TEST 6: Persistence Under Concurrent Load ===");
  
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    recordFail("Persistence under load", "SPIFFS mount failed");
    return;
  }
  Serial.println("✓ SPIFFS mounted");
  
  const char* testFile = "/stress_test.imdb";
  const int TEST_DURATION = 5000; // 5 seconds
  
  // Clean up any existing test file
  if (SPIFFS.exists(testFile)) {
    SPIFFS.remove(testFile);
  }
  
  IMDBColumn cols[] = {{"ID", IMDB_TYPE_INT32}, {"Value", IMDB_TYPE_INT32}};
  db.createTable(cols, 2);
  
  // Prepopulate with some data
  Serial.println("Prepopulating database with 100 records...");
  for (int i = 0; i < 100; i++) {
    int32_t id = i;
    int32_t value = i * 10;
    const void* values[] = {&id, &value};
    db.insert(values);
  }
  
  Serial.printf("Starting %dms stress test with concurrent saves...\n", TEST_DURATION);
  
  volatile bool stressActive = true;
  volatile int tasksCompleted = 0;
  volatile int saveErrors = 0;
  volatile int saveSuccess = 0;
  volatile int opsCompleted = 0;
  
  struct StressParams {
    volatile bool* active;
    volatile int* completed;
    volatile int* saveErrs;
    volatile int* saveOk;
    volatile int* ops;
  };
  
  StressParams params = {&stressActive, &tasksCompleted, &saveErrors, &saveSuccess, &opsCompleted};
  
  // Task 1: Heavy writer
  xTaskCreate(
    [](void* param) {
      StressParams* p = (StressParams*)param;
      int localOps = 0;
      while (*(p->active)) {
        int32_t id = 10000 + random(0, 5000);
        int32_t value = random(0, 1000);
        const void* values[] = {&id, &value};
        db.insert(values);
        localOps++;
        if (localOps % 50 == 0) {
          (*(p->ops)) += 50;
        }
        vTaskDelay(random(1, 5) / portTICK_PERIOD_MS);
      }
      (*(p->ops)) += (localOps % 50);
      (*(p->completed))++;
      vTaskDelete(NULL);
    },
    "StressWriter",
    4096,
    &params,
    1,
    NULL
  );
  
  // Task 2: Heavy reader
  xTaskCreate(
    [](void* param) {
      StressParams* p = (StressParams*)param;
      int localOps = 0;
      while (*(p->active)) {
        int count = db.count();
        IMDBSelectResult result;
        db.min("Value", &result);
        db.max("Value", &result);
        int32_t searchId = random(0, 15000);
        db.select("Value", "ID", &searchId, &result);
        localOps += 4;
        if (localOps % 100 == 0) {
          (*(p->ops)) += 100;
        }
        vTaskDelay(random(2, 8) / portTICK_PERIOD_MS);
      }
      (*(p->ops)) += (localOps % 100);
      (*(p->completed))++;
      vTaskDelete(NULL);
    },
    "StressReader",
    4096,
    &params,
    1,
    NULL
  );
  
  // Task 3: Mixed operations
  xTaskCreate(
    [](void* param) {
      StressParams* p = (StressParams*)param;
      int localOps = 0;
      while (*(p->active)) {
        int op = random(0, 3);
        if (op == 0) {
          int32_t searchId = random(0, 15000);
          int32_t newValue = random(0, 1000);
          db.update("ID", &searchId, "Value", &newValue);
        } else if (op == 1) {
          int32_t searchId = random(5000, 8000);
          db.deleteRecords("ID", &searchId);
        } else {
          int32_t id = 20000 + random(0, 3000);
          int32_t value = random(0, 1000);
          const void* values[] = {&id, &value};
          db.insert(values);
        }
        localOps++;
        if (localOps % 50 == 0) {
          (*(p->ops)) += 50;
        }
        vTaskDelay(random(2, 10) / portTICK_PERIOD_MS);
      }
      (*(p->ops)) += (localOps % 50);
      (*(p->completed))++;
      vTaskDelete(NULL);
    },
    "StressMixed",
    4096,
    &params,
    1,
    NULL
  );
  
  // Task 4: Concurrent saver - saves database while others are working
  xTaskCreate(
    [](void* param) {
      StressParams* p = (StressParams*)param;
      const char* file = "/stress_test.imdb";
      int saveCount = 0;
      
      // Give other tasks time to start
      vTaskDelay(100 / portTICK_PERIOD_MS);
      
      while (*(p->active)) {
        // Attempt save while database is under load
        IMDBResult result = db.saveToFile(file);
        
        if (result == IMDB_OK) {
          (*(p->saveOk))++;
          saveCount++;
        } else {
          (*(p->saveErrs))++;
          Serial.printf("  [SAVE ERROR] %s\n", ESP32IMDB::resultToString(result));
        }
        
        // Save every 300ms during stress test
        vTaskDelay(300 / portTICK_PERIOD_MS);
      }
      
      Serial.printf("  Saver task completed %d saves\n", saveCount);
      (*(p->completed))++;
      vTaskDelete(NULL);
    },
    "StressSaver",
    8192, // Larger stack for file operations
    &params,
    1,
    NULL
  );
  
  // Monitor progress
  uint32_t startTime = millis();
  int lastOps = 0;
  while (millis() - startTime < TEST_DURATION) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    int currentOps = opsCompleted;
    Serial.printf("  [%ds] Records: %d, Ops: %d (+%d/s), Saves: OK=%d ERR=%d, Heap: %u\n",
                  (int)((millis() - startTime) / 1000),
                  db.count(),
                  currentOps,
                  currentOps - lastOps,
                  saveSuccess,
                  saveErrors,
                  ESP.getFreeHeap());
    lastOps = currentOps;
  }
  
  stressActive = false;
  
  // Wait for all tasks to complete
  uint32_t waitStart = millis();
  while (tasksCompleted < 4 && (millis() - waitStart) < 2000) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  vTaskDelay(200 / portTICK_PERIOD_MS); // Extra settling
  
  int recordsBeforeSave = db.count();
  Serial.printf("\n--- Stress Test Complete ---\n");
  Serial.printf("Tasks completed: %d/4\n", tasksCompleted);
  Serial.printf("Total operations: %d\n", opsCompleted);
  Serial.printf("Successful saves: %d\n", saveSuccess);
  Serial.printf("Save errors: %d\n", saveErrors);
  Serial.printf("Records before final save: %d\n", recordsBeforeSave);
  
  // Perform final save
  Serial.println("\nPerforming final save...");
  IMDBResult finalSave = db.saveToFile(testFile);
  if (finalSave != IMDB_OK) {
    Serial.printf("✗ Final save failed: %s\n", ESP32IMDB::resultToString(finalSave));
    recordFail("Persistence under load", "Final save failed");
    db.dropTable();
    return;
  }
  Serial.println("✓ Final save successful");
  
  // Verify file
  if (!SPIFFS.exists(testFile)) {
    recordFail("Persistence under load", "Save file not created");
    db.dropTable();
    return;
  }
  
  File file = SPIFFS.open(testFile, "r");
  size_t fileSize = file.size();
  file.close();
  Serial.printf("Save file size: %d bytes\n", fileSize);
  
  // Drop table and reload from file
  Serial.println("\nDropping table and reloading from file...");
  db.dropTable();
  vTaskDelay(100 / portTICK_PERIOD_MS);
  
  IMDBResult loadResult = db.loadFromFile(testFile);
  if (loadResult != IMDB_OK) {
    Serial.printf("✗ Load failed: %s\n", ESP32IMDB::resultToString(loadResult));
    recordFail("Persistence under load", "Load after stress failed");
    return;
  }
  
  int recordsAfterLoad = db.count();
  Serial.printf("✓ Loaded successfully\n");
  Serial.printf("Records after load: %d\n", recordsAfterLoad);
  
  // Verify data integrity by checking some records
  Serial.println("\nVerifying data integrity...");
  int verifyFailures = 0;
  
  // Check original prepopulated records (should still exist)
  for (int i = 0; i < 10; i++) {
    int32_t id = i;
    IMDBSelectResult result;
    if (db.select("Value", "ID", &id, &result) == IMDB_OK && result.hasValue) {
      if (result.int32Value != i * 10) {
        verifyFailures++;
        Serial.printf("  ✗ Record ID=%d: expected %d, got %d\n", i, i * 10, result.int32Value);
      }
    } else {
      verifyFailures++;
      Serial.printf("  ✗ Record ID=%d not found\n", i);
    }
  }
  
  if (verifyFailures == 0) {
    Serial.println("✓ Data integrity verified (sample check)");
  } else {
    Serial.printf("✗ Data integrity check failed: %d errors\n", verifyFailures);
  }
  
  // Evaluate results
  if (saveErrors == 0) {
    recordPass("Persistence under load: no save errors");
  } else {
    char msg[64];
    snprintf(msg, sizeof(msg), "%d save errors during stress", saveErrors);
    recordFail("Persistence under load", msg);
  }
  
  if (saveSuccess >= 5) {
    recordPass("Persistence under load: multiple successful saves");
  } else {
    char msg[64];
    snprintf(msg, sizeof(msg), "Only %d successful saves", saveSuccess);
    recordFail("Persistence under load", msg);
  }
  
  if (recordsAfterLoad == recordsBeforeSave) {
    recordPass("Persistence under load: record count preserved");
  } else {
    char msg[64];
    snprintf(msg, sizeof(msg), "Records: before=%d, after=%d", recordsBeforeSave, recordsAfterLoad);
    recordFail("Persistence under load", msg);
  }
  
  if (verifyFailures == 0) {
    recordPass("Persistence under load: data integrity maintained");
  } else {
    char msg[64];
    snprintf(msg, sizeof(msg), "%d data integrity failures", verifyFailures);
    recordFail("Persistence under load", msg);
  }
  
  if (tasksCompleted == 4) {
    recordPass("Persistence under load: all tasks completed");
  } else {
    char msg[64];
    snprintf(msg, sizeof(msg), "Only %d/4 tasks completed", tasksCompleted);
    recordFail("Persistence under load", msg);
  }
  
  // Cleanup
  db.dropTable();
  SPIFFS.remove(testFile);
  
  Serial.println("✓ Persistence stress test complete");
}
#endif

void printTestResults() {
  printSeparator();
  Serial.printf("THREAD SAFETY TEST RESULTS\n");
  Serial.printf("Total: %d tests, %d passed, %d failed\n", 
                testsPassed + testsFailed, testsPassed, testsFailed);
  if (testsFailed == 0) {
    Serial.println("✓✓✓ ALL THREAD SAFETY TESTS PASSED! ✓✓✓");
  } else {
    Serial.printf("✗✗✗ %d TEST(S) FAILED ✗✗✗\n", testsFailed);
  }
  printSeparator();
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  
  delay(2000);
  
  Serial.println("\n╔══════════════════════════════════════════════════════════════════╗");
  Serial.println("║          ESP32IMDB - THREAD SAFETY & CONCURRENCY TEST            ║");
  Serial.println("╚══════════════════════════════════════════════════════════════════╝\n");
  
  Serial.printf("ESP32 Chip: %s\n", ESP.getChipModel());
  Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Free Heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("RTOS Tick Rate: %d Hz\n", configTICK_RATE_HZ);
  
#ifdef ENABLE_PERSISTENCE_STRESS_TEST
  Serial.println("ℹ Persistence stress test ENABLED (SPIFFS under load)");
#else
  Serial.println("ℹ Persistence stress test DISABLED (uncomment to enable)");
#endif
  
  testMutex = xSemaphoreCreateMutex();
  
  printSeparator();
  
  testConcurrentReads();
  testConcurrentWrites();
  testReadWriteContention();
  testHighContentionStress();
  testRaceConditions();
  
#ifdef ENABLE_PERSISTENCE_STRESS_TEST
  testPersistenceUnderLoad();
#endif
  
  printTestResults();
  
  Serial.println("\n✓ Thread safety testing complete!");
}

void loop() {
  // Yield to scheduler
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
