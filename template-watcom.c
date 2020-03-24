#define TEMPLATE_IS_WATCOM

#include "gen-make.h"

static const char *template_watcom[] = {
  TEMPLATE_TOP,
  "",
  "OBJ_DIR = Watcom_obj",
  "",
  "CC      = wcc386",
  "CFLAGS  = -3s -zm -zw -zq -fr=nul -wx -bd -bm -d3 -bt=nt -oilrtfm",
  "CFLAGS += -D_WIN32_WINNT=0x0501 -I. -I$(%WATCOM)\\h -I$(%WATCOM)\\h\\nt #! Add include dirs as needed",
  "LDFLAGS = system nt",
  "",
  "!if \"$(USE_OPENSSL)\" == \"1\"",
  "CFLAGS  += " TEMPLATE_OPENSSL_CFLAGS,
  "EX_LIBS += " TEMPLATE_OPENSSL_EX_LIBS,
  "!endif",
  "",
  "EX_LIBS += " TEMPLATE_EX_LIBS,
  "",
  "SOURCES = %s",
  "",
  "OBJECTS = %o",
  "",
  "GENERATED = config.h",
  "",
  "all: .SYMBOLIC $(GENERATED) $(OBJ_DIR) $(PROGRAM)",
  "\t@echo Welcome to $(PROGRAM) (Watcom).",
  "",
  "$(PROGRAM): $(OBJECTS) #! maybe add a 'foo.res' here?",
  "\twlink $(LDFLAGS) option quiet, caseexact, map name $(PROGRAM) file { $(OBJECTS) } library $(EX_LIBS)",
  "",
  "$(OBJ_DIR):",
  "\t- md $(OBJ_DIR)",
  "",
  "%c",
  "%l",
  ".ERASE",
  "$(OBJ_DIR)/foo.res: foo.rc $(THIS_FILE)",
  "\twrc -q -r -zm -D__WATCOMC__ -fo=$@ $[@",
  "\t@echo",
  "",
  "clean: .SYMBOLIC",
  "\trm -f $(OBJECTS) $(PROGRAM:.exe=.map)",
  "\t- rd $(OBJ_DIR)",
  "",
  "vclean realclean: .SYMBOLIC clean",
  "\trm -f $(PROGRAM) $(GENERATED)",
  "",
  TEMPLATE_CONFIG,
};

const char *watcom_makefile_name = "Makefile.Watcom";

const char *watcom_c_rule =
           ".ERASE\n"
           ".c.obj:\n"
           "\t*$(CC) $(CFLAGS) $[@ -fo=$@\n"
           "\t@echo\n";

const char *watcom_cc_rule =
           ".ERASE\n"
           ".cc.obj:\n"
           "\t*wpp386 $(CFLAGS) $[@ -fo=$@\n"
           "\t@echo\n";

const char *watcom_cpp_rule =
           ".ERASE\n"
           ".cpp.obj:\n"
           "\t*wpp386 $(CFLAGS) $[@ -fo=$@\n"
           "\t@echo\n";

const char *watcom_cxx_rule =
           ".ERASE\n"
           ".cxx.obj:\n"
           "\t*wpp386 $(CFLAGS) $[@ -fo=$@\n"
           "\t@echo\n";

const char *watcom_lib_rule =
            ".ERASE\n"
            "foo.lib: $(LIB_OBJ) $(THIS_FILE)\n"
            "\twlib -q -b -c $^@ $(LIB_OBJ) +-\n"
            "\t@echo\n";

int generate_watcom_wmake (FILE *out)
{
  size_t i;

  for (i = 0; i < DIM(template_watcom); i++)
      write_template_line(out, template_watcom[i]);
  fputs ("\n", out);
  return (0);
}

