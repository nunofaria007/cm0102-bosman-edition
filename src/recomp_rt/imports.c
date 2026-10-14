/* Replacements for the imports of the original, by name. All are stdcall. An import without a line here
 * stops the program with its name (the recompiler writes that stub). */
#include "rt.h"
#include "rt_host.h"

RT_IMPORT(GlobalAlloc) { rt_count("GlobalAlloc"); rt_ret_stdcall(rt_alloc(rt_arg(1)), 2); }
RT_IMPORT(GlobalFree) { rt_count("GlobalFree"); rt_ret_stdcall(0, 1); }
RT_IMPORT(DeleteFileA) { rt_count("DeleteFileA"); rt_ret_stdcall(1, 1); }   /* deletes nothing */
