TARGET=ppm2jpeg
CC = gcc
LD = gcc
VENV = .venv
ifdef CI
	PYTHON = python3
	VENV_DEP=
else
	PYTHON = $(VENV)/bin/python3
	PIP = $(VENV)/bin/pip
	REQS = requirements.txt
	VENV_DEP= $(VENV)/bin/activate
endif

CFLAGS += -Wall -Wextra -std=c99 -Iinclude -Itests/unity -MMD -MP 
LDFLAGS = -lm

UNITY_DIR = tests/unity
UNITY_FILES = $(UNITY_DIR)/unity.c $(UNITY_DIR)/unity.h $(UNITY_DIR)/unity_internals.h
URL_BASE = https://raw.githubusercontent.com/ThrowTheSwitch/Unity/master/src
OBJ_DIR   = obj
JPG_DIR   = out
SRC_FILES = $(wildcard src/*.c)
OBJ_FILES = $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))
TEST_OBJS = $(filter-out $(OBJ_DIR)/main.o, $(OBJ_FILES))
TEST_SRCS = $(wildcard tests/test_*.c)
TEST_BINS = $(TEST_SRCS:.c=.bin)
DEPS 	  = $(OBJ_FILES:.o=.d) $(patsubst tests/%.c,$(OBJ_DIR)/%.d,$(TEST_SRCS))

.PHONY: all debug tests perf clean realclean help
##@ compilation
all: CFLAGS  += -g -O2 -fsanitize=address --coverage $(ERR)
all: LDFLAGS += -g -fsanitize=address --coverage
all: $(TARGET) $(TEST_BINS) 

debug: CFLAGS  += -g -Og -fsanitize=address 
debug: LDFLAGS += -g -fsanitize=address 
debug: clean $(TARGET) $(TEST_BINS) ##! Nettoie et recompile en mode debug, car gdb sans glitch c'est mieux

tests: $(VENV_DEP) all ##! Compile et lance les tests
	-$(PYTHON) -m pytest -k "test_c_unity or test_integration" #-n 4

perf: CFLAGS += -O3 -fno-omit-frame-pointer -ffast-math
perf: clean $(TARGET) ##! Nettoie et recompile en mode perf (O3 sans instrumentation) et évalue la perf de votre bouzin
	-$(PYTHON) -m pytest -k "test_performance or test_memory" 

help:
	@awk 'BEGIN {FS = ":.*##!"; printf "Usage: make \033[32m<commande>\033[0m \
	\nCommandes par \033[36mcatégories :\n"} \
	/^[a-zA-Z0-9_-]+:.*##!/ { printf "  \033[32m%-15s\033[0m %s\n", $$1, $$2 } \
	/^##@/ { printf "\n\033[36m%s\033[0m\n", substr($$0, 5) }' $(MAKEFILE_LIST)

# Inclusion des dépendances auto
-include $(DEPS)

$(JPG_DIR) $(UNITY_DIR) $(OBJ_DIR):
	@mkdir -p $@

$(UNITY_DIR)/%: | $(UNITY_DIR)
	@curl -Ls $(URL_BASE)/$(notdir $@) -o $@

$(OBJ_DIR)/unity.o: $(UNITY_FILES) | $(OBJ_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR)/test_%.o: tests/test_%.c  | $(OBJ_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

tests/test_%.bin: $(OBJ_DIR)/test_%.o $(TEST_OBJS) $(OBJ_DIR)/unity.o
	$(LD) $^ $(LDFLAGS) -o $@

$(TARGET): $(OBJ_FILES) | $(JPG_DIR)
	$(LD) $^ $(LDFLAGS) -o $@

##@ Outillage
couverture: clean tests ##! Tester, c'est douter ? Améliorer votre couverture de code
	gcovr --exclude-unreachable-branches --print-summary
	firefox coverage.html

profilage: perf  ##! Rien de mieux pour observer et optimiser la performance de votre code
	valgrind --tool=callgrind --callgrind-out-file=callgrind.biiiiiig.out ./$(TARGET) --outfile=out/biiiiiig.jpg images/etu/biiiiiig.ppm
	kcachegrind callgrind.biiiiiig.out

##@ Hygiène

$(VENV)/bin/activate: $(REQS)
	@echo "🔧 Création de l'environnement virtuel..."
	python3 -m venv $(VENV)
	@$(PIP) install --upgrade pip
	@$(PIP) install -r $<
	@touch $@

clean: ##! Nettoie
	rm -rf $(TARGET) $(OBJ_FILES) tests/*.bin $(OBJ_DIR)/test_%.o $(OBJ_DIR)/unity.o

realclean: clean ##! Nettoie à fond : idéal avant un commit
	rm -rf tests/*.d $(OBJ_DIR) $(JPG_DIR) coverage.* callgrind.* report.* *.xml *.html *.json public massif.*
	find . -type d -name "__pycache__" -exec rm -rf {} +
	find . -type d -name ".pytest_cache" -exec rm -rf {} +
