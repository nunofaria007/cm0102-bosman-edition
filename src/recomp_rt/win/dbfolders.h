/* Custom data sets (Q143): folders in Databases, in the save folder.
 *
 * The player copies the files of a database (for example an Update Team zip) into a folder such as
 * Databases/May 2026. The Database choice on Setup Game lists each such folder by its name. A folder
 * counts when it holds all 20 database files that the game loads (the .dat files of the clean Data folder). The
 * check ignores the case of file names. That all 20 are needed is an inference: the Update Team
 * databases that were read replace the 20 files of the plain 3.9.68 data. */
#ifndef RECOMP_DBFOLDERS_H
#define RECOMP_DBFOLDERS_H

#include <stddef.h>

#define DBFOLDER_MAX 32      /* the most folders that are listed */
#define DBFOLDER_NAME 128    /* the longest folder name in bytes, with the end */

/* 1 when the folder holds every database file. */
int dbfolders_valid(const char *dir);

/* Lists the usable folders of <save_dir>/Databases, sorted by name without regard to case. Returns the
 * count, at most max. A missing Databases folder gives 0. */
int dbfolders_list(const char *save_dir, char names[][DBFOLDER_NAME], int max);

/* The host path of the folder name in <save_dir>/Databases. Returns 0 when the name has a path part,
 * the folder is missing or it is not usable. */
int dbfolders_path(const char *save_dir, const char *name, char *out, size_t room);

/* The name as button text in code page 1252: a letter that the code page lacks becomes '?'. At most
 * room - 1 bytes. */
void dbfolders_label(const char *utf8, char *out, size_t room);

#endif
