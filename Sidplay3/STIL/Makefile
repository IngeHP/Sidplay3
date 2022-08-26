# Makefile template for STILView.

BASEDIR         = .
SRCDIR          = $(BASEDIR)

CXX             = c++
CXXFLAGS        = -O -Wall
DEFS            =

LDFLAGS         =
INCLUDES        = -I$(SRCDIR)

TARGET = stilview

#--------------------------------------------------------------

SOURCES =           \
	stilcomm.cpp    \
	stilview.cpp    \
	stil.cpp

OBJECTS = $(SOURCES:.cpp=.o)

.EXPORT_ALL_VARIABLES:

.SUFFIXES:
.SUFFIXES: .cpp .h .o

%.o:    %.cpp
		$(CXX) $(CXXFLAGS) $(INCLUDES) $(DEFS) -c $< -o $@

$(TARGET): $(OBJECTS)
		$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

.PHONY: all
all:    $(OBJECTS) $(TARGET)

.PHONY: clean
clean:
		$(RM) *.o $(TARGET) core *~ *% #*
