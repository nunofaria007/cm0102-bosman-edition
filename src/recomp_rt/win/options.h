/* The options of the settings file settings.txt in the user folder.
 *
 * The file has one line "name = value" per option, and comment lines that start with "#". The runtime
 * reads every option once at start (options_load). It keeps all other lines of the file as they are:
 * it never rewrites or reorders a line of the player. It adds a line, with one comment line before it,
 * for each option that the file lacks, at the end of the file. A missing file is written with every
 * option at its default.
 *
 * A switch accepts on/off, yes/no and true/false in any case. A value that does not fit the option
 * gets a message on stderr, and the option keeps its default. */
#ifndef RECOMP_OPTIONS_H
#define RECOMP_OPTIONS_H

typedef enum {
    OPT_DISPLAY,               /* enum: widescreen (default), classic */
    OPT_SPACE_TO_CONTINUE,     /* switch, on */
    OPT_SHORTCUTS,             /* switch, on */
    OPT_SHARP_PICTURE,         /* switch, on */
    OPT_SAVE_FOLDER,           /* text, empty: the normal save folder */
    OPT_SOUND,                 /* switch, on (the game's own Sound setting applies as well) */
    OPT_CLICKABLE_NAMES,       /* switch, on */
    OPT_ATTRIBUTE_COLOURS,     /* switch, on */
    OPT_FASTER_DAYS,           /* switch, on */
    OPT_MATCH_COLUMNS,         /* switch, on */
    OPT_SAVE_BACKUPS,          /* switch, on */
    OPT_COUNT
} OptionId;

/* Values of OPT_DISPLAY for options_enum. */
enum { DISPLAY_WIDESCREEN = 0, DISPLAY_CLASSIC = 1 };

/* Reads the settings file at path, or writes it when it is missing, and adds the lines that it lacks.
 * Call it once, at start. Before it, and when the file cannot be read, every getter gives the default. */
void options_load(const char *path);

/* The value of a switch option: 1 or 0. */
int options_bool(OptionId id);
/* The value of a text option. Never NULL. The text has no spaces at its ends. */
const char *options_string(OptionId id);
/* The value of an enum option: the index in its list of values. */
int options_enum(OptionId id);

/* The name of an option in the file, for messages. */
const char *options_name(OptionId id);

#endif
