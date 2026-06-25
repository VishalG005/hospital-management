#include <iostream>
#include <string>
#include <ctime> // Added for Admin timestamp functionality.

using namespace std;

const int MAX_MED       = 50;
const int MAX_TIMES     = 10;
const int MAX_LOGS      = 200;
const int MAX_USERS     = 500;
const int LOGIN_LOGS    = 1000;

// ============== DATA STRUCTURES ==============

struct User {
    string name;
    string password;
    string role;   // "Doctor", "Staff", or "Admin"
};

struct Medicine {
    string name;
    string dosage;              // current dosage info
    string prescribedDosage;    // Dr. prescribed (e.g. "500mg")
    string prescribedUnit;      // unit (e.g. "mg", "ml", "tablets")
    double prescribedAmount;    // numeric value (e.g. 500.0)
    bool dosageSetByDoctor;     // whether doctor has set dosage
    string instructions;
    string times[MAX_TIMES];
    int timeCount;
    bool active;
};

struct LogEntry {
    string medName;
    string dosage;
    string schedTime;
    string takenTime;
    string actualDosageTaken;   // what user actually took
    bool dosageMatched;         // matched prescribed dosage?
    bool taken;
    bool missed;
};

// Struct for Admin to track login history
struct LoginAudit {
    string timestamp;
    string usernameAttempt;
    string roleAttempt;
    string status; // "Success" or "Failed"
};

// ============== GLOBALS ==============

User      users[MAX_USERS];
int       userCount = 0;

Medicine  meds[MAX_MED];
int       medCount  = 0;

LogEntry  logs[MAX_LOGS];
int       logCount  = 0;

LoginAudit loginHistory[LOGIN_LOGS];
int       loginHistoryCount = 0;

// Currently logged-in user (set after successful login/registration)
string    currentUserName = "";
string    currentUserRole = "";

// ============== UTILITY FUNCTIONS ==============

string padRight(const string& s, int width) {
    string result = s;
    while ((int)result.length() < width) result += ' ';
    return result;
}

// validTime fun: checks if a string is a valid HH:MM format.
bool validTime(const string& t) {
    if (t.length() != 5 || t[2] != ':') return false;
    for (int i = 0; i < 5; i++)
        if (i != 2 && (t[i] < '0' || t[i] > '9')) return false;
    int h = (t[0] - '0') * 10 + (t[1] - '0');
    int m = (t[3] - '0') * 10 + (t[4] - '0');
    return h <= 23 && m <= 59;
}

// toMinutes fun: Converts the HH:MM format to only minutes.
int toMinutes(const string& t) {
    return ((t[0] - '0') * 10 + (t[1] - '0')) * 60
         + (t[3] - '0') * 10 + (t[4] - '0');
}

//alreadyTaken fun: Checks if the given medician name and time is aready given to the file
bool alreadyTaken(const string& name, const string& time) {
    for (int i = 0; i < logCount; i++)
        if (logs[i].medName == name &&
            logs[i].schedTime == time &&
            logs[i].taken)
            return true;
    return false;
}

//pulls the leading number out
double parseNumber(const string& s) {
    string numPart = "";
    for (int i = 0; i < (int)s.length(); i++) {
        if ((s[i] >= '0' && s[i] <= '9') || s[i] == '.')
            numPart += s[i];
        else if (numPart.length() > 0)
            break;
    }
    if (numPart.empty()) return -1.0;

    double result = 0.0;
    double decimal = 0.0;
    double divisor = 1.0;
    bool afterDot = false;

    for (int i = 0; i < (int)numPart.length(); i++) {
        if (numPart[i] == '.') {
            afterDot = true;
        } else if (afterDot) {
            divisor *= 10.0;
            decimal = decimal * 10.0 + (numPart[i] - '0');
        } else {
            result = result * 10.0 + (numPart[i] - '0');
        }
    }
    return result + decimal / divisor;
}

/*parseUnit fun: It extracts the unit (letter part) from a string
and skips all digits, dots, and spaces, and returns only the alphabetic characters.*/
string parseUnit(const string& s) {
    string unit = "";
    for (int i = 0; i < (int)s.length(); i++) {
        if ((s[i] >= '0' && s[i] <= '9') || s[i] == '.' || s[i] == ' ')
            continue;
        unit += s[i];
    }
    return unit;
}

// findUserByName fun: It in keeping track of the staff name for admin
int findUserByName(const string& name) {
    for (int i = 0; i < userCount; i++)
        if (users[i].name == name)
            return i;
    return -1;
}

// Helper to log login attempts for Admin
void recordLoginEvent(const string& user, const string& role, const string& status) {
    if (loginHistoryCount >= LOGIN_LOGS) return;
    
    time_t now = time(0);
    char* dt = ctime(&now);
    string ts(dt);
    if (!ts.empty() && ts.back() == '\n') ts.pop_back(); // Remove newline

    loginHistory[loginHistoryCount].timestamp = ts;      // Keep the track of staff login time.
    loginHistory[loginHistoryCount].usernameAttempt = user;     // Keep the track of staff name, then logined in the system.
    loginHistory[loginHistoryCount].roleAttempt = role;     // Keep the track of the role of the staff, How logined in the system.
    loginHistory[loginHistoryCount].status = status;   // Keep the track that is the staff is still logined.
    loginHistoryCount++;
}

// ============== AUTH: REGISTER & LOGIN ==============

bool registerUser() {
    if (userCount >= MAX_USERS) {
        cout << "User storage full! Cannot register more users.\n";
        return false;
    }

    cout << "\n+====================================+\n";
    cout << "|        USER REGISTRATION           |\n";
    cout << "+====================================+\n";

    cout << "Are you a:\n";
    cout << "  1. Doctor\n";
    cout << "  2. Staff\n";
    cout << "  3. Admin\n";
    cout << "Choice (1-3): ";
    int roleCh;
    cin  >> roleCh;
    if (cin.fail()) {
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "Invalid input!\n";
        return false;
    }

    string role;
    if (roleCh == 1)      role = "Doctor";
    else if (roleCh == 2) role = "Staff";
    else if (roleCh == 3) role = "Admin";
    else {
        cout << "Invalid role choice!\n";
        return false;
    }

    cin.ignore(10000, '\n');

    string name;
    cout << "Enter your name: ";
    getline(cin, name);
    if (name.empty()) {
        cout << "Name cannot be empty!\n";
        return false;
    }

    if (findUserByName(name) != -1) {
        cout << ">> A user with the name '" << name
             << "' already exists. Please login instead.\n";
        return false;
    }

    string password;
    cout << "Enter password: ";
    getline(cin, password);
    if (password.empty()) {
        cout << "Password cannot be empty!\n";
        return false;
    }

    string confirm;
    cout << "Confirm password: ";
    getline(cin, confirm);
    if (password != confirm) {
        cout << "Passwords do not match! Registration cancelled.\n";
        return false;
    }

    users[userCount].name     = name;
    users[userCount].password = password;
    users[userCount].role     = role;
    userCount++;

    cout << "\n>> Registration successful!\n";
    cout << ">> Welcome, " << name << " (" << role << ")\n";
    return true;
}

bool loginUser() {
    cout << "\n+====================================+\n";
    cout << "|             USER LOGIN             |\n";
    cout << "+====================================+\n";

    cout << "Are you a:\n";
    cout << "  1. Doctor\n";
    cout << "  2. Staff\n";
    cout << "  3. Admin\n";
    cout << "Choice (1-3): ";
    int roleCh;
    cin  >> roleCh;
    if (cin.fail()) {
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "Invalid input!\n";
        return false;
    }

    string role;
    if (roleCh == 1)      role = "Doctor";
    else if (roleCh == 2) role = "Staff";
    else if (roleCh == 3) role = "Admin";
    else {
        cout << "Invalid role choice!\n";
        return false;
    }

    cin.ignore(10000, '\n');

    string name;
    cout << "Enter your name: ";
    getline(cin, name);
    if (name.empty()) {
        cout << "Name cannot be empty!\n";
        return false;
    }

    string password;
    cout << "Enter password: ";
    getline(cin, password);

    int idx = findUserByName(name);
    
    // Log the attempt
    string loginStatus = "Failed";

    // Check if the user id Registered to the system or not.
    if (idx == -1) {
        cout << "\n!! User '" << name << "' not found.\n";
        cout << "Plz... Register and try to login.\n";
        recordLoginEvent(name, role, loginStatus);
        return false;
    }

    if (users[idx].role != role) {
        cout << "\n!! Role mismatch. '" << name
             << "' is registered as " << users[idx].role
             << ", not " << role << ".\n";
        recordLoginEvent(name, role, loginStatus);
        return false;
    }

    if (users[idx].password != password) {
        cout << "\n!! Incorrect password. Please try again.\n";
        recordLoginEvent(name, role, loginStatus);
        return false;
    }

    // Success
    loginStatus = "Success";
    recordLoginEvent(users[idx].name, users[idx].role, loginStatus);

    currentUserName = users[idx].name;
    currentUserRole = users[idx].role;

    cout << "\n>> Login successful!\n";
    cout << ">> Welcome back, " << currentUserName
         << " (" << currentUserRole << ")\n";
    return true;
}

void showCurrentUser() {
    cout << "\nLogged in as: " << currentUserName
         << "  |  Role: " << currentUserRole << "\n";
}

void logoutUser() {
    cout << "\n>> Logging out " << currentUserName << "...\n";
    currentUserName = "";
    currentUserRole = "";
}

// ============== DOCTOR DOSAGE MANAGEMENT ==============

void doctorSetDosage() {
    cout << "\n+====================================+\n";
    cout << "|   DOCTOR DOSAGE PRESCRIPTION       |\n";
    cout << "+====================================+\n";

    if (medCount == 0) {
        cout << "No medicines available.\n";
        return;
    }

    // List medicines
    cout << "\nAvailable Medicines:\n";
    for (int i = 0; i < medCount; i++) {
        cout << "  [" << i + 1 << "] " << meds[i].name
             << " (current: " << meds[i].dosage << ")";
        if (meds[i].dosageSetByDoctor) {
            cout << " [Rx: " << meds[i].prescribedAmount
                 << meds[i].prescribedUnit << "]";
        }
        cout << "\n";
    }

    int ch;
    cout << "\nSelect medicine (0 to cancel): ";
    cin >> ch;
    if (ch <= 0 || ch > medCount) {
        cout << "Cancelled.\n";
        return;
    }

    Medicine& m = meds[ch - 1];

    cout << "\n--- Setting dosage for: " << m.name << " ---\n";

    string amountStr;
    cin.ignore();
    cout << "Enter prescribed amount (e.g. 500, 2.5): ";
    getline(cin, amountStr);

    double amount = parseNumber(amountStr);
    if (amount <= 0) {
        cout << "Invalid amount! Must be a positive number.\n";
        return;
    }

    string unit;
    cout << "Enter unit (mg, ml, tablets, drops, etc.): ";
    getline(cin, unit);
    if (unit.empty()) {
        cout << "Unit cannot be empty!\n";
        return;
    }

    m.prescribedAmount   = amount;
    m.prescribedUnit     = unit;
    m.prescribedDosage   = amountStr;
    m.dosageSetByDoctor  = true;

    // Also update the dosage string to match
    m.dosage = amountStr + unit;

    cout << "\n>> Dr. prescribed dosage set: " << m.name
         << " = " << m.prescribedAmount << m.prescribedUnit << "\n";
    cout << ">> Patients will now be verified against this dosage.\n";
}

void doctorSetAllDosages() {
    cout << "\n+====================================+\n";
    cout << "|   SET DOSAGES FOR ALL MEDICINES   |\n";
    cout << "+====================================+\n";

    if (medCount == 0) {
        cout << "No medicines available.\n";
        return;
    }

    cin.ignore();

    for (int i = 0; i < medCount; i++) {
        Medicine& m = meds[i];
        cout << "\n--- [" << i + 1 << "/" << medCount << "] "
             << m.name << " ---\n";
        cout << "Current dosage: " << m.dosage << "\n";
        if (m.dosageSetByDoctor)
            cout << "Current Rx: " << m.prescribedAmount
                 << m.prescribedUnit << "\n";

        cout << "Enter prescribed amount (0 or empty to skip): ";
        string amountStr;
        getline(cin, amountStr);

        if (amountStr.empty() || amountStr == "0") {
            cout << "  Skipped.\n";
            continue;
        }

        double amount = parseNumber(amountStr);
        if (amount <= 0) {
            cout << "  Invalid amount, skipped.\n";
            continue;
        }

        cout << "Enter unit: ";
        string unit;
        getline(cin, unit);
        if (unit.empty()) {
            cout << "  Empty unit, skipped.\n";
            continue;
        }

        m.prescribedAmount  = amount;
        m.prescribedUnit    = unit;
        m.prescribedDosage  = amountStr;
        m.dosageSetByDoctor = true;
        m.dosage            = amountStr + unit;

        cout << "  >> Set: " << m.prescribedAmount
             << m.prescribedUnit << "\n";
    }

    cout << "\nAll dosages updated.\n";
}

void showPrescriptions() {
    cout << "\n=== Dr. Prescriptions ===\n";
    if (medCount == 0) {
        cout << "No medicines.\n";
        return;
    }

    cout << padRight("Medicine", 20)
         << padRight("Prescribed", 16)
         << padRight("Current", 14)
         << "Status\n";
    cout << string(60, '-') << "\n";

    for (int i = 0; i < medCount; i++) {
        string rx = meds[i].dosageSetByDoctor
            ? to_string((int)meds[i].prescribedAmount) + meds[i].prescribedUnit
            : "NOT SET";
        string status = meds[i].dosageSetByDoctor ? "VERIFIED" : "UNVERIFIED";

        cout << padRight(meds[i].name, 20)
             << padRight(rx, 16)
             << padRight(meds[i].dosage, 14)
             << status << "\n";
    }
}

// ============== ADMIN FUNCTIONS ==============

void viewLoginLogs() {
    cout << "\n+====================================+\n";
    cout << "|        ADMIN: LOGIN AUDIT LOG      |\n";
    cout << "+====================================+\n";
    if (loginHistoryCount == 0) {
        cout << "No login history recorded.\n";
        return;
    }

    cout << padRight("Timestamp", 25)
         << padRight("Username", 15)
         << padRight("Role", 10)
         << "Status\n";
    cout << string(70, '-') << "\n";

    // Show most recent first
    for (int i = loginHistoryCount - 1; i >= 0; i--) {
        cout << padRight(loginHistory[i].timestamp, 25)
             << padRight(loginHistory[i].usernameAttempt, 15)
             << padRight(loginHistory[i].roleAttempt, 10)
             << loginHistory[i].status << "\n";
    }
}

// ============== DOSAGE VERIFICATION ==============

bool verifyDosage(int medIndex, const string& actualInput, string& message) {
    Medicine& m = meds[medIndex];

    // If no doctor prescription, skip verification
    if (!m.dosageSetByDoctor) {
        message = "No Dr. prescription on file - dosage NOT verified.";
        return true;  // allow but warn
    }

    double actualAmount = parseNumber(actualInput);
    string actualUnit   = parseUnit(actualInput);

    if (actualAmount <= 0) {
        message = "ERROR: Could not parse dosage amount!";
        return false;
    }

    // Check unit match (case-insensitive simple comparison)
    string expectedUnit = m.prescribedUnit;
    string givenUnit    = actualUnit;

    for (int i = 0; i < (int)expectedUnit.length(); i++)
        if (expectedUnit[i] >= 'A' && expectedUnit[i] <= 'Z')
            expectedUnit[i] += 32;
    for (int i = 0; i < (int)givenUnit.length(); i++)
        if (givenUnit[i] >= 'A' && givenUnit[i] <= 'Z')
            givenUnit[i] += 32;

    if (!givenUnit.empty() && givenUnit != expectedUnit) {
        message = "WARNING: Unit mismatch! Prescribed: "
                + m.prescribedUnit + ", Given: " + actualUnit;
        return false;
    }

    double diff = actualAmount - m.prescribedAmount;
    if (diff < 0) diff = -diff;

    double tolerance = m.prescribedAmount * 0.05; // 5% tolerance
    if (tolerance < 0.01) tolerance = 0.01;

    if (diff > tolerance) {
        message = "WARNING: Dosage mismatch! Prescribed: "
                + to_string((int)m.prescribedAmount) + m.prescribedUnit
                + ", You entered: " + actualInput;
        return false;
    }

    message = "OK: Dosage matches prescription ("
            + to_string((int)m.prescribedAmount) + m.prescribedUnit + ")";
    return true;
}

// ============== MEDICINE MANAGEMENT ==============

void addMedicine() {
    if (medCount >= MAX_MED) {
        cout << "Storage full!\n";
        return;
    }

    Medicine& m = meds[medCount];
    cin.ignore();

    cout << "\n=== Add New Medicine ===\n";
    cout << "Name: ";
    getline(cin, m.name);
    if (m.name.empty()) { cout << "Name required!\n"; return; }

    cout << "Dosage (e.g. 500mg): ";
    getline(cin, m.dosage);

    cout << "Instructions (optional): ";
    getline(cin, m.instructions);

    m.active           = true;
    m.timeCount        = 0;
    m.dosageSetByDoctor = false;
    m.prescribedAmount  = 0.0;
    m.prescribedUnit    = "";
    m.prescribedDosage  = "";

    cout << "Add reminder times (HH:MM, empty to finish):\n";
    while (m.timeCount < MAX_TIMES) {
        cout << "  Time: ";
        string t;
        getline(cin, t);
        if (t.empty()) break;
        if (!validTime(t)) {
            cout << "  Invalid! Use HH:MM\n";
            continue;
        }
        m.times[m.timeCount++] = t;
        cout << "  Added " << t << "\n";
    }

    if (m.timeCount == 0) {
        cout << "At least one time required!\n";
        return;
    }

    medCount++;
    cout << "'" << m.name << "' added successfully!\n";

    // Ask if doctor wants to set dosage now (only if logged in user is Doctor or Admin)
    if (currentUserRole == "Doctor" || currentUserRole == "Admin") {
        cout << "\nSet prescribed dosage now? (1=Yes, 0=No): ";
        int ans;
        cin >> ans;
        if (ans == 1) {
            doctorSetDosage();
        }
    }
}

void listMedicines() {
    cout << "\n=== Your Medicines ===\n";
    if (medCount == 0) { cout << "No medicines yet.\n"; return; }

    for (int i = 0; i < medCount; i++) {
        cout << "\n[" << i + 1 << "] " << meds[i].name
             << " - " << meds[i].dosage
             << " [" << (meds[i].active ? "ACTIVE" : "INACTIVE") << "]";

        if (meds[i].dosageSetByDoctor) {
            cout << " [Rx: " << meds[i].prescribedAmount
                 << meds[i].prescribedUnit << "]";
        } else {
            cout << " [Rx: NOT SET]";
        }

        cout << "\n    Times: ";
        for (int j = 0; j < meds[i].timeCount; j++) {
            if (j > 0) cout << ", ";
            cout << meds[i].times[j];
        }
        cout << "\n";
        if (!meds[i].instructions.empty())
            cout << "    Note: " << meds[i].instructions << "\n";
    }
}

void deleteMedicine() {
    listMedicines();
    if (medCount == 0) return;

    int ch;
    cout << "\nDelete which? (0 to cancel): ";
    cin >> ch;
    if (ch <= 0 || ch > medCount) { cout << "Cancelled.\n"; return; }

    string name = meds[ch - 1].name;
    for (int i = ch - 1; i < medCount - 1; i++)
        meds[i] = meds[i + 1];
    medCount--;
    cout << "'" << name << "' deleted.\n";
}

void toggleMedicine() {
    listMedicines();
    if (medCount == 0) return;

    int ch;
    cout << "\nToggle which? (0 to cancel): ";
    cin >> ch;
    if (ch <= 0 || ch > medCount) { cout << "Cancelled.\n"; return; }

    meds[ch - 1].active = !meds[ch - 1].active;
    cout << "'" << meds[ch - 1].name << "' is now "
         << (meds[ch - 1].active ? "ACTIVE" : "INACTIVE") << ".\n";
}

// ============== REMINDER SYSTEM ==============

void checkReminders() {
    cout << "\nEnter current time (HH:MM): ";
    string now;
    cin >> now;
    if (!validTime(now)) { cout << "Invalid time!\n"; return; }

    int nowMin = toMinutes(now);
    bool found = false;

    for (int i = 0; i < medCount; i++) {
        if (!meds[i].active) continue;
        for (int j = 0; j < meds[i].timeCount; j++) {
            int diff = toMinutes(meds[i].times[j]) - nowMin;
            if (diff >= -5 && diff <= 30 &&
                !alreadyTaken(meds[i].name, meds[i].times[j])) {
                if (!found) cout << "\n=== Pending Reminders ===\n";
                cout << "\n  ** " << meds[i].name
                     << " - " << meds[i].dosage << " **\n";
                cout << "     Scheduled: " << meds[i].times[j] << "\n";
                if (meds[i].dosageSetByDoctor) {
                    cout << "     Prescribed: "
                         << meds[i].prescribedAmount
                         << meds[i].prescribedUnit << "\n";
                }
                if (!meds[i].instructions.empty())
                    cout << "     Note: " << meds[i].instructions << "\n";
                found = true;
            }
        }
    }
    if (!found) cout << "No pending reminders.\n";
}

void markTaken() {
    cout << "\n=== Mark as Taken ===\n";

    struct Pending { int mi; int ti; };
    Pending pending[MAX_MED * MAX_TIMES];
    int pCount = 0;

    for (int i = 0; i < medCount; i++) {
        if (!meds[i].active) continue;
        for (int j = 0; j < meds[i].timeCount; j++) {
            if (!alreadyTaken(meds[i].name, meds[i].times[j])) {
                pending[pCount].mi = i;
                pending[pCount].ti = j;
                pCount++;
                cout << "[" << pCount << "] " << meds[i].name
                     << " - " << meds[i].times[j];
                if (meds[i].dosageSetByDoctor)
                    cout << " [Rx: " << meds[i].prescribedAmount
                         << meds[i].prescribedUnit << "]";
                cout << "\n";
            }
        }
    }

    if (pCount == 0) { cout << "Nothing pending.\n"; return; }

    int ch;
    cout << "\nMark which? (0 to cancel): ";
    cin >> ch;
    if (ch <= 0 || ch > pCount) { cout << "Cancelled.\n"; return; }

    if (logCount >= MAX_LOGS) { cout << "Log full!\n"; return; }

    int mi = pending[ch - 1].mi;
    int ti = pending[ch - 1].ti;

    // ---- DOSAGE VERIFICATION ----
    string actualDosage;
    bool dosageOk = false;
    string verifyMsg;

    if (meds[mi].dosageSetByDoctor) {
        cout << "\n  >> Dr. prescribed: " << meds[mi].prescribedAmount
             << meds[mi].prescribedUnit << "\n";
        cout << "  Enter dosage you are taking"
             << " (e.g. " << meds[mi].prescribedAmount
             << meds[mi].prescribedUnit << "): ";
    } else {
        cout << "  Enter dosage taken (e.g. " << meds[mi].dosage << "): ";
    }
    cin.ignore();
    getline(cin, actualDosage);

    if (actualDosage.empty()) actualDosage = meds[mi].dosage;

    dosageOk = verifyDosage(mi, actualDosage, verifyMsg);

    cout << "\n  [Dosage Check] " << verifyMsg << "\n";

    if (!dosageOk) {
        cout << "\n  !! DOSAGE MISMATCH DETECTED !!\n";
        cout << "  Proceed anyway? (1=Yes, 0=No): ";
        int force;
        cin >> force;
        if (force != 1) {
            cout << "  Marking cancelled. Please take the correct dosage.\n";
            return;
        }
        cout << "  Proceeded with override.\n";
    }

    // ---- LOG THE ENTRY ----
    cout << "  Taken at (HH:MM): ";
    string tt;
    cin >> tt;
    if (!validTime(tt)) { cout << "Invalid time!\n"; return; }

    LogEntry& l    = logs[logCount++];
    l.medName       = meds[mi].name;
    l.dosage        = meds[mi].dosage;
    l.schedTime     = meds[mi].times[ti];
    l.takenTime     = tt;
    l.actualDosageTaken = actualDosage;
    l.dosageMatched = dosageOk;
    l.taken         = true;
    l.missed        = false;

    cout << "\n  '" << l.medName << "' marked taken at " << tt << ".\n";
    if (meds[mi].dosageSetByDoctor) {
        cout << "  Dosage verification: "
             << (dosageOk ? "PASSED" : "FAILED (override)") << "\n";
    }
}

// ============== HISTORY & STATS ==============

void showHistory() {
    // Requirement: Dr. can only see time and Dosage given
    if (currentUserRole == "Doctor") {
        cout << "\n=== Doctor View: Medication History ===\n";
        if (logCount == 0) { cout << "No history yet.\n"; return; }

        // Limited columns for Doctor
        cout << padRight("Time Taken", 15) << "Dosage Given\n";
        cout << string(35, '-') << "\n";

        for (int i = 0; i < logCount; i++) {
            if (logs[i].taken) {
                cout << padRight(logs[i].takenTime, 15)
                     << logs[i].actualDosageTaken << "\n";
            }
        }
    } 
    else {
        // Staff and Admin see full details
        cout << "\n=== Medication History ===\n";
        if (logCount == 0) { cout << "No history yet.\n"; return; }

        cout << padRight("Medicine", 18)
             << padRight("Dosage", 10)
             << padRight("Sched", 8)
             << padRight("Taken", 8)
             << padRight("Actual", 10)
             << padRight("RxCheck", 10)
             << "Status\n";
        cout << string(82, '-') << "\n";

        for (int i = 0; i < logCount; i++) {
            string status = logs[i].taken  ? "TAKEN"
                          : logs[i].missed ? "MISSED"
                          :                   "PENDING";
            string rxStatus = !logs[i].taken ? "-"
                            : logs[i].dosageMatched ? "PASS"
                            : "FAIL";

            cout << padRight(logs[i].medName,           18)
                 << padRight(logs[i].dosage,             10)
                 << padRight(logs[i].schedTime,          8)
                 << padRight(logs[i].takenTime,          8)
                 << padRight(logs[i].actualDosageTaken,  10)
                 << padRight(rxStatus,                   10)
                 << status << "\n";
        }
    }
}

void showStats() {
    cout << "\n=== Statistics ===\n";
    if (logCount == 0) { cout << "No data.\n"; return; }

    int taken = 0, missed = 0, dosageFails = 0;
    for (int i = 0; i < logCount; i++) {
        if (logs[i].taken)  taken++;
        if (logs[i].missed) missed++;
        if (logs[i].taken && !logs[i].dosageMatched) dosageFails++;
    }

    int pct = (logCount > 0) ? (taken * 100 / logCount) : 0;
    int rxPct = (taken > 0) ? ((taken - dosageFails) * 100 / taken) : 0;

    cout << "Total reminders     : " << logCount     << "\n";
    cout << "Taken               : " << taken         << "\n";
    cout << "Missed              : " << missed        << "\n";
    cout << "Adherence           : " << pct           << "%\n";
    cout << "Dosage mismatches   : " << dosageFails   << "\n";
    cout << "Dosage accuracy     : " << rxPct         << "%\n";
}

// ============== MENU SYSTEM ==============

void printMenu() {
    cout << "\n+========================================+\n";
    cout << "|      MEDICINE REMINDER SYSTEM         |\n";
    cout << "|  User: " << currentUserName
         << " (" << currentUserRole << ")";
    // Calculate padding to align right edge
    int headerContentLen = 12 + currentUserName.length() + 2 + currentUserRole.length();
    int totalWidth = 40;
    int pad = totalWidth - headerContentLen;
    if(pad < 0) pad = 0;
    cout << string(pad, ' ') << "|\n";
    cout << "+========================================+\n";
    
    // Common/Staff Menu (1-8)
    cout << "|  1. Add Medicine                      |\n";
    cout << "|  2. List Medicines                    |\n";
    cout << "|  3. Delete Medicine                   |\n";
    cout << "|  4. Toggle Status                     |\n";
    cout << "|  5. Check Reminders                   |\n";
    cout << "|  6. Mark as Taken (with verification) |\n";
    cout << "|  7. View History                      |\n";
    cout << "|  8. Statistics                        |\n";
    
    // Doctor Menu (9-11) - Shown if Doctor OR Admin
    if (currentUserRole == "Doctor" || currentUserRole == "Admin") {
        cout << "+----------------------------------------+\n";
        cout << "|  --- Doctor Panel ---                  |\n";
        cout << "|  9.  Set Dosage (single medicine)     |\n";
        cout << "|  10. Set All Dosages                  |\n";
        cout << "|  11. View Prescriptions               |\n";
    }

    // Admin Menu (12) - Shown only if Admin
    if (currentUserRole == "Admin") {
        cout << "+----------------------------------------+\n";
        cout << "|  --- Admin Panel ---                   |\n";
        cout << "|  12. View Login Logs                   |\n";
    }

    // Common Options
    cout << "+----------------------------------------+\n";
    cout << "|  13. View Current User                |\n";
    cout << "|  0.  Exit                             |\n";
    cout << "+========================================+\n";
    cout << "Choice: ";
}

int main() {
    cout << "+========================================+\n";
    cout << "|      MEDICINE REMINDER SYSTEM         |\n";
    cout << "|    Stay healthy, stay on schedule!    |\n";
    cout << "+========================================+\n";

    // ---------- LOGIN / REGISTRATION GATE ----------
    bool loggedIn = false;
    while (!loggedIn) {
        cout << "\n+========================================+\n";
        cout << "|       WELCOME - PLEASE IDENTIFY       |\n";
        cout << "+========================================+\n";
        cout << "|  1. Login (existing user)             |\n";
        cout << "|  2. Register (new user)               |\n";
        cout << "|  0. Exit                              |\n";
        cout << "+========================================+\n";
        cout << "Choice: ";

        int ch;
        cin >> ch;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Invalid input!\n";
            continue;
        }

        if (ch == 1) {
            loggedIn = loginUser();
        } else if (ch == 2) {
            if (registerUser()) {
                // Auto-login after registration
                recordLoginEvent(users[userCount - 1].name, users[userCount - 1].role, "Success");
                currentUserName = users[userCount - 1].name;
                currentUserRole = users[userCount - 1].role;
                loggedIn = true;
            }
        } else if (ch == 0) {
            cout << "Goodbye! Stay healthy!\n";
            return 0;
        } else {
            cout << "Invalid choice.\n";
        }
    }

    // ---------- MAIN APPLICATION LOOP ----------
    while (true) {
        printMenu();
        int ch;
        cin >> ch;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Invalid input!\n";
            continue;
        }

        switch (ch) {
            case 1:  addMedicine();        break;
            case 2:  listMedicines();      break;
            case 3:  deleteMedicine();     break;
            case 4:  toggleMedicine();     break;
            case 5:  checkReminders();     break;
            case 6:  markTaken();          break;
            case 7:  showHistory();        break;
            case 8:  showStats();          break;

            // ---- Doctor Panel (Doctor or Admin only) ----
            case 9:
                if (currentUserRole != "Doctor" && currentUserRole != "Admin") {
                    cout << "\n!! ACCESS DENIED: Only Doctors can set dosage.\n";
                    break;
                }
                doctorSetDosage();
                break;
            case 10:
                if (currentUserRole != "Doctor" && currentUserRole != "Admin") {
                    cout << "\n!! ACCESS DENIED: Only Doctors can set dosages.\n";
                    break;
                }
                doctorSetAllDosages();
                break;
            case 11:
                if (currentUserRole != "Doctor" && currentUserRole != "Admin") {
                    cout << "\n!! ACCESS DENIED: Only Doctors can view prescriptions.\n";
                    break;
                }
                showPrescriptions();
                break;

            // ---- Admin Panel (Admin only) ----
            case 12:
                if (currentUserRole != "Admin") {
                    cout << "\n!! ACCESS DENIED: Admin privileges required.\n";
                    break;
                }
                viewLoginLogs();
                break;
                
            case 13:
                showCurrentUser();
                break;
            
            // ---- Logout ----
            case 99: 
                     break;

            case 0:
                cout << "Goodbye! Stay healthy, "
                     << currentUserName << "!\n";
                return 0;

            default:
                cout << "Invalid choice.\n";
        }
 
    }
}
