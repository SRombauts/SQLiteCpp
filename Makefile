
### Options: ###

# C++ compiler 
CXX = g++

# flags for C++ 
CXXFLAGS ?= -Wall

# [Debug,Release]
BUILD ?= Debug

### Conditionally set variables: ###

ifeq ($(BUILD),Debug)
BUILD_FLAGS = -g -rdynamic -fno-inline -O0 -DDEBUG -D_DEBUG
endif
ifeq ($(BUILD),Release)
BUILD_FLAGS = -O2
endif
ifeq ($(BUILD),Debug)
LINK_FLAGS = -g -rdynamic
endif
ifeq ($(BUILD),Release)
LINK_FLAGS =
endif

### Variables: ###

CPPDEPS = -MT $@ -MF`echo $@ | sed -e 's,\.o$$,.d,'` -MD -MP

SQLITE_CXXFLAGS = $(BUILD_FLAGS) $(CXXFLAGS)
SQLITE_EXAMPLE1_OBJECTS =  \
	$(BUILD)/main.o
	
### Targets: ###

all: $(BUILD) $(BUILD)/example1

clean: 
	rm -f $(BUILD)/*.o
	rm -f $(BUILD)/*.d
	rm -f $(BUILD)/example1

$(BUILD): $(BUILD)/
	mkdir -p $(BUILD)


$(BUILD)/example1: $(SQLITE_EXAMPLE1_OBJECTS)
	$(CXX) -o $@ $(SQLITE_EXAMPLE1_OBJECTS) $(LINK_FLAGS) -lsqlite3


$(BUILD)/main.o: src/main.cpp
	$(CXX) -c -o $@ $(SQLITE_CXXFLAGS) $(CPPDEPS) $<


.PHONY: all clean


# Dependencies tracking:
-include $(BUILD)/*.d

