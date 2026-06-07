
// -*-c++-*-
//
// CTHUGHA-L 							options.h
//
// Functions make it easy to change to new flame, wave & co. functions,
// and all the stuff needed to accept command-line-options and to
// read the ini-files.
//

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#if HAVE_GETOPT_H == 1
#include <getopt.h>
#else
#include "getopt.h"
#endif

#include "cthugha.h"
#include "EffectControl.h"

extern char extra_lib_path[]; /* extra path to search for image, tab, map and ini */
extern char ini_file_override[];

int get_params(int argc, char* argv[]);
int get_pre_params(int argc, char* argv[]);
int params_request_help(int argc, char* argv[]);
int params_request_version(int argc, char* argv[]);
int do_param(int c, int value, char* str);

extern struct option long_options[];

/*
 * Stuff about ini-files
 */
int read_ini(); /* read settings from ini-files */
int read_effect_control_usage_and_presets();
int write_ini();
int write_continuation_ini();

int open_ini_start();
int open_ini_file();
const char* ini_file_name(int ini_nr);
extern FILE* ini_file;
int open_ini_sys();
int get_ini_str_sys(const char* name, char* value);

// reading from ini files
int getini(const char* entry, char* value);
int getini(const char* entry, int* value);
int getini_yesno(const char* entry, int* value);
int getini(EffectControl& opt);

// writing to ini files
int putini(const char* entry, const char* value);
int putini(const Option& opt);

#endif
