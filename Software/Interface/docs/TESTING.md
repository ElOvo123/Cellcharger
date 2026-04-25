# Testing Guide

The project uses QtTest and CTest. Tests are split into three layers so failures are easier to understand.

## Layers

| Layer | Target | Purpose |
| --- | --- | --- |
| Unit | `make test-unit` | Logic, models, formatters, parsers, isolated widgets |
| Integration | `make test-integration` | Cross-component flows such as ComsController, simulated backend, PCP database |
| Smoke | `make test-smoke` | Main window construction, panel creation, offscreen rendering, critical UI wiring |
| All | `make test` | Every registered test |
| Verification | `make verify` | Full tests plus coverage gate |

CTest presets mirror the layers:

```bash
ctest --preset unit
ctest --preset integration
ctest --preset smoke
```

## Coverage

Run:

```bash
make coverage
```

Artifacts:

- `build-coverage/coverage-summary.txt`: raw gcov summary
- console output: overall project coverage, lowest-covered production files, coverage gate result

The default threshold is:

```text
COVERAGE_MIN=85.0
```

Override:

```bash
make coverage COVERAGE_MIN=90
```

## Writing New Tests

Use the narrowest layer that covers the behavior:

- Add logic-only tests to `core_tests.cpp` or the relevant focused suite.
- Add widget behavior to `widget_tests.cpp` or `profile_setup_tests.cpp`.
- Add cross-component behavior to `integration_tests.cpp`.
- Add application-shell checks to `smoke_tests.cpp`.

Keep tests deterministic:

- Use `QT_QPA_PLATFORM=offscreen`.
- Prefer the simulated Coms backend.
- Avoid real serial/CAN hardware.
- If a local OS facility is unavailable, skip with `QSKIP` and a clear reason.

## CI

The GitHub Actions workflow runs:

```bash
make test
make coverage
```

This mirrors the local professional gate. The workflow also sets Qt to offscreen mode, has a timeout, cancels superseded runs on the same branch, and uploads `coverage-summary.txt` as an artifact.
