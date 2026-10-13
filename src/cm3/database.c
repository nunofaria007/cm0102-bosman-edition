#include "cm_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * CM 01/02 game database loader.
 *
 * The game stores its database in Data/ as multiple .dat files.
 * Record layouts must be recovered from the binary via REA
 * (FUN_0068d520 calls into Database.cpp / Db_files.cpp).
 *
 * This stub provides the loading interface. Record struct
 * definitions will be added as each table is reverse-engineered.
 */

typedef struct {
    const char *data_dir;
    bool loaded;
} cm_database_t;

static cm_database_t g_db = {0};

bool database_init(const char *data_dir)
{
    g_db.data_dir = data_dir;
    g_db.loaded   = false;

    /* TODO: load .dat files from data_dir */

    return true;
}

void database_shutdown(void)
{
    g_db.data_dir = NULL;
    g_db.loaded   = false;
}
