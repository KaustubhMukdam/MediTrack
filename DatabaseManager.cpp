// File: DatabaseManager.cpp

#include "DatabaseManager.h"
#include "mediTrack_ver_3.cpp" // Include definitions for Patient, Records, etc.
#include <iostream>
#include <sstream>

using namespace std;

// --- Constructor / Destructor ---
DatabaseManager::DatabaseManager(const string& filename) : db_file(filename), DB(nullptr) {}
DatabaseManager::~DatabaseManager() {
    if (DB) {
        sqlite3_close(DB);
    }
}

// --- Connection and Table Creation ---
bool DatabaseManager::open() {
    if (sqlite3_open(db_file.c_str(), &DB) != SQLITE_OK) {
        cerr << "Error opening database: " << sqlite3_errmsg(DB) << endl;
        return false;
    }
    cout << "Database opened successfully.\n";
    return true;
}

void DatabaseManager::createTables() {
    char* errMsg = 0;
    const char* sql_statements = 
        "CREATE TABLE IF NOT EXISTS patients ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, age INTEGER, contact TEXT);"

        "CREATE TABLE IF NOT EXISTS health_records ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, patient_id INTEGER, type TEXT, "
        "value1 REAL, value2 REAL, timestamp INTEGER, "
        "FOREIGN KEY(patient_id) REFERENCES patients(id) ON DELETE CASCADE);"
        
        "CREATE TABLE IF NOT EXISTS medications ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, patient_id INTEGER, name TEXT, "
        "dosage TEXT, schedule TEXT, "
        "FOREIGN KEY(patient_id) REFERENCES patients(id) ON DELETE CASCADE);"

        "CREATE TABLE IF NOT EXISTS reminders ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, patient_id INTEGER, message TEXT, "
        "date TEXT, time TEXT, "
        "FOREIGN KEY(patient_id) REFERENCES patients(id) ON DELETE CASCADE);";

    if (sqlite3_exec(DB, sql_statements, 0, 0, &errMsg) != SQLITE_OK) {
        cerr << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
    } else {
        cout << "Tables created or already exist.\n";
    }
}

// --- Saving and Loading ---
void DatabaseManager::saveAllPatients(const vector<unique_ptr<Patient>>& patients) {
    // Clear existing data to prevent duplicates
    sqlite3_exec(DB, "DELETE FROM patients;", 0, 0, 0);
    sqlite3_exec(DB, "DELETE FROM health_records;", 0, 0, 0);
    sqlite3_exec(DB, "DELETE FROM medications;", 0, 0, 0);
    sqlite3_exec(DB, "DELETE FROM reminders;", 0, 0, 0);

    sqlite3_stmt *stmt_p, *stmt_r, *stmt_m, *stmt_rem;
    const char* sql_p = "INSERT INTO patients (name, age, contact) VALUES (?, ?, ?);";
    const char* sql_r = "INSERT INTO health_records (patient_id, type, value1, value2, timestamp) VALUES (?, ?, ?, ?, ?);";
    const char* sql_m = "INSERT INTO medications (patient_id, name, dosage, schedule) VALUES (?, ?, ?, ?);";
    const char* sql_rem = "INSERT INTO reminders (patient_id, message, date, time) VALUES (?, ?, ?, ?);";

    for (const auto& patient : patients) {
        // Save Patient
        sqlite3_prepare_v2(DB, sql_p, -1, &stmt_p, 0);
        sqlite3_bind_text(stmt_p, 1, patient->getName().c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt_p, 2, patient->getAge());
        sqlite3_bind_text(stmt_p, 3, patient->getContact().c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt_p);
        sqlite3_finalize(stmt_p);
        
        long long patient_id = sqlite3_last_insert_rowid(DB);

        // Save Health Records
        for (const auto& rec : patient->getRecords()) {
            sqlite3_prepare_v2(DB, sql_r, -1, &stmt_r, 0);
            sqlite3_bind_int(stmt_r, 1, patient_id);
            sqlite3_bind_int64(stmt_r, 5, rec->getTimestamp());
            if (auto r = dynamic_cast<BloodPressureRecord*>(rec.get())) {
                sqlite3_bind_text(stmt_r, 2, "BP", -1, SQLITE_STATIC);
                sqlite3_bind_double(stmt_r, 3, r->getSystolic());
                sqlite3_bind_double(stmt_r, 4, r->getDiastolic());
            } else if (auto r = dynamic_cast<WeightRecord*>(rec.get())) {
                sqlite3_bind_text(stmt_r, 2, "Weight", -1, SQLITE_STATIC);
                sqlite3_bind_double(stmt_r, 3, r->getWeight());
            } else if (auto r = dynamic_cast<BloodSugarRecord*>(rec.get())) {
                sqlite3_bind_text(stmt_r, 2, "Sugar", -1, SQLITE_STATIC);
                sqlite3_bind_double(stmt_r, 3, r->getSugar());
            }
            sqlite3_step(stmt_r);
            sqlite3_finalize(stmt_r);
        }

        // Save Medications
        for (const auto& med : patient->getMedications()) {
            sqlite3_prepare_v2(DB, sql_m, -1, &stmt_m, 0);
            sqlite3_bind_int(stmt_m, 1, patient_id);
            sqlite3_bind_text(stmt_m, 2, med.getName().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt_m, 3, med.getDosage().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt_m, 4, med.getSchedule().c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt_m);
            sqlite3_finalize(stmt_m);
        }

        // Save Reminders
        for (const auto& rem : patient->getReminders()) {
            sqlite3_prepare_v2(DB, sql_rem, -1, &stmt_rem, 0);
            sqlite3_bind_int(stmt_rem, 1, patient_id);
            sqlite3_bind_text(stmt_rem, 2, rem.getMessage().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt_rem, 3, rem.getDate().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt_rem, 4, rem.getTime().c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt_rem);
            sqlite3_finalize(stmt_rem);
        }
    }
    cout << "All data saved to database.\n";
}

void DatabaseManager::loadPatients(vector<unique_ptr<Patient>>& patients) {
    patients.clear();
    sqlite3_stmt *stmt_p, *stmt_r, *stmt_m, *stmt_rem;
    const char* sql_p = "SELECT id, name, age, contact FROM patients;";
    
    sqlite3_prepare_v2(DB, sql_p, -1, &stmt_p, 0);
    while (sqlite3_step(stmt_p) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt_p, 0);
        string name = (const char*)sqlite3_column_text(stmt_p, 1);
        int age = sqlite3_column_int(stmt_p, 2);
        string contact = (const char*)sqlite3_column_text(stmt_p, 3);
        
        auto patient = make_unique<Patient>(name, age, contact);

        // Load Health Records
        const char* sql_r = "SELECT type, value1, value2, timestamp FROM health_records WHERE patient_id = ?;";
        sqlite3_prepare_v2(DB, sql_r, -1, &stmt_r, 0);
        sqlite3_bind_int(stmt_r, 1, id);
        while (sqlite3_step(stmt_r) == SQLITE_ROW) {
            string type = (const char*)sqlite3_column_text(stmt_r, 0);
            double val1 = sqlite3_column_double(stmt_r, 1);
            double val2 = sqlite3_column_double(stmt_r, 2);
            time_t ts = sqlite3_column_int64(stmt_r, 3);
            if (type == "BP") patient->addRecord(make_unique<BloodPressureRecord>(val1, val2, ts));
            else if (type == "Weight") patient->addRecord(make_unique<WeightRecord>(val1, ts));
            else if (type == "Sugar") patient->addRecord(make_unique<BloodSugarRecord>(val1, ts));
        }
        sqlite3_finalize(stmt_r);

        // Load Medications
        const char* sql_m = "SELECT name, dosage, schedule FROM medications WHERE patient_id = ?;";
        sqlite3_prepare_v2(DB, sql_m, -1, &stmt_m, 0);
        sqlite3_bind_int(stmt_m, 1, id);
        while(sqlite3_step(stmt_m) == SQLITE_ROW) {
            string m_name = (const char*)sqlite3_column_text(stmt_m, 0);
            string m_dosage = (const char*)sqlite3_column_text(stmt_m, 1);
            string m_schedule = (const char*)sqlite3_column_text(stmt_m, 2);
            patient->addMedication(Medication(m_name, m_dosage, m_schedule));
        }
        sqlite3_finalize(stmt_m);
        
        // Load Reminders
        const char* sql_rem = "SELECT message, date, time FROM reminders WHERE patient_id = ?;";
        sqlite3_prepare_v2(DB, sql_rem, -1, &stmt_rem, 0);
        sqlite3_bind_int(stmt_rem, 1, id);
        while(sqlite3_step(stmt_rem) == SQLITE_ROW) {
            string r_msg = (const char*)sqlite3_column_text(stmt_rem, 0);
            string r_date = (const char*)sqlite3_column_text(stmt_rem, 1);
            string r_time = (const char*)sqlite3_column_text(stmt_rem, 2);
            patient->addReminder(Reminder(r_msg, r_date, r_time));
        }
        sqlite3_finalize(stmt_rem);

        patients.push_back(move(patient));
    }
    sqlite3_finalize(stmt_p);
    cout << "Loaded " << patients.size() << " patients from database.\n";
}