/*
 * Quick and dirty makefile generator.
 *
 *  By Gisle Vanem <gvanem@yahoo.no> 2011.
 *
 * Needs CygWin/MinGW to compile, but produces makefiles for:
 *
 * option '-G mingw':   GNU make 4.x targeting MinGW
 * option '-G cygwin':  GNU make 4.x targeting Cygwin
 * option '-G windows': GNU make 4.x targeting MinGW + MSVC (32/64-bits).
 * option '-G msvc':    targeting MSVC       (32/64-bits).
 * option '-G watcom':  targeting OpenWatcom (32-bit).
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>
#include <conio.h>
#include <limits.h>
#include <sys/stat.h>

/* Assume if the generated Makefile was able to compile this, it also
 * generated a 'config.h' file here. And the "PROGRAM = foo.exe"
 */
#if !defined(IN_THE_REAL_MAKEFILE)
  #include "./config.h"
  char *program_name = "foo";

#elif defined(_MSC_VER)
  #include <msvc/getopt_long.h>
  char *program_name = "gen-make";

#else
  #include <getopt.h>
#endif

#define TEMPLATE_IS_NONE

#include "gen-make.h"
#include "smartlist.h"

#if defined(__CYGWIN__)
#define _MAX_PATH   _POSIX_PATH_MAX   /* 256 */
#endif

enum Generators {
     GEN_UNKNOWN = 0,
     GEN_MINGW   = 1,
     GEN_CYGWIN  = 2,
     GEN_MSVC    = 3,
     GEN_WATCOM  = 4,
     GEN_WINDOWS = 5
   };

static int         debug = 0;
static int         dry_run = 0;
static int         force_overwrite = 0;
static int         write_res_rule = 0;
static int         write_lib_rule = 0;
static const char *res_rule = NULL;
static const char *lib_rule = NULL;
static const char *c_rule = NULL;
static const char *cc_rule = NULL;
static const char *cpp_rule = NULL;
static const char *cxx_rule = NULL;
static char prog [_MAX_PATH] = { "gen-make.exe" };

static const char *line_end = "\\";
static const char *obj_suffix = "o";

static enum Generators generator = GEN_UNKNOWN;

static smartlist_t *c_files;
static smartlist_t *cc_files;
static smartlist_t *cpp_files;
static smartlist_t *cxx_files;
static smartlist_t *rc_files;
static smartlist_t *h_in_files;
static smartlist_t *vpaths;

static size_t num_c_files = 0;
static size_t num_cc_files = 0;
static size_t num_cpp_files = 0;
static size_t num_cxx_files = 0;
static size_t num_rc_files = 0;
static size_t num_h_in_files = 0;

static int main_found = 0;
static int WinMain_found = 0;
static int DllMain_found = 0;

static char *strreplace (int ch1, int ch2, char *str);
static int   find_sources (void);
static int   generate_rc_macro (FILE *out);

#define FILE_EXISTS(file)  (access(file,0) == 0)
#define FREE(p)            (p ? free(p) : (void)0)

#define DEBUG(level, fmt, ...)  do {                                      \
                                  if (debug >= level) {                   \
                                    fprintf (stderr, "%s(%u): " fmt,      \
                                      __FILE__, __LINE__, ##__VA_ARGS__); \
                                  }                                       \
                                } while (0)

void Abort (const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vfprintf (stderr, fmt, args);
  exit (-1);
}

#if defined(IN_THE_REAL_MAKEFILE)

static void usage (void)
{
  printf ("gen-make ver %d.%d.%d; A simple makefile generator.\n"
          "%s <options> [-G generator]:\n",
          VER_MAJOR, VER_MINOR, VER_MICRO, prog);
  printf ("  -d, --debug:   sets debug-level.\n"
          "  -n, --dry-run: test-mode; writes to stdout.\n"
          "  -f, --force:   ovewrite existing makefile without questions.\n"
          "  -r, --res:     write rule(s) for creating a res-file.\n"
          "  -l, --lib:     write rule(s) for creating a lib-file.\n"
          "  -G             Generators can be: 'cygwin', 'mingw', 'msvc', 'watcom' or\n"
          "                 'windows' for a GNU-makefile dual-targeting MinGW and MSVC.\n");
  exit (0);
}

static enum Generators select_generator (const char *gen)
{
  if (!stricmp(gen,"mingw"))
     return (GEN_MINGW);

  if (!stricmp(gen,"cygwin"))
     return (GEN_CYGWIN);

  if (!stricmp(gen,"msvc"))
     return (GEN_MSVC);

  if (!stricmp(gen,"watcom"))
     return (GEN_WATCOM);

  if (!stricmp(gen,"windows"))
     return (GEN_WINDOWS);

  return (GEN_UNKNOWN);
}

static void parse_args (int argc, char *const *argv)
{
  static const struct option long_opt[] = {
        { "help",    0, NULL, 'h' },   /* 0 */
        { "debug",   0, NULL, 'd' },
        { "dry-run", 0, NULL, 'n' },   /* 2 */
        { "force",   0, NULL, 'f' },
        { "res",     0, NULL, 'r' },   /* 4 */
        { "lib",     0, NULL, 'l' },
        { NULL,      0, NULL, 0 }
      };

  while (1)
  {
    int idx = 0;
    int c = getopt_long (argc, argv, "hdnfG:lr", long_opt, &idx);

    if (c == -1)
       break;

    switch (c)
    {
      case 0:
           if (idx == 0)
              usage();
           if (idx == 1)
              debug++;
           if (idx == 2)
              dry_run = 1;
           if (idx == 3)
              force_overwrite = 1;
           if (idx == 4)
              write_res_rule = 1;
           if (idx == 5)
              write_lib_rule = 1;
           break;
      case 'h':
           usage();
           break;
      case 'd':
           debug++;
           break;
      case 'n':
           dry_run = 1;
           break;
      case 'f':
           force_overwrite = 1;
           break;
      case 'r':
           write_res_rule = 1;
           break;
      case 'l':
           write_lib_rule = 1;
           break;
      case 'G':
           generator = select_generator (optarg);
           if (generator == GEN_UNKNOWN)
              usage();
           break;
      default:
           printf ("Illegal option: '%c'\n", c);
           usage();
           break;
    }
  }
}

static FILE *init_generator (const char *make_filename)
{
  FILE *fil;

  if (generator == GEN_WATCOM)
     line_end = "&";

  if (generator == GEN_MSVC || generator == GEN_WATCOM)
     obj_suffix = "obj";

  if (dry_run)
     return (stdout);

  if (!force_overwrite && FILE_EXISTS(make_filename))
  {
    printf ("%s already exist. Overwrite (y/N)? ", make_filename);
    fflush (stdout);
    if (toupper(getch()) != 'Y')
       Abort ("not overwriting.\n");
  }

  fil = fopen (make_filename, "w+");
  if (!fil)
     Abort ("Failed to create %s.\n", make_filename);
  return (fil);
}

static void cleanup (void)
{
  smartlist_free (c_files);
  smartlist_free (cc_files);
  smartlist_free (cpp_files);
  smartlist_free (cxx_files);
  smartlist_free (rc_files);
  smartlist_free (h_in_files);
  smartlist_free (vpaths);
}
#endif /*IN_THE_REAL_MAKEFILE */


int main (int argc, char **argv)
{
#if !defined(IN_THE_REAL_MAKEFILE)
  Abort ("It seems a \"gen-make.exe\" generated Makefile was able to compile and build this %s program.\n"
         "Congratulations! But I will not let you do any damage here.\n", argv[0]);
#else
  FILE *fil = stdout;

  GetModuleFileName (NULL, prog, sizeof(prog));
  parse_args (argc, argv);

  if (generator == GEN_UNKNOWN)
     usage();

  tzset();
  if (!find_sources())
  {
    puts ("I found no .c/.cpp/.cxx sources");
    return (1);
  }

  switch (generator)
  {
    case GEN_MINGW:
         fil = init_generator (mingw_makefile_name);
         c_rule   = mingw_c_rule;
         cc_rule  = mingw_cc_rule;
         cpp_rule = mingw_cpp_rule;
         cxx_rule = mingw_cxx_rule;
         res_rule = mingw_res_rule;
         lib_rule = mingw_lib_rule;
         generate_mingw_make (fil);
         break;

    case GEN_CYGWIN:
         fil = init_generator (cygwin_makefile_name);
         c_rule   = cygwin_c_rule;
         cc_rule  = cygwin_cc_rule;
         cpp_rule = cygwin_cpp_rule;
         cxx_rule = cygwin_cxx_rule;
         res_rule = cygwin_res_rule;
         lib_rule = cygwin_lib_rule;
         generate_cygwin_make (fil);
         break;

    case GEN_MSVC:
         fil = init_generator (msvc_makefile_name);
         c_rule   = msvc_c_rule;
         cc_rule  = msvc_cc_rule;
         cpp_rule = msvc_cpp_rule;
         cxx_rule = msvc_cxx_rule;
         res_rule = msvc_res_rule;
         lib_rule = msvc_lib_rule;
         generate_msvc_nmake (fil);
      // generate_msvc_deps (fil);
         break;

    case GEN_WATCOM:
         fil = init_generator (watcom_makefile_name);
         c_rule   = watcom_c_rule;
         cc_rule  = watcom_cc_rule;
         cpp_rule = watcom_cpp_rule;
         cxx_rule = watcom_cxx_rule;
         res_rule = watcom_res_rule;
         lib_rule = watcom_lib_rule;
         generate_watcom_wmake (fil);
      // generate_watcom_deps (fil);
         break;

    case GEN_WINDOWS:
         fil = init_generator (windows_makefile_name);
         c_rule   = windows_c_rule;
         cc_rule  = windows_cc_rule;
         cpp_rule = windows_cpp_rule;
         cxx_rule = windows_cxx_rule;
         res_rule = windows_res_rule;
         lib_rule = windows_lib_rule;
         generate_windows_make (fil);
         break;

    default:
       break;
  }

  if (fil != stdout)
     fclose (fil);
  cleanup();
#endif       /* IN_THE_REAL_MAKEFILE */
  return (0);
}

static void add_file (int is_c, int is_cc, int is_cpp, int is_cxx, int is_rc, int is_h_in, char *file)
{
  smartlist_t *array = is_c    ? c_files    :
                       is_cc   ? cc_files   :
                       is_cpp  ? cpp_files  :
                       is_cxx  ? cxx_files  :
                       is_rc   ? rc_files   :
                       is_h_in ? h_in_files : NULL;

  size_t *num = is_c    ? &num_c_files    :
                is_cc   ? &num_cc_files   :
                is_cpp  ? &num_cpp_files  :
                is_cxx  ? &num_cxx_files  :
                is_rc   ? &num_rc_files   :
                is_h_in ? &num_h_in_files : NULL;

  smartlist_add (array, file);
  *num = smartlist_len (array);
}

#if 0
#define ADD_C_FILE(f)     do {                                                 \
                            c_files [num_c_files++] = f;                       \
                            if (num_c_files == DIM(c_files)-1)                 \
                               Abort ("Too many .c-files (max %u files)\n",    \
                                      DIM(c_files));                           \
                          } while (0)

#define ADD_CC_FILE(f)    do {                                                 \
                            cc_files [num_cc_files++] = f;                     \
                            if (num_cc_files == DIM(cc_files)-1)               \
                               Abort ("Too many .cc-files (max %u files)\n",   \
                                      DIM(cc_files));                          \
                          } while (0)

#define ADD_CPP_FILE(f)   do {                                                 \
                            cpp_files [num_cpp_files++] = f;                   \
                            if (num_cpp_files == DIM(cpp_files)-1)             \
                               Abort ("Too many .cpp-files (max %u files)\n",  \
                                      DIM(cpp_files));                         \
                          } while (0)

#define ADD_CXX_FILE(f)   do {                                                 \
                            cxx_files [num_cxx_files++] = f;                   \
                            if (num_cxx_files == DIM(cxx_files)-1)             \
                               Abort ("Too many .cxx-files (max %u files)\n",  \
                                      DIM(cxx_files));                         \
                          } while (0)

#define ADD_RC_FILE(f)    do {                                                 \
                            rc_files [num_rc_files++] = f;                     \
                            if (num_rc_files == DIM(rc_files)-1)               \
                               Abort ("Too many .rc-files (max %u files)\n",   \
                                      DIM(rc_files));                          \
                          } while (0)

#define ADD_H_IN_FILE(f)  do {                                                 \
                            h_in_files [num_h_in_files++] = f;                 \
                            if (num_h_in_files == DIM(h_in_files)-1)           \
                               Abort ("Too many .h.in-files (max %u files)\n", \
                                      DIM(h_in_files));                        \
                          } while (0)
#endif

static int file_walker (const char *path, const WIN32_FIND_DATA *ff)
{
  const char *start, *end;
  char       *p, *dot, *slash, dir [MAX_PATH];
  int         is_c = 0, is_cc = 0, is_cpp = 0, is_cxx = 0, is_rc = 0, is_h_in = 0;
  int         considered;
  int         i, len;

  len = strlen (path);
  end = strrchr (path, '\0');
  dot = strrchr (path, '.');

  if (len <= 2 || (size_t)(end - dot) > sizeof(".cpp"))
    considered = 0;

  else if (!strcmp(dot,".c"))
    is_c = 1;

  else if (!strcmp(dot,".cc"))
    is_cc = 1;

  else if (!strcmp(dot,".cpp"))
    is_cpp = 1;

  else if (!strcmp(dot,".cxx"))
    is_cxx = 1;

  else if (!strcmp(dot-2,".h.in"))
    is_h_in = 1;

  else if (!strcmp(dot,".rc"))
    is_rc = 1;

#if 0     /* to-do: make dependency list */
  else if (!strcmp(dot,".h"))
    is_h = 1;
#endif

  considered = is_c + is_cc + is_cpp + is_cxx + is_rc;

  DEBUG (2, "%-40s %sconsidered. is_c: %d, is_cc: %d, is_cpp: %d, is_cxx: %d, is_rc: %d\n",
         path, considered ? "" : "not ", is_c, is_cc, is_cpp, is_cxx, is_rc);

#if 0
  /*
   * to-do: check the source-file for a 'main(', 'WinMain(' or a 'DllMain('.
   */
  if ((is_c || is_cc || is_cpp || is_cxx) && !main_found && !WinMain_found && !DllMain_found)
  {
    grep_file (path, &main_found, &WinMain_found, &DllMain_found);
  }
#endif

  if (!considered)
     return (0);

  start = path;
  if (!strncmp(start,".\\",2))
     start += 2;

  p = strreplace ('\\','/', strdup(start));
  add_file (is_c, is_cc, is_cpp, is_cxx, is_rc, is_h_in, p);

  /* Check if this file has a unique directory part that needs to be added to vpaths[].
   */
  slash = strchr (p,'/');
  if (!slash)
     return (0);

  memcpy (dir, p, slash-p);
  dir[slash-p] = '\0';

  for (i = 0; i < smartlist_len(vpaths); i++)
      if (!strcmp(dir,smartlist_get(vpaths,i)))
         return (0);    /* Already has this VPATH */

  smartlist_add (vpaths, strdup(dir));
  (void) ff;
  return (0);
}

static int print_sources (const char *which, const smartlist_t *sl)
{
  int i, max = smartlist_len (sl);

  for (i = 0; i < max; i++)
      DEBUG (2, "%s[%2d]: '%s'\n", which, i, (const char*)smartlist_get(sl,i));
  return (max);
}

static int find_sources (void)
{
  int num;

  c_files    = smartlist_new();
  cc_files   = smartlist_new();
  cpp_files  = smartlist_new();
  cxx_files  = smartlist_new();
  rc_files   = smartlist_new();
  h_in_files = smartlist_new();
  vpaths     = smartlist_new();

  main_found = WinMain_found = DllMain_found = 0;

  file_tree_walk (".", file_walker);

  num  = print_sources ("c_files", c_files);
  num += print_sources ("cc_files", cc_files);
  num += print_sources ("cpp_files", cpp_files);
  num += print_sources ("cxx_files", cxx_files);

  print_sources ("rc_files",   rc_files);
  print_sources ("h_in_files", h_in_files);
  return (num);
}

/*
 * Replace 'ch1' to 'ch2' in string 'str'.
 */
static char *strreplace (int ch1, int ch2, char *str)
{
  char *s;

  assert (str != NULL);
  s = str;
  while (*s)
  {
    if (*s == ch1)
        *s = ch2;
    s++;
  }
  return (str);
}

static char *basename (const char *fname)
{
  const char *base = fname;

  if (fname && *fname)
  {
    if (fname[1] == ':')
    {
      fname += 2;
      base = fname;
    }

    while (*fname)
    {
      if (*fname == '\\' || *fname == '/')
        base = fname + 1;
      fname++;
    }
  }
  return (char*) base;
}

static const char *get_bitness (void)
{
  const char *env = getenv ("CPU");

  if (!env || !stricmp(env,"x86"))
     return ("32");
  if (!stricmp(env,"x64"))
     return ("64");
  return ("??");
}

static void write_files (FILE *out, smartlist_t *sl, int indent)
{
  const char *file;
  int    i, max = smartlist_len (sl);
  size_t len;
  static size_t longest = 0;

  if (max == 0)
     return;

  for (i = 0; i < max; i++)
  {
    file = smartlist_get (sl, i);
    len = strlen (file);
    if (len > longest)
       longest = len;
  }

  for (i = 0; i < max; i++)
  {
    file = smartlist_get (sl, i);
    len = strlen (file);
    if (i == 0)
         fputs (file, out);
    else fprintf (out, "%-*s%s", indent, "", file);

    if (i < max-1)
         fprintf (out, " %*s%s\n", longest-len, "", line_end);
    else fputc ('\n', out);
  }
}

static void write_one_object (FILE *out, smartlist_t *sl, int idx, int *len)
{
  char *file = basename (smartlist_get(sl,idx));
  char *dot  = strrchr (file, '.');

  *len += fprintf (out, " $(OBJ_DIR)/%.*s.%s", dot-file, file, obj_suffix);
  if (*len > 60 && idx < smartlist_len(sl)-1)
  {
    fprintf (out, " %s\n         ", line_end);
    *len = 0;
  }
}

static void write_objects (FILE *out, smartlist_t *sl, int num, int add_line_end)
{
  int i, len = 0;

  if (add_line_end)
     fprintf (out, " %s\n         ", line_end);

  for (i = 0; i < num; i++)
      write_one_object (out, sl, i, &len);
}

/*
 * Write out a line from the template. Handle these formats:
 *  '%c' -> write the .c/.cc/.cxx/.cpp -> object rule(s).
 *  '%D' -> write the date/time stamp.
 *  '%o' -> write list of object files (prefixed with $(OBJ_DIR)).
 *  '%r' -> write the .rc -> .res rule(s).
 *  '%R' -> write a GNU-make macro to for a foo.rc file.
 *  '%l' -> write the library rule.
 *  '%s' -> write list of .c-files for the SOURCES line.
 *  '%v' -> write a VPATH statement if needed.
 */
int write_template_line (FILE *out, const char *templ)
{
  const char *p;
  int         i;

  p = strchr (templ,'%');
  if (p && p[1] == 's')
  {
    int indent = p - templ;

    fprintf (out, "%.*s", indent, templ);

    /* Write the list of .c/.cc/.cpp-files at this point.
     */
    write_files (out, c_files, indent);
    fprintf (out, "%*s# %d .c SOURCES files found (recursively)\n", indent, "", num_c_files);

    if (num_cc_files > 0)
    {
      fprintf (out, "#\n#! Add these $(CC_SOURCES) to $(OBJECTS) as needed.\n#\nCC_SOURCES = ");
      write_files (out, cc_files, indent+3);
    }

    if (num_cpp_files > 0)
    {
      fprintf (out, "\n#\n#! Add these $(CPP_SOURCES) to $(OBJECTS) as needed.\n#\nCPP_SOURCES = ");
      write_files (out, cpp_files, indent+4);
    }

    if (num_cxx_files > 0)
    {
      fprintf (out, "\n#\n#! Add these $(CXX_SOURCES) to $(OBJECTS) as needed.\n#\nCXX_SOURCES = ");
      write_files (out, cxx_files, indent+4);
    }

    if (num_h_in_files > 0)
       fprintf (out, "%*s# Found %d .h.in-file(s); add rules for them.\n", indent, "", num_h_in_files);

    if (num_rc_files > 0 && write_res_rule)
       fprintf (out, "%*s# Found %d .rc-file(s); add a 'RC_FILES' list.\n", indent, "", num_rc_files);
    return (1);
  }

  p = strchr (templ,'%');
  if (p && p[1] == 'D')
  {
    time_t t = time (NULL);
    struct tm *tm = localtime (&t);

    p += 2;
    return fprintf (out, "%.*s<%.24s>%s\n", p-templ-2, templ, asctime(tm), p);
  }

  p = strchr (templ,'%');
  if (p && p[1] == 'b')
  {
    fprintf (out, "%.*s%s", p-templ, templ, get_bitness());

    templ = p + 2;  /* skip past '%b' and fall-through */
  }

  p = strchr (templ,'%');
  if (p && p[1] == 'l')
  {
    if (write_lib_rule)
    {
      assert (lib_rule);
      return fprintf (out, "%s\n", lib_rule);
    }
    templ = p + 2;  /* skip past '%l' and fall-through */
  }

  p = strchr (templ,'%');
  if (p && p[1] == 'r')
  {
    if (write_res_rule)
    {
      assert (res_rule);
      return fprintf (out, "%s\n", res_rule);
    }
    templ = p + 2;  /* skip past '%r' and fall-through */
  }

  if (p && p[1] == 'R')
  {
    if (write_res_rule)
       return generate_rc_macro (out);
    templ = p + 2;  /* skip past '%R' and fall-through */
  }

  p = strchr (templ,'%');
  if (p && p[1] == 'c')
  {
    assert (c_rule);
    assert (cc_rule);
    assert (cpp_rule);
    assert (cxx_rule);

    if (num_c_files > 0)
       fprintf (out, "%s\n", c_rule);
    if (num_cc_files > 0)
       fprintf (out, "%s\n", cc_rule);
    if (num_cpp_files > 0)
       fprintf (out, "%s\n", cpp_rule);
    if (num_cxx_files > 0)
       fprintf (out, "%s\n", cxx_rule);

    templ = p + 2;  /* skip past '%c' and fall-through */
  }

  p = strchr (templ,'%');
  if (p && p[1] == 'v')
  {
    int max = smartlist_len (vpaths);

    if (max > 0)
    {
      fprintf (out, "VPATH = ");
      for (i = 0; i < max; i++)
          fprintf (out, "%s ", (const char*)smartlist_get(vpaths,i));
      fprintf (out, "#! %d VPATHs\n", max);
      return (1);
    }
    templ = p + 2;  /* skip past '%v' and fall-through */
  }

  p = strchr (templ,'%');
  if (p && p[1] == 'o')     /* Write the list of objects. Only used in template-watcom.c */
  {
    fprintf (out, "%.*s", p-templ-1, templ);

    write_objects (out, c_files, num_c_files, 0);
    write_objects (out, cc_files, num_cc_files, num_c_files > 0);
    write_objects (out, cpp_files, num_cpp_files, num_c_files > 0 || num_cc_files > 0);
    fprintf (out, "\n        # %d object files\n", num_c_files + num_cc_files + num_cpp_files);
    templ = p + 2;  /* skip past '%o' */
  }

  if (!p && !strncmp(templ,"all: ",5))
  {
    if (!main_found || !WinMain_found)
       fprintf (out, "#\n#! Failed to find a 'main()' or a 'WinMain()' in the SOURCES. Is it a .DLL?\n#\n");
    else if (DllMain_found)
       fprintf (out, "#\n#! Found a 'DllMain()' in the SOURCES. Rewrite the '$(PROGRAM)' rule into a 'link_DLL' rule.\n#\n");
  }
  fprintf (out, "%s\n", templ);
  return (1);
}

static const char *rc_macro[] = {
    "$(OBJ_DIR)/foo.rc: $(THIS_FILE)",
    "\t$(info Generating $@...)",
    "\t$(file > $@, /* $(WARNING) */)",
    "\t$(file >>$@,$(FOO_RC))",
    "",
    "define FOO_RC",
    "  #include <winver.h>",
    "",
    "  #if defined(__MINGW64_VERSION_MAJOR)",
    "    #define RC_HOST        \"MinGW-TDM\"",
    "    #define RC_DBG_REL     \"\"",
    "    #define RC_FILEFLAGS   0",
    "",
    "  #elif defined(__MINGW32__)",
    "    #define RC_HOST        \"MinGW\"",
    "    #define RC_DBG_REL     \"\"",
    "    #ifdef _DEBUG",
    "      #define RC_FILEFLAGS 1",
    "      #define RC_DBG_REL   \" debug\"",
    "    #else",
    "      #define RC_FILEFLAGS 0",
    "      #define RC_DBG_REL   \"\"",
    "    #endif",
    "",
    "  #elif defined(_MSC_VER)",
    "    #define RC_HOST        \"MSVC\"",
    "    #ifdef _DEBUG",
    "      #define RC_FILEFLAGS 1",
    "      #define RC_DBG_REL   \" debug\"",
    "    #else",
    "      #define RC_FILEFLAGS 0",
    "      #define RC_DBG_REL   \" release\"",
    "    #endif",
    "",
    "  #else",
    "    #error Who are you?",
    "  #endif",
    "",
    "  #define RC_VERSION    $(VER_MAJOR),$(VER_MINOR),$(VER_PATCH),0",
    "",
    "  LANGUAGE  0x09,0x01",
    "",
    "  VS_VERSION_INFO VERSIONINFO",
    "    FILEVERSION    RC_VERSION",
    "    PRODUCTVERSION RC_VERSION",
    "    FILEFLAGSMASK  0x3FL",
    "    FILEOS         VOS__WINDOWS32",
    "    FILETYPE       VFT_APP",
    "    FILESUBTYPE    0x0L",
    "    FILEFLAGS      RC_FILEFLAGS",
    "",
    "  BEGIN",
    "    BLOCK \"StringFileInfo\"",
    "    BEGIN",
    "      BLOCK \"040904B0\"",
    "      BEGIN",
    "        VALUE \"CompanyName\",     \"http://www.foo.com/\"",
    "        VALUE \"FileDescription\", \"foo-bar (\" RC_HOST RC_DBG_REL \").\"",
    "        VALUE \"FileVersion\",     \"$(VERSION).\"",
    "        VALUE \"InternalName\",    \"foo-bar.\"",
    "        VALUE \"LegalCopyright\",  \"GNU GENERAL PUBLIC LICENSE v2 or comercial licence.\"",
    "        VALUE \"Comments\",        \"Built on $(DATE) by ...\"",
    "      END",
    "    END",
    "",
    "    BLOCK \"VarFileInfo\"",
    "    BEGIN",
    "      VALUE \"Translation\", 0x409, 1200",
    "    END",
    "  END",
    "endef"
  };

static int generate_rc_macro (FILE *out)
{
  size_t i;

  if (num_rc_files > 0)
       fprintf (out,
                "\n#\n"
                "#! Since I found at least one .rc-file, I won't write a macro to generate a .rc-file.\n"
                "#\n");
  else
  {
    fprintf (out,
             "\n#\n"
             "# I have added this .rc rule and macro since you specfied the\n"
             "# '--res' option.\n"
             "#\n");
    for (i = 0; i < DIM(rc_macro); i++)
        write_template_line (out, rc_macro[i]);
  }
  return (1);
}


