# Development Guide

This project is a Qt/C++ desktop application with a Makefile-first workflow and CMake presets for editor and CI integration.

## Local Loop

Use the Makefile targets for day-to-day work:

```bash
make build
make test
make coverage
```

For the complete gate:

```bash
make verify
```

`make verify` runs formatting, tests, and coverage. It requires `clang-format` to be installed.

## Formatting

Formatting is controlled by `.clang-format`.

Apply formatting:

```bash
make format
```

Check formatting without modifying files:

```bash
make format-check
```

Only tracked C++ source and header files are formatted. This keeps generated files, build output, and local scratch files out of the formatting path.

## Build Configuration

The default Makefile build uses:

- `CMAKE_BUILD_TYPE=Debug`
- `USE_SIM_COMS=ON`
- `QT_PREFIX=/usr/lib/x86_64-linux-gnu/cmake`

Override variables as needed:

```bash
make build CMAKE_BUILD_TYPE=Release
make coverage COVERAGE_MIN=90
```

## Presets

CMake presets are available for tools and IDEs:

```bash
cmake --preset dev
cmake --preset release
cmake --preset coverage
```
