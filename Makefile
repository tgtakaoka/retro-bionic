# Convenience targets for the PlatformIO project.

PIO ?= pio
ENV ?= teensy41

.PHONY: compiledb test
compiledb: ## Regenerate compile_commands.json for clangd
	$(PIO) run -e $(ENV) -t compiledb

test: ## Run the host-side unit tests
	$(PIO) test -e native
