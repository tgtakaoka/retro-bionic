# Host tests and their fixtures; from test/:
#
#   make                  run every host test
#   make test T=cpm       the CP/M disk layout (Python)
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
	$(PYTHON) -m unittest discover -s $(PROJ)/test/cpm
else
ifneq ($(strip $(filter-out cpm,$(T))),)
	$(PIO) test -d $(PROJ) -e native $(foreach t,$(filter-out cpm,$(T)),-f "$(t)*")
endif
ifneq ($(filter cpm,$(T)),)
	$(PYTHON) -m unittest discover -s $(PROJ)/test/cpm
endif
endif

fixtures:
	$(PYTHON) $(PROJ)/test/recordings_to_fixtures.py $(T)

check-fixtures:
	$(PYTHON) $(PROJ)/test/recordings_to_fixtures.py --check $(T)

help:
	@sed -n '1,/^$$/s/^# \{0,1\}//p' $(firstword $(MAKEFILE_LIST))
