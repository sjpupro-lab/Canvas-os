# Canvas OS — Engine Makefile
# Usage:
#   make          — build engine binaries
#   make test     — run 357 determinism tests (0 failures expected)

CC      := gcc
CFLAGS  := -Wall -Wextra -Wpedantic -std=c11 \
           -Iinclude \
           -DSJ_GPU=0 \
           -D_POSIX_C_SOURCE=200809L

# Strict: treat warnings as errors in test build
CFLAGS_TEST := $(CFLAGS)

SRC_DIR  := src
TEST_DIR := tests
BUILD    := build

SRCS := $(SRC_DIR)/engine.c \
        $(SRC_DIR)/whitehole.c \
        $(SRC_DIR)/blackhole.c \
        $(SRC_DIR)/lane_exec.c \
        $(SRC_DIR)/canvas_branch.c \
        $(SRC_DIR)/scheduler.c \
        $(SRC_DIR)/shell.c \
        $(SRC_DIR)/pixelcode.c \
        $(SRC_DIR)/tervas.c \
        $(SRC_DIR)/canvas_gpu_stub.c \
        $(SRC_DIR)/fd_canvas_bridge.c \
        $(SRC_DIR)/vm_runtime_bridge.c

TEST_SRCS := $(TEST_DIR)/test_main.c \
             $(TEST_DIR)/test_dk.c \
             $(TEST_DIR)/test_engine.c \
             $(TEST_DIR)/test_whitehole.c \
             $(TEST_DIR)/test_blackhole.c \
             $(TEST_DIR)/test_rulecell.c \
             $(TEST_DIR)/test_scheduler.c \
             $(TEST_DIR)/test_shell.c \
             $(TEST_DIR)/test_gpu.c \
             $(TEST_DIR)/test_determinism.c

OBJS      := $(patsubst $(SRC_DIR)/%.c, $(BUILD)/%.o, $(SRCS))
TEST_OBJS := $(patsubst $(TEST_DIR)/%.c, $(BUILD)/test_%.o, $(TEST_SRCS))
TEST_BIN  := $(BUILD)/canvasos_test

.PHONY: all test clean dirs

all: dirs $(TEST_BIN)

dirs:
	@mkdir -p $(BUILD)

$(BUILD)/%.o: $(SRC_DIR)/%.c | dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/test_%.o: $(TEST_DIR)/%.c | dirs
	$(CC) $(CFLAGS_TEST) -Itests -c $< -o $@

$(TEST_BIN): $(OBJS) $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

test: $(TEST_BIN)
	@echo ""
	@echo "Running Canvas OS test suite..."
	@$(TEST_BIN)

clean:
	@rm -rf $(BUILD)

# Convenience targets
.PHONY: check
check: test
