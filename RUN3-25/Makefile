CC = gcc
MPICC = mpicc
LDFLAGS = -lm

BUILD_DIR = build

TARGETS = $(BUILD_DIR)/generator $(BUILD_DIR)/mul_sequential $(BUILD_DIR)/mul_parallel

all: $(BUILD_DIR) $(TARGETS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/generator: generator.c
	$(CC) generator.c -o $(BUILD_DIR)/generator

$(BUILD_DIR)/mul_sequential: mul_sequential.c
	$(CC) mul_sequential.c -o $(BUILD_DIR)/mul_sequential

$(BUILD_DIR)/mul_parallel: mul_parallel.c
	$(MPICC) mul_parallel.c -o $(BUILD_DIR)/mul_parallel $(LDFLAGS)

clean:
	rm -f $(BUILD_DIR)/generator $(BUILD_DIR)/mul_sequential $(BUILD_DIR)/mul_parallel
	rmdir $(BUILD_DIR) 2>/dev/null || true

.PHONY: all clean
