# Linux requires wxWidgets 3.2.X, which Ubuntu amd64 doesn't have. See:
# https://docs.codelite.org/wxWidgets/repo320/#ubuntu-and-debian

CXX = g++
#CXXFLAGS = -std=c++17 -c -g -Wall -pedantic
CXXFLAGS = -std=c++17 -c -O3 -DNDEBUG -Wall -pedantic

DEPENDS_DIR = ../depends
RANDOMX_DIR = $(DEPENDS_DIR)/RandomX-1.2.1
GTEST_DIR = $(DEPENDS_DIR)/googletest-1.14.0

INC = -I$(RANDOMX_DIR)/src
LIBS = $(RANDOMX_DIR)/build/librandomx.a -lcrypto

GTESTINC = \
	-I$(GTEST_DIR)/googletest \
	-I$(GTEST_DIR)/googletest/include \
	-I$(GTEST_DIR)/googletest/src

WX_CONFIG := wx-config
WX_CXXFLAGS := $(shell $(WX_CONFIG) --cxxflags)
WX_LIBS := $(shell $(WX_CONFIG) --libs)

POWER_CORE_SOURCES = core.cpp
POWER_CORE_OBJECTS = $(POWER_CORE_SOURCES:.cpp=.o)
POWER_CORE_LIB = libpowercore.a

POWER_MAIN_SOURCES = wxwidgets_main.cpp
POWER_MAIN_OBJECTS = $(POWER_MAIN_SOURCES:.cpp=.o)
POWER_MAIN_EXEC = wxpower

POWER_TEST_SOURCES = test.cpp
POWER_TEST_OBJECTS = $(POWER_TEST_SOURCES:.cpp=.o)
POWER_TEST_EXEC = wxpowertest

all: $(POWER_CORE_LIB) $(POWER_MAIN_EXEC) $(POWER_TEST_EXEC)

$(POWER_CORE_LIB): $(POWER_CORE_OBJECTS)
	@echo "** Packaging '$@'"
	ar rcs $@ $(POWER_CORE_OBJECTS)

$(POWER_MAIN_EXEC): $(POWER_CORE_LIB) $(POWER_MAIN_OBJECTS)
	@echo "** Linking '$@'"
	$(CXX) -o $@ $(POWER_MAIN_OBJECTS) \
		$(POWER_CORE_LIB) $(LIBS) $(WX_LIBS)

$(POWER_TEST_EXEC): $(POWER_CORE_LIB) $(POWER_TEST_OBJECTS)
	@echo "** Linking '$@'"
	$(CXX) -o $@ $(POWER_TEST_OBJECTS) $(POWER_CORE_LIB) $(LIBS)

.cpp.o:
	@echo "** Compiling '$<'"
	$(CXX) $(CXXFLAGS) $(WX_CXXFLAGS) $(INC) $(GTESTINC) -o $@ $<

clean:
	rm -f $(POWER_TEST_EXEC) $(POWER_MAIN_EXEC) $(POWER_CORE_LIB) *.o *~

.PHONY: clean
