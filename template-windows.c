#define TEMPLATE_IS_WINDOWS

#include "gen-make.h"

static const char *template_windows[] = {
  TEMPLATE_TOP,
  "",
  "MINGW_ROOT   = $(realpath $(MINGW32))",
  "VC_ROOT      = $(realpath $(VSINSTALLDIR))",
  "",
  "define Usage",
  "",
  "  Usage: $(MAKE) -f $(THIS_FILE) CC=[gcc | cl | clang-cl] [all | depend | clean | vclean | install]",
  "endef",
  "",
  "ifeq ($(CC),gcc)",
  "  INSTALL_BIN = $(MINGW_ROOT)/bin",
  "  INSTALL_LIB = $(MINGW_ROOT)/lib",
  "  CFLAGS      = " TEMPLATE_GCC_CFLAGS,
  "  LDFLAGS     = " TEMPLATE_GCC_LDFLAGS,
  "  OBJ_DIR     = MinGW_obj",
  "  O           = o",
  "",
  "  ifeq ($(USE_CRT_DEBUG),1)",
  "    CFLAGS  += -O0 -ggdb",
  "  else",
  "    CFLAGS  += -O2 -g",
  "    LDFLAGS += -s",
  "  endif",
  "",
  "  ifeq ($(USE_OPENSSL),1)",
  "    EX_LIBS += " TEMPLATE_OPENSSL_GCC_EX_LIBS,
  "  endif",
  "",
  "  EX_LIBS += " TEMPLATE_GCC_EX_LIBS,
  "",
  "else ifeq ($(CC),cl)",
  "  OBJ_DIR = MSVC_obj",
  "",
  "else ifeq ($(CC),clang-cl)",
  "  export CL=",
  "  OBJ_DIR = clang_obj",
  "",
  "else",
  "  $(error $(Usage))",
  "endif",
  "",
  "ifneq ($(CC),gcc)",
  "  INSTALL_BIN = $(VC_ROOT)/bin",
  "  INSTALL_LIB = $(VC_ROOT)/lib",
  "  CFLAGS      = " TEMPLATE_CL_CFLAGS,
  "  LDFLAGS     = " TEMPLATE_CL_LDFLAGS,
  "  O           = obj",
  "",
  "  ifeq ($(USE_CRT_DEBUG),1)",
  "    CFLAGS += -MDd -GF -GS -RTCs -RTCu -RTCc",
  "  else",
  "    CFLAGS += -MD",
  "  endif",
  "",
  "  ifeq ($(USE_OPENSSL),1)",
  "    EX_LIBS += " TEMPLATE_OPENSSL_EX_LIBS,
  "  endif",
  "",
  "  EX_LIBS += " TEMPLATE_EX_LIBS,
  "endif",
  "",
  "CFLAGS += " TEMPLATE_COMMON_CFLAGS,
  "",
  "ifeq ($(USE_OPENSSL),1)",
  "  CFLAGS += " TEMPLATE_OPENSSL_CFLAGS,
  "endif",
  "",
  "SOURCES = %s",
  "",
  "OBJECTS = $(addprefix $(OBJ_DIR)/, \\",
  "            $(notdir $(SOURCES:.c=.$(O))))",
  "",
  "GENERATED = config.h",
  "",
  "all: $(GENERATED) $(OBJ_DIR) $(PROGRAM)",
  "\t$(call green_msg, Welcome to $(PROGRAM) (CC=$(CC)).)",
  "",
  "$(OBJ_DIR):",
  "\t- mkdir $(OBJ_DIR)",
  "",
  "foo.exe: $(OBJECTS) #! maybe add a '$(OBJ_DIR)/foo.res' here?",
  "\t$(call link_EXE, $@, $^ $(EX_LIBS))",
  "",
  "foo_imp.lib:  foo.dll",
  "libfoo.dll.a: foo.dll",
  "",
  "#",
  "#! Unless the '$(OBJECTS)' exports something, this could create no 'foo_imp.lib' file.",
  "#",
  "ifeq ($(CC),gcc)",
  "foo.dll: $(OBJECTS)",
  "\t$(call link_DLL, $@, libfoo.dll.a, $^ $(EX_LIBS))",
  "else",
  "foo.dll: $(OBJECTS)",
  "\t$(call link_DLL, $@, foo_imp.lib, $^ $(EX_LIBS))",
  "endif",
  "",
  "%c",
  "%l",
  "$(OBJ_DIR)/%.res: %.rc",
  "\t$(call make_res, $<, $@)",
  "",
  "install: $(PROGRAM)",
  "\tcp --update $^ $(INSTALL_BIN)",
  "\t@echo",
  "",
  "clean:",
  "\trm -f $(OBJECTS) $(OBJ_DIR)/foo.res",
  "\t- rmdir $(OBJ_DIR)",
  "",
  "vclean realclean: clean",
  "\trm -f .depend.Windows $(PROGRAM) $(PROGRAM:.exe=.map) $(PROGRAM:.exe=.pdb) $(GENERATED)",
  "",
  TEMPLATE_GMAKE_MACROS,
  TEMPLATE_CONFIG,
  "%R",
  "",
  TEMPLATE_DEPEND
};

const char *windows_makefile_name = THIS_FILE;

const char *windows_c_rule =
           "$(OBJ_DIR)/%.o: %.c\n"
           "\t$(CC) $(CFLAGS) -o $@ -c $<\n"
           "\t@echo\n"
           "\n"
           "$(OBJ_DIR)/%.obj: %.c\n"
           "\t$(CC) $(CFLAGS) -Fo./$@ -c $<\n"
           "\t@echo\n";

const char *windows_cc_rule =
           "$(OBJ_DIR)/%.o: %.cc\n"
           "\t$(CC) -x c++ $(CFLAGS) -o $@ -c $<\n"
           "\t@echo\n"
           "\n"
           "$(OBJ_DIR)/%.obj: %.cc\n"
           "\t$(CC) -TP -EHsc $(CFLAGS) -Fo./$@ -c $<\n"
           "\t@echo\n";

const char *windows_cpp_rule =
           "$(OBJ_DIR)/%.o: %.cpp\n"
           "\t$(CC) -x c++ $(CFLAGS) -o $@ -c $<\n"
           "\t@echo\n"
           "\n"
           "$(OBJ_DIR)/%.obj: %.cpp\n"
           "\t$(CC) -TP -EHsc $(CFLAGS) -Fo./$@ -c $<\n"
           "\t@echo\n";

const char *windows_cxx_rule =
           "$(OBJ_DIR)/%.o: %.cxx\n"
           "\t$(CC) -x c++ $(CFLAGS) -o $@ -c $<\n"
           "\t@echo\n"
           "\n"
           "$(OBJ_DIR)/%.obj: %.cxx\n"
           "\t$(CC) -TP -EHsc $(CFLAGS) -Fo./$@ -c $<\n"
           "\t@echo\n";

const char *windows_lib_rule =
           "#\n"
           "# Link $(PROGRAM) with this instead?\n"
           "#\n"
           "libfoo.a foo.lib: $(LIB_OBJ)\n"
           "\t $(call make_lib, $@, $^)\n";

int generate_windows_make (FILE *out)
{
  size_t i;

  for (i = 0; i < DIM(template_windows); i++)
      write_template_line (out, template_windows[i]);
  fputs ("\n", out);
  return (0);
}

