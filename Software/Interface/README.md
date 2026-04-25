# CellCharger Interface

Qt/C++ desktop interface for configuring CellCharger communication, monitoring charger status, viewing logs/console traffic, and running charge profile setup workflows.

## Features

- Multi-panel Qt interface for Coms, Console, Log, Charger Status, and Profile Setup.
- PCP YAML database loading, encoding, decoding, and formatted console output.
- Simulated communications backend for local development and automated tests.
- Profile setup plotting with active markers for voltage, current, and temperature.
- Unit, integration, smoke, and coverage-gated test workflow.

## Requirements

- CMake 3.16+
- C++17 compiler
- Qt 6 modules: Widgets, SerialPort, Network, Test
- yaml-cpp
- gcov/gcc coverage tools for `make coverage`

On Ubuntu-like systems the core dependencies are typically:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake qt6-base-dev qt6-serialport-dev libyaml-cpp-dev
```

## Build And Run

```bash
make build
make run
```

The default build uses the simulated communications backend (`USE_SIM_COMS=ON`) so the application and tests can run without hardware.

For an explicit configure step:

```bash
make configure
```

Useful Make variables:

- `CMAKE_BUILD_TYPE=Debug|Release`
- `QT_PREFIX=/path/to/qt/cmake`
- `COVERAGE_MIN=85.0`

## Test Workflow

Run the full suite:

```bash
make test
```

Run specific layers:

```bash
make test-unit
make test-integration
make test-smoke
```

Run coverage with the default professional gate:

```bash
make coverage
```

The default coverage threshold is `85.0%`. Override it when intentionally ratcheting the bar:

```bash
make coverage COVERAGE_MIN=90
```

## CMake Presets

The repository includes `CMakePresets.json` for tool-friendly workflows:

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

Release preset:

```bash
cmake --preset release
cmake --build --preset release
```

Coverage preset:

```bash
cmake --preset coverage
cmake --build --preset coverage
ctest --preset coverage
```

Layer-specific test presets are available:

```bash
ctest --preset unit
ctest --preset integration
ctest --preset smoke
```

## Project Layout

- `backend/`: communications, logger, PCP database/codec logic.
- `controller/`: UI/backend orchestration.
- `ui/`: Qt widgets and panels.
- `tests/`: QtTest-based unit, integration, and smoke tests.
- `config/pcp.yaml`: PCP device/message/signal database.
- `scripts/`: project tooling, including coverage summary gate.

## Quality Expectations

Before sharing changes, run:

```bash
make verify
```

This runs the full test suite and the coverage gate.

Additional project hygiene:

```bash
make format-check
```

CI runs the same professional gate and uploads the coverage summary as a workflow artifact.
