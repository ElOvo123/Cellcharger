BUILD_DIR := build
BUILD_COVERAGE_DIR := build-coverage
TARGET := qt_app
QT_PREFIX := /usr/lib/x86_64-linux-gnu/cmake
COVERAGE_MIN ?= 85.0
CMAKE_BUILD_TYPE ?= Debug
CTEST_ARGS ?= --output-on-failure

.PHONY: all
all: build

.PHONY: configure
configure:
	@echo "Configuring project..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_PREFIX_PATH=$(QT_PREFIX) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) ..

.PHONY: build
build: configure
	@echo "Building project..."
	@cd $(BUILD_DIR) && cmake --build .

.PHONY: run
run: build
	@echo "Running application..."
	@cd $(BUILD_DIR) && \
	export QT_QPA_PLATFORM=xcb && \
	export DISPLAY=:0 && \
	./$(TARGET)


.PHONY: clean
clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)

.PHONY: test
test: build
	@echo "Running tests..."
	@cd $(BUILD_DIR) && ctest $(CTEST_ARGS)

.PHONY: test-unit
test-unit: build
	@echo "Running unit tests..."
	@cd $(BUILD_DIR) && ctest -L unit $(CTEST_ARGS)

.PHONY: test-integration
test-integration: build
	@echo "Running integration tests..."
	@cd $(BUILD_DIR) && ctest -L integration $(CTEST_ARGS)

.PHONY: test-smoke
test-smoke: build
	@echo "Running smoke tests..."
	@cd $(BUILD_DIR) && ctest -L smoke $(CTEST_ARGS)

.PHONY: coverage
coverage:
	@echo "Building project with coverage instrumentation..."
	@rm -rf $(BUILD_COVERAGE_DIR)
	@mkdir -p $(BUILD_COVERAGE_DIR)
	@cd $(BUILD_COVERAGE_DIR) && cmake -DCMAKE_PREFIX_PATH=$(QT_PREFIX) -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
	@cd $(BUILD_COVERAGE_DIR) && cmake --build .
	@echo "Running tests with coverage build..."
	@cd $(BUILD_COVERAGE_DIR) && ctest $(CTEST_ARGS)
	@echo "Coverage artifacts generated under $(BUILD_COVERAGE_DIR)."
	@find $(BUILD_COVERAGE_DIR) -name '*.gcda' -print0 | \
	xargs -0 -n1 gcov -n > $(BUILD_COVERAGE_DIR)/coverage-summary.txt
	@python3 scripts/coverage_summary.py $(BUILD_COVERAGE_DIR) $(COVERAGE_MIN)
.PHONY: coverage-report
coverage-report: coverage
	@echo "Coverage summary saved to $(BUILD_COVERAGE_DIR)/coverage-summary.txt"
	@grep -E "^File '$(BUILD_COVERAGE_DIR)|^Lines executed:" $(BUILD_COVERAGE_DIR)/coverage-summary.txt || true

.PHONY: rebuild
rebuild: clean build

.PHONY: format-check
format-check:
	@command -v clang-format >/dev/null || { echo "clang-format is not installed."; exit 1; }
	@find . -path './build*' -prune -o \( -name '*.cpp' -o -name '*.h' \) -print | \
	xargs clang-format --dry-run --Werror

.PHONY: ci
ci: test coverage

.PHONY: verify
verify: ci
