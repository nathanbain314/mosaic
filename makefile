CXX += -std=c++11 -O3 -g

SRC = lib
INCLUDE = include
OBJDIR = build
OUTDIR = bin
EXECUTABLE = $(OUTDIR)/RunMosaic $(OUTDIR)/RunRotationalMosaic $(OUTDIR)/RunContinuous

CFLAGS += -I$(INCLUDE) -Wno-everything
LDLIBS = -lncurses
VIPS_FLAGS = `pkg-config vips-cpp --cflags --libs`

all: $(EXECUTABLE)

$(OUTDIR)/%: $(OBJDIR)/%.o $(OBJDIR)/mosaic.o $(OBJDIR)/progressbar.o
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(LDLIBS) $(VIPS_FLAGS) $^ -o $@

.PRECIOUS: $(OBJDIR)/%.o

$(OBJDIR)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJDIR)/%.o: $(SRC)/%.c++
	@mkdir -p $(dir $@)
	$(CXX) -c $(CFLAGS) $(VIPS_FLAGS) $< -o $@

clean:
	rm -rf $(OBJDIR) $(OUTDIR)
