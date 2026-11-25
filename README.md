# TypeMaster — C++ Typing Speed Test Web Application

A learning-focused mini project with a browser frontend and a C++17 backend.

## Features
- Register and log in
- Easy, Medium and Hard typing passages
- Preset and custom test duration (5–600 seconds)
- Live WPM and accuracy display
- Incorrect-character highlighting and error count
- Personal test history
- Global top-10 leaderboard (best score per user)
- Persistent storage in text files

## Technology
- C++17 backend using cpp-httplib
- HTML, CSS and JavaScript frontend
- Text-file storage (`data/users.txt`, `data/results.txt`)
- CMake build system

## Requirements
- Windows 10/11
- Visual Studio Code (optional)
- A C++17 compiler (MinGW-w64 / MSYS2 UCRT64 or Visual Studio)
- CMake 3.16+
- Git (CMake downloads cpp-httplib on the first configure)

## Build on Windows (PowerShell)
Open PowerShell in the project folder:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Run the server from the project root (important: the `web` and `data` folders are relative paths):

```powershell
.\build\Release\typemaster.exe
```

If your generator places the executable directly in `build`, use:

```powershell
.\build\typemaster.exe
```

Then open **http://localhost:8081** in Chrome or Edge.

Stop the server with `Ctrl+C`.

## Build with MSYS2 UCRT64
Open the **MSYS2 UCRT64** terminal, install tools if needed:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake git
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
./build/typemaster.exe
```

Open http://localhost:8081 in your browser.

## How to use
1. Create an account (username: 3–20 letters/numbers/underscore; password: 4–64 characters).
2. Log in.
3. Choose difficulty and a time limit.
4. Press **Start test**. The countdown starts with your first typed character.
5. Type the passage. The test ends when the time expires or the passage is completed.
6. View your WPM, accuracy, errors, history and leaderboard.

## OOP concepts demonstrated
- **Encapsulation:** `User` and `Result` keep fields private and expose getters.
- **Abstraction:** `TypingTest` is an abstract class with pure virtual functions.
- **Inheritance:** `EasyTest`, `MediumTest` and `HardTest` derive from `TypingTest`.
- **Runtime polymorphism:** the derived classes override `getPassage()` and `getDifficulty()`. The base-class interface allows the application to work with different test types.
- **File handling:** `FileManager` uses `ifstream` and `ofstream` to read/write user and result records.
- **Constructors:** initialize User and Result objects.
- **Virtual destructor:** supports safe destruction through a base-class pointer.

## File storage format
- `data/users.txt`: quoted username and password fields
- `data/results.txt`: quoted username, WPM, accuracy, errors, duration and timestamp

## Important educational-project limitations
This is a local demonstration, not a production authentication system:
- Passwords are stored as plain text to keep the first file-handling lesson easy. Never reuse a real password. A production version must use a strong salted password hash (such as Argon2id or bcrypt), HTTPS, CSRF protection, rate limiting and secure session cookies.
- Session tokens are held in server memory and are lost when the server restarts.
- Scores are submitted by the browser, so they are not tamper-proof. A production version would validate test timing and input on the server.
- The app binds to `127.0.0.1` only, so it is intended for use on the same computer.

## Troubleshooting
- If CMake cannot download cpp-httplib, check your internet connection and Git installation, then rerun `cmake -S . -B build`.
- If `cmake` is not recognized, install CMake and reopen the terminal.
- If `g++` is not recognized, open the MSYS2 UCRT64 terminal or configure your compiler PATH.
- If port 8081 is busy, close the other local server using that port before starting TypeMaster.
