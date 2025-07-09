#pragma once

#define VER_MAJOR 1
#define VER_MINOR 1
#define VER_MICRO 0

#if !defined(RC_INVOKED)  /* Rest of file */

#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <io.h>
#include <windows.h>

#define DIM(array)        (sizeof(array) / sizeof(array[0]))
#define FILE_EXISTS(file) (access(file,0) == 0)

extern const char *c_rule, *cc_rule, *cpp_rule, *cxx_rule;
extern const char *make_template[];

extern void Abort (const char *fmt, ...);
extern int  write_template_line (FILE *out, const char *str);

typedef int (*walker_func) (const char *found, const WIN32_FIND_DATA *ff_data);

extern DWORD file_tree_walk (const char *dir, walker_func func);
extern int   file_tree_walk_recursive;

#endif  /* RC_INVOKED */
