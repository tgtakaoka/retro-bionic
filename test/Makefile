# Host tests and their fixtures; from test/:
#
#   make                  run every host test
#   make test T=ins8070   run some, by test directory
#   make fixtures         rings and goldens from the recordings
#   make check-fixtures   make them aside and report which differ
#
# T limits fixtures and check-fixtures too: T="ins8070 mc6800".

PROJ := $(abspath ..)
PIO ?= pio
PYTHON ?= python3
T ?=

.PHONY: all test fixtures check-fixtures help

all: test

test:
ifeq ($(strip $(T)),)
	$(PIO) test -d $(PROJ) -e native
else
	$(PIO) test -d $(PROJ) -e native $(foreach t,$(T),-f "$(t)*")
endif

fixtures:
	$(PYTHON) $(PROJ)/test/recordings_to_fixtures.py $(T)

check-fixtures:
	$(PYTHON) $(PROJ)/test/recordings_to_fixtures.py --check $(T)

help:
	@sed -n '1,/^$$/s/^# \{0,1\}//p' $(firstword $(MAKEFILE_LIST))
