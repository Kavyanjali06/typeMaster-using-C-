#include <httplib.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// -------------------- Utility functions --------------------
string jsonEscape(const string& value) {
    string out;
    for (char c : value) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) >= 32) out += c;
        }
    }
    return out;
}

string makeToken() {
    static mt19937_64 rng(random_device{}());
    static const char* chars = "0123456789abcdef";
    string token(32, '0');
    for (char& c : token) c = chars[rng() % 16];
    return token;
}

string currentTime() {
    time_t now = time(nullptr);
    tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    stringstream ss;
    ss << put_time(&local, "%Y-%m-%d %H:%M");
    return ss.str();
}

// -------------------- Result class: encapsulation --------------------
class Result {
private:
    string username;
    double wpm;
    double accuracy;
    int errors;
    int duration;
    string date;

public:
    Result(string user, double speed, double acc, int wrong, int seconds, string when)
        : username(move(user)), wpm(speed), accuracy(acc),
          errors(wrong), duration(seconds), date(move(when)) {}

    string getUsername() const { return username; }
    double getWpm() const { return wpm; }
    double getAccuracy() const { return accuracy; }
    int getErrors() const { return errors; }
    int getDuration() const { return duration; }
    string getDate() const { return date; }
};

// -------------------- User class: encapsulation --------------------
class User {
private:
    string username;
    string password;

public:
    User(string name, string pass) : username(move(name)), password(move(pass)) {}
    string getUsername() const { return username; }
    string getPassword() const { return password; }
};

// -------------------- Abstract base class: abstraction --------------------
class TypingTest {
public:
    virtual string getPassage() const = 0;
    virtual string getDifficulty() const = 0;
    virtual ~TypingTest() = default;
};

// -------------------- Derived classes: inheritance + overriding --------------------
class EasyTest : public TypingTest {
public:
    string getPassage() const override {
        return "Practice every day to improve your typing speed. Small steps lead to great results.";
    }
    string getDifficulty() const override { return "Easy"; }
};

class MediumTest : public TypingTest {
public:
    string getPassage() const override {
        return "Consistent practice improves accuracy, concentration, and confidence. Focus on rhythm rather than rushing through every sentence.";
    }
    string getDifficulty() const override { return "Medium"; }
};

class HardTest : public TypingTest {
public:
    string getPassage() const override {
        return "Reliable software requires analytical thinking, disciplined experimentation, and careful evaluation of unexpected edge cases.";
    }
    string getDifficulty() const override { return "Hard"; }
};

// -------------------- FileManager: file handling --------------------
class FileManager {
private:
    const string usersFile = "data/users.txt";
    const string resultsFile = "data/results.txt";
    mutex fileMutex;

public:
    FileManager() {
        ofstream users(usersFile, ios::app);
        ofstream results(resultsFile, ios::app);
    }

    bool userExists(const string& name) {
        lock_guard<mutex> lock(fileMutex);
        ifstream in(usersFile);
        string savedName, savedPassword;
        while (in >> quoted(savedName) >> quoted(savedPassword)) {
            if (savedName == name) return true;
        }
        return false;
    }

    bool addUser(const User& user) {
        lock_guard<mutex> lock(fileMutex);
        ofstream out(usersFile, ios::app);
        if (!out) return false;
        out << quoted(user.getUsername()) << ' ' << quoted(user.getPassword()) << '\n';
        return static_cast<bool>(out);
    }

    bool validateUser(const string& name, const string& password) {
        lock_guard<mutex> lock(fileMutex);
        ifstream in(usersFile);
        string savedName, savedPassword;
        while (in >> quoted(savedName) >> quoted(savedPassword)) {
            if (savedName == name && savedPassword == password) return true;
        }
        return false;
    }

    bool saveResult(const Result& result) {
        lock_guard<mutex> lock(fileMutex);
        ofstream out(resultsFile, ios::app);
        if (!out) return false;
        out << quoted(result.getUsername()) << ' '
            << result.getWpm() << ' ' << result.getAccuracy() << ' '
            << result.getErrors() << ' ' << result.getDuration() << ' '
            << quoted(result.getDate()) << '\n';
        return static_cast<bool>(out);
    }

    vector<Result> getAllResults() {
        lock_guard<mutex> lock(fileMutex);
        vector<Result> records;
        ifstream in(resultsFile);
        string name, date;
        double wpm, accuracy;
        int errors, duration;
        while (in >> quoted(name) >> wpm >> accuracy >> errors >> duration >> quoted(date)) {
            records.emplace_back(name, wpm, accuracy, errors, duration, date);
        }
        return records;
    }
};

// -------------------- Authentication service --------------------
class AuthService {
private:
    FileManager& storage;
    map<string, string> sessions;
    mutex sessionMutex;

public:
    explicit AuthService(FileManager& files) : storage(files) {}

    bool registerUser(const string& name, const string& password, string& message) {
        if (name.size() < 3 || name.size() > 20) {
            message = "Username must contain 3 to 20 characters.";
            return false;
        }
        if (password.size() < 4 || password.size() > 64) {
            message = "Password must contain 4 to 64 characters.";
            return false;
        }
        for (char c : name) {
            if (!isalnum(static_cast<unsigned char>(c)) && c != '_') {
                message = "Username may contain letters, numbers and underscore only.";
                return false;
            }
        }
        if (storage.userExists(name)) {
            message = "That username already exists.";
            return false;
        }
        if (!storage.addUser(User(name, password))) {
            message = "Could not save user data.";
            return false;
        }
        message = "Account created successfully.";
        return true;
    }

    string login(const string& name, const string& password) {
        if (!storage.validateUser(name, password)) return "";
        string token = makeToken();
        lock_guard<mutex> lock(sessionMutex);
        sessions[token] = name;
        return token;
    }

    string usernameFor(const string& token) {
        lock_guard<mutex> lock(sessionMutex);
        auto it = sessions.find(token);
        return it == sessions.end() ? "" : it->second;
    }

    void logout(const string& token) {
        lock_guard<mutex> lock(sessionMutex);
        sessions.erase(token);
    }
};

// -------------------- JSON response helpers --------------------
string resultsToJson(const vector<Result>& records, bool leaderboard) {
    vector<Result> chosen = records;
    if (leaderboard) {
        sort(chosen.begin(), chosen.end(), [](const Result& a, const Result& b) {
            if (a.getWpm() != b.getWpm()) return a.getWpm() > b.getWpm();
            return a.getAccuracy() > b.getAccuracy();
        });
        // Keep each user's best score only.
        vector<Result> best;
        map<string, bool> seen;
        for (const auto& r : chosen) {
            if (!seen[r.getUsername()]) {
                seen[r.getUsername()] = true;
                best.push_back(r);
            }
        }
        chosen = best;
      if (chosen.size() > 10) {
    chosen.erase(chosen.begin() + 10, chosen.end());
}
    }

    string json = "[";
    bool first = true;
    for (const auto& r : chosen) {
        if (!first) json += ",";
        first = false;
        json += "{\"username\":\"" + jsonEscape(r.getUsername()) +
                "\",\"wpm\":" + to_string(r.getWpm()) +
                ",\"accuracy\":" + to_string(r.getAccuracy()) +
                ",\"errors\":" + to_string(r.getErrors()) +
                ",\"duration\":" + to_string(r.getDuration()) +
                ",\"date\":\"" + jsonEscape(r.getDate()) + "\"}";
    }
    return json + "]";
}

int main() {
    try {
        FileManager files;
        AuthService auth(files);
        httplib::Server server;

        server.set_mount_point("/", "./web");

        auto requireUser = [&auth](const httplib::Request& req) {
            auto it = req.headers.find("X-Session-Token");
            if (it == req.headers.end()) return string("");
            return auth.usernameFor(it->second);
        };

        server.Post("/api/register", [&auth](const httplib::Request& req, httplib::Response& res) {
            string name = req.get_param_value("username");
            string password = req.get_param_value("password");
            string message;
            bool ok = auth.registerUser(name, password, message);
            res.set_content("{\"ok\":" + string(ok ? "true" : "false") +
                            ",\"message\":\"" + jsonEscape(message) + "\"}", "application/json");
        });

        server.Post("/api/login", [&auth](const httplib::Request& req, httplib::Response& res) {
            string name = req.get_param_value("username");
            string password = req.get_param_value("password");
            string token = auth.login(name, password);
            bool ok = !token.empty();
            res.set_content("{\"ok\":" + string(ok ? "true" : "false") +
                            ",\"token\":\"" + token + "\",\"username\":\"" +
                            jsonEscape(name) + "\",\"message\":\"" +
                            (ok ? "Welcome back!" : "Incorrect username or password.") + "\"}",
                            "application/json");
        });

        server.Post("/api/logout", [&auth, requireUser](const httplib::Request& req, httplib::Response& res) {
            auto it = req.headers.find("X-Session-Token");
            if (it != req.headers.end()) auth.logout(it->second);
            res.set_content("{\"ok\":true}", "application/json");
        });

        server.Get("/api/me", [requireUser](const httplib::Request& req, httplib::Response& res) {
            string name = requireUser(req);
            if (name.empty()) {
                res.status = 401;
                res.set_content("{\"error\":\"Please log in.\"}", "application/json");
            } else {
                res.set_content("{\"username\":\"" + jsonEscape(name) + "\"}", "application/json");
            }
        });

        // Runtime polymorphism: choose a derived TypingTest through a base-class pointer.
        server.Get("/api/passage", [](const httplib::Request& req, httplib::Response& res) {
    string difficulty = req.has_param("difficulty")
        ? req.get_param_value("difficulty")
        : "easy";

    unique_ptr<TypingTest> test;

    if (difficulty == "easy") {
        test = make_unique<EasyTest>();
    }
    else if (difficulty == "medium") {
        test = make_unique<MediumTest>();
    }
    else if (difficulty == "hard") {
        test = make_unique<HardTest>();
    }
    else {
        res.status = 400;
        res.set_content("{\"error\":\"Unknown difficulty.\"}", "application/json");
        return;
    }

    res.set_content(
        "{\"difficulty\":\"" + test->getDifficulty() +
        "\",\"passage\":\"" + jsonEscape(test->getPassage()) + "\"}",
        "application/json"
    );
});

        server.Post("/api/results", [&files, requireUser](const httplib::Request& req, httplib::Response& res) {
            string name = requireUser(req);
            if (name.empty()) {
                res.status = 401;
                res.set_content("{\"error\":\"Please log in.\"}", "application/json");
                return;
            }
            try {
                double wpm = stod(req.get_param_value("wpm"));
                double accuracy = stod(req.get_param_value("accuracy"));
                int errors = stoi(req.get_param_value("errors"));
                int duration = stoi(req.get_param_value("duration"));
                if (wpm < 0 || wpm > 1000 || accuracy < 0 || accuracy > 100 ||
                    errors < 0 || duration < 1 || duration > 3600) {
                    throw invalid_argument("out of range");
                }
                Result result(name, wpm, accuracy, errors, duration, currentTime());
                if (!files.saveResult(result)) throw runtime_error("save failed");
                res.set_content("{\"ok\":true,\"message\":\"Result saved.\"}", "application/json");
            } catch (...) {
                res.status = 400;
                res.set_content("{\"error\":\"Invalid result data.\"}", "application/json");
            }
        });

        server.Get("/api/history", [&files, requireUser](const httplib::Request& req, httplib::Response& res) {
            string name = requireUser(req);
            if (name.empty()) {
                res.status = 401;
                res.set_content("{\"error\":\"Please log in.\"}", "application/json");
                return;
            }
            vector<Result> mine;
            for (const auto& r : files.getAllResults())
                if (r.getUsername() == name) mine.push_back(r);
            reverse(mine.begin(), mine.end());
            res.set_content(resultsToJson(mine, false), "application/json");
        });

        server.Get("/api/leaderboard", [&files](const httplib::Request&, httplib::Response& res) {
            res.set_content(resultsToJson(files.getAllResults(), true), "application/json");
        });

        cout << "====================================\n";
        cout << "       TYPEMASTER WEB SERVER\n";
        cout << "====================================\n";
        cout << "Open http://localhost:8081 in your browser.\n";
        cout << "Press Ctrl+C to stop the server.\n";
        server.listen("127.0.0.1", 8081);
    } catch (const exception& ex) {
        cerr << "Startup error: " << ex.what() << '\n';
        return 1;
    }
    return 0;
}
