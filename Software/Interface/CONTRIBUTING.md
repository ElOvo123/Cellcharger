# Contributing

## Development Standard

Changes should be small, focused, and covered by the appropriate test layer:

- Unit tests for pure logic, models, parser/formatter behavior, and widget-local behavior.
- Integration tests for flows across controller/backend/database boundaries.
- Smoke tests for application shell construction, panel creation, rendering, and critical UI wiring.

Prefer existing project patterns over new abstractions. Keep hardware-dependent behavior behind simulated or guarded paths so CI and local development remain reliable.

## Before Submitting

Run:

```bash
make verify
```

For a faster local loop:

```bash
make test-unit
make test-integration
make test-smoke
```

For style drift:

```bash
make format-check
```

## Coverage

`make coverage` enforces `COVERAGE_MIN`, defaulting to `85.0`.

To raise the bar:

```bash
make coverage COVERAGE_MIN=90
```

Do not lower the coverage threshold to hide missing tests. If a line is difficult to cover because it depends on hardware or OS facilities, prefer an integration-safe seam, a simulated backend path, or a guarded skip with a clear reason.

## Style

- C++ standard: C++17.
- Build defaults: Debug with exported compile commands for editor tooling.
- Qt style follows the existing codebase.
- Project warnings are enabled by default through CMake. Keep new code warning-clean.
- Keep comments short and useful.
- Avoid unrelated refactors in feature or bug-fix changes.
- Do not commit build outputs, coverage outputs, generated files, or local IDE files.

## Test Labels

CTest labels are available:

```bash
ctest -L unit --output-on-failure
ctest -L integration --output-on-failure
ctest -L smoke --output-on-failure
```

The Makefile exposes these as `make test-unit`, `make test-integration`, and `make test-smoke`.

## Pull Requests

Use the pull request template checklist. Include the commands you ran and note any skipped hardware-dependent checks with a clear reason.
