NAME = dice
CXX = g++
CXX_FLAGS = -Wall -Wextra -std=c++17 
CXX_DBG_FLAGS = -ggdb
LD = g++
SRC_DIR = src
OBJ_DIR = obj
INCLUDE = -I src
LIBS =
SRC =	main.cpp \
	Randomizer.cpp
_OBJ := $(SRC:%.cpp=%.o)
OBJ = $(patsubst %, $(OBJ_DIR)/%, $(_OBJ))
MAKEFILE = makefile

DEPDIR := obj/deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

.PHONY: all, dbg_build
all: dbg_build

# debug build ------------------------------------------------------------------
dbg_build: $(MAKEFILE) $(OBJ_DIR) $(DEPDIR) $(NAME)

$(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(DEPDIR)/%.d | $(DEPDIR)
	@echo "CXX $@"
	@$(CXX) $(CXX_FLAGS) $(CXX_DBG_FLAGS) $(DEPFLAGS) -o $@ $< $(INCLUDE) -c

$(NAME): $(OBJ) $(MAKEFILE)
	@echo "LD $@"
	@$(LD) -o $@ $(OBJ) $(LIBS)

# release build ----------------------------------------------------------------
.PHONY: release
release:
	@echo "release is not configured yet"

# cleaning
.PHONY: clean
clean:
	@rm -rfv $(OBJ_DIR)
	@rm -fv $(NAME)

# dependency files
$(DEPDIR):
	mkdir -p $@
	
DEPFILES := $(_OBJ:%.o=$(DEPDIR)/%.d)
$(DEPFILES):

include $(wildcard $(DEPFILES))
