# Dev Dialogue — Desktop Client (Qt 6 / QML)

Native C++20 desktop client for the Dev Dialogue technical interview simulator.
The application mirrors the behaviour of the Next.js web client: it talks to
the same REST API, but stays fully usable offline through the embedded
interview engine and the local question bank.

## Requirements

| Dependency | Version | Notes |
| :--- | :--- | :--- |
| Qt | 6.5+ (developed against 6.12) | Core, Gui, Qml, Quick, QuickControls2, Network, Svg, Concurrent, LinguistTools, Test |
| CMake | 3.25+ | |
| nlohmann_json | 3.11.3+ | via `vcpkg.json` or the system package |
| GoogleTest | 1.14+ | only required for the unit tests |

## Build

```bash
cmake -S apps/desktop -B apps/desktop/build -DCMAKE_BUILD_TYPE=Release
cmake --build apps/desktop/build --parallel
```

The executable is written to `apps/desktop/build/DevDialogue.exe`
(`DevDialogue` on Linux/macOS).

### Windows (MSYS2 / MinGW)

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH
cmake -S apps/desktop -B apps/desktop/build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="C:/msys64/ucrt64"
cmake --build apps/desktop/build
```

## Tests

```bash
ctest --test-dir apps/desktop/build --output-on-failure
```

The suite has three parts:

| Suite | Framework | Contents |
| :--- | :--- | :--- |
| `DevDialogueUnitTests` | GoogleTest | JSON contract roundtrips and the offline engine |
| `DevDialogueViewModelTests` | Qt Test (`QSignalSpy`) | Reactive behaviour of the view models |
| `QmlTests` | Qt Quick Test | QML singletons and components |

## Translations

The UI strings live in `translations/devdialogue_uk.ts` and are compiled into
the binary as `:/i18n/devdialogue_uk.qm`. After changing a `qsTr()` string the
catalog is refreshed with:

```bash
cd apps/desktop
lupdate qml src -ts translations/devdialogue_uk.ts -no-obsolete
```

The language can be switched at runtime from the navigation bar (or the
landing screen); the choice is persisted with `QSettings`.

## Packaging

```powershell
pwsh apps/desktop/scripts/package-windows.ps1 -Installer
```

The script builds the release configuration, deploys the Qt runtime with
`windeployqt`, stages the bundle in `dist/package` and (with `-Installer`)
compiles `installer/installer.iss` into `dist/DevDialogue-Setup-x64.exe`.

## Layout

```
apps/desktop/
├── CMakeLists.txt        # targets, QML module, translations
├── include/              # public headers (core, models, network, services, viewmodels)
├── src/                  # implementation
├── qml/                  # QML module (styles, components, views)
├── resources/            # icons and the Geist variable fonts
├── translations/         # Qt Linguist catalogs
├── tests/                # GoogleTest, Qt Test and Qt Quick Test suites
├── installer/            # Inno Setup script
└── scripts/              # packaging helpers
```
