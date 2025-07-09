/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
/*
 * Recursively descent the directory hierarchy rooted in DIR,
 * calling FUNC for each object in the hierarchy.  We cannot
 * use ftw(), because it uses some non-ANSI functions which
 * will pollute ANSI namespace, while we need this function
 * in some ANSI functions (e.g., rename()).  Thus, this function
 * is closely modeled on ftw(), but uses DOS directory search
 * functions and structures instead of opendir()/readdir()/stat().
 *
 * Copyright (c) 1995, 1996 Eli Zaretskii <eliz@is.elta.co.il>
 *
 * This software may be used freely as long as this copyright notice is
 * left intact.  There is no warranty on this software.
 *
 * Adapted for Win32 by Gisle Vanem <gvanem@yahoo.no> 2007-2012.
 */

#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <io.h>
#include <windows.h>

int file_tree_walk_recursive = 1;

typedef int (*walker_func)(const char *path, const WIN32_FIND_DATA *ff_data);

DWORD file_tree_walk (const char *dir, walker_func func)
{
  char            searchspec [MAX_PATH];
  char            path [MAX_PATH], *dir_end;
  DWORD           rc;
  HANDLE          fhandle;
  WIN32_FIND_DATA ff_data;

  if (!dir || !*dir || !func)
  {
    SetLastError (ERROR_BAD_ARGUMENTS);
    return (DWORD)-1;
  }

  /* Construct the search spec for FindFirstFile(). Treat ``d:'' as ``d:.''.
   */
  strcpy (searchspec, dir);
  dir_end = searchspec + strlen(searchspec) - 1;
  if (*dir_end == ':')
  {
    *++dir_end = '.';
    *++dir_end = '\0';
  }
  else if (*dir_end == '/' || *dir_end == '\\')
    *dir_end   = '\0';
  else
    ++dir_end;

  strcpy (dir_end, "\\*.*");

  /* Prepare the buffer where the full pathname of the found files
   * will be placed.
   */
  strcpy (path, dir);
  dir_end = path + strlen(path) - 1;
  if (*dir_end == ':')
  {
    *++dir_end = '.';
    dir_end[1] = '\0';
  }
  if (*dir_end != '/' && *dir_end != '\\')
  {
    *++dir_end = '\\';
    *++dir_end = '\0';
  }
  else
    ++dir_end;

  fhandle = FindFirstFile (searchspec, &ff_data);
  if (fhandle == INVALID_HANDLE_VALUE)
     return (DWORD)-1;

  do
  {
    int func_result;

    /* Skip `.' and `..' entries.  */
    if (ff_data.cFileName[0] == '.' &&
        (ff_data.cFileName[1] == '\0' || ff_data.cFileName[1] == '.'))
       continue;

    /* Construct full pathname in path[].
     */
    strcpy (dir_end, ff_data.cFileName);

    /* Invoke '(*func)()' on this file/directory.
     */
    func_result = (*func) (path, &ff_data);
    if (func_result != 0)
       return (func_result);

    /* If this is a directory, walk its siblings. Recursion!
     */
    if (file_tree_walk_recursive && (ff_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      DWORD rc = file_tree_walk (path, func);

      if (rc != 0)
         return (rc);
    }
  }
  while (FindNextFile(fhandle, &ff_data));

  rc = GetLastError();
  FindClose (fhandle);

  if (rc == ERROR_ACCESS_DENIED)     /* move on to another directory */
     return (0);

  if (rc == ERROR_NO_MORE_FILES)     /* normal case: tree exhausted */
     return (0);

  return (rc);
}

#ifdef TEST

static unsigned total;
static DWORD64  total_size;

int ff_walker (const char *path, const WIN32_FIND_DATA *ff)
{
  DWORD64 size = ((DWORD64)ff->nFileSizeHigh << 32) + (DWORD64)ff->nFileSizeLow;
  char    attr[] = "------" ;

  if (ff->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
     attr[5] = 'R';

  if (ff->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
     attr[4] = 'H';

  if (ff->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
     attr[3] = 'S';

  if (ff->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)
     attr[2] = 'C';

  if (ff->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
     attr[1] = 'D';

  if (ff->dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
     attr[0] = 'A';

  printf ("%.6s %7I64u %s\n", attr, size, path);
  total++;
  total_size += size;
  return (0);
}

int main (int argc, char **argv)
{
  if (argc > 1)
  {
    DWORD rc;

    puts ("Attr      Size Path\n"
          "-----------------------------------------------------------------------------");
    rc = file_tree_walk (argv[1], ff_walker);

    printf ("file_tree_walk: %lu, total: %u, total-size: %I64u bytes.",
            rc, total, total_size);
    if (rc != 0)
       printf (", rc: %lu (0x%lX), GetLastError(): %lu.", rc, rc, GetLastError());
    puts ("");
  }
  else
    printf ("Usage: %s dir-spec\n", argv[0]);

  return (0);
}
#endif
