// DatabaseManager.h
#include "sqlite3.h"
#include <string>
#include <vector>
#include <memory>
// Forward declare Patient so we don't need to include the full file
class Patient;

class DatabaseManager {
    sqlite3* DB;
    std::string db_file;

public:
    DatabaseManager(const std::string& filename);
    ~DatabaseManager();

    bool open();
    void createTables();
    void savePatient(const Patient& patient); // We'll save one patient at a time
    void loadPatients(std::vector<std::unique_ptr<Patient>>& patients);
};