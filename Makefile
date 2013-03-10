
### Options: ###

# C++ compiler 
CXX = g++

# flags for C++
CXXFLAGS ?= -Wall -Wextra -pedantic -Weffc++ -Wformat-security -Winit-self -Wswitch-default -Wswitch-enum -Wfloat-equal -Wundef -Wshadow -Wcast-qual -Wconversion -Wlogical-op -Winline -Wsuggest-attribute=pure -Wsuggest-attribute=const -Wsuggest-attribute=noreturn

# [Debug,Release]
BUILD ?= Debug

### Conditionally set variables: ###

ifeq ($(BUILD),Debug)
BUILD_FLAGS = -g3 -rdynamic -fstack-protector-all -fno-inline -O0 -D_DEBUG
endif
ifeq ($(BUILD),Release)
BUILD_FLAGS = -O2 -DNDEBUG
endif
ifeq ($(BUILD),Debug)
LINK_FLAGS = -g3 -rdynamic
endif
ifeq ($(BUILD),Release)
LINK_FLAGS =
endif

### Variables: ###

CPPDEPS = -MT $@ -MF`echo $@ | sed -e 's,\.o$$,.d,'` -MD -MP

SQLITE_CXXFLAGS = $(BUILD_FLAGS) $(CXXFLAGS)
SQLITE_EXAMPLE1_OBJECTS =  \
	$(BUILD)/main.o \
	$(BUILD)/Column.o \
	$(BUILD)/Database.o \
	$(BUILD)/Statement.o \
	$(BUILD)/Transaction.o \
	
### Targets: ###

all: $(BUILD) $(BUILD)/example1

clean: 
	rm -f $(BUILD)/*.o
	rm -f $(BUILD)/*.d
	rm -f $(BUILD)/example1

$(BUILD): $(BUILD)/
	mkdir -p $(BUILD)


$(BUILD)/example1: $(SQLITE_EXAMPLE1_OBJECTS)
	$(CXX) -o $@ $(SQLITE_EXAMPLE1_OBJECTS) $(LINK_FLAGS) -lsqlite3 -lpthread 


$(BUILD)/main.o: examples/example1/main.cpp
	$(CXX) -c -o $@ $(SQLITE_CXXFLAGS) $(CPPDEPS) $<

$(BUILD)/Column.o: src/Column.cpp
	$(CXX) -c -o $@ $(SQLITE_CXXFLAGS) $(CPPDEPS) $<

$(BUILD)/Database.o: src/Database.cpp
	$(CXX) -c -o $@ $(SQLITE_CXXFLAGS) $(CPPDEPS) $<

$(BUILD)/Statement.o: src/Statement.cpp
	$(CXX) -c -o $@ $(SQLITE_CXXFLAGS) $(CPPDEPS) $<

$(BUILD)/Transaction.o: src/Transaction.cpp
	$(CXX) -c -o $@ $(SQLITE_CXXFLAGS) $(CPPDEPS) $<


.PHONY: all clean


# Dependencies tracking:
-include $(BUILD)/*.d


