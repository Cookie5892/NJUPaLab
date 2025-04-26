.DEFAULT_GOAL = app

# Add necessary options if the target is a shared library
ifeq ($(SHARE),1)
SO = -so
CFLAGS  += -fPIC -fvisibility=hidden
LDFLAGS += -shared -fPIC
endif

WORK_DIR  = $(shell pwd)
BUILD_DIR = $(WORK_DIR)/build

INC_PATH := $(WORK_DIR)/include $(INC_PATH)
OBJ_DIR  = $(BUILD_DIR)/obj-$(NAME)$(SO)
BINARY   = $(BUILD_DIR)/$(NAME)$(SO)

# Compilation flags
ifeq ($(CC),clang)
CXX := clang++
else
CXX := g++
endif
LD := $(CXX)
INCLUDES = $(addprefix -I, $(INC_PATH))
CFLAGS  := -O2 -MMD -Wall -Werror $(INCLUDES) $(CFLAGS)
LDFLAGS := -O2 $(LDFLAGS)

OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o) $(CXXSRC:%.cc=$(OBJ_DIR)/%.o)

$(OBJ_DIR)/%.i:%.c
	@mkdir -p $(dir $@)
	$(CC) -O2 $(INCLUDES) -E -o $@ $<

# Compilation patterns
$(OBJ_DIR)/%.o: %.c
	@echo + CC $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c -o $@ $<
	$(call call_fixdep, $(@:.o=.d), $@)

$(OBJ_DIR)/%.o: %.cc
	@echo + CXX $<
	@mkdir -p $(dir $@)
	@$(CXX) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<
	$(call call_fixdep, $(@:.o=.d), $@)

# Depencies
-include $(OBJS:.o=.d)

# Some convenient rules

.PHONY: app clean preprocess preprocess-file

preprocess: $(OBJS:.o=.i)




# $(dir ...)提取路径部分，确保只创建目录，而不是文件本身。
preprocess-file:
	@if [ -z "$(SRC)" ]; then \
        echo "Error: $(COLOR_RED)Please specify the source file using 'make preprocess-file SRC=<source_file>'$(COLOR_END)"; \
        exit 1; \
    fi
	@mkdir -p $(OBJ_DIR)
	$(CC) -O2 $(INCLUDES) -E -o $(OBJ_DIR)/$(notdir $(SRC:.c=.i)) $(SRC)
	@echo "Preprocessed file generated: $(OBJ_DIR)/$(notdir($(SRC:.c=.i)))"

app: $(BINARY)

$(BINARY):: $(OBJS) $(ARCHIVES)
	@echo + LD $@
	@$(LD) -o $@ $(OBJS) $(LDFLAGS) $(ARCHIVES) $(LIBS)

clean:
	-rm -rf $(BUILD_DIR)
