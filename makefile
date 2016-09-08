CXX += -std=c++11 -stdlib=libc++ -O3 -g

EXECUTABLE = RunMosaic RunContinuous

SRC = lib
INCLUDE = include
OBJDIR = build

CFLAGS += -I$(INCLUDE) -Wimplicit-function-declaration -Wall -Wextra -pedantic
LDLIBS = -lncurses
VIPS_FLAGS = `pkg-config vips-cpp --cflags --libs`

all: $(EXECUTABLE)

RunMosaic: $(OBJDIR)/RunMosaic.o $(OBJDIR)/mosaic.o $(OBJDIR)/progressbar.o
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(LDLIBS) $(VIPS_FLAGS) $(OBJDIR)/RunMosaic.o $(OBJDIR)/mosaic.o $(OBJDIR)/progressbar.o -o RunMosaic

RunContinuous: $(OBJDIR)/RunContinuous.o $(OBJDIR)/mosaic.o $(OBJDIR)/progressbar.o
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(LDLIBS) $(VIPS_FLAGS) $(OBJDIR)/RunContinuous.o $(OBJDIR)/mosaic.o $(OBJDIR)/progressbar.o -o RunContinuous

$(OBJDIR)/%.o: $(SRC)/%.c $(INCLUDE)/%.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJDIR)/%.o: $(SRC)/%.c++ $(INCLUDE)/mosaic.h
	$(CXX) -c $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(VIPS_FLAGS) $< -o $@

clean:
	rm -f $(OBJDIR)/*.o $(EXECUTABLE)