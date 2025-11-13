# ==============================================================================
# Unit Tests (C++ + Google Test)
# ==============================================================================

# Unit test executable name
UNIT_TEST_NAME = unit_tester

# Compiler flags for unit tests (C++17 for Google Test)
UNIT_TEST_CXXFLAGS = -std=c++17 -Wall -Wextra -Werror

# Directories
UNIT_TEST_DIR = tests/unit
GTEST_DIR = googletest
UNIT_TEST_BUILD_DIR = build/tests

# Google Test settings
GTEST_VERSION = 1.17.0
GTEST_URL = https://github.com/google/googletest/archive/refs/tags/v$(GTEST_VERSION).tar.gz

# Source files
UNIT_TEST_SRC = $(wildcard $(UNIT_TEST_DIR)/*.cpp)
UNIT_TEST_OBJ = $(patsubst $(UNIT_TEST_DIR)/%.cpp, $(UNIT_TEST_BUILD_DIR)/%.o, $(UNIT_TEST_SRC))
UNIT_TEST_DEP = $(patsubst $(UNIT_TEST_DIR)/%.cpp, $(UNIT_TEST_BUILD_DIR)/%.d, $(UNIT_TEST_SRC))

# Exclude main from test object files
OBJ_EXCLUDE_MAIN = $(filter-out $(BUILD_DIR)/main.o,$(OBJ))

# Google Test build marker
GTEST_MARKER = $(GTEST_DIR)/.built

# Main test rule
.PHONY: unit
unit: all $(UNIT_TEST_NAME)
	@echo "Running unit tests..."
	@./$(UNIT_TEST_NAME)

# Build the test executable
$(UNIT_TEST_NAME): $(GTEST_MARKER) $(UNIT_TEST_OBJ) $(OBJ_EXCLUDE_MAIN)
	@echo "Linking $@..."
	@$(CXX) $(UNIT_TEST_CXXFLAGS) $(UNIT_TEST_OBJ) $(OBJ_EXCLUDE_MAIN) -o $@ -L$(GTEST_DIR)/lib -lgtest -lgtest_main -lpthread

-include $(UNIT_TEST_DEP)

# Compile test source files
$(UNIT_TEST_BUILD_DIR)/%.o: $(UNIT_TEST_DIR)/%.cpp
	@mkdir -p $(@D)
	@echo "Compiling $<..."
	@$(CXX) $(UNIT_TEST_CXXFLAGS) $(DEP_FLAGS) -I$(GTEST_DIR)/googletest/include -I$(INC_DIR) -c $< -o $@

# Build Google Test
$(GTEST_MARKER): $(GTEST_DIR)/CMakeLists.txt
	@echo "Building Google Test..."
	@cd $(GTEST_DIR) && cmake -DCMAKE_CXX_FLAGS=-std=c++17 . && make
	@touch $@

$(GTEST_DIR)/CMakeLists.txt:
	@echo "Downloading Google Test..."
	@mkdir -p $(GTEST_DIR)
	@curl -L $(GTEST_URL) | tar xz -C $(GTEST_DIR) --strip-components=1

# Clean unit test files
.PHONY: unit-clean
unit-clean:
	@echo "Cleaning unit test files..."
	@$(RM) -r $(UNIT_TEST_BUILD_DIR) $(UNIT_TEST_NAME) $(GTEST_DIR)

# Full rebuild of unit tests
.PHONY: unit-re
unit-re: unit-clean unit

# ==============================================================================
# E2E Tests (Python + pytest)
# ==============================================================================

.PHONY: e2e
e2e: $(NAME)
	@if [ ! -d "tests/e2e/.venv" ]; then \
		echo "Setting up e2e test environment..."; \
		cd tests/e2e && uv sync; \
	fi
	@echo "Starting IRC server..."
	@./$(NAME) 6667 password > /dev/null 2>&1 & \
	SERVER_PID=$$!; \
	echo "Server started (PID: $$SERVER_PID)"; \
	sleep 1; \
	echo "Running e2e tests..."; \
	cd tests/e2e && uv run pytest -v; \
	TEST_EXIT=$$?; \
	echo "Stopping IRC server (PID: $$SERVER_PID)..."; \
	kill $$SERVER_PID 2>/dev/null || true; \
	wait $$SERVER_PID 2>/dev/null || true; \
	exit $$TEST_EXIT

.PHONY: e2e-setup
e2e-setup:
	@echo "Installing uv and setting up e2e environment..."
	@if ! command -v uv > /dev/null 2>&1; then \
		echo "Installing uv..."; \
		curl -LsSf https://astral.sh/uv/install.sh | sh; \
	fi
	cd tests/e2e && uv sync
	@echo "E2E environment ready!"

.PHONY: e2e-clean
e2e-clean:
	$(RM) -r tests/e2e/.venv tests/e2e/.pytest_cache

# ==============================================================================
# Combined test commands
# ==============================================================================

.PHONY: test
test: unit

.PHONY: test-all
test-all: unit e2e

.PHONY: test-clean
test-clean: unit-clean e2e-clean
