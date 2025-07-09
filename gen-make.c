/*
 * Quick and dirty makefile generator for MSVC or clang-cl.
 *
 * By Gisle Vanem <gvanem@yahoo.no>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>
#include <conio.h>
#include <limits.h>
#include <sys/stat.h>

/* Assume if the generated Makefile was able to compile this, it also
 * generated a '$(OBJ_DIR)/config.h' file here. And the "TARGETS = bin/foo.exe".
 */
#if !defined(IN_THE_REAL_MAKEFILE)
  #include "config.h"

  /* These are needed if the generated Makefile should be able to link "bin/foo.exe"
   */
  const char *program_name = "bin/foo";
  int write_template_line (FILE *out, const char *templ)
  {
    (void) out;
    (void) templ;
    return (0);
  }

#else
  #include <getopt_long.h>
  const char *program_name = "bin/gen-make";
#endif

#include "gen-make.h"
#include "smartlist.h"

#if defined(IN_THE_REAL_MAKEFILE)

static char prog [_MAX_PATH] = { "bin/gen-make.exe" };

static const char *line_end = "\\";

static smartlist_t *c_files;
static smartlist_t *cc_files;
static smartlist_t *cpp_files;
static smartlist_t *cxx_files;
static smartlist_t *rc_files;
static smartlist_t *h_in_files;
static smartlist_t *vpaths;

static size_t num_c_files    = 0;
static size_t num_cc_files   = 0;
static size_t num_cpp_files  = 0;
static size_t num_cxx_files  = 0;
static size_t num_rc_files   = 0;
static size_t num_h_in_files = 0;
static int    debug_level    = 0;

static bool use_py_mako   = false; /* todo */
static bool main_found    = false;
static bool WinMain_found = false;
static bool DllMain_found = false;

static char *str_replace (int ch1, int ch2, char *str);
static int   find_sources (void);

#define DEBUG(level, fmt, ...)  do {                                       \
                                  if (debug_level >= level)                \
                                     fprintf (stderr, "%s(%u): " fmt,      \
                                       __FILE__, __LINE__, ##__VA_ARGS__); \
                                } while (0)

void Abort (const char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  vfprintf (stderr, fmt, args);
  exit (-1);
}

static void usage (FILE *out)
{
  printf ("gen-make ver %d.%d.%d; A simple makefile generator.\n"
          "%s <options>:\n",  VER_MAJOR, VER_MINOR, VER_MICRO, prog);
  printf ("  -d, --debug:      sets debug-level.\n"
          "  -r, --no-recurse: do not search recursively for source-files.\n");
  exit (0);
}

static void parse_args (int argc, char *const *argv)
{
  static const struct option long_opt[] = {
        { "help",       0, NULL, 'h' },   /* 0 */
        { "debug",      0, NULL, 'd' },
        { "no-recurse", 0, NULL, 'r' },   /* 2 */
        { NULL,         0, NULL, 0 }
      };

  while (1)
  {
    int idx = 0;
    int c = getopt_long (argc, argv, "hdrp", long_opt, &idx);

    if (c == -1)
       break;

    switch (c)
    {
      case 0:
           if (idx == 0)
              usage (stdout);
           if (idx == 1)
              debug_level++;
           if (idx == 2)
              file_tree_walk_recursive = 0;
           break;
      case 'h':
           usage (stdout);
           break;
      case 'd':
           debug_level++;
           break;
      case 'r':
           file_tree_walk_recursive = 0;
           break;
      case 'p':
           use_py_mako = true;
           break;
      default:
           fprintf (stderr, "Illegal option: '%c'\n", c);
           usage (stderr);
           break;
    }
  }
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
           "It seems a \"bin/gen-make.exe\" generated Makefile was able to compile and build this '%s' program.\n"
           "Congratulations! But I will not let you do any damage here.\n", argv[0]);
  exit (0);

#else
  size_t i;

  GetModuleFileName (NULL, prog, sizeof(prog));
  parse_args (argc, argv);
  tzset();
  if (!find_sources())
  {
    fputs ("I found no .c/.cc/.cpp/.cxx sources", stderr);
    return (1);
  }

  for (i = 0; make_template[i]; i++)
      write_template_line (stdout, make_template[i]);
  fputs ("\n", stdout);

  cleanup();
  fprintf (stderr, "Generated makefile to stdout.\n");
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
  size_t      i, len;
  bool        add_it;

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

  else if (stricmp(path, "gen-make.rc") && !strcmp(dot, ".rc"))
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

  memcpy (dir, p, slash - p);
  dir [slash - p] = '\0';
  add_it = true;           /* assume not found */

  for (i = 0; i < smartlist_len(vpaths); i++)
      if (!strcmp(dir, smartlist_get(vpaths, (int)i)))
      {
        add_it = false;    /* already has this in 'vpaths[]' */
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

  main_found = WinMain_found = DllMain_found = false;

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

static void write_files (FILE *out, smartlist_t *sl, size_t indent)
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
    else fprintf (out, "%-*s%s", (int)indent, "", file);

    if (i < max-1)
         fprintf (out, " %*s%s\n", (int)(longest - len), "", line_end);
    else fputc ('\n', out);
  }
}

/*
 * Handler for format '%v'.
 */
static void write_vpaths (FILE *out)
{
  int i, max = smartlist_len (vpaths);

  if (max == 0)
     return;

  fprintf (out, "VPATH = ");
  for (i = 0; i < max; i++)
      fprintf (out, "%s ", (const char*)smartlist_get(vpaths, i));

  fprintf (out, "  #! Found %d VPATHs\n", max);
}

/*
 * Write out a line from the template. Handle these formats:
 *  '%a' -> '1' if 'astyle.exe' is found on PATH. '0' otherwise.
 *  '%c' -> write the .c/.cc/.cxx/.cpp -> object rule(s).
 *  '%T' -> write the time stamp.
 *  '%s' -> write list of .c-files for the SOURCES line.
 *  '%v' -> write a VPATH statement if needed.
 */
int write_template_line (FILE *out, const char *templ)
{
  const char *p;

  p = strchr (templ, '%');
  if (p && p[1] == 'a')
  {
    char  exe [256] = "?";
    DWORD len = SearchPath (getenv("PATH"), "astyle.exe", NULL, sizeof(exe), exe, NULL);
    bool  found = (len > 0);

    p += 2;
    return fprintf (out, "%.*s%d%s\n", (int)(p - templ - 2), templ, found, p);
  }

  p = strchr (templ, '%');
  if (p && p[1] == 's')
  {
    size_t indent = p - templ;

    fprintf (out, "%.*s", (int)indent, templ);

    /* Write the list of .c/.cc/.cpp-files at this point.
     */
    write_files (out, c_files, indent);
    fprintf (out, "%*s#! %zd .c SOURCES files found (recursively: %d)\n", (int)indent, "", num_c_files, file_tree_walk_recursive);

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
       fprintf (out, "%*s#! Found %zd .h.in-file(s); add rules for these as needed.\n", (int)indent, "", num_h_in_files);

    if (num_rc_files)
       fprintf (out, "%*s#! Found %zd .rc-file(s).\n", (int)indent, "", num_rc_files);
    return (1);
  }

  p = strchr (templ, '%');
  if (p && p[1] == 'T')
  {
    time_t now = time (NULL);

    p += 2;
    return fprintf (out, "%.*s%.24s%s\n", (int)(p - templ - 2), templ, ctime(&now), p);
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

  if (!p && !strncmp(templ, "all: ", 5))
  {
    if (!main_found && !WinMain_found)
       fprintf (out, "#\n#! Failed to find a 'main()' or a 'WinMain()' in the SOURCES. Is it a .DLL?\n#\n");
    else if (DllMain_found)
       fprintf (out, "#\n#! Found a 'DllMain()' in the SOURCES. Rewrite the '$(PROGRAM)' rule into a 'link_DLL' rule.\n#\n");
  }
  fprintf (out, "%s\n", templ);
  return (1);
}
#endif /* IN_THE_REAL_MAKEFILE */

