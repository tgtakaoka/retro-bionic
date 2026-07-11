# Convenience targets for the PlatformIO project.

PIO ?= pio
ENV ?= teensy41

.PHONY: compiledb
compiledb: ## Regenerate compile_commands.json for clangd
	$(PIO) run -e $(ENV) -t compiledb
