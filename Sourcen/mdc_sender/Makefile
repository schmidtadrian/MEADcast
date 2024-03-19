TARGET_EXEC := send

SRC_DIRS := ./src
BUILD_DIR := ./build

SRCS := $(shell find $(SRC_DIRS) -name '*.c')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# CFLAGS := -Wall -Wextra -lpthread -g
CFLAGS := -Wall -Wextra -lpthread -O3
LDFLAGS := -lJudy -latomic


# The final build step.
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Build step for C source
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: run
run:
	$(BUILD_DIR)/$(TARGET_EXEC) $(IP)

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)
