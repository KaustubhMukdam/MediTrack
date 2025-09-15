#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <memory>
#include <limits>
#include <typeinfo>
#include <sstream>
#include "sqlite3.h" // The SQLite C header

using namespace std;

// --- Forward Declarations ---
class Patient;
class DatabaseManager;
void clearInputBuffer();
void addNewPatient(vector<unique_ptr<Patient>>& patients);
void listAllPatients(const vector<unique_ptr<Patient>>& patients);
void patientSubMenu(Patient* patient);
void selectPatient(vector<unique_ptr<Patient>>& patients);


// ------------------- Health Record Classes (with Getters for DB) -------------------
class HealthRecord {
protected:
    time_t timestamp;
public:
    HealthRecord() : timestamp(time(0)) {}
    HealthRecord(time_t loaded_time) : timestamp(loaded_time) {}
    virtual void display() const = 0;
    virtual ~HealthRecord() = default;
    
    string getFormattedTimestamp() const {
        char buffer[20];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", localtime(&timestamp));
        return string(buffer);
    }
    time_t getTimestamp() const { return timestamp; }
};

class BloodPressureRecord : public HealthRecord {
    int systolic, diastolic;
public:
    BloodPressureRecord(int s, int d) : HealthRecord(), systolic(s), diastolic(d) {}
    BloodPressureRecord(int s, int d, time_t t) : HealthRecord(t), systolic(s), diastolic(d) {}
    
    void display() const override {
        cout << getFormattedTimestamp() << " - Blood Pressure: " << systolic << "/" << diastolic << " mmHg";
        if (systolic >= 140 || diastolic >= 90) cout << "  <-- ⚠️ ALERT: High Blood Pressure!";
        else if (systolic <= 90 || diastolic <= 60) cout << "  <-- ⚠️ ALERT: Low Blood Pressure!";
        cout << "\n";
    }
    int getSystolic() const { return systolic; }
    int getDiastolic() const { return diastolic; }
};

class WeightRecord : public HealthRecord {
    double weight;
public:
    WeightRecord(double w) : HealthRecord(), weight(w) {}
    WeightRecord(double w, time_t t) : HealthRecord(t), weight(w) {}
    void display() const override { cout << getFormattedTimestamp() << " - Weight: " << weight << " kg\n"; }
    double getWeight() const { return weight; }
};

class BloodSugarRecord : public HealthRecord {
    double sugar;
public:
    BloodSugarRecord(double s) : HealthRecord(), sugar(s) {}
    BloodSugarRecord(double s, time_t t) : HealthRecord(t), sugar(s) {}
    void display() const override {
        cout << getFormattedTimestamp() << " - Blood Sugar: " << sugar << " mg/dL";
        if (sugar >= 126) cout << "  <-- ⚠️ ALERT: High Blood Sugar (Potential Diabetes)!";
        else if (sugar < 70) cout << "  <-- ⚠️ ALERT: Low Blood Sugar (Hypoglycemia)!";
        cout << "\n";
    }
    double getSugar() const { return sugar; }
};


// ------------------- Medication & Reminder Classes (with Getters for DB) -------------------
class Medication {
    string name, dosage, schedule;
public:
    Medication(string n, string d, string s) : name(n), dosage(d), schedule(s) {}
    void display() const { cout << "Medication: " << name << " | Dosage: " << dosage << " | Schedule: " << schedule << "\n"; }
    string getName() const { return name; }
    string getDosage() const { return dosage; }
    string getSchedule() const { return schedule; }
};

class Reminder {
    string message, date, reminderTime;
public:
    Reminder(string m, string d, string t) : message(m), date(d), reminderTime(t) {}
    bool isDue() const {
        time_t now = time(0);
        tm *ltm = localtime(&now);
        char todayDate[11], currentTime[6];
        sprintf(todayDate, "%04d-%02d-%02d", 1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday);
        sprintf(currentTime, "%02d:%02d", ltm->tm_hour, ltm->tm_min);
        return (date == todayDate && reminderTime <= currentTime);
    }
    void display() const { cout << "Reminder: " << message << " on " << date << " at " << reminderTime << "\n"; }
    string getMessage() const { return message; }
    string getDate() const { return date; }
    string getTime() const { return reminderTime; }
};


// ------------------- Patient Class -------------------
class Patient {
    string name;
    int age;
    string contactInfo;
    vector<unique_ptr<HealthRecord>> records;
    vector<Medication> medications;
    vector<Reminder> reminders;
public:
    Patient(string n, int a, string c) : name(n), age(a), contactInfo(c) {}

    void addRecord(unique_ptr<HealthRecord> r) { records.push_back(move(r)); }
    void addMedication(const Medication& m) { medications.push_back(m); }
    void addReminder(const Reminder& r) { reminders.push_back(r); }

    void calculateAndDisplayBMI() const;
    void displayHealthTrend() const;
    void display() const;
    void checkReminders() const;

    string getName() const { return name; }
    int getAge() const { return age; }
    string getContact() const { return contactInfo; }
    const vector<unique_ptr<HealthRecord>>& getRecords() const { return records; }
    const vector<Medication>& getMedications() const { return medications; }
    const vector<Reminder>& getReminders() const { return reminders; }
};


// ------------------- DatabaseManager Class (Tier 3) -------------------
class DatabaseManager {
private:
    sqlite3* DB;
    string db_file;

public:
    DatabaseManager(const string& filename) : db_file(filename), DB(nullptr) {}
    
    ~DatabaseManager() {
        if (DB) {
            sqlite3_close(DB);
        }
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
        const char* sql_statements = 
            "PRAGMA foreign_keys = ON;"
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

    void saveAllPatients(const vector<unique_ptr<Patient>>& patients) {
        sqlite3_exec(DB, "DELETE FROM patients;", 0, 0, 0); // Cascading delete will clear other tables

        sqlite3_stmt *stmt_p, *stmt_r, *stmt_m, *stmt_rem;
        const char* sql_p = "INSERT INTO patients (name, age, contact) VALUES (?, ?, ?);";
        const char* sql_r = "INSERT INTO health_records (patient_id, type, value1, value2, timestamp) VALUES (?, ?, ?, ?, ?);";
        const char* sql_m = "INSERT INTO medications (patient_id, name, dosage, schedule) VALUES (?, ?, ?, ?);";
        const char* sql_rem = "INSERT INTO reminders (patient_id, message, date, time) VALUES (?, ?, ?, ?);";

        for (const auto& patient : patients) {
            sqlite3_prepare_v2(DB, sql_p, -1, &stmt_p, 0);
            sqlite3_bind_text(stmt_p, 1, patient->getName().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt_p, 2, patient->getAge());
            sqlite3_bind_text(stmt_p, 3, patient->getContact().c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt_p);
            sqlite3_finalize(stmt_p);
            
            long long patient_id = sqlite3_last_insert_rowid(DB);

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
            for (const auto& med : patient->getMedications()) {
                sqlite3_prepare_v2(DB, sql_m, -1, &stmt_m, 0);
                sqlite3_bind_int(stmt_m, 1, patient_id);
                sqlite3_bind_text(stmt_m, 2, med.getName().c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt_m, 3, med.getDosage().c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt_m, 4, med.getSchedule().c_str(), -1, SQLITE_STATIC);
                sqlite3_step(stmt_m);
                sqlite3_finalize(stmt_m);
            }
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

    void loadPatients(vector<unique_ptr<Patient>>& patients) {
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
            const char* sql_m = "SELECT name, dosage, schedule FROM medications WHERE patient_id = ?;";
            sqlite3_prepare_v2(DB, sql_m, -1, &stmt_m, 0);
            sqlite3_bind_int(stmt_m, 1, id);
            while(sqlite3_step(stmt_m) == SQLITE_ROW) {
                patient->addMedication(Medication((const char*)sqlite3_column_text(stmt_m, 0), (const char*)sqlite3_column_text(stmt_m, 1), (const char*)sqlite3_column_text(stmt_m, 2)));
            }
            sqlite3_finalize(stmt_m);
            const char* sql_rem = "SELECT message, date, time FROM reminders WHERE patient_id = ?;";
            sqlite3_prepare_v2(DB, sql_rem, -1, &stmt_rem, 0);
            sqlite3_bind_int(stmt_rem, 1, id);
            while(sqlite3_step(stmt_rem) == SQLITE_ROW) {
                patient->addReminder(Reminder((const char*)sqlite3_column_text(stmt_rem, 0), (const char*)sqlite3_column_text(stmt_rem, 1), (const char*)sqlite3_column_text(stmt_rem, 2)));
            }
            sqlite3_finalize(stmt_rem);
            patients.push_back(move(patient));
        }
        sqlite3_finalize(stmt_p);
        cout << "Loaded " << patients.size() << " patients from database.\n";
    }
};


// ------------------- Patient Method Implementations -------------------
// (We declare these down here because they use UI elements like cout/cin)
void Patient::calculateAndDisplayBMI() const {
    double lastWeight = 0.0;
    for (auto it = records.rbegin(); it != records.rend(); ++it) {
        if (WeightRecord* wr = dynamic_cast<WeightRecord*>(it->get())) {
            lastWeight = wr->getWeight();
            break;
        }
    }
    if (lastWeight <= 0) {
        cout << "\nBMI cannot be calculated. No weight records found.\n";
        return;
    }
    double heightMeters;
    cout << "\nPlease enter patient's height in meters (e.g., 1.75): ";
    cin >> heightMeters;
    if (cin.fail() || heightMeters <= 0) {
        cout << "Invalid height. Cannot calculate BMI.\n";
        cin.clear();
        clearInputBuffer();
        return;
    }
    double bmi = lastWeight / (heightMeters * heightMeters);
    cout << "\n--- BMI Calculation for " << name << " ---\n";
    cout << "Using most recent weight: " << lastWeight << " kg\n";
    cout << "Height: " << heightMeters << " m\n";
    cout << "Calculated BMI is: " << bmi << "\n";
    if (bmi < 18.5) cout << "Category: Underweight\n";
    else if (bmi < 25) cout << "Category: Normal weight\n";
    else if (bmi < 30) cout << "Category: Overweight\n";
    else cout << "Category: Obesity\n";
    cout << "---------------------------------\n";
}

void Patient::displayHealthTrend() const {
    int choice;
    cout << "\n--- View Health Trends for " << name << " ---\n";
    cout << "1. Blood Pressure Trend\n";
    cout << "2. Weight Trend\n";
    cout << "3. Blood Sugar Trend\n";
    cout << "Enter your choice: ";
    cin >> choice;
    if (cin.fail()) {
        cin.clear(); clearInputBuffer();
        cout << "Invalid input.\n";
        return;
    }
    cout << "\n--- Trend Report ---\n";
    bool found = false;
    switch(choice) {
        case 1: for(const auto& rec : records) if(dynamic_cast<BloodPressureRecord*>(rec.get())) { rec->display(); found = true; } break;
        case 2: for(const auto& rec : records) if(dynamic_cast<WeightRecord*>(rec.get())) { rec->display(); found = true; } break;
        case 3: for(const auto& rec : records) if(dynamic_cast<BloodSugarRecord*>(rec.get())) { rec->display(); found = true; } break;
        default: cout << "Invalid choice.\n"; return;
    }
    if (!found) cout << "No records of that type found.\n";
    cout << "--------------------\n";
}

void Patient::display() const {
    cout << "\n--- Patient Profile ---\n";
    cout << "Name: " << name << "\nAge: " << age << "\nContact: " << contactInfo << "\n";
    cout << "\n--- Health Records ---\n";
    if (records.empty()) cout << "No health records found.\n"; else for (const auto& r : records) r->display();
    cout << "\n--- Medications ---\n";
    if (medications.empty()) cout << "No medications found.\n"; else for (const auto &m : medications) m.display();
    cout << "\n--- Reminders ---\n";
    if (reminders.empty()) cout << "No reminders found.\n"; else for (const auto &rem : reminders) rem.display();
    cout << "-----------------------\n";
}

void Patient::checkReminders() const {
    cout << "\n--- Checking Reminders for " << name << " ---\n";
    bool found = false;
    for (const auto &rem : reminders) {
        if (rem.isDue()) {
            cout << "⚠️ Reminder Due: ";
            rem.display();
            found = true;
        }
    }
    if (!found) cout << "No reminders are currently due.\n";
}

// ------------------- UI Functions -------------------
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
    if (patients.empty()) { cout << "No patients in the system.\n"; return; }
    for (size_t i = 0; i < patients.size(); ++i) {
        cout << i + 1 << ". " << patients[i]->getName() << "\n";
    }
}

void patientSubMenu(Patient* patient) {
    int choice;
    do {
        cout << "\n--- Managing Patient: " << patient->getName() << " ---\n";
        cout << "1. View Patient Profile\n";
        cout << "2. Add Health Record\n";
        cout << "3. Add Medication\n";
        cout << "4. Add Reminder\n";
        cout << "5. Calculate BMI\n";
        cout << "6. View Health Trends\n";
        cout << "7. Return to Main Menu\n";
        cout << "Enter your choice: ";
        cin >> choice;
        if (cin.fail()) {
            cout << "Invalid input.\n";
            cin.clear(); clearInputBuffer();
            choice = 0; continue;
        }
        switch (choice) {
            case 1: patient->display(); break;
            case 2: {
                int recordChoice;
                cout << "\nSelect record type:\n1. Blood Pressure\n2. Weight\n3. Blood Sugar\nChoice: ";
                cin >> recordChoice;
                if (recordChoice == 1) {
                    int s, d;
                    cout << "Enter Systolic: "; cin >> s;
                    cout << "Enter Diastolic: "; cin >> d;
                    patient->addRecord(make_unique<BloodPressureRecord>(s, d));
                } else if (recordChoice == 2) {
                    double w;
                    cout << "Enter weight in kg: "; cin >> w;
                    patient->addRecord(make_unique<WeightRecord>(w));
                } else if (recordChoice == 3) {
                    double s;
                    cout << "Enter blood sugar in mg/dL: "; cin >> s;
                    patient->addRecord(make_unique<BloodSugarRecord>(s));
                } else { cout << "Invalid record type.\n"; }
                if(!cin.fail()) cout << "Record added.\n";
                break;
            }
            case 3: {
                string name, dosage, schedule;
                cout << "Medication name: "; clearInputBuffer(); getline(cin, name);
                cout << "Dosage (e.g., 500mg): "; getline(cin, dosage);
                cout << "Schedule (e.g., Twice a day): "; getline(cin, schedule);
                patient->addMedication(Medication(name, dosage, schedule));
                cout << "Medication added.\n";
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
            case 5: patient->calculateAndDisplayBMI(); break;
            case 6: patient->displayHealthTrend(); break;
            case 7: cout << "Returning to main menu...\n"; break;
            default: cout << "Invalid choice. Please try again.\n";
        }
    } while (choice != 7);
}

void selectPatient(vector<unique_ptr<Patient>>& patients) {
    if (patients.empty()) { cout << "No patients to select.\n"; return; }
    listAllPatients(patients);
    cout << "Select a patient by number: ";
    int patientIndex;
    cin >> patientIndex;
    if (cin.fail() || patientIndex <= 0 || patientIndex > patients.size()) {
        cout << "Invalid selection.\n";
        if(cin.fail()){ cin.clear(); clearInputBuffer(); }
    } else {
        patientSubMenu(patients[patientIndex - 1].get());
    }
}


// ------------------- Main Function -------------------
int main() {
    DatabaseManager db("meditrack.db");
    if (!db.open()) {
        return 1;
    }
    db.createTables();

    vector<unique_ptr<Patient>> patients;
    db.loadPatients(patients);

    cout << "\nWelcome to MediTrack: Your health, Our priority\n";
    for (const auto& p : patients) {
        p->checkReminders();
    }
    
    int choice;
    do {
        cout << "\n===== MediTrack Main Menu =====\n";
        cout << "1. Add New Patient\n";
        cout << "2. Select Patient\n";
        cout << "3. List All Patients\n";
        cout << "4. Save and Exit\n";
        cout << "Enter your choice: ";
        cin >> choice;

        if (cin.fail()) {
            cout << "Invalid input. Please enter a number.\n";
            cin.clear();
            clearInputBuffer();
            choice = 0;
            continue;
        }

        switch (choice) {
            case 1: addNewPatient(patients); break;
            case 2: selectPatient(patients); break;
            case 3: listAllPatients(patients); break;
            case 4: 
                db.saveAllPatients(patients);
                cout << "Exiting MediTrack. Goodbye!\n";
                break;
            default: 
                cout << "Invalid choice. Please try again.\n";
        }
    } while (choice != 4);

    return 0;
}