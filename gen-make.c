/*
 * Quick and dirty makefile generator.
 *
 *  By Gisle Vanem <gvanem@yahoo.no> 2011 - 2020.
 *
 * Needs MinGW, MSVC or clang-cl to compile, but produces makefiles for:
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
 * generated a 'config.h' file here. And the "PROGRAM = foo.exe".
 */
#if !defined(IN_THE_REAL_MAKEFILE)
  #include "./config.h"

  /* These are needed if the generated Makefile should be able to link "foo.exe"
   */
  char *program_name = "foo";
  int write_template_line (FILE *out, const char *templ)
  {
    (void) out;
    (void) templ;
    return (0);
  }

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

#if defined(IN_THE_REAL_MAKEFILE)

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
static int         gnumake_generator = 0;
static const char *lib_rule = NULL;
static const char *c_rule = NULL;
static const char *cc_rule = NULL;
static const char *cpp_rule = NULL;
static const char *cxx_rule = NULL;
static char prog [_MAX_PATH] = { "gen-make.exe" };

static const char *line_end = "\\";
static const char *obj_suffix = "o";
static char       *cpu_env = "";

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

static char *str_replace (int ch1, int ch2, char *str);
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

  va_start (args, fmt);
  vfprintf (stderr, fmt, args);
  exit (-1);
}

static void usage (void)
{
  printf ("gen-make ver %d.%d.%d; A simple makefile generator.\n"
          "%s <options> [-G generator]:\n",
          VER_MAJOR, VER_MINOR, VER_MICRO, prog);
  printf ("  -d, --debug:   sets debug-level.\n"
          "  -n, --dry-run: test-mode; writes to stdout.\n"
          "  -f, --force:   ovewrite existing makefile without questions.\n"
          "  -G             Makefile generator for:\n"
          "                   windows -- GNU-make, targeting MinGW, MSVC and clang-cl.\n"
          "                   cygwin  -- GNU-make\n"
          "                   mingw   -- GNU-make\n"
          "                   msvc    -- Nmake\n"
          "                   watcom  -- Wmake\n");
  exit (0);
}

static enum Generators select_generator (const char *gen)
{
  if (!stricmp(gen, "mingw"))
     return (GEN_MINGW);

  if (!stricmp(gen, "cygwin"))
     return (GEN_CYGWIN);

  if (!stricmp(gen, "msvc"))
     return (GEN_MSVC);

  if (!stricmp(gen, "watcom"))
     return (GEN_WATCOM);

  if (!stricmp(gen, "windows"))
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
      case 'G':
           generator = select_generator (optarg);
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
#endif /* IN_THE_REAL_MAKEFILE */


int main (int argc, char **argv)
{
#if !defined(IN_THE_REAL_MAKEFILE)
  fprintf (stderr,
           "It seems a \"gen-make.exe\" generated Makefile was able to compile and build this '%s' program.\n"
           "Congratulations! But I will not let you do any damage here.\n", argv[0]);
  exit (0);
#else
  FILE *fil = stdout;
  const char *makefile = "?";

  GetModuleFileName (NULL, prog, sizeof(prog));
  parse_args (argc, argv);

  if (generator == GEN_UNKNOWN)
     usage();
  if (generator == GEN_MINGW || generator == GEN_CYGWIN || generator == GEN_WINDOWS)
     gnumake_generator = 1;

  cpu_env = getenv ("CPU");
  if (!cpu_env)
     cpu_env = "x86";

  tzset();
  if (!find_sources())
  {
    puts ("I found no .c/.cc/.cpp/.cxx sources");
    return (1);
  }

  switch (generator)
  {
    case GEN_MINGW:
         fil = init_generator (makefile = mingw_makefile_name);
         c_rule   = mingw_c_rule;
         cc_rule  = mingw_cc_rule;
         cpp_rule = mingw_cpp_rule;
         cxx_rule = mingw_cxx_rule;
         lib_rule = mingw_lib_rule;
         generate_mingw_make (fil);
         break;

    case GEN_CYGWIN:
         fil = init_generator (makefile = cygwin_makefile_name);
         c_rule   = cygwin_c_rule;
         cc_rule  = cygwin_cc_rule;
         cpp_rule = cygwin_cpp_rule;
         cxx_rule = cygwin_cxx_rule;
         lib_rule = cygwin_lib_rule;
         generate_cygwin_make (fil);
         break;

    case GEN_MSVC:
         fil = init_generator (makefile = msvc_makefile_name);
         c_rule   = msvc_c_rule;
         cc_rule  = msvc_cc_rule;
         cpp_rule = msvc_cpp_rule;
         cxx_rule = msvc_cxx_rule;
         lib_rule = msvc_lib_rule;
         generate_msvc_nmake (fil);
      // generate_msvc_deps (fil);
         break;

    case GEN_WATCOM:
         fil = init_generator (makefile = watcom_makefile_name);
         c_rule   = watcom_c_rule;
         cc_rule  = watcom_cc_rule;
         cpp_rule = watcom_cpp_rule;
         cxx_rule = watcom_cxx_rule;
         lib_rule = watcom_lib_rule;
         generate_watcom_wmake (fil);
      // generate_watcom_deps (fil);
         break;

    case GEN_WINDOWS:
         fil = init_generator (makefile = windows_makefile_name);
         c_rule   = windows_c_rule;
         cc_rule  = windows_cc_rule;
         cpp_rule = windows_cpp_rule;
         cxx_rule = windows_cxx_rule;
         lib_rule = windows_lib_rule;
         generate_windows_make (fil);
         break;

    default:
       break;
  }

  if (fil != stdout)
     fclose (fil);
  cleanup();
  fprintf (stderr, "Generated %s.\n", makefile);
#endif       /* IN_THE_REAL_MAKEFILE */
  return (0);
}

#if defined(IN_THE_REAL_MAKEFILE)
static void add_file (int is_c, int is_cc, int is_cpp, int is_cxx, int is_rc, int is_h_in, const char *file)
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

  char *f = strdup (file);

  assert (array);
  assert (num);

  if (gnumake_generator || generator == GEN_WATCOM)
     str_replace ('\\', '/', f);

  smartlist_add (array, f);
  *num = smartlist_len (array);
}

static int file_walker (const char *path, const WIN32_FIND_DATA *ff)
{
  const char *p, *end;
  char       *dot, *slash, dir [MAX_PATH];
  int         is_c = 0, is_cc = 0, is_cpp = 0, is_cxx = 0, is_rc = 0, is_h_in = 0;
  int         considered;
  int         i, len, add_it;

  /* Alway ignore '.git' entries
   */
  if (!strncmp(path, ".\\.git\\", 7))
     return (0);

  len = strlen (path);
  end = strrchr (path, '\0');
  dot = strrchr (path, '.');

  if (len <= 2 || (size_t)(end - dot) > sizeof(".cpp"))
     considered = 0;

  else if (!strcmp(dot, ".c"))
     is_c = 1;

  else if (!strcmp(dot, ".cc"))
     is_cc = 1;

  else if (!strcmp(dot, ".cpp"))
     is_cpp = 1;

  else if (!strcmp(dot, ".cxx"))
     is_cxx = 1;

  else if (!strcmp(dot-2, ".h.in"))
     is_h_in = 1;

  else if (!strcmp(dot, ".rc"))
     is_rc = 1;

#if 0     /* to-do: make dependency list */
  else if (!strcmp(dot, ".h"))
     is_h = 1;
#endif

  considered = is_c + is_cc + is_cpp + is_cxx + is_rc;

  DEBUG (2, "%-40s %sconsidered. is_c: %d, is_cc: %d, is_cpp: %d, is_cxx: %d, is_rc: %d\n",
         path, considered ? "" : "not ", is_c, is_cc, is_cpp, is_cxx, is_rc);

#if 0
  /*
   * to-do: check the source-file for a 'main(', 'WinMain(' or a 'DllMain('.
   */
  if (!main_found && !WinMain_found && !DllMain_found && (is_c || is_cc || is_cpp || is_cxx))
     grep_file (path, &main_found, &WinMain_found, &DllMain_found);
#endif

  if (!considered)
     return (0);

  p = path;
  if (!strncmp(p, ".\\", 2))
     p = path + 2;

  add_file (is_c, is_cc, is_cpp, is_cxx, is_rc, is_h_in, p);

  /* Check if this file has a unique directory part that needs to be added to 'vpaths[]'.
   */
  slash = strchr (p, '\\');
  if (!slash)
     return (0);

  memcpy (dir, p, slash-p);
  dir[slash-p] = '\0';
  add_it = 1;           /* assume not found */

  for (i = 0; i < smartlist_len(vpaths); i++)
      if (!strcmp(dir,smartlist_get(vpaths,i)))
      {
        add_it = 0;    /* already has this in 'vpaths[]' */
        break;
      }
  if (add_it)
    smartlist_add (vpaths, strdup(dir));

  DEBUG (2, "Did %sadd '%s' to 'vpaths[].'\n", add_it ? "" : "not ", dir);
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
static char *str_replace (int ch1, int ch2, char *str)
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

static const char *get_rc_target (void)
{
  if (!stricmp(cpu_env,"x86"))
     return ("pe-i386");
  if (!stricmp(cpu_env,"x64"))
     return ("pe-x86-64");
  return ("??");
}

static const char *get_m_bitness (void)
{
  if (!stricmp(cpu_env,"x86"))
     return ("32");
  if (!stricmp(cpu_env,"x64"))
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

static void write_one_object (FILE *out, smartlist_t *sl, int idx, int *line_len)
{
  char *file = basename (smartlist_get(sl,idx));
  char *dot  = strrchr (file, '.');

  *line_len += fprintf (out, " $(OBJ_DIR)/%.*s.%s", dot-file, file, obj_suffix);
  if (*line_len > 60 && idx < smartlist_len(sl)-1)
  {
    fprintf (out, " %s\n         ", line_end);
    *line_len = 0;
  }
}

static void write_objects (FILE *out, smartlist_t *sl, int num, int add_line_end)
{
  int i, line_len = 0;

  if (add_line_end)
     fprintf (out, " %s\n         ", line_end);

  for (i = 0; i < num; i++)
      write_one_object (out, sl, i, &line_len);
}

/*
 * Wmake: write one 'what: path1;path2' statement for a file-extension;
 * ".c", ".cc", ".cxx", or ".cpp" files.
 */
static void write_vpath_wmake (FILE *out, const char *what, smartlist_t *files)
{
  int i, max;

  if (smartlist_len(files) == 0)
     return;

  fprintf (out, ".%s", what);
  max = smartlist_len (vpaths);
  for (i = 0; i < max; i++)
  {
    fprintf (out, "%s", (const char*)smartlist_get(vpaths,i));
    if (i < max-1)
         fputc (';', out);
    else fputc ('\n', out);
  }
}

/*
 * Handler for format '%v'.
 *
 * VPATH handling for OpenWatcom's 'wmake' is speciail. It seems only to have
 * these statements:
 *  .c:    path1;path2
 *  .cc:   path1;path2
 *  .cpp:  path1;path2
 */
static void write_vpaths (FILE *out)
{
  int i, max = smartlist_len (vpaths);

  if (max == 0)
     return;

  if (generator == GEN_WATCOM)
  {
    write_vpath_wmake (out, "c:   ", c_files);
    write_vpath_wmake (out, "cc:  ", cc_files);
    write_vpath_wmake (out, "cxx: ", cxx_files);
    write_vpath_wmake (out, "cpp: ", cpp_files);
  }
  else
  {
    fprintf (out, "VPATH = ");
    for (i = 0; i < max; i++)
        fprintf (out, "%s ", (const char*)smartlist_get(vpaths,i));
  }
  fprintf (out, "  #! %d VPATHs\n", max);
}

/*
 * Write out a line from the template. Handle these formats:
 *  '%c' -> write the .c/.cc/.cxx/.cpp -> object rule(s).
 *  '%D' -> write the date/time stamp.
 *  '%o' -> write list of object files (prefixed with $(OBJ_DIR)).
 *  '%R' -> write a GNU-make macro for a foo.rc file.
 *  '%l' -> write the library rule.
 *  '%s' -> write list of .c-files for the SOURCES line.
 *  '%v' -> write a VPATH statement if needed.
 *  '%t' -> write out bitness for gcc targets:
 *           '-m%t' and CPU=x86 -> '-m32'.
 *           '-m%t' and CPU=x64 -> '-m64'.
 *  '%T' -> write out 'windres --target=x' for gcc targets:
 *           '--target=%T' and CPU=x86 -> '--target=pe-i386'.
 *           '--target=%T' and CPU=x64 -> '--target=pe-x86-64'.
 */
int write_template_line (FILE *out, const char *templ)
{
  const char *p;

  p = strchr (templ, '%');
  if (p && p[1] == 's')
  {
    int indent = p - templ;

    fprintf (out, "%.*s", indent, templ);

    /* Write the list of .c/.cc/.cpp-files at this point.
     */
    write_files (out, c_files, indent);
    fprintf (out, "%*s#! %d .c SOURCES files found (recursively)\n", indent, "", num_c_files);

    if (num_cc_files > 0)
    {
      fprintf (out, "\n#\n#! Add these $(CC_SOURCES) to $(OBJECTS) as needed.\n#\nCC_SOURCES = ");
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
       fprintf (out, "%*s#! Found %d .h.in-file(s); add rules for these as needed.\n", indent, "", num_h_in_files);

    if (num_rc_files)
       fprintf (out, "%*s#! Found %d .rc-file(s).\n", indent, "", num_rc_files);
    return (1);
  }

  p = strchr (templ, '%');
  if (p && p[1] == 'D')
  {
    time_t t = time (NULL);
    struct tm *tm = localtime (&t);

    p += 2;
    return fprintf (out, "%.*s<%.24s>%s\n", p-templ-2, templ, asctime(tm), p);
  }

  /* '%t' and '%T' are only used for gcc targets.
   */
  p = strchr (templ, '%');
  if (p && p[1] == 't')
  {
    fprintf (out, "%.*s%s", p-templ, templ, get_m_bitness());
    templ = p + 2;  /* skip past '%t' and fall-through */
  }

  /* '%T' writes out the correct flags for 'windres'.
   */
  p = strchr (templ, '%');
  if (p && p[1] == 'T')
  {
    fprintf (out, "%.*s%s", p-templ, templ, get_rc_target());
    templ = p + 2;
  }

  p = strchr (templ, '%');
  if (p && p[1] == 'l')
  {
    assert (lib_rule);
    return fprintf (out, "%s\n", lib_rule);
    templ = p + 2;  /* skip past '%l' and fall-through */
  }

  if (p && p[1] == 'R')
  {
    if (num_rc_files == 0)
       return generate_rc_macro (out);
    templ = p + 2;  /* skip past '%R' and fall-through */
  }

  p = strchr (templ, '%');
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

  p = strchr (templ, '%');
  if (p && p[1] == 'v')
  {
    write_vpaths (out);
    return (1);
  }

  p = strchr (templ, '%');
  if (p && p[1] == 'o')     /* Write the list of objects. Only used in template-watcom.c */
  {
    fprintf (out, "%.*s", p-templ-1, templ);

    write_objects (out, c_files, num_c_files, 0);

    if (num_cc_files > 0)
       write_objects (out, cc_files, num_cc_files, num_c_files > 0);

    if (num_cpp_files > 0)
       write_objects (out, cpp_files, num_cpp_files, num_c_files > 0 || num_cc_files > 0);

    fprintf (out, "\n        #! %d object files\n", num_c_files + num_cc_files + num_cpp_files);
    return (1);  /* should be nothing after '%o' */
  }

  if (!p && !strncmp(templ, "all: ", 5))
  {
    if (!main_found || !WinMain_found)
       fprintf (out, "#\n#! Failed to find a 'main()' or a 'WinMain()' in the SOURCES. Is it a .DLL?\n#\n");
    else if (DllMain_found)
       fprintf (out, "#\n#! Found a 'DllMain()' in the SOURCES. Rewrite the '$(PROGRAM)' rule into a 'link_DLL' rule.\n#\n");
  }
  fprintf (out, "%s\n", templ);
  return (1);
}

/*
 * For GNU-make only
 */
static const char *rc_macro[] = {
    "$(OBJ_DIR)/foo.rc: $(THIS_FILE)",
    "\t$(call green_msg, Generating $@)",
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
    "  #elif defined(__clang__)",
    "    #define RC_HOST        \"clang\"",
    "    #ifdef _DEBUG",
    "      #define RC_FILEFLAGS 1",
    "      #define RC_DBG_REL   \" debug\"",
    "    #else",
    "      #define RC_FILEFLAGS 0",
    "      #define RC_DBG_REL   \" release\"",
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

  if (!gnumake_generator)
     return (0);

  fprintf (out, "\n#\n"
           "# I have added this .rc macro since I found no .rc file.\n"
           "#\n");
  for (i = 0; i < DIM(rc_macro); i++)
      write_template_line (out, rc_macro[i]);
  return (1);
}
#endif /* IN_THE_REAL_MAKEFILE */

