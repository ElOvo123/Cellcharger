BUILD_DIR := build
BUILD_COVERAGE_DIR := build-coverage
TARGET := qt_app
QT_PREFIX := /usr/lib/x86_64-linux-gnu/cmake

.PHONY: all
all: build

.PHONY: build
build:
	@echo "Building project..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_PREFIX_PATH=$(QT_PREFIX) ..
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
	@cd $(BUILD_DIR) && ctest --output-on-failure

.PHONY: coverage
coverage:
	@echo "Building project with coverage instrumentation..."
	@rm -rf $(BUILD_COVERAGE_DIR)
	@mkdir -p $(BUILD_COVERAGE_DIR)
	@cd $(BUILD_COVERAGE_DIR) && cmake -DCMAKE_PREFIX_PATH=$(QT_PREFIX) -DENABLE_COVERAGE=ON ..
	@cd $(BUILD_COVERAGE_DIR) && cmake --build .
	@echo "Running tests with coverage build..."
	@cd $(BUILD_COVERAGE_DIR) && ctest --output-on-failure
	@echo "Coverage artifacts generated under $(BUILD_COVERAGE_DIR)."
	@find $(BUILD_COVERAGE_DIR) -name '*.gcda' -print0 | \
	xargs -0 -n1 gcov -n > $(BUILD_COVERAGE_DIR)/coverage-summary.txt
	@python3 scripts/coverage_summary.py $(BUILD_COVERAGE_DIR)
.PHONY: coverage-report
coverage-report: coverage
	@echo "Coverage summary saved to $(BUILD_COVERAGE_DIR)/coverage-summary.txt"
	@grep -E "^File '$(BUILD_COVERAGE_DIR)|^Lines executed:" $(BUILD_COVERAGE_DIR)/coverage-summary.txt || true

.PHONY: rebuild
rebuild: clean build
