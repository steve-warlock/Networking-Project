# Binaries
CLIENT_TARGET = build/runner_client.out
SERVER_TARGET = build/runner_server.out

# Directories
BUILD_DIR = build # Build output directory
LOG_DIR = logs # Logs directory

# Client directories
CLIENT_DIR = client
CLIENT_SRC_DIR = $(CLIENT_DIR)/src
CLIENT_LOGIC = $(CLIENT_SRC_DIR)/backend
CLIENT_GUI = $(CLIENT_SRC_DIR)/gui
CLIENT_HEADERS_DIR = $(CLIENT_DIR)/headers

# Server directories
SERVER_DIR = server
SERVER_SRC_DIR = $(SERVER_DIR)/src
SERVER_HEADERS_DIR = $(SERVER_DIR)/headers

# Source files
CLIENT_SRCS := $(wildcard $(CLIENT_LOGIC)/*.cpp $(CLIENT_GUI)/*.cpp) $(CLIENT_DIR)/runner_client.cpp
SERVER_SRCS := $(wildcard $(SERVER_SRC_DIR)/*.cpp) $(SERVER_DIR)/runner_server.cpp

# Link SFML libraries
LIBS = -lsfml-graphics -lsfml-window -lsfml-system

# Compiler settings
CXX = clang++
CLIENT_CXXFLAGS = -std=c++17 -Wall -g -I$(CLIENT_HEADERS_DIR) -I$(shell brew --prefix sfml@2)/include
SERVER_CXXFLAGS = -std=c++17 -Wall -g -I$(SERVER_HEADERS_DIR) # Compiler server flags

# Add SFML library path for linking
LDFLAGS = -L$(shell brew --prefix sfml@2)/lib

# Create a environment variable
PWD := $(shell pwd)

# Default rule to build everything
all: $(CURRENT_PATH) $(BUILD_DIR) $(LOG_DIR) $(CLIENT_TARGET) $(SERVER_TARGET)

# Rule to set env variable
$(CURRENT_PATH): export $(PWD)

# Rule to create the client binary
$(CLIENT_TARGET): $(CLIENT_SRCS)
	$(CXX) $(CLIENT_CXXFLAGS) $(LDFLAGS) $(LIBS) $^ -o $@ # Link all client source files into runner_client.out

# Rule to create the server binary
$(SERVER_TARGET): $(SERVER_SRCS)
	$(CXX) $(SERVER_CXXFLAGS) $^ -o $@ # Link all server source files into runner_server.out

# Rule to create the output directories if they don't exist
$(BUILD_DIR):
	mkdir $(BUILD_DIR) # Create the build directory

$(LOG_DIR):
	mkdir $(LOG_DIR) # Create the logs directory

# Rule to clean up the build artifacts
clean: # Remove build and log directories
	rm -rf $(BUILD_DIR)
	rm -rf $(LOG_DIR)

# Declare phony targets (these are not actual files)
.PHONY: all clean
