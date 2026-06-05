# Output directories
SRC_DIR := src
BIN_DIR := bin
OBJ_DIR := obj
INC_DIR := include


LINUX_OBJ_DIR := obj/linux
WIN32_OBJ_DIR := obj/win32
WIN64_OBJ_DIR := obj/win64


# Target binaries
LINUX_BIN := $(BIN_DIR)/linux/psx-combine-cli
WIN32_BIN := $(BIN_DIR)/win32/psx-combine-cli.exe
WIN64_BIN := $(BIN_DIR)/win64/psx-combine-cli.exe


# Source files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)


# Object files
LINUX_OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(LINUX_OBJ_DIR)/%.o, $(SRCS))
WIN32_OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(WIN32_OBJ_DIR)/%.o, $(SRCS))
WIN64_OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(WIN64_OBJ_DIR)/%.o, $(SRCS))


# Compilers
LINUX_CC := g++
WIN32_CC := i686-w64-mingw32-g++
WIN64_CC := x86_64-w64-mingw32-g++


###
# Linux Compile and Linker Flags
LINUX_CFLAGS  := -I$(INC_DIR) -O2 -Wall -std=c++17
LINUX_LDFLAGS := -static -static-libgcc -static-libstdc++
LINUX_LDLIBS  :=


###
# Windows Compile and Linker Flags
# (Don't know if those work, haven't used windows in years, it's better this way..)
WIN64_CFLAGS  := -I$(INC_DIR) -O2 -Wall
WIN64_LDFLAGS := -static -static-libgcc -static-libstdc++
WIN64_LDLIBS  :=

WIN32_CFLAGS  := -I$(INC_DIR) -O2 -Wall
WIN32_LDFLAGS := -static -static-libgcc -static-libstdc++
WIN32_LDLIBS  :=


# Phony targets
.PHONY: all linux win32 win64 clean

# Default target: linux
all: linux


# Linux build
linux: $(LINUX_BIN)

$(LINUX_BIN): $(LINUX_OBJS) | $(BIN_DIR)/linux
	$(LINUX_CC) $(LINUX_LDFLAGS) $^ $(LINUX_LDLIBS) -o $@

$(LINUX_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(LINUX_OBJ_DIR)
	$(LINUX_CC) $(LINUX_CFLAGS) -c $< -o $@


# Windows 64-bit build
win64: $(WIN64_BIN)

$(WIN64_BIN): $(WIN64_OBJS) | $(BIN_DIR)/win64
	$(WIN64_CC) $^ $(WIN64_LDLIBS) -o $@

$(WIN64_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(WIN64_OBJ_DIR)
	$(WIN64_CC) $(WIN64_CFLAGS) -c $< -o $@
    
    
# Windows 32-bit build
win32: $(WIN32_BIN)

$(WIN32_BIN): $(WIN32_OBJS) | $(BIN_DIR)/win32
	$(WIN32_CC) $^ $(WIN32_LDLIBS) -o $@

$(WIN32_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(WIN32_OBJ_DIR)
	$(WIN32_CC) $(WIN32_CFLAGS) -c $< -o $@


# Create directories
$(BIN_DIR)/linux $(BIN_DIR)/win32 $(BIN_DIR)/win64:
	mkdir -p $@

$(LINUX_OBJ_DIR) $(WIN32_OBJ_DIR) $(WIN64_OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)
