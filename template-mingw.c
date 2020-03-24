#define TEMPLATE_IS_MINGW

#include "gen-make.h"

static const char *template_mingw[] = {
  TEMPLATE_TOP,
  "MINGW_ROOT   = $(realpath $(MINGW32))",
  "",
  "CC      = gcc",
  "CFLAGS  = " TEMPLATE_GCC_CFLAGS " " TEMPLATE_COMMON_CFLAGS,
  "LDFLAGS = " TEMPLATE_GCC_LDFLAGS,
  "OBJ_DIR = MinGW_obj",
  "",
  "ifeq ($(USE_CRT_DEBUG),1)",
  "  CFLAGS  += -O0 -ggdb",
  "else",
  "  CFLAGS  += -O2 -g",
  "  LDFLAGS += -s",
  "endif",
  "",
  "ifeq ($(USE_OPENSSL),1)",
  "  CFLAGS  += " TEMPLATE_OPENSSL_CFLAGS,
  "  EX_LIBS += " TEMPLATE_OPENSSL_GCC_EX_LIBS,
  "endif",
  "",
  "EX_LIBS += " TEMPLATE_GCC_EX_LIBS,
  "",
  "SOURCES = %s",
  "",
  "OBJECTS = $(addprefix $(OBJ_DIR)/, \\",
  "            $(notdir $(SOURCES:.c=.o)))",
  "",
  "GENERATED = config.h",
  "",
  "all: $(GENERATED) $(OBJ_DIR) $(PROGRAM)",
  "\t$(call green_msg, Welcome to $(PROGRAM) (MinGW).)",
  "",
  "$(OBJ_DIR):",
  "\t- mkdir $(OBJ_DIR)",
  "",
  "$(PROGRAM): $(OBJECTS) #! maybe add a '$(OBJ_DIR)/foo.res' here?",
  "\t$(call green_msg, Linking $@)",
  "\t$(CC) $(LDFLAGS) -o $@ $^ $(EX_LIBS) > $(@:.exe=.map)",
  "\t@echo",
  "",
  "%c",
  "$(OBJ_DIR)/%.res: %.rc",
  "\twindres --target=%T -D__MINGW32__ -O COFF -o $@ $<",
  "\t@echo",
  "",
  "%l",
  "clean:",
  "\trm -f $(OBJECTS)",
  "\t- rmdir $(OBJ_DIR)",
  "",
  "vclean realclean: clean",
  "\trm -f .depend.MinGW $(PROGRAM) $(PROGRAM:.exe=.map) $(GENERATED)",
  "",
  TEMPLATE_CONFIG,
  "",
  TEMPLATE_DEPEND
};

const char *mingw_makefile_name = THIS_FILE;

const char *mingw_c_rule =
           "$(OBJ_DIR)/%.o: %.c\n"
           "\t$(CC) $(CFLAGS) -o $@ -c $<\n"
           "\t@echo\n";

const char *mingw_cc_rule =
           "$(OBJ_DIR)/%.o: %.cc\n"
           "\t$(CC) -x c++ $(CFLAGS) -o $@ -c $<\n"
           "\t@echo\n";

const char *mingw_cpp_rule =
           "$(OBJ_DIR)/%.o: %.cpp\n"
           "\t$(CC) -x c++ $(CFLAGS) -o $@ -c $<\n"
           "\t@echo\n";

const char *mingw_cxx_rule =
           "$(OBJ_DIR)/%.o: %.cxx\n"
           "\t$(CC) -x c++ $(CFLAGS) -o $@ -c $<\n"
           "\t@echo\n";

const char *mingw_lib_rule =
           "#\n#! Use $(OBJECTS) or $(LIB_OBJ) for $(PROGRAM) or libfoo.a?\n#\n"
           "libfoo.a: $(LIB_OBJ)\n"
           "\trm -f $@\n"
           "\tar rs $@ $^\n"
           "\t@echo\n";

int generate_mingw_make (FILE *out)
{
  size_t i;

  for (i = 0; i < DIM(template_mingw); i++)
      write_template_line (out, template_mingw[i]);
  fputs ("\n", out);
  return (0);
}

