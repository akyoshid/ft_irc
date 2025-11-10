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
