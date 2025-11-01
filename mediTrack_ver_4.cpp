#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <memory>
#include <limits>
#include <typeinfo>
#include <sstream>
#include "sqlite3.h"

using namespace std;


// --- Forward Declarations ---
class Patient;
class DatabaseManager;

void clearInputBuffer();
void addNewPatient(vector<unique_ptr<Patient>>& patients);
void listAllPatients(const vector<unique_ptr<Patient>>& patients);
void patientSubMenu(Patient* patient);
void selectPatient(vector<unique_ptr<Patient>>& patients);

// --- SQLite Database ---
class DatabaseManager {
private:
    sqlite3* DB;
    string db_file;
public:
    DatabaseManager(const string& filename) : db_file(filename), DB(nullptr) {}
    ~DatabaseManager() {
        if (DB) sqlite3_close(DB);
    }
    bool open() {
        if (sqlite3_open(db_file.c_str(), &DB) != SQLITE_OK) {
            cerr << "Error opening database: " << sqlite3_errmsg(DB) << endl;
            return false;
        }
        cout << "Database opened successfully.\n";
        return true;
    }
    void createTables() {
        char* errMsg = 0;
        const char* sql = 
            "PRAGMA foreign_keys = ON;"
            "CREATE TABLE IF NOT EXISTS patients ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT NOT NULL, age INTEGER, contact TEXT);"
            "CREATE TABLE IF NOT EXISTS health_records ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, patient_id INTEGER, type TEXT,"
            "value1 REAL, value2 REAL, timestamp INTEGER,"
            "FOREIGN KEY(patient_id) REFERENCES patients(id) ON DELETE CASCADE);"
            "CREATE TABLE IF NOT EXISTS reminders ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, patient_id INTEGER, message TEXT,"
            "date TEXT, time TEXT,"
            "FOREIGN KEY(patient_id) REFERENCES patients(id) ON DELETE CASCADE);";
        if (sqlite3_exec(DB, sql, 0, 0, &errMsg) != SQLITE_OK) {
            cerr << "SQL error: " << errMsg << endl;
            sqlite3_free(errMsg);
        } else {
            cout << "Tables created or already exist.\n";
        }
    }
    // ...save and load function signatures for step 2/3!
    void saveAllPatients(const vector<unique_ptr<Patient>>& patients);
    void loadPatients(vector<unique_ptr<Patient>>& patients);
};

// --- Health Base Class ---
class HealthRecord {
protected:
    time_t timestamp;
public:
    HealthRecord() : timestamp(time(0)) {}
    HealthRecord(time_t loaded_time) : timestamp(loaded_time) {}
    virtual void display() const = 0;
    virtual ~HealthRecord() = default;
    virtual void updateBloodPressure(int systolic, int diastolic) {}
    virtual void updateWeight(double weight) {}
    virtual void displayTypeAndValue() const = 0;
    string getFormattedTimestamp() const {
        char buffer[20];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", localtime(&timestamp));
        return string(buffer);
    }
    time_t getTimestamp() const { return timestamp; }
};

// --- Derived classes
class BloodPressureRecord : public HealthRecord {
    int systolic, diastolic;
public:
    BloodPressureRecord(int s, int d, time_t ts = time(0)) : HealthRecord(ts), systolic(s), diastolic(d) {}
    int getSystolic() const { return systolic; }
    int getDiastolic() const { return diastolic; }

    void display() const override {
        cout << getFormattedTimestamp() << " - Blood Pressure: " << systolic << "/" << diastolic << " mmHg\n";
    }
    void displayTypeAndValue() const override {
        cout << "Blood Pressure: " << systolic << "/" << diastolic << " mmHg\n";
    }
    void updateBloodPressure(int s, int d) override { systolic = s; diastolic = d; }
};

class WeightRecord : public HealthRecord {
    double weight;
public:
    WeightRecord(double w, time_t ts = time(0)) : HealthRecord(ts), weight(w) {}
    void display() const override {
        cout << getFormattedTimestamp() << " - Weight: " << weight << " kg\n";
    }
    void displayTypeAndValue() const override {
        cout << "Weight: " << weight << " kg\n";
    }
    void updateWeight(double w) override { weight = w; }
    double getWeight() const { return weight; }
};

class BloodSugarRecord : public HealthRecord {
    int sugar;
public:
    BloodSugarRecord(int s, time_t ts = time(0)) : HealthRecord(ts), sugar(s) {}
    int getSugar() const { return sugar; }
    void display() const override {
        cout << getFormattedTimestamp() << " - Blood Sugar: " << sugar << " mg/dL\n";
    }
    void displayTypeAndValue() const override {
        cout << "Blood Sugar: " << sugar << " mg/dL\n";
    }
};

// Simple Reminder class
class Reminder {
    string message, date, time;
public:
    Reminder(const string& m, const string& d, const string& t)
        : message(m), date(d), time(t) {}
    string getMessage() const { return message; }
    string getDate() const { return date; }
    string getTime() const { return time; }
};

// --- Patient Base Class ---
class Patient {
    string name, contact;
    int age;
    vector<unique_ptr<HealthRecord>> records;
    vector<Reminder> reminders;
public:
    Patient(const string& n, int a, const string& c)
        : name(n), age(a), contact(c) {}

    string getName() const { return name; }
    int getAge() const { return age; }
    string getContact() const { return contact; }

    const std::vector<Reminder>& getReminders() const { return reminders; }

    void addRecord(unique_ptr<HealthRecord> rec) {
        records.push_back(move(rec));
    }
    int numRecords() const { return records.size(); }
    HealthRecord* getRecordByIndex(int idx) {
        return (idx >= 0 && idx < records.size()) ? records[idx].get() : nullptr;
    }
    void display() const {
        cout << "\n--- Patient Details ---\n";
        cout << "Name: " << name << "\nAge: " << age << "\nContact: " << contact << "\n";
        displayRecordsWithIndices();
        checkReminders();
        cout << "----------------------\n";
    }
    void displayRecordsWithIndices() const;
    void checkReminders() const;
    void addReminder(const Reminder& rem) {
        reminders.push_back(rem);
    }
    // BMI calculation (step 5)
    void calculateAndDisplayBMI() const;
    // Health trend (step 5)
    void displayHealthTrend() const;
};

// --- Derived Classes
void Patient::displayRecordsWithIndices() const {
    if (records.empty()) {
        cout << "No records.\n";
        return;
    }
    for (size_t i = 0; i < records.size(); ++i) {
        cout << i + 1 << ". ";
        records[i]->display();  // nicely timestamped per record type
    }
}

void Patient::checkReminders() const {
    if (reminders.empty()) {
        cout << "No reminders set.\n";
        return;
    }
    cout << "\n=== Medication/Health Reminders ===\n";
    for (const auto& rem : reminders) {
        cout << rem.getDate() << " " << rem.getTime() << " - " << rem.getMessage() << "\n";
    }
}

void Patient::calculateAndDisplayBMI() const {
    double lastWeight = -1;
    for (auto it = records.rbegin(); it != records.rend(); ++it) {
        WeightRecord* wrec = dynamic_cast<WeightRecord*>(it->get());
        if (wrec) {
            lastWeight = wrec->getWeight();
            break;
        }
    }
    if (lastWeight == -1) {
        cout << "No weight record available for BMI.\n";
        return;
    }
    cout << "Please enter patient's height in meters (e.g., 1.75): ";
    double height;
    cin >> height;
    if (cin.fail() || height < 0.5 || height > 2.5) {
        cout << "Invalid height. Cannot calculate BMI. Enter meters (like 1.75).\n";
        cin.clear(); clearInputBuffer();
        return;
    }
    double bmi = lastWeight / (height * height);
    cout << "Calculated BMI is: " << bmi << endl;
    if (bmi < 18.5) cout << "Category: Underweight\n";
    else if (bmi < 25) cout << "Category: Normal weight\n";
    else if (bmi < 30) cout << "Category: Overweight\n";
    else cout << "Category: Obese\n";
}

void Patient::displayHealthTrend() const {
    cout << "\n--- View Health Trends for " << name << " ---\n";
    cout << "1. Blood Pressure Trend\n";
    cout << "2. Weight Trend\n";
    cout << "3. Blood Sugar Trend\n";
    cout << "Enter your choice: ";
    int choice;
    cin >> choice;
    if (cin.fail()) { cin.clear(); clearInputBuffer(); cout << "Invalid input.\n"; return;}
    cout << "--- Trend Report ---\n";
    bool found = false;
    switch (choice) {
        case 1:
            for (const auto &rec : records)
                if (dynamic_cast<BloodPressureRecord*>(rec.get())) { rec->display(); found = true; }
            break;
        case 2:
            for (const auto &rec : records)
                if (dynamic_cast<WeightRecord*>(rec.get())) { rec->display(); found = true; }
            break;
        case 3:
            for (const auto &rec : records)
                if (dynamic_cast<BloodSugarRecord*>(rec.get())) { rec->display(); found = true; }
            break;
        default: cout << "Invalid choice.\n"; return;
    }
    if (!found) cout << "No records of that type found.\n";
    cout << "--------------------\n";
}

// --- Profile Base Class ---
class Profile {
public:
    virtual void start(vector<unique_ptr<Patient>>& patients) = 0;
    virtual ~Profile() = default;
};

// --- Derived Classes ---
class DoctorProfile : public Profile {
public:
    void start(vector<unique_ptr<Patient>>& patients) override {
        int choice;
        do {
            cout << "\n===== Doctor Menu =====\n";
            cout << "1. Add New Patient\n";
            cout << "2. Select Patient\n";
            cout << "3. List All Patients\n";
            cout << "4. Save and Exit\n";
            cout << "Enter your choice: ";
            cin >> choice;
            if (cin.fail()) {
                cin.clear(); clearInputBuffer(); choice = 0; continue;
            }
            switch (choice) {
                case 1: addNewPatient(patients); break;
                case 2: selectPatient(patients); break;
                case 3: listAllPatients(patients); break;
                case 4: cout << "Exiting Doctor Mode. Goodbye!\n"; break;
                default: cout << "Invalid choice. Please try again.\n";
            }
        } while (choice != 4);
    }
};

class UserProfile : public Profile {
public:
    void start(vector<unique_ptr<Patient>>& patients) override {
        string userName;
        cout << "\nEnter your name: ";
        clearInputBuffer();
        getline(cin, userName);

        Patient* user = nullptr;
        for (auto& p : patients) {
            if (p->getName() == userName) {
                user = p.get();
                break;
            }
        }
        if (!user) {
            cout << "No record found for user '" << userName << "'.\n";
            return;
        }
        int choice;
        do {
            cout << "\n===== User Menu =====\n";
            cout << "1. View My Records\n";
            cout << "2. Check Reminders\n";
            cout << "3. Exit\n";
            cout << "Enter your choice: ";
            cin >> choice;
            if (cin.fail()) {
                cin.clear(); clearInputBuffer(); choice = 0; continue;
            }
            switch (choice) {
                case 1: user->display(); break;
                case 2: user->checkReminders(); break;
                case 3: cout << "Goodbye!\n"; break;
                default: cout << "Invalid choice. Please try again.\n";
            }
        } while (choice != 3);
    }
};

void clearInputBuffer() {
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void addNewPatient(vector<unique_ptr<Patient>>& patients) {
    string name, contact;
    int age;
    cout << "\nEnter patient's full name: ";
    clearInputBuffer();
    getline(cin, name);
    cout << "Enter patient's age: ";
    cin >> age;
    cout << "Enter patient's contact info: ";
    clearInputBuffer();
    getline(cin, contact);
    patients.push_back(make_unique<Patient>(name, age, contact));
    cout << "Patient '" << name << "' added successfully!\n";
}

void listAllPatients(const vector<unique_ptr<Patient>>& patients) {
    cout << "\n--- All Patients ---\n";
    if (patients.empty()) {
        cout << "No patients in the system.\n";
        return;
    }
    for (size_t i = 0; i < patients.size(); ++i) {
        cout << i + 1 << ". " << patients[i]->getName() << "\n";
    }
}

void patientSubMenu(Patient* patient) {
    int choice;
    do {
        cout << "\n--- Doctor Patient Menu ---\n";
        cout << "1. View Patient\n";
        cout << "2. Add Health Record\n";
        cout << "3. Update Health Record\n";
        cout << "4. Add Medication Reminder\n";
        cout << "5. Calculate BMI\n";
        cout << "6. View Health Trends\n";
        cout << "7. Back\n";
        cout << "Enter your choice: ";
        cin >> choice;
        if (cin.fail()) { cin.clear(); clearInputBuffer(); choice = 0; continue; }
        switch (choice) {
            case 1:
                patient->display();
                break;
            case 2: {
                cout << "\nSelect record type:\n1. Blood Pressure\n2. Weight\n3. Blood Sugar\nChoice: ";
                int recType; cin >> recType;
                if (recType == 1) {
                    int s, d;
                    cout << "Enter Systolic: "; cin >> s;
                    cout << "Enter Diastolic: "; cin >> d;
                    patient->addRecord(make_unique<BloodPressureRecord>(s, d));
                    cout << "Record added.\n";
                } else if (recType == 2) {
                    double w;
                    cout << "Enter weight in kg: "; cin >> w;
                    patient->addRecord(make_unique<WeightRecord>(w));
                    cout << "Record added.\n";
                } else if (recType == 3) {
                    int sugar;
                    cout << "Enter blood sugar in mg/dL: "; cin >> sugar;
                    patient->addRecord(make_unique<BloodSugarRecord>(sugar));
                    cout << "Record added.\n";
                } else cout << "Invalid choice.\n";
                break;
            }
            case 3: {
                patient->displayRecordsWithIndices();
                cout << "Enter record number to update: ";
                int idx; cin >> idx;
                HealthRecord* rec = patient->getRecordByIndex(idx - 1);
                if (!rec) { cout << "Invalid record number.\n"; break; }
                cout << "Update options:\n";
                if (BloodPressureRecord* bp = dynamic_cast<BloodPressureRecord*>(rec)) {
                    int s, d;
                    cout << "New Systolic: "; cin >> s;
                    cout << "New Diastolic: "; cin >> d;
                    bp->updateBloodPressure(s, d);
                    cout << "Blood pressure updated.\n";
                } else if (WeightRecord* w = dynamic_cast<WeightRecord*>(rec)) {
                    double weight;
                    cout << "New Weight: "; cin >> weight;
                    w->updateWeight(weight);
                    cout << "Weight updated.\n";
                } else cout << "Only BP or Weight records can be updated for now.\n";
                break;
            }
            case 4: {
                string msg, date, time;
                cout << "Reminder message: "; clearInputBuffer(); getline(cin, msg);
                cout << "Date (YYYY-MM-DD): "; getline(cin, date);
                cout << "Time (HH:MM, 24-hr): "; getline(cin, time);
                patient->addReminder(Reminder(msg, date, time));
                cout << "Reminder added.\n";
                break;
            }
            case 5:
                patient->calculateAndDisplayBMI();
                break;
            case 6:
                patient->displayHealthTrend();
                break;
            case 7:
                cout << "Returning to Doctor menu...\n";
                break;
            default:
                cout << "Invalid choice.\n";
        }
    } while (choice != 7);
}

void selectPatient(vector<unique_ptr<Patient>>& patients) {
    if (patients.empty()) {
        cout << "No patients available.\n";
        return;
    }
    listAllPatients(patients);
    cout << "Select a patient by number: ";
    int idx; cin >> idx;
    if (idx < 1 || idx > patients.size()) {
        cout << "Invalid selection.\n";
        return;
    }
    patientSubMenu(patients[idx - 1].get());
}

// --- Database Derived Classes ---
void DatabaseManager::loadPatients(vector<unique_ptr<Patient>>& patients) {
    patients.clear();
    const char* sql = "SELECT id, name, age, contact FROM patients;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(DB, sql, -1, &stmt, 0) != SQLITE_OK) {
        cerr << "Failed to prepare statement for loading patients.\n";
        return;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int pid = sqlite3_column_int(stmt, 0);
        string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        int age = sqlite3_column_int(stmt, 2);
        string contact = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        auto patient = make_unique<Patient>(name, age, contact);

        // Load health records
        string rec_sql = "SELECT type, value1, value2, timestamp FROM health_records WHERE patient_id=? ORDER BY timestamp ASC;";
        sqlite3_stmt* rstmt;
        sqlite3_prepare_v2(DB, rec_sql.c_str(), -1, &rstmt, 0);
        sqlite3_bind_int(rstmt, 1, pid);

        while (sqlite3_step(rstmt) == SQLITE_ROW) {
            string type = reinterpret_cast<const char*>(sqlite3_column_text(rstmt, 0));
            double v1 = sqlite3_column_double(rstmt, 1);
            double v2 = sqlite3_column_double(rstmt, 2);
            time_t ts = sqlite3_column_int64(rstmt, 3);

            if (type == "BP")
                patient->addRecord(make_unique<BloodPressureRecord>((int)v1, (int)v2, ts));
            else if (type == "Weight")
                patient->addRecord(make_unique<WeightRecord>(v1, ts));
            else if (type == "Sugar")
                patient->addRecord(make_unique<BloodSugarRecord>((int)v1, ts));
        }
        sqlite3_finalize(rstmt);

        // Load reminders
        string rem_sql = "SELECT message, date, time FROM reminders WHERE patient_id=?;";
        sqlite3_stmt* remstmt;
        sqlite3_prepare_v2(DB, rem_sql.c_str(), -1, &remstmt, 0);
        sqlite3_bind_int(remstmt, 1, pid);

        while (sqlite3_step(remstmt) == SQLITE_ROW) {
            string msg = reinterpret_cast<const char*>(sqlite3_column_text(remstmt, 0));
            string date = reinterpret_cast<const char*>(sqlite3_column_text(remstmt, 1));
            string time = reinterpret_cast<const char*>(sqlite3_column_text(remstmt, 2));
            patient->addReminder(Reminder(msg, date, time));
        }
        sqlite3_finalize(remstmt);

        patients.push_back(move(patient));
    }
    sqlite3_finalize(stmt);

    cout << "Loaded " << patients.size() << " patients from database.\n";
}

void DatabaseManager::saveAllPatients(const vector<unique_ptr<Patient>>& patients) {
    char* errMsg = 0;
    sqlite3_exec(DB, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
    // Clear old data
    sqlite3_exec(DB, "DELETE FROM health_records;", nullptr, nullptr, &errMsg);
    sqlite3_exec(DB, "DELETE FROM reminders;", nullptr, nullptr, &errMsg);
    sqlite3_exec(DB, "DELETE FROM patients;", nullptr, nullptr, &errMsg);

    for (const auto& p : patients) {
        sqlite3_stmt* stmt;
        const char* sql = "INSERT INTO patients(name, age, contact) VALUES (?, ?, ?);";
        sqlite3_prepare_v2(DB, sql, -1, &stmt, 0);
        sqlite3_bind_text(stmt, 1, p->getName().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, p->getAge());
        sqlite3_bind_text(stmt, 3, p->getContact().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        int patientId = (int)sqlite3_last_insert_rowid(DB);
        sqlite3_finalize(stmt);

        // Save health records
        for (int i = 0; i < p->numRecords(); ++i) {
            HealthRecord* rec = p->getRecordByIndex(i);

            string type;
            double v1 = 0, v2 = 0;
            if (BloodPressureRecord* bp = dynamic_cast<BloodPressureRecord*>(rec)) {
                type = "BP"; v1 = bp->getSystolic(); v2 = bp->getDiastolic();
            } else if (WeightRecord* w = dynamic_cast<WeightRecord*>(rec)) {
                type = "Weight"; v1 = w->getWeight();
            } else if (BloodSugarRecord* s = dynamic_cast<BloodSugarRecord*>(rec)) {
                type = "Sugar"; v1 = s->getSugar();
            }

            sqlite3_stmt* rstmt;
            string rec_sql = "INSERT INTO health_records(patient_id, type, value1, value2, timestamp) VALUES (?, ?, ?, ?, ?);";
            sqlite3_prepare_v2(DB, rec_sql.c_str(), -1, &rstmt, 0);
            sqlite3_bind_int(rstmt, 1, patientId);
            sqlite3_bind_text(rstmt, 2, type.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_double(rstmt, 3, v1);
            sqlite3_bind_double(rstmt, 4, v2);
            sqlite3_bind_int64(rstmt, 5, rec->getTimestamp());
            sqlite3_step(rstmt);
            sqlite3_finalize(rstmt);
        }

        // Save reminders
        for (const Reminder& rem : p->getReminders()) {
            sqlite3_stmt* remstmt;
            string rem_sql = "INSERT INTO reminders(patient_id, message, date, time) VALUES (?, ?, ?, ?);";
            sqlite3_prepare_v2(DB, rem_sql.c_str(), -1, &remstmt, 0);
            sqlite3_bind_int(remstmt, 1, patientId);
            sqlite3_bind_text(remstmt, 2, rem.getMessage().c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(remstmt, 3, rem.getDate().c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(remstmt, 4, rem.getTime().c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(remstmt);
            sqlite3_finalize(remstmt);
        }
    }
    sqlite3_exec(DB, "END TRANSACTION;", nullptr, nullptr, &errMsg);
    cout << "Saved all patient data to database.\n";
}


// --- Main function ---
int main() {
    DatabaseManager db("meditrack.db");
    if (!db.open()) return 1;
    db.createTables();

    vector<unique_ptr<Patient>> patients;
    db.loadPatients(patients);

    cout << "\nSelect your profile type:\n";
    cout << "1. Doctor\n";
    cout << "2. User\n";
    cout << "Enter your choice: ";
    int profileChoice;
    cin >> profileChoice;
    Profile* profile = nullptr;
    if (profileChoice == 1) profile = new DoctorProfile();
    else if (profileChoice == 2) profile = new UserProfile();
    else { cout << "Invalid profile choice.\n"; return 0; }

    profile->start(patients);

    db.saveAllPatients(patients);
    delete profile;
    cout << "\nThank you for using MediTrack!\n";
    return 0;
}


