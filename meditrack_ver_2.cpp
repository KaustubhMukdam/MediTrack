#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <memory>
#include <limits>
#include <typeinfo>
#include <sstream> // Required for the robust loadData function

using namespace std;

// --- Forward Declarations ---
class Patient;
void saveData(const vector<unique_ptr<Patient>>& patients);
void loadData(vector<unique_ptr<Patient>>& patients);
void clearInputBuffer();

// ------------------- Health Record Base & Derived Classes -------------------
class HealthRecord {
protected:
    time_t timestamp;

public:
    HealthRecord() : timestamp(time(0)) {}
    HealthRecord(time_t loaded_time) : timestamp(loaded_time) {}
    virtual void display() const = 0;
    virtual string getType() const = 0;
    virtual void save(ofstream& file) const = 0;
    virtual ~HealthRecord() = default;

    string getFormattedTimestamp() const {
        char buffer[20];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", localtime(&timestamp));
        return string(buffer);
    }
};

class BloodPressureRecord : public HealthRecord {
    int systolic, diastolic;
public:
    BloodPressureRecord(int s, int d) : HealthRecord(), systolic(s), diastolic(d) {}
    BloodPressureRecord(int s, int d, time_t t) : HealthRecord(t), systolic(s), diastolic(d) {}
    
    string getType() const override { return "BP"; }
    
    void display() const override {
        cout << getFormattedTimestamp() << " - Blood Pressure: " << systolic << "/" << diastolic << " mmHg";
        if (systolic >= 140 || diastolic >= 90) {
            cout << "  <-- ⚠️ ALERT: High Blood Pressure!";
        } else if (systolic <= 90 || diastolic <= 60) {
            cout << "  <-- ⚠️ ALERT: Low Blood Pressure!";
        }
        cout << "\n";
    }

    void save(ofstream& file) const override {
        file << getType() << " " << systolic << " " << diastolic << " " << timestamp << "\n";
    }
};

class WeightRecord : public HealthRecord {
    double weight;
public:
    WeightRecord(double w) : HealthRecord(), weight(w) {}
    WeightRecord(double w, time_t t) : HealthRecord(t), weight(w) {}
    string getType() const override { return "Weight"; }
    
    void display() const override {
        cout << getFormattedTimestamp() << " - Weight: " << weight << " kg\n";
    }

    void save(ofstream& file) const override {
        file << getType() << " " << weight << " " << timestamp << "\n";
    }
    
    double getWeight() const { return weight; }
};

class BloodSugarRecord : public HealthRecord {
    double sugar;
public:
    BloodSugarRecord(double s) : HealthRecord(), sugar(s) {}
    BloodSugarRecord(double s, time_t t) : HealthRecord(t), sugar(s) {}
    string getType() const override { return "Sugar"; }
    
    void display() const override {
        cout << getFormattedTimestamp() << " - Blood Sugar: " << sugar << " mg/dL";
        if (sugar >= 126) {
            cout << "  <-- ⚠️ ALERT: High Blood Sugar (Potential Diabetes)!";
        } else if (sugar < 70) {
            cout << "  <-- ⚠️ ALERT: Low Blood Sugar (Hypoglycemia)!";
        }
        cout << "\n";
    }
    
    void save(ofstream& file) const override {
        file << getType() << " " << sugar << " " << timestamp << "\n";
    }
};

// ------------------- Medication & Reminder Classes -------------------
class Medication {
    string name, dosage, schedule;
public:
    Medication(string n, string d, string s) : name(n), dosage(d), schedule(s) {}
    void display() const {
        cout << "Medication: " << name << " | Dosage: " << dosage << " | Schedule: " << schedule << "\n";
    }
    void save(ofstream& file) const {
        file << name << "|" << dosage << "|" << schedule << "\n";
    }
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
    void display() const {
        cout << "Reminder: " << message << " on " << date << " at " << reminderTime << "\n";
    }
    void save(ofstream& file) const {
        file << message << "|" << date << "|" << reminderTime << "\n";
    }
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

    void calculateAndDisplayBMI() const {
        double lastWeight = 0.0;
        for (auto it = records.rbegin(); it != records.rend(); ++it) {
            if (WeightRecord* wr = dynamic_cast<WeightRecord*>(it->get())) {
                lastWeight = wr->getWeight();
                break;
            }
        }
        if (lastWeight <= 0) {
            cout << "\nBMI cannot be calculated. No weight records found for " << name << ".\n";
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
    
    void displayHealthTrend() const {
        int choice;
        cout << "\n--- View Health Trends for " << name << " ---\n";
        cout << "1. Blood Pressure Trend\n";
        cout << "2. Weight Trend\n";
        cout << "3. Blood Sugar Trend\n";
        cout << "Enter your choice: ";
        cin >> choice;
        if(cin.fail()){
            cin.clear();
            clearInputBuffer();
            cout << "Invalid input.\n";
            return;
        }
        cout << "\n--- Trend Report ---\n";
        bool found = false;
        switch(choice) {
            case 1:
                for(const auto& rec : records) if(dynamic_cast<BloodPressureRecord*>(rec.get())) { rec->display(); found = true; }
                break;
            case 2:
                for(const auto& rec : records) if(dynamic_cast<WeightRecord*>(rec.get())) { rec->display(); found = true; }
                break;
            case 3:
                for(const auto& rec : records) if(dynamic_cast<BloodSugarRecord*>(rec.get())) { rec->display(); found = true; }
                break;
            default: cout << "Invalid choice.\n"; return;
        }
        if (!found) cout << "No records of that type found.\n";
        cout << "--------------------\n";
    }

    void display() const {
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

    void checkReminders() const {
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

    string getName() const { return name; }
    int getAge() const { return age; }
    string getContact() const { return contactInfo; }
    const vector<unique_ptr<HealthRecord>>& getRecords() const { return records; }
    const vector<Medication>& getMedications() const { return medications; }
    const vector<Reminder>& getReminders() const { return reminders; }
};

// ------------------- Data Persistence & UI Functions -------------------
const string FILENAME = "meditrack_data.txt";

void saveData(const vector<unique_ptr<Patient>>& patients) {
    ofstream file(FILENAME);
    if (!file) { cerr << "Error: Could not open file for writing.\n"; return; }
    file << patients.size() << "\n";
    for (const auto& p : patients) {
        file << p->getName() << "|" << p->getAge() << "|" << p->getContact() << "\n";
        const auto& records = p->getRecords();
        file << records.size() << "\n";
        for (const auto& rec : records) { rec->save(file); }
        const auto& meds = p->getMedications();
        file << meds.size() << "\n";
        for (const auto& med : meds) { med.save(file); }
        const auto& rems = p->getReminders();
        file << rems.size() << "\n";
        for (const auto& rem : rems) { rem.save(file); }
    }
    file.close();
    cout << "Data saved successfully to " << FILENAME << "\n";
}

void loadData(vector<unique_ptr<Patient>>& patients) {
    ifstream file(FILENAME);
    if (!file) { cout << "No previous data found. Starting a new session.\n"; return; }
    int patientCount;
    if (!(file >> patientCount) || patientCount < 0 || patientCount > 10000) {
        cout << "Error: Invalid patient count in data file.\n"; return;
    }
    file.ignore(); 
    for (int i = 0; i < patientCount; ++i) {
        string line;
        if (!getline(file, line)) { cout << "Error reading patient data line.\n"; break; }
        if (!line.empty() && line.back() == '\r') { line.pop_back(); }
        stringstream ss(line);
        string name, ageStr, contact;
        if (!getline(ss, name, '|') || !getline(ss, ageStr, '|') || !getline(ss, contact)) {
            cout << "Error: Malformed patient data line: " << line << "\n"; continue;
        }
        int age = stoi(ageStr);
        auto patient = make_unique<Patient>(name, age, contact);
        int recordCount;
        file >> recordCount;
        file.ignore();
        for (int j = 0; j < recordCount; ++j) {
            getline(file, line);
            if (!line.empty() && line.back() == '\r') { line.pop_back(); }
            stringstream rec_ss(line);
            string type;
            rec_ss >> type;
            time_t timestamp;
            if (type == "BP") {
                int s, d;
                rec_ss >> s >> d >> timestamp;
                patient->addRecord(make_unique<BloodPressureRecord>(s, d, timestamp));
            } else if (type == "Weight") {
                double w;
                rec_ss >> w >> timestamp;
                patient->addRecord(make_unique<WeightRecord>(w, timestamp));
            } else if (type == "Sugar") {
                double s;
                rec_ss >> s >> timestamp;
                patient->addRecord(make_unique<BloodSugarRecord>(s, timestamp));
            }
        }
        int medCount;
        file >> medCount;
        file.ignore();
        for (int j = 0; j < medCount; ++j) {
            getline(file, line);
            if (!line.empty() && line.back() == '\r') { line.pop_back(); }
            stringstream med_ss(line);
            string medName, dosage, schedule;
            getline(med_ss, medName, '|');
            getline(med_ss, dosage, '|');
            getline(med_ss, schedule);
            patient->addMedication(Medication(medName, dosage, schedule));
        }
        int remCount;
        file >> remCount;
        file.ignore();
        for (int j = 0; j < remCount; ++j) {
            getline(file, line);
            if (!line.empty() && line.back() == '\r') { line.pop_back(); }
            stringstream rem_ss(line);
            string msg, date, time;
            getline(rem_ss, msg, '|');
            getline(rem_ss, date, '|');
            getline(rem_ss, time);
            patient->addReminder(Reminder(msg, date, time));
        }
        patients.push_back(move(patient));
    }
    file.close();
    cout << "Data loaded successfully from " << FILENAME << "\n";
}

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
    cout << "Enter patient's contact info (e.g., phone number): ";
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
            cout << "Invalid input. Please enter a number.\n";
            cin.clear();
            clearInputBuffer();
            choice = 0;
            continue;
        }
        switch (choice) {
            case 1: patient->display(); break;
            case 2: {
                int recordChoice;
                cout << "\nSelect record type:\n1. Blood Pressure\n2. Weight\n3. Blood Sugar\nChoice: ";
                cin >> recordChoice;
                if (recordChoice == 1) {
                    int s, d;
                    cout << "Enter Systolic pressure: "; cin >> s;
                    cout << "Enter Diastolic pressure: "; cin >> d;
                    patient->addRecord(make_unique<BloodPressureRecord>(s, d));
                    cout << "Blood pressure record added.\n";
                } else if (recordChoice == 2) {
                    double w;
                    cout << "Enter weight in kg: "; cin >> w;
                    patient->addRecord(make_unique<WeightRecord>(w));
                    cout << "Weight record added.\n";
                } else if (recordChoice == 3) {
                    double s;
                    cout << "Enter blood sugar in mg/dL: "; cin >> s;
                    patient->addRecord(make_unique<BloodSugarRecord>(s));
                    cout << "Blood sugar record added.\n";
                } else { cout << "Invalid record type.\n"; }
                break;
            }
            case 3: {
                string name, dosage, schedule;
                cout << "Enter medication name: "; clearInputBuffer(); getline(cin, name);
                cout << "Enter dosage (e.g., 500mg): "; getline(cin, dosage);
                cout << "Enter schedule (e.g., Twice a day): "; getline(cin, schedule);
                patient->addMedication(Medication(name, dosage, schedule));
                cout << "Medication added.\n";
                break;
            }
            case 4: {
                string msg, date, time;
                cout << "Enter reminder message: "; clearInputBuffer(); getline(cin, msg);
                cout << "Enter date (YYYY-MM-DD): "; getline(cin, date);
                cout << "Enter time (HH:MM): "; getline(cin, time);
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
    if (patients.empty()) { cout << "No patients in the system to select.\n"; return; }
    listAllPatients(patients);
    cout << "Select a patient by number: ";
    int patientIndex;
    cin >> patientIndex;
    if (patientIndex > 0 && patientIndex <= patients.size()) {
        patientSubMenu(patients[patientIndex - 1].get());
    } else {
        cout << "Invalid selection.\n";
    }
}

// ------------------- Main Function -------------------
int main() {
    vector<unique_ptr<Patient>> patients;
    loadData(patients);
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
            case 4: saveData(patients); cout << "Exiting MediTrack. Goodbye!\n"; break;
            default: cout << "Invalid choice. Please try again.\n";
        }
    } while (choice != 4);
    return 0;
}