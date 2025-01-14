/* common.hh -- extern declarations for functions defined in common.cc. */

/* Copyright (C) 1993-2020 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#if !defined(__COMMON_H)
#define __COMMON_H

#include "command.hh"

namespace bash
{

#define ISOPTION(s, c) (s[0] == '-' && s[1] == c && !s[2])
#define ISHELP(s) (STREQ ((s), "--help"))

#define GETOPT_EOF -1
#define GETOPT_HELP -99

#define CHECK_HELPOPT(l)                                                      \
  do                                                                          \
    {                                                                         \
      if ((l) && (l)->word && ISHELP ((l)->word->word.c_str ()))              \
        {                                                                     \
          builtin_help ();                                                    \
          return EX_USAGE;                                                    \
        }                                                                     \
    }                                                                         \
  while (0)

#define CASE_HELPOPT                                                          \
  case GETOPT_HELP:                                                           \
    builtin_help ();                                                          \
    return EX_USAGE

/* Flag values for parse_and_execute () */
enum parse_flags
{
  SEVAL_NOFLAGS = 0,
  SEVAL_NONINT = 0x001,
  SEVAL_INTERACT = 0x002,
  SEVAL_NOHIST = 0x004,
  SEVAL_NOFREE = 0x008,
  SEVAL_RESETLINE = 0x010,
  SEVAL_PARSEONLY = 0x020,
  SEVAL_NOTHROW = 0x040,
  SEVAL_FUNCDEF = 0x080,  // only allow function definitions
  SEVAL_ONECMD = 0x100,   // only allow a single command
  SEVAL_NOHISTEXP = 0x200 // inhibit history expansion
};

static inline parse_flags &
operator|= (parse_flags &a, const parse_flags &b)
{
  a = static_cast<parse_flags> (static_cast<uint32_t> (a)
                                | static_cast<uint32_t> (b));
  return a;
}

static inline parse_flags
operator| (const parse_flags &a, const parse_flags &b)
{
  return static_cast<parse_flags> (static_cast<uint32_t> (a)
                                   | static_cast<uint32_t> (b));
}

static inline parse_flags
operator& (const parse_flags &a, const parse_flags &b)
{
  return static_cast<parse_flags> (static_cast<uint32_t> (a)
                                   & static_cast<uint32_t> (b));
}

/* Flags for describe_command, shared between type.def and command.def */
enum cmd_desc_flags
{
  CDESC_ALL = 0x001,        /* type -a */
  CDESC_SHORTDESC = 0x002,  /* command -V */
  CDESC_REUSABLE = 0x004,   /* command -v */
  CDESC_TYPE = 0x008,       /* type -t */
  CDESC_PATH_ONLY = 0x010,  /* type -p */
  CDESC_FORCE_PATH = 0x020, /* type -ap or type -P */
  CDESC_NOFUNCS = 0x040,    /* type -f */
  CDESC_ABSPATH = 0x080,    /* convert to absolute path, no ./ */
  CDESC_STDPATH = 0x100     /* command -p */
};

/* Flags for get_job_by_name */
enum get_job_flags
{
  JM_PREFIX = 0x01,    /* prefix of job name */
  JM_SUBSTRING = 0x02, /* substring of job name */
  JM_EXACT = 0x04,     /* match job name exactly */
  JM_STOPPED = 0x08,   /* match stopped jobs only */
  JM_FIRSTMATCH = 0x10 /* return first matching job */
};

static inline get_job_flags &
operator|= (get_job_flags &a, const get_job_flags &b)
{
  a = static_cast<get_job_flags> (static_cast<uint32_t> (a)
                                | static_cast<uint32_t> (b));
  return a;
}

static inline get_job_flags
operator| (const get_job_flags &a, const get_job_flags &b)
{
  return static_cast<get_job_flags> (static_cast<uint32_t> (a)
                                   | static_cast<uint32_t> (b));
}

/* Flags for remember_args and value of changed_dollar_vars */
enum remember_args_flags
{
  ARGS_NOFLAGS = 0x0,
  ARGS_INVOC = 0x01,
  ARGS_FUNC = 0x02,
  ARGS_SETBLTIN = 0x04
};

static inline remember_args_flags &
operator|= (remember_args_flags &a, const remember_args_flags &b)
{
  a = static_cast<remember_args_flags> (static_cast<uint32_t> (a)
                                        | static_cast<uint32_t> (b));
  return a;
}

/* Maximum number of attribute letters */
static constexpr int MAX_ATTRIBUTES = 16;

#if 0
struct builtin *builtin_address_internal (const char *, int);
Shell::sh_builtin_func_t find_shell_builtin (const char *);
Shell::sh_builtin_func_t builtin_address (const char *);
Shell::sh_builtin_func_t find_special_builtin (const char *);
void initialize_shell_builtins ();

/* Functions from exit.def */
void bash_logout ();

/* Functions from getopts.def */
void getopts_reset (int);

/* Functions from help.def */
void builtin_help ();

/* Functions from read.def */
void read_tty_cleanup ();
int read_tty_modified ();

/* Functions from set.def */
char minus_o_option_value (const char *);
void list_minus_o_opts (int, int);
const char **get_minus_o_opts ();
int set_minus_o_option (int, const char *);

void set_shellopts ();
void parse_shellopts (const char *);
void initialize_shell_options (bool);

void reset_shell_options ();

char *get_current_options ();
void set_current_options (const char *);

/* Functions from shopt.def */
void reset_shopt_options ();
char **get_shopt_options ();

int shopt_setopt (const char *, int);
int shopt_listopt (const char *, int);

int set_login_shell (const char *, int);

void set_bashopts ();
void parse_bashopts (const char *);
void initialize_bashopts (int);

void set_compatibility_opts ();

/* Functions from type.def */
bool describe_command (const char *, int);

/* Functions from setattr.def */
int set_or_show_attributes (WORD_LIST *, int, int);
int show_all_var_attributes (int, int);
int show_local_var_attributes (int, int);
int show_var_attributes (SHELL_VAR *, int, int);
int show_name_attributes (const char *, int);
int show_localname_attributes (const char *, int);
int show_func_attributes (const char *, int);
void set_var_attribute (const char *, int, int);
int var_attribute_string (SHELL_VAR *, int, char *);

/* Functions from pushd.def */
char *get_dirstack_from_string (const char *);
char *get_dirstack_element (int64_t, int);
void set_dirstack_element (int64_t, int, const char *);
WORD_LIST *get_directory_stack (int);

#endif

} // namespace bash

#endif /* !__COMMON_H */
