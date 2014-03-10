TOP_DIR = ..

SLICE_ICE := $(wildcard *.ice)
SLICE_HDR := $(SLICE_ICE:.ice=.h)
SLICE_SRC := $(SLICE_ICE:.ice=.cpp)
SLICE_OBJ := $(SLICE_SRC:.cpp=.o)

SRCS := $(filter-out $(SLICE_SRC),$(wildcard *.cpp))
OBJS := $(SRCS:.cpp=.o) 

all: $(SLICE_HDR) $(SLICE_OBJ) $(OBJS) $(SLICE_OBJ) 

include $(TOP_DIR)/Make.rules

$(SLICE_OBJ): $(SLICE_SRC) $(SLICE_HDR)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< 

# $(DEPS_HDRS): $(SLICE_HDR)
# 	mv -f $(SLICE_HDR) $(TOP_DIR)/include

$(SLICE_SRC) $(SLICE_HDR): $(SLICE_ICE)
	slice2cpp $(SLICE_ICE)
	mv -f $(SLICE_HDR) $(TOP_DIR)/include

clean::
	@for i in $(SUBDIRS); do \
		echo "enter subdir $$i to make $@..."; \
		(cd $$i && $(MAKE) $@); \
	done
	rm -f *.o $(SLICE_HDR) $(SLICE_SRC)
