# Group 6 - Budget Manager

Budget Manager helps a college student who lives on money sent once a term (or
once a month) answer one question: how much can I spend until my next transfer?

The student enters the money they received, sets some aside for saving, and
records each purchase. The home page shows the money left to spend, whether it
will last until the next transfer at the current pace, and how much is left for
this week.

- User requirements: [REQUIREMENTS.md](REQUIREMENTS.md)
- Specifications: [SPECIFICATIONS.md](SPECIFICATIONS.md)
- CRC diagram: [docs/CRC_diagram.drawio.png](docs/CRC_diagram.drawio.png)
- ER diagram: [docs/ER_diagram.drawio.png](docs/ER_diagram.drawio.png)

## Prerequisites

- C++17 compiler (Visual Studio 2022 on Windows, Xcode command line tools on macOS, g++ on Linux)
- CMake 3.20 or newer
- Qt 6 with the Widgets and SQL modules and the SQLite driver

On Ubuntu, install them with:

```bash
sudo apt-get install -y build-essential cmake qt6-base-dev libqt6sql6-sqlite
```

## Set up

Clone the repo and move into it:

```bash
git clone https://github.com/jaydensu666666-creator/Group6-BudgetManager.git
cd Group6-BudgetManager
```

Configure and build:

```bash
cmake -S . -B build
cmake --build build
```

If CMake can't find Qt, point it at your Qt install, for example:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.10.3/msvc2022_64
```

On macOS with the 26.5 SDK, configure with:

```bash
cmake -S . -B build -DCMAKE_OSX_SYSROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk
```

## Run the app

macOS and Linux:

```bash
./build/app
```

Windows PowerShell (Visual Studio puts the program in a `Debug` folder, and Qt's
`bin` folder must be on `PATH`; change the Qt path to match your install):

```powershell
$env:PATH = "C:\Qt\6.10.3\msvc2022_64\bin;$env:PATH"
.\build\Debug\app.exe
```

Your data is saved in `budget.db` in your app data folder (for example
`%APPDATA%\Group6BudgetManager` on Windows), so it is still there next time.

## Run the tests

macOS and Linux:

```bash
ctest --test-dir build --output-on-failure
```

Windows PowerShell (Visual Studio builds can hold more than one configuration, so
name it; Qt's `bin` folder must be on `PATH` as above):

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

CI runs the same build and tests on every push (`.github/workflows/ci.yml`).

## How to use it

1. Open **Term setup**. Enter the money you received, the amount to set aside
   for saving, the term start date and the date your next transfer arrives.
   Press **Save term**.
2. Open **Expenses**. Enter an amount, a date and an optional note, then press
   **Add expense** (or Enter). To fix one, select it and press **Edit selected**
   (or double-click it), change it, and press **Save changes**. To delete one,
   select it and press **Remove selected**. **Undo** reverses your last change.
   After each change the page shows how much you have spent and how much is left.
3. Open **Where I stand** to see the money left to spend, how much you can still
   spend this week, and whether your money will last until the next transfer.
4. When your next transfer arrives, open **Term setup**. The new start date is
   filled in for you. Enter the new money and press **Save term** to start the
   next term. Last term's expenses are kept but no longer listed.

Example: you received $1,300 on Nov 2, set aside $100, and your next transfer is
Dec 2. After spending $400 by Nov 11 ($40 a day), the app says your money will
just last. Spend $410 instead and it warns you will run out around Dec 1.

## Project architecture

The app follows MVC. Views send every action to a controller, controllers update
the model, and only the model touches the database (only `BudgetRepository` runs
SQL). The database keeps one budget row per term, and every purchase names the
term it belongs to, so starting a new term can not pull last term's purchases
into it. Views also use two small helpers that hold no data: `Money` to show
amounts and `ViewStyle` for shared colors.

| Layer | File | What it does |
|---|---|---|
| Model | `src/Budget.h/.cpp` | The term's money and its expenses; every calculation (money left, pace, this week) and undo |
| Model | `src/Expense.h` | One recorded purchase |
| Model | `src/BudgetRepository.h/.cpp` | Saves and loads the current term and its purchases with SQLite, one budget row per term (the only class that runs SQL) |
| Helper | `src/Money.h/.cpp` | Reads amounts the user typed and formats them as `-$2,500.00` (used by every layer) |
| Controller | `src/TermController.h/.cpp` | Feature: Planning the term |
| Controller | `src/ExpenseController.h/.cpp` | Feature: Recording money in and out (with undo) |
| Controller | `src/StatusController.h/.cpp` | Feature: Checking where they stand |
| View | `src/TermSetupView.h/.cpp` | Term setup page |
| View | `src/ExpensesView.h/.cpp` | Expenses page |
| View | `src/StatusView.h/.cpp` | Where I stand page (home) |
| View | `src/MainWindow.h/.cpp` | Window title and tabs |
| View | `src/ViewStyle.h` | Colors and date format shared by the views |
| - | `src/ActionResult.h` | What a controller tells a view: worked or not, and the message |
| - | `src/main.cpp` | Opens the database and connects the pieces |
| Tests | `tests/*.cpp` | doctest unit tests for the model classes |

Tech stack: C++17, Qt 6 Widgets for the interface, Qt SQL with SQLite for storage,
doctest for unit tests, CMake for building, and GitHub Actions for CI.
