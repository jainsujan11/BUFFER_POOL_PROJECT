#include <iostream>
#include <cstdlib>
#include <ctime>
#include <set>
#include <string>
#include <fstream>
#include <filesystem>
#include "sqlite3.h"
#include "disk.h"
#include "buffer_mgr.h"

using namespace std;

#define NUM_RECORDS 100

// Global I/O counters (used in DiskManager/BufferPool)
extern int readCount, writeCount;

// Check if simulation.db already exists
bool fileExists(const string& filename) {
    return filesystem::exists(filename);
}

// Create a valid SQLite DB using default VFS
bool createInitialDB(const string& filename) {
    sqlite3 *db = nullptr;
    int rc = sqlite3_open_v2(filename.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Error creating DB with default VFS: " << sqlite3_errmsg(db) << endl;
        return false;
    }

    const char *sql1 = "CREATE TABLE stud_sport (roll TEXT PRIMARY KEY, sport TEXT);";
    const char *sql2 = "CREATE TABLE stud_city (roll TEXT PRIMARY KEY, city TEXT);";
    const char *sql3 = "CREATE TABLE stud_scores (roll TEXT PRIMARY KEY, score INTEGER);";


    rc = sqlite3_exec(db, sql1, nullptr, nullptr, nullptr);
    rc |= sqlite3_exec(db, sql2, nullptr, nullptr, nullptr);
    rc |= sqlite3_exec(db, sql3, nullptr, nullptr, nullptr);

    sqlite3_close(db);
    cout << "Initial DB created using default VFS." << endl;
    return rc == SQLITE_OK;
}

int main() {
    srand(time(NULL));

    const string dbFile = "simulation.db";

    // Step 1: If DB doesn't exist, create it using default VFS
    if (!fileExists(dbFile)) {
        if (!createInitialDB(dbFile)) {
            cerr << "Failed to create initial DB." << endl;
            return 1;
        }
    }

    // Step 2: Register your custom VFS
    sqlite3_os_init();  // Only if required; might already be done in your VFS source
    cout << "Custom VFS registered." << endl;

    // For Debugging: Set up SQLite logging to see errors and messages
    sqlite3_config(SQLITE_CONFIG_LOG, [](void*, int errCode, const char *msg) {
        cerr << "SQLite LOG [" << errCode << "]: " << msg << endl;
    }, nullptr);

    // Step 3: Open DB with custom VFS
    sqlite3 *db = nullptr;
    int rc = sqlite3_open_v2(dbFile.c_str(), &db, SQLITE_OPEN_READWRITE, "myvfs");
    if (rc != SQLITE_OK) {
        cerr << "Error opening database: " << sqlite3_errmsg(db) << endl;
        return 1;
    }
    cout << "Database opened using custom VFS." << endl;

    // Insert random records into stud_sport and stud_city
    cout << "Inserting records..." << endl;

    set<string> rolls;
    const char *sports[] = { "BasketBall", "Cricket", "Baseball", "Tennis", "Badminton", "TableTennis", "Swimming" };
    const char *cities[] = { "Kharagpur", "Patiala", "Sangrur", "Nabha", "Ludhiana", "Amritsar", "Gurdaspur" };
    
    char sqlInsert[256];
    int numRecords = NUM_RECORDS;

    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    for (int i = 0; i < numRecords; i++) {
        int yr = rand() % 13 + 11;
        int rollNum = rand() % 10000 + 10000;
        string roll = to_string(yr) + "CS" + to_string(rollNum);
    
        // Ensure unique roll
        if (rolls.find(roll) != rolls.end()) {
            i--; // Decrement the counter to retry
            continue;
        }
        rolls.insert(roll);
    
        // Randomly choose a sport and city
        int sportIndex = rand() % 7;
        int cityIndex = rand() % 7;
    
        // Insert into stud_sport
        char sqlInsert[256];
        sprintf(sqlInsert, "INSERT INTO stud_sport VALUES('%s', '%s');", roll.c_str(), sports[sportIndex]);
        int rc1 = sqlite3_exec(db, sqlInsert, nullptr, nullptr, nullptr);
        if (rc1 != SQLITE_OK) {
            cerr << "Insert into stud_sport failed: " << sqlite3_errmsg(db) << "\n";
        }
    
        // Insert into stud_city
        sprintf(sqlInsert, "INSERT INTO stud_city VALUES('%s', '%s');", roll.c_str(), cities[cityIndex]);
        int rc2 = sqlite3_exec(db, sqlInsert, nullptr, nullptr, nullptr);
        if (rc2 != SQLITE_OK) {
            cerr << "Insert into stud_city failed: " << sqlite3_errmsg(db) << "\n";
        }

        // Insert into stud_scores
        int score = rand() % 101;
        sprintf(sqlInsert, "INSERT INTO stud_scores VALUES('%s', %d);", roll.c_str(), score);
        int rc3 = sqlite3_exec(db, sqlInsert, nullptr, nullptr, nullptr);
        if (rc3 != SQLITE_OK) {
            cerr << "Insert into stud_scores failed: " << sqlite3_errmsg(db) << "\n";
        }
    }
    // // Debug: Verify if the records are inserted successfully
    // const char *checkSQL = "SELECT * FROM stud_sport;";
    // cout << "Checking stud_sport table:" << endl;
    // sqlite3_exec(db, checkSQL, [](void*, int argc, char **argv, char **azColName) -> int {
    //     for (int i = 0; i < argc; i++) {
    //         cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << "  ";
    //     }
    //     cout << "\n";
    //     return 0;
    // }, nullptr, nullptr);

    // const char *checkCitySQL = "SELECT * FROM stud_city;";
    // cout << "Checking stud_city table:" << endl;
    // sqlite3_exec(db, checkCitySQL, [](void*, int argc, char **argv, char **azColName) -> int {
    //     for (int i = 0; i < argc; i++) {
    //         cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << "  ";
    //     }
    //     cout << "\n";
    //     return 0;
    // }, nullptr, nullptr);

    // const char *checkScoresSQL = "SELECT * FROM stud_scores;";
    // cout << "Checking stud_scores table:" << endl;
    // sqlite3_exec(db, checkScoresSQL, [](void*, int argc, char **argv, char **azColName) -> int {
    //     for (int i = 0; i < argc; i++) {
    //         cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << "  ";
    //     }
    //     cout << "\n";
    //     return 0;
    // }, nullptr, nullptr);


    // sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    // cout << "Inserted " << rolls.size() << " records." << endl;

    // int ret = sqlite3_exec(db, "PRAGMA integrity_check;", [](void*, int argc, char** argv, char**) -> int {
    //     cout << "Integrity check: " << argv[0] << endl;
    //     return 0;
    // }, nullptr, nullptr);
    // if (ret != SQLITE_OK) {
    //     cerr << "Some Error: " << sqlite3_errmsg(db) << endl;
    // }

    // // Run join query
    // const char *joinSQL =
    //     "SELECT stud_sport.roll, stud_sport.sport, stud_city.city, stud_scores.score "
    //     "FROM stud_sport "
    //     "JOIN stud_city ON stud_sport.roll = stud_city.roll "
    //     "JOIN stud_scores ON stud_sport.roll = stud_scores.roll;";
    
    // cout << "Join query results:" << endl;
    // sqlite3_exec(db, joinSQL, [](void*, int argc, char **argv, char **azColName) -> int {
    //     for (int i = 0; i < argc; i++) {
    //         cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << "  ";
    //     }
    //     cout << "\n";
    //     return 0;
    // }, nullptr, nullptr);

    // Run SUM(score) query
    const char *sumSQL = "SELECT SUM(score) AS total_score FROM stud_scores;";
    cout << "Total sum of all scores:" << endl;
    sqlite3_exec(db, sumSQL, [](void*, int argc, char **argv, char **azColName) -> int {
        for (int i = 0; i < argc; i++) {
            cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << endl;
        }
        return 0;
    }, nullptr, nullptr);

    // Print I/O stats
    cout << "Disk reads: " << readCount << "\n";
    cout << "Disk writes: " << writeCount << "\n";

    sqlite3_close(db);
    return 0;
}