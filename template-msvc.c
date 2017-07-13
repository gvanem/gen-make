#define TEMPLATE_IS_MSVC

#include "gen-make.h"

static const char *template_msvc[] = {
  TEMPLATE_TOP,
   "",
  "CC      = cl",
  "CFLAGS  = " TEMPLATE_CL_CFLAGS " -DHAVE_CONFIG_H",
  "LDFLAGS = " TEMPLATE_CL_LDFLAGS,
  "",
  "!if \"$(USE_OPENSSL)\" == \"1\"",
  "CFLAGS  = $(CFLAGS) " TEMPLATE_OPENSSL_CFLAGS,
  "EX_LIBS = $(EX_LIBS) " TEMPLATE_OPENSSL_EX_LIBS,
  "!endif",
  "",
  "EX_LIBS = $(EX_LIBS) " TEMPLATE_EX_LIBS,
  "",
  "SOURCES = %s",
  "",
  "OBJECTS = $(SOURCES:.c=.obj)",
  "",
  "GENERATED = config.h",
  "",
  "all: $(GENERATED) $(PROGRAM)",
  "\t@echo 'Welcome to $(PROGRAM). (MSVC)'",
  "",
  "$(PROGRAM): $(OBJECTS)",
  "\tlink $(LDFLAGS) -out:$@ -map:$(@:.exe=.map) $** $(EX_LIBS) > link.tmp",
  "\ttype link.tmp >> $(@:.exe=.map)",
  "\t@echo",
  "",
  "%c",
  "%r",
  "%l",
  "clean:",
  "\tdel $(OBJECTS) $(PROGRAM:.exe=.map)",
  "",
  "vclean realclean: clean",
  "\tdel $(PROGRAM) $(GENERATED)",
  "",
  TEMPLATE_CONFIG
};

const char *msvc_makefile_name = "Makefile." TEMPLATE_NAME;

const char *msvc_c_rule =
           ".c.obj:\n"
           "\t$(CC) $(CFLAGS) -Fo$*.obj -c $<\n"
           "\t@echo\n";

const char *msvc_cc_rule =
           ".cc.obj:\n"
           "\t$(CC) -TP $(CFLAGS) -Fo$*.obj -c $<\n"
           "\t@echo\n";

const char *msvc_cpp_rule =
           ".cpp.obj:\n"
           "\t$(CC) -TP $(CFLAGS) -Fo$*.obj -c $<\n"
           "\t@echo\n";

const char *msvc_cxx_rule =
           ".cxx.obj:\n"
           "\t$(CC) -TP $(CFLAGS) -Fo$*.obj -c $<\n"
           "\t@echo\n";

const char *msvc_res_rule =
           ".rc.res:\n"
           "\trc -nologo -D_MSC_VER -Fo./$*.res $<\n"
           "\t@echo\n";

const char *msvc_lib_rule =
           "foo.lib: $(LIB_OBJ)\n"
           "\tlib -nologo -machine:x86 -out:$@ $**\n"
           "\t@echo\n";

int generate_msvc_nmake (FILE *out)
{
  size_t i;

  for (i = 0; i < DIM(template_msvc); i++)
      write_template_line(out, template_msvc[i]);
  fputs ("\n", out);
  return (0);
}

