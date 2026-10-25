# Group 6 - Budget Manager

## Project Description

Budget Manager is a C++ application that helps users record expenses and track their remaining budget.

Our Walking Skeleton focuses on recording expenses. Users can enter an expense amount, and the application automatically updates the remaining budget.

## Requirements

- C++17
- CMake 3.16 or newer
- Qt6 (Widgets and SQL)

## Setup

1. Clone the repository:

```bash
git clone https://github.com/jaydensu666666-creator/Group6-BudgetManager.git
```

2. Navigate to the project folder:

```bash
cd Group6-BudgetManager
```

3. Configure and build the project:

```bash
cmake -S . -B build
cmake --build build
```

For macOS users experiencing SDK compatibility issues, if macOS 26.5 SDK is installed, configure with:

```bash
cmake -S . -B build -DCMAKE_OSX_SYSROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk
```

## Run the Application

```bash
./build/app
```

## Run Unit Tests

```bash
ctest --test-dir build --output-on-failure
```
