/* Readline.hh -- the names of functions callable from within readline. */

/* Copyright (C) 1987-2020 Free Software Foundation, Inc.
   Copyright 2022, Jake Hamby.

   This file is part of the GNU Readline Library (Readline), a library
   for reading lines of text with interactive input and history editing.

   Readline is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Readline is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Readline.  If not, see <http://www.gnu.org/licenses/>.
*/

#if !defined(_READLINE_H_)
#define _READLINE_H_

#include "rldefs.hh"
#include "rlprivate.hh"

#include "history.hh"
#include "rlshell.hh"
#include "tilde.hh"

#include "rltty.hh"

#if defined(HAVE_SYS_IOCTL_H)
#include <sys/ioctl.h> /* include for declaration of ioctl */
#endif

#include "tcap.hh"

#include "posixdir.hh"

#if defined(HAVE_PWD_H)
#include <pwd.h>
#endif

#include <csignal>

// Remove __attribute__ if we're not using GCC or Clang.
#ifndef __attribute__
#if !defined(__clang__) && !defined(__GNUC__)
#define __attribute__(x)
#endif
#endif

/* Hex-encoded Readline version number. */
#define RL_READLINE_VERSION 0x0900 /* Readline 9.0 (why not?) */

#define RL_VERSION_MAJOR 9
#define RL_VERSION_MINOR 0

/* Put everything in the readline namespace. */
namespace readline
{

// moved from rldefs.h
#if defined(HAVE_STRCASECMP)
#define _rl_stricmp strcasecmp
#define _rl_strnicmp strncasecmp
#else
int _rl_stricmp (const char *, const char *);
int _rl_strnicmp (const char *, const char *, int);
#endif

#if defined(HAVE_STRPBRK) && !defined(HANDLE_MULTIBYTE)
#define _rl_strpbrk(a, b) strpbrk ((a), (b))
#else
char *_rl_strpbrk (const char *, const char *);
#endif

/* Readline data structures. */

/* Maintaining the state of undo.  We remember individual deletes and inserts
   on a chain of things to do. */

/* The actions that undo knows how to undo.  Notice that UNDO_DELETE means
   to insert some text, and UNDO_INSERT means to delete some text.   I.e.,
   the code tells undo what to undo, not how to undo it. */
enum undo_code
{
  UNDO_DELETE,
  UNDO_INSERT,
  UNDO_BEGIN,
  UNDO_END
};

/* What an element of the undo_list looks like. */
class UNDO_ENTRY
{
public:
  UNDO_ENTRY (undo_code what_, size_t start_, size_t end_, char *text_)
      : text (text_), start (start_), end (end_), what (what_)
  {
    delete[] text_; // we've made a copy of the original
  }

  UNDO_ENTRY (undo_code what_, size_t start_, size_t end_,
              string_view text_)
      : text (text_), start (start_), end (end_), what (what_)
  {
  }

  std::string text;  /* The text to insert, if undoing a delete. */
  size_t start, end; /* Where the change took place. */
  undo_code what;    /* Delete, Insert, Begin, End. */
};

/* Wrapper for a vector of UNDO_ENTRY that history can delete. */
class UNDO_LIST : public hist_data
{
public:
  UNDO_LIST () {}
  virtual ~UNDO_LIST () override;

  inline void
  append (const UNDO_ENTRY &ue)
  {
    entries.push_back (ue);
  }

  inline UNDO_ENTRY &
  back ()
  {
    return entries.back ();
  }

  inline void
  pop_back ()
  {
    entries.pop_back ();
  }

  std::vector<UNDO_ENTRY> entries;
};

/* A global function for the use of tputs () */
extern "C" int _rl_output_character_function (int c);

// The output stream that the global tputs() function uses.
extern "C" FILE *_rl_out_stream;

/* Global atomic signal caught flag. */
extern volatile sig_atomic_t _rl_caught_signal;

constexpr char FACE_NORMAL = '0';
constexpr char FACE_STANDOUT = '1';
constexpr char FACE_INVALID = static_cast<char> (1);

/* Whether we used any colors in the output so far.  If so, we will
   need to restore the default color later.  If not, we will need to
   call prep_non_filename_text before using color for the first time. */
enum indicator_no
{
  C_LEFT,
  C_RIGHT,
  C_END,
  C_RESET,
  C_NORM,
  C_FILE,
  C_DIR,
  C_LINK,
  C_FIFO,
  C_SOCK,
  C_BLK,
  C_CHR,
  C_MISSING,
  C_ORPHAN,
  C_EXEC,
  C_DOOR,
  C_SETUID,
  C_SETGID,
  C_STICKY,
  C_OTHER_WRITABLE,
  C_STICKY_OTHER_WRITABLE,
  C_CAP,
  C_MULTIHARDLINK,
  C_CLR_TO_EOL
};

/* The number of color indicators is unlikely to change. */
constexpr int NUM_COLORS = 24;

/* This must be large enough to hold bindings for all of the characters
   in a desired character set (e.g, 128 for ASCII, 256 for ISO Latin-x,
   and so on) plus one for subsequence matching. It also needs to be a
   preprocessor #define, as it's used by #ifdef checks. */
#define KEYMAP_SIZE 257

/* The extra keymap entry after 0-127 or 0-255. */
constexpr int ANYOTHERKEY = KEYMAP_SIZE - 1;

/* The values that TYPE can have in a keymap entry. */
enum keymap_entry_type
{
  ISFUNC = 0,
  ISKMAP = 1,
  ISMACR = 2
};

/* Possible values for do_replace argument to rl_filename_quoting_function,
   called by rl_complete_internal. */
enum replace_type
{
  NO_MATCH = 0,
  SINGLE_MATCH = 1,
  MULT_MATCH = 2
};

// moved from rlprivate.h

/* search types */
enum rl_search_type
{
  RL_SEARCH_ISEARCH = 0x01, // incremental search
  RL_SEARCH_NSEARCH = 0x02, // non-incremental search
  RL_SEARCH_CSEARCH = 0x04  // intra-line char search
};

/* search flags */
enum rl_search_flags
{
  SF_NONE = 0x00,
  SF_REVERSE = 0x01,
  SF_FOUND = 0x02,
  SF_FAILED = 0x04,
  SF_CHGKMAP = 0x08,
  SF_PATTERN = 0x10,
  SF_NOCASE = 0x20 /* unused so far */
};

static inline rl_search_flags &
operator|= (rl_search_flags &a, const rl_search_flags &b)
{
  a = static_cast<rl_search_flags> (static_cast<uint32_t> (a)
                                    | static_cast<uint32_t> (b));
  return a;
}

static inline rl_search_flags
operator| (const rl_search_flags &a, const rl_search_flags &b)
{
  return static_cast<rl_search_flags> (static_cast<uint32_t> (a)
                                       | static_cast<uint32_t> (b));
}

static inline rl_search_flags &
operator&= (rl_search_flags &a, const rl_search_flags &b)
{
  a = static_cast<rl_search_flags> (static_cast<uint32_t> (a)
                                    & static_cast<uint32_t> (b));
  return a;
}

static inline rl_search_flags
operator& (const rl_search_flags &a, const rl_search_flags &b)
{
  return static_cast<rl_search_flags> (static_cast<uint32_t> (a)
                                       & static_cast<uint32_t> (b));
}

static inline rl_search_flags
operator~(const rl_search_flags &a)
{
  return static_cast<rl_search_flags> (~static_cast<uint32_t> (a));
}

static constexpr int DEFAULT_LINE_BUFFER_SIZE = 1024;

#if defined(HANDLE_SIGNALS)

extern "C"
{
  /* This typedef is now a pointer-to-function, which makes more sense. */
  typedef void (*SigHandler) (int sig);
}

#if defined(HAVE_POSIX_SIGNALS)
typedef struct sigaction sighandler_cxt;
#define rl_sigaction(s, nh, oh) sigaction (s, nh, oh)
#else
typedef struct
{
  SigHandler sa_handler;
  int sa_mask, sa_flags;
} sighandler_cxt;
#define sigemptyset(m)
#endif /* !HAVE_POSIX_SIGNALS */

#ifndef SA_RESTART
#define SA_RESTART 0
#endif

#endif /* HANDLE_SIGNALS */

#if defined(TERMIOS_TTY_DRIVER)
#define TIOTYPE struct termios
#else
#define TIOTYPE struct termio
#endif /* !TERMIOS_TTY_DRIVER */

// Main Readline class containing all of the methods and shared state.
class Readline : public History
{
public:
  // This constructor has parameters for all of the defaults that bash
  // overrides. Passing const char * is easier here than using std::string.
  Readline (const char *search_delimiter_chars = nullptr,
            const char *event_delimiter_chars = HISTORY_EVENT_DELIMITERS,
            rl_linebuf_func_t inhibit_expansion_function = nullptr,
            bool quotes_inhibit_expansion = false);

  virtual ~Readline () override;

  /* Bindable functions */

  typedef int (Readline::*rl_command_func_t) (int, int);

  /* Typedefs for the completion system */

  // Returns a newly-allocated string, or nullptr.
  typedef std::string *(Readline::*rl_compentry_func_t) (string_view, int);

  // Completion function that takes an input string and start/end positions,
  // and returns a newly-allocated vector of strings, or nullptr.
  typedef std::vector<std::string *> *(Readline::*rl_completion_func_t) (
      string_view, size_t, size_t);

  typedef std::string (Readline::*rl_quote_func_t) (string_view,
                                                    replace_type, char *);

  typedef std::string (Readline::*rl_dequote_func_t) (string_view,
                                                      int);

  typedef int (Readline::*rl_compignore_func_t) (std::vector<std::string *> &);

  typedef void (Readline::*rl_compdisp_func_t) (std::vector<std::string *> &,
                                                size_t);

  /* Type for input and pre-read hook functions like rl_event_hook */
  typedef int (Readline::*rl_hook_func_t) ();

  /* Input function type */
  typedef int (Readline::*rl_getc_func_t) (FILE *);

  /* `Generic' function pointer typedefs */

  typedef int (Readline::*rl_icppfunc_t) (std::string &);

  typedef void (Readline::*rl_voidfunc_t) ();
  typedef void (Readline::*rl_vintfunc_t) (int);
  typedef void (Readline::*rl_vcpfunc_t) (std::string &);

  typedef char *(Readline::*rl_cpvfunc_t) ();
  typedef char *(Readline::*rl_cpifunc_t) (int);
  typedef char *(Readline::*rl_cpcpfunc_t) (std::string &);

  typedef int (Readline::*_rl_sv_func_t) (const char *);

  /* typedef from bind.cc */
  typedef int (Readline::*_rl_parser_func_t) (char *);

  /* fill in more as needed */
  /* `Generic' callback data and functions */
  struct _rl_callback_generic_arg
  {
    _rl_callback_generic_arg (int count_) : count (count_) {}
    int count;
    int i1, i2;
    /* add here as needed */
  };

  typedef int (Readline::*_rl_callback_func_t) (_rl_callback_generic_arg *);

  typedef void (Readline::*_rl_sigcleanup_func_t) (int, void *);

  /* The data structure for mapping textual names to code addresses. */
  struct FUNMAP
  {
    const char *name;
    rl_command_func_t function;
  };

  struct boolean_var_t
  {
    boolean_var_t (const char *name_, bool *value_, int flags_)
        : name (name_), value (value_), flags (flags_)
    {
    }

    const char *name;
    bool *value;
    int flags; // XXX should this be an enum type?
  };

  typedef boolean_var_t bvt;

  inline std::vector<boolean_var_t> *
  boolean_varlist ()
  {
    if (boolean_varlist_.empty ())
      init_boolean_varlist ();

    return &boolean_varlist_;
  }

  struct KEYMAP_ENTRY;
  typedef KEYMAP_ENTRY *Keymap;

  // A keymap contains one entry for each key in the ASCII set. Each entry
  // consists of a type and a pointer. FUNCTION is the address of a function to
  // run, or the address of a keymap to indirect through. TYPE says which kind
  // of thing FUNCTION is.
  struct KEYMAP_ENTRY
  {
    KEYMAP_ENTRY () : type (ISFUNC) {}

    KEYMAP_ENTRY (keymap_entry_type t, rl_command_func_t f)
        : value (f), type (t)
    {
    }

    KEYMAP_ENTRY (keymap_entry_type t, Keymap m) : value (m), type (t) {}

    // Callers should delete the string on struct deletion. If we add a
    // destructor to do that, then we also need a copy constructor that avoids
    // double deletion after initializing the keymap array. It's easier to keep
    // the memory allocation behavior of the original C version.
    KEYMAP_ENTRY (keymap_entry_type t, char *macro) : value (macro), type (t)
    {
    }

    // This union should be the size of one pointer.
    union _keymap_value_type
    {
      _keymap_value_type () : function (nullptr) {}
      _keymap_value_type (rl_command_func_t func_) : function (func_) {}
      _keymap_value_type (Keymap map_) : map (map_) {}
      _keymap_value_type (char *macro_) : macro (macro_) {}

      rl_command_func_t function;
      Keymap map;
      char *macro;
    } value;

    // Keymap type enum (32-bit).
    keymap_entry_type type;
  };

  typedef KEYMAP_ENTRY KME; // convenience typedef

  /* bind.cc: auxiliary functions to manage keymaps. */

  struct name_and_keymap
  {
    name_and_keymap (const char *name_, Keymap map_) : name (name_), map (map_)
    {
    }

    const char *name;
    Keymap map;
  };

  typedef name_and_keymap nmk;

  // Lazy init of the keymap names with builtin keymaps.
  inline std::vector<name_and_keymap> *
  keymap_names ()
  {
    if (keymap_names_.empty ())
      init_keymap_names ();

    return &keymap_names_;
  }

  // Use a linear search because the list is in order for reverse lookups (by
  // map), and also because users can add their own keymaps.
  inline int
  _rl_get_keymap_by_name (const char *name)
  {
    std::vector<nmk> *nml = keymap_names ();
    int i = 0;

    for (std::vector<nmk>::iterator it = nml->begin (); it != nml->end ();
         ++it, ++i)
      if (_rl_stricmp (name, it->name) == 0)
        return i;

    return -1;
  }

  // Use a linear search since this is a reverse lookup and we want the first
  // entry found.
  inline int
  _rl_get_keymap_by_map (Keymap map)
  {
    std::vector<nmk> *nml = keymap_names ();
    int i = 0;

    for (std::vector<nmk>::iterator it = nml->begin (); it != nml->end ();
         ++it, ++i)
      if (map == it->map)
        return i;

    return -1;
  }

  inline const char *
  rl_get_keymap_name (Keymap map)
  {
    int i = _rl_get_keymap_by_map (map);
    return (i >= 0) ? keymap_names_[static_cast<size_t> (i)].name : nullptr;
  }

  /* keymaps.cc */

  Keymap new_emacs_standard_keymap ();
  Keymap new_emacs_meta_keymap ();
  Keymap new_emacs_ctlx_keymap ();

  // Return emacs standard keymap, with lazy initialization.
  // This keymap also sets up the emacs meta and ctlx keymaps.
  inline Keymap
  emacs_standard_keymap ()
  {
    if (!emacs_standard_keymap_)
      emacs_standard_keymap_ = new_emacs_standard_keymap ();
    return emacs_standard_keymap_;
  }

  // Return emacs meta keymap, with lazy initialization.
  inline Keymap
  emacs_meta_keymap ()
  {
    if (!emacs_meta_keymap_)
      emacs_meta_keymap_ = new_emacs_meta_keymap ();
    return emacs_meta_keymap_;
  }

  // Return emacs ctlx keymap, with lazy initialization.
  inline Keymap
  emacs_ctlx_keymap ()
  {
    if (!emacs_ctlx_keymap_)
      emacs_ctlx_keymap_ = new_emacs_ctlx_keymap ();
    return emacs_ctlx_keymap_;
  }

#if defined(VI_MODE)
  Keymap new_vi_insertion_keymap ();
  Keymap new_vi_movement_keymap ();

  // Return vi insertion keymap, with lazy initialization.
  inline Keymap
  vi_insertion_keymap ()
  {
    if (!vi_insertion_keymap_)
      vi_insertion_keymap_ = new_vi_insertion_keymap ();
    return vi_insertion_keymap_;
  }

  // Return vi insertion keymap, with lazy initialization.
  inline Keymap
  vi_movement_keymap ()
  {
    if (!vi_movement_keymap_)
      vi_movement_keymap_ = new_vi_movement_keymap ();
    return vi_movement_keymap_;
  }
#endif

  /* ************************************************************** */
  /*								    */
  /*	     Functions available to bind to key sequences	    */
  /*								    */
  /* ************************************************************** */

  /* Bindable commands for numeric arguments. */

  int rl_digit_argument (int, int);
  int rl_universal_argument (int, int);

  /* Bindable commands for moving the cursor. */

  int rl_forward_byte (int, int);
  int rl_forward_char (int, int);
  int rl_forward (int, int);
  int rl_backward_byte (int, int);
  int rl_backward_char (int, int);
  int rl_backward (int, int);
  int rl_beg_of_line (int, int);
  int rl_end_of_line (int, int);
  int rl_forward_word (int, int);
  int rl_backward_word (int, int);
  int rl_refresh_line (int, int);
  int rl_clear_screen (int, int);
  int rl_clear_display (int, int);
  int rl_skip_csi_sequence (int, int);
  int rl_arrow_keys (int, int);

  int rl_previous_screen_line (int, int);
  int rl_next_screen_line (int, int);

  /* Bindable commands for inserting and deleting text. */

  int rl_insert (int, int);
  int rl_quoted_insert (int, int);
  int rl_tab_insert (int, int);
  int rl_newline (int, int);
  int rl_do_lowercase_version (int, int);
  int rl_rubout (int, int);
  int rl_delete (int, int);
  int rl_rubout_or_delete (int, int);
  int rl_delete_horizontal_space (int, int);
  int rl_delete_or_show_completions (int, int);
  int rl_insert_comment (int, int);

  /* Bindable commands for changing case. */

  int rl_upcase_word (int, int);
  int rl_downcase_word (int, int);
  int rl_capitalize_word (int, int);

  /* Bindable commands for transposing characters and words. */

  int rl_transpose_words (int, int);
  int rl_transpose_chars (int, int);

  /* Bindable commands for searching within a line. */

  int rl_char_search (int, int);
  int rl_backward_char_search (int, int);

  /* Bindable commands for readline's interface to the command history. */

  int rl_beginning_of_history (int, int);
  int rl_end_of_history (int, int);
  int rl_get_next_history (int, int);
  int rl_get_previous_history (int, int);
  int rl_operate_and_get_next (int, int);

  /* Bindable commands for managing the mark and region. */

  int rl_set_mark (int, int);
  int rl_exchange_point_and_mark (int, int);

  /* Bindable commands to set the editing mode (emacs or vi). */

  int rl_vi_editing_mode (int, int);
  int rl_emacs_editing_mode (int, int);

  /* Bindable commands to change the insert mode (insert or overwrite) */

  int rl_overwrite_mode (int, int);

  /* Bindable commands for managing key bindings. */

  int rl_re_read_init_file (int, int);
  int rl_dump_functions (int, int);
  int rl_dump_macros (int, int);
  int rl_dump_variables (int, int);

  /* Bindable commands for word completion. */

  int rl_complete (int, int);
  int rl_possible_completions (int, int);
  int rl_insert_completions (int, int);
  int rl_old_menu_complete (int, int);
  int rl_menu_complete (int, int);
  int rl_backward_menu_complete (int, int);

  // Bindable commands for killing and yanking text, and managing the kill
  // ring.

  int rl_kill_word (int, int);
  int rl_backward_kill_word (int, int);
  int rl_kill_line (int, int);
  int rl_backward_kill_line (int, int);
  int rl_kill_full_line (int, int);
  int rl_unix_word_rubout (int, int);
  int rl_unix_filename_rubout (int, int);
  int rl_unix_line_discard (int, int);
  int rl_copy_region_to_kill (int, int);
  int rl_kill_region (int, int);
  int rl_copy_forward_word (int, int);
  int rl_copy_backward_word (int, int);
  int rl_yank (int, int);
  int rl_yank_pop (int, int);
  int rl_yank_nth_arg (int, int);
  int rl_yank_last_arg (int, int);
  int rl_bracketed_paste_begin (int, int);
  int _rl_copy_word_as_kill (int, int);

  /* Not available unless _WIN32 is defined. */
#if defined(_WIN32)
  int rl_paste_from_clipboard (int, int);
#endif

  /* Bindable commands for incremental searching. */

  int rl_reverse_search_history (int, int);
  int rl_forward_search_history (int, int);
  int rl_search_history (int, int);

  /* Bindable keyboard macro commands. */

  int rl_start_kbd_macro (int, int);
  int rl_end_kbd_macro (int, int);
  int rl_call_last_kbd_macro (int, int);
  int rl_print_last_kbd_macro (int, int);

  /* Bindable undo commands. */

  int rl_revert_line (int, int);
  int rl_undo_command (int, int);

  /* Bindable tilde expansion commands. */

  int rl_tilde_expand (int, int);

  /* Bindable terminal control commands. */

  int rl_restart_output (int, int);
  int rl_stop_output (int, int);

  /* Miscellaneous bindable commands. */

  int rl_abort (int, int);
  int rl_tty_status (int, int);

  // Bindable commands for incremental and non-incremental history searching.

  int rl_history_search_forward (int, int);
  int rl_history_search_backward (int, int);
  int rl_history_substr_search_forward (int, int);
  int rl_history_substr_search_backward (int, int);
  int rl_noninc_forward_search (int, int);
  int rl_noninc_reverse_search (int, int);
  int rl_noninc_forward_search_again (int, int);
  int rl_noninc_reverse_search_again (int, int);

  /* Bindable command used when inserting a matching close character. */

  int rl_insert_close (int, int);

  /* Not available unless READLINE_CALLBACKS is defined. */

  void rl_callback_handler_install (string_view, rl_vcpfunc_t);
  void rl_callback_read_char ();
  void rl_callback_handler_remove ();
  void rl_callback_sigcleanup ();

  // Things for vi mode. Not available unless readline is compiled -DVI_MODE.

  /* VI-mode bindable commands. */

  int rl_vi_redo (int, int);
  int rl_vi_undo (int, int);
  int rl_vi_yank_arg (int, int);
  int rl_vi_fetch_history (int, int);
  int rl_vi_search_again (int, int);
  int rl_vi_search (int, int);
  int rl_vi_complete (int, int);
  int rl_vi_tilde_expand (int, int);
  int rl_vi_prev_word (int, int);
  int rl_vi_next_word (int, int);
  int rl_vi_end_word (int, int);
  int rl_vi_insert_beg (int, int);
  int rl_vi_append_mode (int, int);
  int rl_vi_append_eol (int, int);
  int rl_vi_eof_maybe (int, int);
  int rl_vi_insertion_mode (int, int);
  int rl_vi_insert_mode (int, int);
  int rl_vi_movement_mode (int, int);
  int rl_vi_arg_digit (int, int);
  int rl_vi_change_case (int, int);
  int rl_vi_put (int, int);
  int rl_vi_column (int, int);
  int rl_vi_delete_to (int, int);
  int rl_vi_change_to (int, int);
  int rl_vi_yank_to (int, int);
  int rl_vi_yank_pop (int, int);
  int rl_vi_rubout (int, int);
  int rl_vi_delete (int, int);
  int rl_vi_back_to_indent (int, int);
  int rl_vi_unix_word_rubout (int, int);
  int rl_vi_first_print (int, int);
  int rl_vi_char_search (int, int);
  int rl_vi_match (int, int);
  int rl_vi_change_char (int, int);
  int rl_vi_subst (int, int);
  int rl_vi_overstrike (int, int);
  int rl_vi_overstrike_delete (int, int);
  int rl_vi_replace (int, int);
  int rl_vi_set_mark (int, int);
  int rl_vi_goto_mark (int, int);

  /* VI-mode utility functions. */

  int rl_vi_domove (int, int *);

  /* A convenience function that calls _rl_vi_set_last to save the last command
     information and enters insertion mode. */
  void
  rl_vi_start_inserting (int key, int repeat, int sign)
  {
    _rl_vi_set_last (key, repeat, sign);
    rl_begin_undo_group (); /* ensure inserts aren't concatenated */
    rl_vi_insertion_mode (1, key);
  }

  /* VI-mode pseudo-bindable commands, used as utility functions. */

  int rl_vi_fWord (int, int);
  int rl_vi_bWord (int, int);
  int rl_vi_eWord (int, int);
  int rl_vi_fword (int, int);
  int rl_vi_bword (int, int);
  int rl_vi_eword (int, int);

  /* **************************************************************** */
  /*								    */
  /*			Well Published Functions		    */
  /*								    */
  /* **************************************************************** */

  /* Readline functions. */

  /* Read a line of input. Prompt with PROMPT (may be empty). */
  std::string readline (string_view);

  void rl_set_prompt (string_view);

  size_t rl_expand_prompt (char *); // this temporarily modifies the input

  int rl_initialize ();

  /* Utility functions to bind keys to readline commands. */

  /* Add NAME to the list of named functions.  Make FUNCTION be the function
     that gets called.  If KEY is not -1, then bind it. */
  inline void
  rl_add_defun (string_view name, rl_command_func_t function, int key)
  {
    if (key != -1)
      rl_bind_key (key, function);
    rl_add_funmap_entry (name, function);
  }

  /* Bind KEY to FUNCTION.  Returns non-zero if KEY is out of range. */
  int rl_bind_key (int, rl_command_func_t);

  /* Bind key sequence KEYSEQ to DEFAULT_FUNC if KEYSEQ is unbound.  Right
     now, this is always used to attempt to bind the arrow keys. */
  inline int
  rl_bind_key_if_unbound_in_map (int key, rl_command_func_t default_func,
                                 Keymap kmap)
  {
    std::string keyseq = rl_untranslate_keyseq (key);
    return rl_bind_keyseq_if_unbound_in_map (keyseq, default_func, kmap);
  }

  /* Bind KEY to FUNCTION in MAP.  Returns non-zero in case of invalid KEY. */
  inline int
  rl_bind_key_in_map (int key, rl_command_func_t function, Keymap map)
  {
    Keymap oldmap = _rl_keymap;
    _rl_keymap = map;
    int result = rl_bind_key (key, function);
    _rl_keymap = oldmap;
    return result;
  }

  /* Make KEY do nothing in MAP. Returns non-zero in case of error. */
  inline int
  rl_unbind_key_in_map (int key, Keymap map)
  {
    return rl_bind_key_in_map (key, nullptr, map);
  }

  int rl_bind_key_if_unbound (int, rl_command_func_t);

  int rl_unbind_function_in_map (rl_command_func_t, Keymap);

  // Unbind all keys bound to COMMAND, a bindable command name, in MAP.
  inline int
  rl_unbind_command_in_map (const char *command, Keymap map)
  {
    rl_command_func_t func = rl_named_function (command);
    if (func == nullptr)
      return 0;

    return rl_unbind_function_in_map (func, map);
  }

  /* Bind the key sequence represented by the string KEYSEQ to
     FUNCTION, starting in the current keymap.  This makes new
     keymaps as necessary. */
  inline int
  rl_bind_keyseq (string_view keyseq, rl_command_func_t function)
  {
    KEYMAP_ENTRY entry (ISFUNC, function);
    return rl_generic_bind (keyseq, entry, _rl_keymap);
  }

  /* Bind the key sequence represented by the string KEYSEQ to
     FUNCTION.  This makes new keymaps as necessary.  The initial
     place to do bindings is in MAP. */
  inline int
  rl_bind_keyseq_in_map (string_view keyseq, rl_command_func_t function,
                         Keymap map)
  {
    KEYMAP_ENTRY entry (ISFUNC, function);
    return rl_generic_bind (keyseq, entry, map);
  }

  inline int
  rl_bind_keyseq_if_unbound (string_view keyseq,
                             rl_command_func_t default_func)
  {
    return rl_bind_keyseq_if_unbound_in_map (keyseq, default_func, _rl_keymap);
  }

  /* Bind the key sequence represented by the string KEYSEQ to
     the string of characters MACRO.  This makes new keymaps as
     necessary.  The initial place to do bindings is in MAP. */
  inline int
  rl_macro_bind (string_view keyseq, string_view macro,
                 Keymap map)
  {
    std::string macro_keys;

    if (rl_translate_keyseq (macro, macro_keys))
      return -1;

    KEYMAP_ENTRY entry (ISMACR, savestring (macro_keys));
    rl_generic_bind (keyseq, entry, map);
    return 0;
  }

  int rl_bind_keyseq_if_unbound_in_map (string_view, rl_command_func_t,
                                        Keymap);

  int rl_generic_bind (string_view keyseq, KEYMAP_ENTRY &k, Keymap map);

  const char *rl_variable_value (const char *);

  int rl_variable_bind (const char *, const char *);

  /* Return a pointer to the function that STRING represents.
     If STRING doesn't have a matching function, then nullptr
     is returned. The string match is case-insensitive. */
  inline rl_command_func_t
  rl_named_function (const char *string)
  {
    rl_initialize_funmap ();

    for (size_t i = 0; funmap[i]; i++)
      if (_rl_stricmp (funmap[i]->name, string) == 0)
        return funmap[i]->function;

    return nullptr;
  }

  inline rl_command_func_t
  rl_function_of_keyseq (string_view keyseq, Keymap map,
                         keymap_entry_type *type)
  {
    return _rl_function_of_keyseq_internal (keyseq, map, type);
  }

  inline rl_command_func_t
  rl_function_of_keyseq_len (string_view keyseq, Keymap map,
                             keymap_entry_type *type)
  {
    return _rl_function_of_keyseq_internal (keyseq, map, type);
  }

  void rl_list_funmap_names ();

  std::vector<std::string> rl_invoking_keyseqs_in_map (rl_command_func_t,
                                                       Keymap);

  // Return a vector of strings which represent the key sequences that can be
  // used to invoke FUNCTION using the current keymap.
  std::vector<std::string>
  rl_invoking_keyseqs (rl_command_func_t function)
  {
    return rl_invoking_keyseqs_in_map (function, _rl_keymap);
  }

  void rl_function_dumper (bool);

  inline void
  rl_macro_dumper (bool print_readably)
  {
    _rl_macro_dumper_internal (print_readably, _rl_keymap, nullptr);
  }

  void rl_variable_dumper (bool);

  int rl_read_init_file (const char *);

  int rl_parse_and_bind (char *);

  /* Functions for manipulating keymaps. */

  // Return a new, empty keymap. Free it with delete[] when you are done.
  Keymap rl_make_bare_keymap ();

  /* Return a new keymap which is a copy of MAP. */
  Keymap rl_copy_keymap (Keymap);

  /* Return a new keymap with the printing characters bound to rl_insert,
     the lowercase Meta characters bound to run their equivalents, and
     the Meta digits bound to produce numeric arguments. */
  Keymap rl_make_keymap ();

  /* These functions actually appear in bind.cc */

  /* Return the keymap corresponding to a given name.  Names look like
     `emacs' or `emacs-meta' or `vi-insert'.  */
  Keymap
  rl_get_keymap_by_name (const char *name)
  {
    int i = _rl_get_keymap_by_name (name);
    return (i >= 0) ? keymap_names_[static_cast<size_t> (i)].map : nullptr;
  }

  /* Return the current keymap. */
  Keymap
  rl_get_keymap ()
  {
    return _rl_keymap;
  }

  /* Set the current keymap to MAP. */
  void
  rl_set_keymap (Keymap map)
  {
    if (map)
      _rl_keymap = map;
  }

  // Set the name of MAP to NAME. This function isn't used by bash.
  int rl_set_keymap_name (const char *, Keymap);

  // Functions for manipulating the funmap, which maps command names to
  // functions.

  void rl_initialize_funmap ();

  void
  rl_add_funmap_entry (string_view name, rl_command_func_t function)
  {
    FUNMAP *new_funmap = new FUNMAP ();
    new_funmap->name = savestring (name);
    new_funmap->function = function;
    funmap.push_back (new_funmap);
  }

  const char **rl_funmap_names ();

  /* Utility functions for managing keyboard macros. */

  inline void
  rl_push_macro_input (std::string &macro)
  {
    _rl_with_macro_input (savestring (macro));
  }

  /* Functions for undoing, from undo.cc */

  // Remember how to undo something.  Concatenate some undos if that seems
  // right.
  inline void
  rl_add_undo (enum undo_code what, size_t start, size_t end,
               char *text = nullptr)
  {
    if (rl_undo_list == nullptr)
      {
        rl_undo_list = new UNDO_LIST ();
      }

    UNDO_ENTRY ue (what, start, end, text);
    rl_undo_list->append (ue);
  }

  /* Free the existing undo list. */
  inline void
  rl_free_undo_list ()
  {
    delete rl_undo_list;
    rl_undo_list = nullptr;
  }

  int rl_do_undo ();

  /* Begin a group.  Subsequent undos are undone as an atomic operation. */
  inline void
  rl_begin_undo_group ()
  {
    rl_add_undo (UNDO_BEGIN, 0, 0);
    _rl_undo_group_level++;
  }

  /* End an undo group started with rl_begin_undo_group (). */
  inline void
  rl_end_undo_group ()
  {
    rl_add_undo (UNDO_END, 0, 0);
    _rl_undo_group_level--;
  }

  void rl_modifying (size_t, size_t);

  /* Functions for redisplay. */
  void rl_redisplay ();
  int rl_on_new_line_with_prompt ();

  /* Move to the start of the next line. */
  inline void
  rl_crlf ()
  {
#if defined(NEW_TTY_DRIVER) || defined(__MINT__)
    if (_rl_term_cr)
      tputs (_rl_term_cr, 1, &_rl_output_character_function);
#endif /* NEW_TTY_DRIVER || __MINT__ */
    putc ('\n', _rl_out_stream);
  }

  /* Functions to manage the mark and region, especially the notion of an
     active mark and an active region. */

  inline void
  rl_keep_mark_active ()
  {
    _rl_keep_mark_active = true;
  }

  inline void
  rl_activate_mark ()
  {
    mark_active = true;
    rl_keep_mark_active ();
  }

  inline void
  rl_deactivate_mark ()
  {
    mark_active = false;
  }

  inline bool
  rl_mark_active_p ()
  {
    return mark_active;
  }

  int rl_message (const char *, ...)
      __attribute__ ((__format__ (printf, 2, 3)));

  int rl_show_char (int);

  /* Save all of the variables associated with the prompt and its display. Most
     of the complexity is dealing with the invisible characters in the prompt
     string and where they are. There are enough of these that I should
     consider a struct. */
  void rl_save_prompt ();

  void rl_restore_prompt ();

  /* Modifying text. */

  /* Replace the current line buffer contents with TEXT.  If CLEAR_UNDO is
     non-zero, we free the current undo list. */
  inline void
  rl_replace_line (string_view text, bool clear_undo)
  {
    rl_line_buffer = text;

    if (clear_undo)
      rl_free_undo_list ();

    _rl_fix_point (true);
  }

  size_t rl_insert_text (string_view);
  size_t rl_delete_text (size_t, size_t);
  void rl_kill_text (size_t, size_t);
  char *rl_copy_text (size_t, size_t);

  /* Terminal and tty mode management. */

  void rl_prep_terminal (int);
  void rl_deprep_terminal ();
  void rl_tty_set_default_bindings (Keymap);
  void rl_tty_unset_default_bindings (Keymap);

  int rl_tty_set_echoing (int);

  // Re-initialize the terminal considering that the TERM/TERMCAP variable has
  // changed.
  inline void
  rl_reset_terminal (const char *terminal_name)
  {
    _rl_screenwidth = 0;
    _rl_screenheight = 0;
    _rl_init_terminal_io (terminal_name);
  }

  inline void
  rl_set_screen_size (size_t rows, size_t cols)
  {
    _rl_set_screen_size (rows, cols);
  }

  inline void
  rl_get_screen_size (size_t *rows, size_t *cols)
  {
    if (rows)
      *rows = _rl_screenheight;
    if (cols)
      *cols = _rl_screenwidth;
  }

  inline void
  rl_reset_screen_size ()
  {
    _rl_get_screen_size (fileno (rl_instream), false);
  }

  inline void
  _rl_sigwinch_resize_terminal ()
  {
    _rl_get_screen_size (fileno (rl_instream), true);
  }

  void
  rl_resize_terminal ()
  {
    _rl_get_screen_size (fileno (rl_instream), true);
    if (_rl_echoing_p)
      {
        if (CUSTOM_REDISPLAY_FUNC ())
          rl_forced_update_display ();
        else if (RL_ISSTATE (RL_STATE_REDISPLAYING) == 0)
          _rl_redisplay_after_sigwinch ();
      }
  }

  const char *rl_get_termcap (const char *);

  /* Functions for character input. */

  bool rl_stuff_char (int);

  /* Make C be the next command to be executed. */
  inline void
  rl_execute_next (int c)
  {
    rl_pending_input = c;
    RL_SETSTATE (RL_STATE_INPUTPENDING);
  }

  /* Clear any pending input pushed with rl_execute_next() */
  inline void
  rl_clear_pending_input ()
  {
    rl_pending_input = 0;
    RL_UNSETSTATE (RL_STATE_INPUTPENDING);
  }

  int rl_read_key ();
  int rl_getc (FILE *);
  int rl_set_keyboard_input_timeout (int);

  /* `Public' utility functions . */

  int rl_ding ();
  int rl_alphabetic (int);
  void rl_free (void *);

  /* Readline signal handling, from signals.cc */

  int rl_set_signals ();
  void rl_clear_signals ();

  // Clean up the terminal and readline state after catching a signal, before
  // resending it to the calling application.
  inline void
  rl_cleanup_after_signal ()
  {
    _rl_clean_up_for_exit ();
    if (rl_deprep_term_function)
      ((*this).*rl_deprep_term_function) ();
    rl_clear_pending_input ();
    rl_clear_signals ();
  }

  /* Reset the terminal and readline state after a signal handler returns. */
  inline void
  rl_reset_after_signal ()
  {
    if (rl_prep_term_function)
      ((*this).*rl_prep_term_function) (_rl_meta_flag);
    rl_set_signals ();
  }

  /* Free up the readline variable line state for the current line (undo list,
     any partial history entry, any keyboard macros in progress, and any
     numeric arguments in process) after catching a signal, before calling
     rl_cleanup_after_signal(). */
  inline void
  rl_free_line_state ()
  {
    HIST_ENTRY *entry;

    rl_free_undo_list ();

    entry = current_history ();
    if (entry)
      entry->data = nullptr;

    _rl_kill_kbd_macro ();
    rl_clear_message ();
    _rl_reset_argument ();
  }

  inline int
  rl_pending_signal ()
  {
    return _rl_caught_signal;
  }

  inline void
  rl_check_signals ()
  {
    /* inlined from C macro in rlprivate.hh */
    if (_rl_caught_signal)
      _rl_signal_handler (_rl_caught_signal);
  }

  void rl_echo_signal_char (int);

  int rl_set_paren_blink_timeout (int);

  void _rl_signal_handler (int sig);

  void _rl_sigwinch_handler_internal (int sig);

  /* History management functions. */

  void rl_clear_history ();

  /* Completion functions. */

  int rl_complete_internal (int);
  void rl_display_match_list (std::vector<std::string *> &, size_t);

  std::vector<std::string *> *rl_completion_matches (string_view,
                                                     rl_compentry_func_t);

  std::string *rl_username_completion_function (string_view, int);
  std::string *rl_filename_completion_function (string_view, int);

  int rl_completion_mode (rl_command_func_t);

  /* bind.cc */

  int sv_bell_style (const char *);
  int sv_combegin (const char *);
  int sv_dispprefix (const char *);
  int sv_compquery (const char *);
  int sv_compwidth (const char *);
  int sv_editmode (const char *);
  int sv_emacs_modestr (const char *);
  int sv_histsize (const char *);
  int sv_isrchterm (const char *);
  int sv_keymap (const char *);
  int sv_seqtimeout (const char *);
  int sv_viins_modestr (const char *);
  int sv_vicmd_modestr (const char *);

  /* Input error; can be returned by (*rl_getc_function) if readline is reading
     a top-level command (RL_ISSTATE (RL_STATE_READCMD)). */
  static constexpr int READERR = (-2);

  /* Definitions available for use by readline clients. */
  static constexpr char RL_PROMPT_START_IGNORE = '\001';
  static constexpr char RL_PROMPT_END_IGNORE = '\002';

  /* Possible state value flags for rl_readline_state. */
  enum state_flags
  {
    RL_STATE_NONE = 0x0000000, // no state; before first call

    RL_STATE_INITIALIZING = 0x0000001, // initializing
    RL_STATE_INITIALIZED = 0x0000002,  // initialization done
    RL_STATE_TERMPREPPED = 0x0000004,  // terminal is prepped
    RL_STATE_READCMD = 0x0000008,      // reading a command key
    RL_STATE_METANEXT = 0x0000010,     // reading input after ESC
    RL_STATE_DISPATCHING = 0x0000020,  // dispatching to a command
    RL_STATE_MOREINPUT = 0x0000040, // reading more input in a command function
    RL_STATE_ISEARCH = 0x0000080,   // doing incremental search
    RL_STATE_NSEARCH = 0x0000100,   // doing non-inc search
    RL_STATE_SEARCH = 0x0000200,    // doing a history search
    RL_STATE_NUMERICARG = 0x0000400,   // reading numeric argument
    RL_STATE_MACROINPUT = 0x0000800,   // getting input from a macro
    RL_STATE_MACRODEF = 0x0001000,     // defining keyboard macro
    RL_STATE_OVERWRITE = 0x0002000,    // overwrite mode
    RL_STATE_COMPLETING = 0x0004000,   // doing completion
    RL_STATE_SIGHANDLER = 0x0008000,   // in readline sighandler
    RL_STATE_UNDOING = 0x0010000,      // doing an undo
    RL_STATE_INPUTPENDING = 0x0020000, // rl_execute_next called
    RL_STATE_TTYCSAVED = 0x0040000,    // tty special chars saved
    RL_STATE_CALLBACK = 0x0080000,     // using the callback interface
    RL_STATE_VIMOTION = 0x0100000,     // reading vi motion arg
    RL_STATE_MULTIKEY = 0x0200000,     // reading multiple-key command
    RL_STATE_VICMDONCE = 0x0400000,    // entered vi command mode at least once
    RL_STATE_CHARSEARCH = 0x0800000,   // vi mode char search
    RL_STATE_REDISPLAYING = 0x1000000, // updating terminal display

    RL_STATE_DONE = 0x2000000 // done; accepted line
  };

  void
  RL_SETSTATE (state_flags x)
  {
    rl_readline_state = static_cast<state_flags> (rl_readline_state | x);
  }

  void
  RL_UNSETSTATE (state_flags x)
  {
    rl_readline_state = static_cast<state_flags> (rl_readline_state & ~x);
  }

  bool
  RL_ISSTATE (state_flags x)
  {
    return rl_readline_state & x;
  }

  bool
  RL_ISSTATE (int x)
  {
    return rl_readline_state & x;
  }

  struct readline_state
  {
    // first, the pointer and long types

    /* line state */
    std::string buffer;
    std::string prompt;
    UNDO_LIST *ul;

    /* global state */
    Keymap kmap;

    /* input state */
    rl_command_func_t lastfunc;
    std::string kseq;

    FILE *inf;
    FILE *outf;
    char *macro;

    /* completion state */
    rl_compentry_func_t entryfunc;
    rl_compentry_func_t menuentryfunc;
    rl_compignore_func_t ignorefunc;
    rl_completion_func_t attemptfunc;
    char *wordbreakchars;

    // now, the 32-bit types (including enums)

    /* line state */
    size_t point;
    size_t mark;

    int pendingin;

    /* global state */
    state_flags rlstate;

    /* input state */
    editing_mode edmode;

    // last, bool and char values

    /* global state */
    bool done;

    /* input state */
    bool insmode;

    /* signal state */
    bool catchsigs;
    bool catchsigwinch;
  };

  int rl_save_state (readline_state *);
  int rl_restore_state (readline_state *);

  // bind.cc methods whose pointers are accessed from a global static array.

  // Push _rl_parsing_conditionalized_out, and set parser state based on ARGS.
  int parser_if (char *);

  int parser_else (char *);

  int parser_endif (char *);

  int parser_include (char *);

  // end of public API functions and members

private:
  struct _rl_search_cxt
  {
    _rl_search_cxt (rl_search_type type_, rl_search_flags sflags_,
                    size_t spoint_, size_t smark_, size_t save_line_,
                    Keymap keymap_)
        : keymap (keymap_), okeymap (keymap_), save_point (spoint_),
          save_mark (smark_), save_line (save_line_),
          last_found_line (save_line_), type (type_), sflags (sflags_)
    {
    }

    std::string search_string;
    std::string sline;

    std::vector<std::string> lines;

    std::string prev_line_found;

    UNDO_LIST *save_undo_list;

    Keymap keymap;  // used when dispatching commands in search string
    Keymap okeymap; // original keymap

    const char *search_terminators;

    // char array values

#if defined(HANDLE_MULTIBYTE)
    char mb[MB_LEN_MAX];
    char pmb[MB_LEN_MAX];
#endif

    // 32-bit or 64-bit size_t values

    size_t save_point;
    size_t save_mark;
    size_t save_line;
    ssize_t sline_index; // may be negative
    size_t last_found_line;
    size_t history_pos;

    // 32-bit int values

    int direction;

    int prevc;
    int lastc;

    // enum values

    rl_search_type type;
    rl_search_flags sflags;
  };

  struct _rl_cmd
  {
    // pointer types first

    Keymap map;
    rl_command_func_t func;

    // 32-bit types next

    int count;
    int key;
  };

  struct _rl_keyseq_cxt
  {
    // pointer types first

    Keymap dmap;
    Keymap oldmap;
    _rl_keyseq_cxt *ocxt;

    // 32-bit types next

    keyseq_flags flags;
    keyseq_flags subseq_retval; /* XXX */
    int subseq_arg;
    int okey;
    int childval;
  };

  typedef num_arg_flags _rl_arg_cxt;

  class _rl_vimotion_cxt
  {
  public:
    _rl_vimotion_cxt (vim_flags op_, size_t start_, size_t end_, int key_)
        : op (op_), numeric_arg (-1), start (start_), end (end_), key (key_)
    {
    }

    void
    init (vim_flags op_, size_t start_, size_t end_, int key_)
    {
      op = op_;
      state = 0;
      ncxt = NUM_NOFLAGS;
      numeric_arg = -1;
      start = start_;
      end = end_;
      key = key_;
      motion = -1;
    }

    vim_flags op;
    int state;
    _rl_arg_cxt ncxt;
    int numeric_arg;
    size_t start; /* rl_point */
    size_t end;   /* rl_end */
    int key;      /* initial key */
    int motion;   /* motion command */
  };

  /*************************************************************************
   *									 *
   * Unsorted local functions and variables unused and undocumented	 *
   *									 *
   *************************************************************************/

  /* Undocumented in the texinfo manual; not really useful to programs. */

  int rl_translate_keyseq (string_view, std::string &);
  std::string rl_untranslate_keyseq (int);

  /* Undocumented; used internally only. */

  inline void
  rl_set_keymap_from_edit_mode ()
  {
    if (rl_editing_mode == emacs_mode)
      _rl_keymap = emacs_standard_keymap ();
#if defined(VI_MODE)
    else if (rl_editing_mode == vi_mode)
      _rl_keymap = vi_insertion_keymap ();
#endif /* VI_MODE */
  }

  const char *
  rl_get_keymap_name_from_edit_mode ()
  {
    if (rl_editing_mode == emacs_mode)
      return "emacs";
#if defined(VI_MODE)
    else if (rl_editing_mode == vi_mode)
      return "vi";
#endif /* VI_MODE */
    else
      return "none";
  }

  /* Undocumented in texinfo manual. */
  inline size_t
  rl_character_len (int c, size_t pos)
  {
    unsigned char uc = static_cast<unsigned char> (c);

    if (META_CHAR (uc))
      return (_rl_output_meta_chars == 0) ? 4 : 1;

    if (uc == '\t')
      {
#if defined(DISPLAY_TABS)
        return ((pos | 7) + 1) - pos;
#else
        return 2;
#endif /* !DISPLAY_TABS */
      }

    if (CTRL_CHAR (c) || c == RUBOUT)
      return 2;

    return (c_isprint (uc)) ? 1 : 2;
  }

  /* parse-colors.cc */

  void _rl_parse_colors ();

  /* terminal.cc */

  void _rl_set_screen_size (size_t, size_t);

  /* tilde.cc */

  char *tilde_expand_word (const char *filename);

  /* util.cc */

  char *_rl_savestring (const char *);

  /***********************************************************************
   *									 *
   * Functions and variables private to the readline library		 *
   *									 *
   ***********************************************************************/

  // defined in bind.cc

  int _rl_skip_to_delim (char *, int, int);

  void _rl_init_file_error (const char *, ...)
      __attribute__ ((__format__ (printf, 2, 3)));

  rl_command_func_t _rl_function_of_keyseq_internal (string_view,
                                                     Keymap,
                                                     keymap_entry_type *);

  char *_rl_read_file (char *, size_t *);
  int _rl_read_init_file (const char *, int);

  const char *_rl_get_string_variable_value (const char *);

  /* Return true if any members of ARRAY are a substring in STRING. */
  inline bool
  substring_member_of_array (const char *string, const char *const *array)
  {
    while (*array)
      {
        if (_rl_strindex (string, *array))
          return true;
        array++;
      }
    return false;
  }

  void _rl_macro_dumper_internal (bool, Keymap, const char *);

  // moved from rlprivate.hh macros

  bool
  CUSTOM_REDISPLAY_FUNC ()
  {
    return rl_redisplay_function != &Readline::rl_redisplay;
  }

  bool
  CUSTOM_INPUT_FUNC ()
  {
    return rl_getc_function != &Readline::rl_getc;
  }

  /* NOTE: Functions and variables prefixed with `_rl_' are
     pseudo-global: they are global so they can be shared
     between files in the readline library, but are not intended
     to be visible to readline callers. */

  /***********************************************************************
   * Undocumented private functions					 *
   ***********************************************************************/

  /* Save the current line in _rl_saved_line_for_history. */
  inline void
  rl_maybe_save_line ()
  {
    if (!_rl_saved_line_for_history)
      {
        _rl_saved_line_for_history = new HIST_ENTRY (rl_line_buffer);
        _rl_saved_line_for_history->data = rl_undo_list;
      }
  }

  /* Restore the _rl_saved_line_for_history if there is one. */
  void rl_maybe_unsave_line ();

  /* Perhaps put back the current line if it has changed. */
  inline void
  rl_maybe_replace_line ()
  {
    HIST_ENTRY *temp = current_history ();
    /* If the current line has changed, save the changes. */
    if (temp && (reinterpret_cast<UNDO_LIST *> (temp->data) != rl_undo_list))
      {
        temp = replace_history_entry (where_history (), rl_line_buffer,
                                      rl_undo_list);
        delete temp;
      }
  }

#if defined(READLINE_CALLBACKS)

  /* readline.cc */

  void readline_internal_setup ();
  std::string readline_internal_teardown (bool);
  int readline_internal_char ();

  int _rl_dispatch_callback (_rl_keyseq_cxt *);

#endif /* READLINE_CALLBACKS */

  /* bind.cc */

  std::string _rl_untranslate_macro_value (string_view, bool);

  int handle_parser_directive (char *);

  void init_boolean_varlist ();

  void init_keymap_names ();

  int find_boolean_var (const char *);

  inline const char *
  boolean_varname (int i)
  {
    // we can assume the list has been set up if we're requesting an index.
    return (i >= 0) ? boolean_varlist_[static_cast<size_t> (i)].name : nullptr;
  }

  void hack_special_boolean_var (int i);

  /* colors.cc */

  /* Print prefix color. Returns 0 on success and 1 on error. */
  int _rl_print_prefix_color ();

  bool _rl_print_color_indicator (const char *);
  void _rl_prep_non_filename_text ();

#if defined(COLOR_SUPPORT)

  void init_rl_color_indicators ();

  inline int
  colored_stat_start (const char *filename)
  {
    _rl_set_normal_color ();
    return _rl_print_color_indicator (filename);
  }

  inline void
  colored_stat_end ()
  {
    _rl_prep_non_filename_text ();
    _rl_put_indicator (_rl_color_indicator[C_CLR_TO_EOL]);
  }

  inline int
  colored_prefix_start ()
  {
    _rl_set_normal_color ();
    return _rl_print_prefix_color ();
  }

  inline void
  colored_prefix_end ()
  {
    colored_stat_end (); /* for now */
  }

  /* Output a color indicator (which may contain nulls).  */
  inline void
  _rl_put_indicator (string_view ind)
  {
    fwrite (ind.data (), ind.size (), 1, rl_outstream);
  }

  inline bool
  is_colored (enum indicator_no colored_filetype)
  {
    size_t len = _rl_color_indicator[colored_filetype].size ();
    char const *s = _rl_color_indicator[colored_filetype].data ();
    return !(len == 0 || (len == 1 && strncmp (s, "0", 1) == 0)
             || (len == 2 && strncmp (s, "00", 2) == 0));
  }

  inline void
  restore_default_color ()
  {
    _rl_put_indicator (_rl_color_indicator[C_LEFT]);
    _rl_put_indicator (_rl_color_indicator[C_RIGHT]);
  }

  inline void
  _rl_set_normal_color ()
  {
    if (is_colored (C_NORM))
      {
        _rl_put_indicator (_rl_color_indicator[C_LEFT]);
        _rl_put_indicator (_rl_color_indicator[C_NORM]);
        _rl_put_indicator (_rl_color_indicator[C_RIGHT]);
      }
  }

#endif /* COLOR_SUPPORT */

  /* complete.cc */

  static void
  _rl_free_match_list (std::vector<std::string *> *matches)
  {
    if (!matches)
      return;

    std::vector<std::string *>::iterator it;
    for (it = matches->begin (); it != matches->end (); ++it)
      delete *it;

    delete matches;
  }

  /* Reset public readline state on a signal or other event. */
  inline void
  _rl_reset_completion_state ()
  {
    rl_completion_found_quote = false;
    rl_completion_quote_character = '\0';
  }

  unsigned char _rl_find_completion_word (rl_qf_flags *, unsigned char *);

  inline void
  _rl_complete_sigcleanup (int sig, void *ptr)
  {
    if (sig == SIGINT) /* XXX - for now */
      {
        _rl_free_match_list (
            reinterpret_cast<std::vector<std::string *> *> (ptr));
        _rl_complete_display_matches_interrupt = true;
      }
  }

  void set_completion_defaults (int);
  int get_y_or_n (bool); // returns 0, 1, or 2.
  int _rl_internal_pager (int);

#if defined(VISIBLE_STATS)
  int stat_char (const char *);
#endif

  string_view printable_part (string_view);
  size_t fnwidth (string_view string);
  size_t fnprint (string_view, size_t, size_t);
  size_t print_filename (string_view, size_t, size_t);
  std::vector<std::string *> *gen_completion_matches (string_view,
                                                      size_t, size_t,
                                                      rl_compentry_func_t,
                                                      rl_qf_flags, int);
  void remove_duplicate_matches (std::vector<std::string *> &);
  void compute_lcd_of_matches (std::vector<std::string *> &,
                               string_view);
  int postprocess_matches (std::vector<std::string *> &, bool);
  size_t complete_get_screenwidth ();
  void display_matches (const std::vector<std::string *> &);
  std::string make_quoted_replacement (string_view, replace_type,
                                       char *);
  void insert_match (string_view, size_t, replace_type, char *);
  int append_to_match (std::string &, unsigned char, unsigned char, bool);
  void insert_all_matches (const std::vector<std::string *> &, size_t, char *);
  int compare_match (string_view, string_view);
  bool complete_fncmp (string_view, string_view);

  std::string rl_quote_filename (string_view, replace_type, char *);

  /* display.cc */

  /* State of visible and invisible lines. */
  struct line_state
  {
    std::string line;
    std::string lface;
    std::vector<size_t> lbreaks;
#if defined(HANDLE_MULTIBYTE)
    size_t *wrapped_line;
    size_t wbsize;
#endif
  };

  /* Just strip out RL_PROMPT_START_IGNORE and RL_PROMPT_END_IGNORE from
     PMT and return the rest of PMT. */
  inline std::string
  _rl_strip_prompt (string_view pmt)
  {
    return expand_prompt (pmt, PMT_NONE, nullptr, nullptr, nullptr, nullptr);
  }

  inline void
  _rl_reset_prompt ()
  {
    rl_visible_prompt_length
        = rl_expand_prompt (const_cast<char *> (rl_prompt.c_str ()));
  }

  void _rl_move_vert (size_t);
  void _rl_save_prompt ();
  void _rl_restore_prompt ();
  std::string _rl_make_prompt_for_search (char);
  void _rl_update_final ();
  void _rl_redisplay_after_sigwinch ();
  void open_some_spaces (size_t);
  void redraw_prompt (string_view);

  const char *prompt_modestr (size_t *);

  enum expand_prompt_flags
  {
    PMT_NONE = 0,
    PMT_MULTILINE = 0x01
  };

  /* Expand prompt and return indices into it. Returns an RAII C++ string. */
  std::string expand_prompt (string_view, expand_prompt_flags,
                             size_t *, size_t *, size_t *, size_t *);

  void update_line (char *old, char *old_face, const char *new_,
                    const char *new_face, size_t current_line, size_t omax,
                    size_t nmax, size_t inv_botlin);

  /* Do whatever tests are necessary and tell update_line that it can do a
     quick, dumb redisplay on the assumption that there are so many
     differences between the old and new lines that it would be a waste to
     compute all the differences.
     Right now, it just sets _rl_quick_redisplay if the current visible line
     is a single line (so we don't have to move vertically or mess with line
     wrapping). */
  inline void
  _rl_optimize_redisplay ()
  {
    if (_rl_vis_botlin == 0)
      _rl_quick_redisplay = true;
  }

  /* Convenience functions to add chars to the invisible line that update the
     face information at the same time. */
  inline void
  invis_addc (char c, char face)
  {
    line_state_invisible->line.push_back (c);
    line_state_invisible->lface.push_back (face);
  }

  inline void
  invis_adds (const char *str, size_t n, char face)
  {
    line_state_invisible->line.append (str, n);
    line_state_invisible->lface.append (n, face);
  }

  inline void
  invis_adds (string_view str, char face)
  {
    line_state_invisible->line.append (str);
    line_state_invisible->lface.append (str.size (), face);
  }

  inline void
  set_active_region (size_t *beg, size_t *end)
  {
    if (rl_point <= rl_end () && rl_mark <= rl_end ())
      {
        *beg = (rl_mark < rl_point) ? rl_mark : rl_point;
        *end = (rl_mark < rl_point) ? rl_point : rl_mark;
      }
  }

  /* Tell the update routines that we have moved onto a new (empty) line. */
  inline void
  rl_on_new_line ()
  {
    line_state_visible->line.clear ();

    _rl_last_c_pos = _rl_last_v_pos = 0;
    _rl_vis_botlin = last_lmargin = 0;

    if (line_state_visible->lbreaks.size () >= 2)
      line_state_visible->lbreaks[0] = line_state_visible->lbreaks[1] = 0;

    visible_wrap_offset = 0;
  }

  /* Clear all screen lines occupied by the current readline line buffer
     (visible line) */
  inline void
  rl_clear_visible_line ()
  {
    /* Make sure we move to column 0 so we clear the entire line */
    _rl_cr ();
    _rl_last_c_pos = 0;

    /* Move to the last screen line of the current visible line */
    _rl_move_vert (_rl_vis_botlin);

    /* And erase screen lines going up to line 0 (first visible line) */
    for (ssize_t curr_line = static_cast<ssize_t> (_rl_last_v_pos);
         curr_line >= 0; curr_line--)
      {
        _rl_move_vert (static_cast<size_t> (curr_line));
        _rl_clear_to_eol (0);
      }
  }

  /* Actually update the display, period. */
  inline void
  rl_forced_update_display ()
  {
    line_state_visible->line.clear ();
    rl_on_new_line ();
    forced_display = true;
    ((*this).*rl_redisplay_function) ();
  }

  /* Redraw only the last line of a multi-line prompt. */
  inline void
  rl_redraw_prompt_last_line ()
  {
    size_t pos = rl_display_prompt.rfind ('\n');

    if (pos != std::string::npos)
      redraw_prompt (rl_display_prompt.substr (pos + 1));
    else
      rl_forced_update_display ();
  }

  /* How to clear things from the "echo-area". */
  inline void
  rl_clear_message ()
  {
    rl_display_prompt = rl_prompt;
    if (msg_saved_prompt)
      {
        rl_restore_prompt ();
        msg_saved_prompt = 0;
      }
    ((*this).*rl_redisplay_function) ();
  }

  inline void
  rl_reset_line_state ()
  {
    rl_on_new_line ();

    rl_display_prompt = rl_prompt;
    forced_display = true;
  }

  /* Quick redisplay hack when erasing characters at the end of the line. */
  inline void
  _rl_erase_at_end_of_line (size_t l)
  {
    _rl_backspace (l);

    for (size_t i = 0; i < l; i++)
      putc (' ', rl_outstream);

    _rl_backspace (l);

    line_state_visible->line.erase (line_state_visible->line.size () - l);

    rl_display_fixed = true;
  }

  // Clear to the end of the line. COUNT is the minimum number of character
  // spaces to clear, but we use a terminal escape sequence if available.
  inline void
  _rl_clear_to_eol (size_t count)
  {
    if (_rl_term_clreol)
      tputs (_rl_term_clreol, 1, _rl_output_character_function);
    else if (count)
      space_to_eol (count);
  }

  // Clear to the end of the line using spaces. COUNT is the minimum number of
  // character spaces to clear.
  inline void
  space_to_eol (size_t count)
  {
    for (size_t i = 0; i < count; i++)
      putc (' ', rl_outstream);

    _rl_last_c_pos += count;
  }

  inline void
  _rl_clear_screen (bool clrscr)
  {
    if (_rl_term_clrpag)
      {
        tputs (_rl_term_clrpag, 1, _rl_output_character_function);
        if (clrscr && _rl_term_clrscroll)
          tputs (_rl_term_clrscroll, 1, _rl_output_character_function);
      }
    else
      rl_crlf ();
  }

  /* Insert COUNT characters from STRING to the output stream at column COL. */
  inline void
  insert_some_chars (std::string &string, size_t count, size_t col)
  {
    open_some_spaces (col);
    _rl_output_some_chars (string.data (), count);
  }

  /* Move to the start of the current line. */
  inline void
  cr ()
  {
    _rl_cr ();
    _rl_last_c_pos = 0;
  }

  inline void
  _rl_clean_up_for_exit ()
  {
    if (_rl_echoing_p)
      {
        if (_rl_vis_botlin > 0) /* minor optimization plus bug fix */
          _rl_move_vert (_rl_vis_botlin);
        _rl_vis_botlin = 0;
        fflush (rl_outstream);
        rl_restart_output (1, 0);
      }
  }

  inline void
  _rl_erase_entire_line ()
  {
    cr ();
    _rl_clear_to_eol (0);
    cr ();
    fflush (rl_outstream);
  }

  inline void
  _rl_ttyflush ()
  {
    fflush (rl_outstream);
  }

  /* return the `current display line' of the cursor -- the number of lines to
     move up to get to the first screen line of the current readline line. */
  inline size_t
  _rl_current_display_line ()
  {
    size_t ret, nleft;

    /* Find out whether or not there might be invisible characters in the
       editing buffer. */
    if (rl_display_prompt == rl_prompt)
      nleft = _rl_last_c_pos - _rl_screenwidth - rl_visible_prompt_length;
    else
      nleft = _rl_last_c_pos - _rl_screenwidth;

    if (nleft > 0)
      ret = 1 + nleft / _rl_screenwidth;
    else
      ret = 0;

    return ret;
  }

  inline void
  _rl_refresh_line ()
  {
    rl_clear_visible_line ();
    rl_redraw_prompt_last_line ();
    rl_keep_mark_active ();
  }

  void delete_chars (int);

  void _rl_move_cursor_relative (size_t, const char *, const char *);

  void putc_face (int, char, char *);

  inline void
  puts_face (string_view str, string_view face, size_t n)
  {
    size_t i;
    char cur_face;

    for (cur_face = FACE_NORMAL, i = 0; i < n; i++)
      putc_face (static_cast<unsigned char> (str[i]), face[i], &cur_face);

    putc_face (EOF, FACE_NORMAL, &cur_face);
  }

  /* input.cc */

  int _rl_input_available ();
  int _rl_nchars_available ();

  inline int
  _rl_input_queued (int t)
  {
    int old_timeout = rl_set_keyboard_input_timeout (t);
    int r = _rl_input_available ();
    rl_set_keyboard_input_timeout (old_timeout);
    return r;
  }

  void _rl_insert_typein (int);
  int rl_gather_tyi ();

  inline bool
  _rl_pushed_input_available ()
  {
    return _rl_push_index != _rl_pop_index;
  }

  /* Return the amount of space available in the buffer for stuffing
     characters. We lose one char because "pop_index == push_index"
     could mean either empty or full otherwise. */
  inline size_t
  ibuffer_space ()
  {
    if (_rl_pop_index > _rl_push_index)
      return _rl_pop_index - _rl_push_index - 1;
    else
      return static_cast<size_t> (sizeof (_rl_ibuffer)) - 1
             - (_rl_push_index - _rl_pop_index);
  }

  /* Get a key from the buffer of characters to be read.
     Return the key in KEY. Result is true if there was a key,
     or false if there wasn't. */
  inline bool
  rl_get_char (int *key)
  {
    if (_rl_push_index == _rl_pop_index)
      return false;

    *key = static_cast<int> (_rl_ibuffer[_rl_pop_index++]);

    if (_rl_pop_index >= static_cast<int> (sizeof (_rl_ibuffer)))
      _rl_pop_index = 0;

    return true;
  }

  /* Stuff KEY into the *front* of the input buffer.
     Returns true if successful, false if there is
     no space left in the buffer. */
  inline bool
  _rl_unget_char (int key)
  {
    if (ibuffer_space ())
      {
        if (_rl_pop_index == 0)
          _rl_pop_index = sizeof (_rl_ibuffer) - 1;
        else
          _rl_pop_index--;

        _rl_ibuffer[_rl_pop_index] = static_cast<unsigned char> (key);

        return true;
      }
    return false;
  }

#if defined(HANDLE_MULTIBYTE)
  int _rl_read_mbchar (char *, int);
  int _rl_read_mbstring (int, char *, int);
#endif /* HANDLE_MULTIBYTE */

  /* isearch.cc */

  int _rl_isearch_dispatch (_rl_search_cxt *, int);
  int _rl_isearch_callback (_rl_search_cxt *);
  int _rl_isearch_cleanup (_rl_search_cxt *, int);

  int _rl_search_getchar (_rl_search_cxt *);

  _rl_search_cxt *_rl_isearch_init (int);
  void _rl_isearch_fini (_rl_search_cxt *);

  void rl_display_search (string_view, rl_search_flags);

  /* kill.cc */

  int _rl_read_bracketed_paste_prefix (int);
  std::string _rl_bracketed_text ();
  int _rl_bracketed_read_key ();
  int _rl_bracketed_read_mbstring (std::string &, int);
  void _rl_copy_to_kill_ring (string_view, bool);
  void region_kill_internal (bool);
  int rl_yank_nth_arg_internal (int, int, int);

  /* macro.cc */

  void _rl_with_macro_input (char *);
  int _rl_peek_macro_key ();
  int _rl_next_macro_key ();
  int _rl_prev_macro_key ();
  void _rl_push_executing_macro ();
  void _rl_pop_executing_macro ();
  void _rl_kill_kbd_macro ();

  /* Add a character to the macro being built. */
  inline void
  _rl_add_macro_char (char c)
  {
    rl_current_macro.push_back (c);
  }

  /* misc.cc */

  int _rl_arg_overflow ();

  inline void
  _rl_arg_init ()
  {
    rl_save_prompt ();
    _rl_argcxt = NUM_NOFLAGS;
    RL_SETSTATE (RL_STATE_NUMERICARG);
  }

  inline int
  _rl_arg_getchar ()
  {
    rl_message ("(arg: %d) ", rl_arg_sign * rl_numeric_arg);
    RL_SETSTATE (RL_STATE_MOREINPUT);
    int c = rl_read_key ();
    RL_UNSETSTATE (RL_STATE_MOREINPUT);

    return c;
  }

  int _rl_arg_callback (_rl_arg_cxt);

  /* Create a default argument. */
  inline void
  _rl_reset_argument ()
  {
    rl_numeric_arg = rl_arg_sign = 1;
    rl_explicit_arg = false;
    _rl_argcxt = NUM_NOFLAGS;
  }

  /* Set the history pointer back to the last entry in the history. */
  inline void
  _rl_start_using_history ()
  {
    using_history ();
    _rl_free_saved_history_line ();
  }

  inline void
  _rl_free_saved_history_line ()
  {
    if (_rl_saved_line_for_history)
      {
        delete _rl_saved_line_for_history;
        _rl_saved_line_for_history = nullptr;
      }
  }

  /* Set insert/overwrite mode. */
  inline void
#ifdef CURSOR_MODE
  _rl_set_insert_mode (bool im, bool force)
#else
  _rl_set_insert_mode (bool im, bool)
#endif
  {
#ifdef CURSOR_MODE
    _rl_set_cursor (im, force);
#endif
    rl_insert_mode = im;
  }

  void _rl_revert_previous_lines ();

  /* Revert all lines in the history by making sure we are at the end of the
     history before calling _rl_revert_previous_lines() */
  inline void
  _rl_revert_all_lines ()
  {
    size_t ind = where_history ();
    using_history ();
    _rl_revert_previous_lines ();
    history_set_pos (ind);
  }

  int _rl_arg_dispatch (_rl_arg_cxt, int);

  void _rl_history_set_point ();

  int rl_digit_loop ();

  inline void
  rl_replace_from_history (HIST_ENTRY *entry)
  {
    /* Can't call with `1' because rl_undo_list might point to an undo list
       from a history entry, just like we're setting up here. */
    rl_replace_line (entry->line, 0);
    rl_undo_list = reinterpret_cast<UNDO_LIST *> (entry->data);
#if defined(VI_MODE)
    if (rl_editing_mode == vi_mode)
      {
        rl_point = 0;
        rl_mark = rl_end ();
      }
    else
#endif
      {
        rl_point = rl_end ();
        rl_mark = 0;
      }
  }

  int set_saved_history ();

  /* nls.cc */

  char *_rl_init_locale ();
  void _rl_init_eightbit ();

  inline const char *
  _rl_get_locale_var (const char *v)
  {
    const char *lspec = sh_get_env_value ("LC_ALL");
    if (lspec == nullptr || *lspec == 0)
      lspec = sh_get_env_value (v);
    if (lspec == nullptr || *lspec == 0)
      lspec = sh_get_env_value ("LANG");

    return lspec;
  }

  /* parens.cc */

  void _rl_enable_paren_matching (bool);

  /* readline.cc */

  int _rl_dispatch (int, Keymap);
  int _rl_dispatch_subseq (int, Keymap, int);
  void _rl_internal_char_cleanup ();

  /* Functions to manage the string that is the current key sequence. */

  void
  _rl_init_executing_keyseq ()
  {
    rl_executing_keyseq.clear ();
  }

  void
  _rl_add_executing_keyseq (int key)
  {
    rl_executing_keyseq += static_cast<char> (key);
  }

#if defined(READLINE_CALLBACKS)
  bool
  readline_internal_charloop ()
  {
    bool eof = true;

    while (!rl_done)
      eof = readline_internal_char ();
    return eof;
  }
#endif /* READLINE_CALLBACKS */

  /* Read a line of input from the global rl_instream, doing output on
     the global rl_outstream.
     If rl_prompt is non-null, then that is our prompt. */
  std::string
  readline_internal ()
  {
    readline_internal_setup ();
    _rl_eof_found = readline_internal_charloop ();
    return readline_internal_teardown (_rl_eof_found);
  }

  void
  _rl_init_line_state ()
  {
    rl_point = rl_mark = 0;
    rl_line_buffer.clear ();
  }

#if defined(READLINE_CALLBACKS)
  _rl_keyseq_cxt *
  _rl_keyseq_cxt_alloc ()
  {
    _rl_keyseq_cxt *cxt;

    cxt = new _rl_keyseq_cxt ();

    cxt->flags = cxt->subseq_retval = KSEQ_NOFLAGS;
    cxt->subseq_arg = 0;

    cxt->okey = 0;
    cxt->ocxt = _rl_kscxt;
    cxt->childval = 42; /* sentinel value */

    return cxt;
  }

  void
  _rl_keyseq_chain_dispose ()
  {
    _rl_keyseq_cxt *cxt;

    while (_rl_kscxt)
      {
        cxt = _rl_kscxt;
        _rl_kscxt = _rl_kscxt->ocxt;
        delete cxt;
      }
  }
#endif

  int
  _rl_subseq_getchar (int key)
  {
    int k;

    if (key == ESC)
      RL_SETSTATE (RL_STATE_METANEXT);
    RL_SETSTATE (RL_STATE_MOREINPUT);
    k = rl_read_key ();
    RL_UNSETSTATE (RL_STATE_MOREINPUT);
    if (key == ESC)
      RL_UNSETSTATE (RL_STATE_METANEXT);

    return k;
  }

  int _rl_subseq_result (int r, Keymap map, int key, int got_subseq);

  void readline_initialize_everything ();

  void readline_default_bindings ();
  void reset_default_bindings ();

  void bind_arrow_keys_internal (Keymap map);
  void bind_arrow_keys ();

  void bind_bracketed_paste_prefix ();

  /* rltty.cc */

  int _rl_disable_tty_signals ();
  int _rl_restore_tty_signals ();
  void save_tty_chars (TIOTYPE *tiop);
  int _get_tty_settings (int tty, TIOTYPE *tiop);
  int get_tty_settings (int tty, TIOTYPE *tiop);
  void prepare_terminal_settings (int meta_flag, TIOTYPE oldtio,
                                  TIOTYPE *tiop);
  void _rl_bind_tty_special_chars (Keymap kmap, TIOTYPE ttybuff);

#if defined(_AIX)
  /* Currently this is only used on AIX */
  inline void
  rltty_warning (string_viewmsg)
  {
    _rl_errmsg ("warning: %s", msg);
  }

  inline void
  setopost (TIOTYPE *tp)
  {
    if ((tp->c_oflag & OPOST) == 0)
      {
        _rl_errmsg ("warning: turning on OPOST for terminal\r");
        tp->c_oflag |= (OPOST | ONLCR);
      }
  }
#endif

  /* search.cc */

  int _rl_nsearch_callback (_rl_search_cxt *);
  int _rl_nsearch_cleanup (_rl_search_cxt *, int);
  void make_history_line_current (HIST_ENTRY *);
  size_t noninc_search_from_pos (string_view, size_t, int,
                                 rl_search_flags, size_t *);
  bool noninc_dosearch (string_view, int, rl_search_flags);
  _rl_search_cxt *_rl_nsearch_init (int, char);
  void _rl_nsearch_abort (_rl_search_cxt *);
  int _rl_nsearch_dispatch (_rl_search_cxt *, int);
  int _rl_nsearch_dosearch (_rl_search_cxt *);
  int noninc_search (int, char);
  int rl_history_search_internal (int, int);
  void rl_history_search_reinit (hist_search_flags flags);

  /* signals.cc */

#if defined(HANDLE_SIGNALS)
  // XXX: do any of these need to be defined for !HANDLE_SIGNALS?

  /* Cause SIGINT to not be delivered until the corresponding call to
     release_sigint(). */
  inline void
  _rl_block_sigint ()
  {
    if (sigint_blocked)
      return;

    sigint_blocked = 1;
  }

  /* Allow SIGINT to be delivered. */
  inline void
  _rl_release_sigint ()
  {
    if (sigint_blocked == 0)
      return;

    sigint_blocked = 0;
    rl_check_signals ();
  }

  void _rl_block_sigwinch ();
  void _rl_release_sigwinch ();

  SigHandler rl_set_sighandler (int, SigHandler, sighandler_cxt *);

#endif /* HANDLE_SIGNALS */

  /* terminal.cc */

  /* Get readline's idea of the screen size.  TTY is a file descriptor open
     to the terminal.  If IGNORE_ENV is true, we do not pay attention to the
     values of $LINES and $COLUMNS.  The tests for TERM_STRING_BUFFER being
     non-null serve to check whether or not we have initialized termcap. */
  void _rl_get_screen_size (int, bool);

  void _rl_init_terminal_io (string_view);

  void _tc_strings_init ();

  /* ************************************************************** */
  /*		Entering and leaving terminal standout mode	    */
  /* ************************************************************** */

  inline void
  _rl_standout_on ()
  {
    if (_rl_term_so && _rl_term_se)
      tputs (_rl_term_so, 1, &_rl_output_character_function);
  }

  inline void
  _rl_standout_off ()
  {
    if (_rl_term_so && _rl_term_se)
      tputs (_rl_term_se, 1, &_rl_output_character_function);
  }

  /* ************************************************************** */
  /*	 	Controlling the Meta Key and Keypad		    */
  /* ************************************************************** */

  inline void
  _rl_enable_meta_key ()
  {
    if (term_has_meta && _rl_term_mm)
      {
        tputs (_rl_term_mm, 1, &_rl_output_character_function);
        _rl_enabled_meta = true;
      }
  }

  inline void
  _rl_disable_meta_key ()
  {
    if (term_has_meta && _rl_term_mo && _rl_enabled_meta)
      {
        tputs (_rl_term_mo, 1, &_rl_output_character_function);
        _rl_enabled_meta = false;
      }
  }

  inline void
  _rl_control_keypad (bool on)
  {
    if (on && _rl_term_ks)
      tputs (_rl_term_ks, 1, &_rl_output_character_function);
    else if (!on && _rl_term_ke)
      tputs (_rl_term_ke, 1, &_rl_output_character_function);
  }

  /* ************************************************************** */
  /*	 		Controlling the Cursor			    */
  /* ************************************************************** */

  /* Set the cursor appropriately depending on IM, which is one of the
     insert modes (insert or overwrite).  Insert mode gets the normal
     cursor.  Overwrite mode gets a very visible cursor.  Only does
     anything if we have both capabilities. */
  inline void
  _rl_set_cursor (bool im, bool force)
  {
    if (_rl_term_ve && _rl_term_vs)
      {
        if (force || im != rl_insert_mode)
          {
            if (im == RL_IM_OVERWRITE)
              tputs (_rl_term_vs, 1, &_rl_output_character_function);
            else
              tputs (_rl_term_ve, 1, &_rl_output_character_function);
          }
      }
  }

  void bind_termcap_arrow_keys (Keymap);

  void get_term_capabilities (char **bp);

#if defined(__EMX__)
  void
  _emx_get_screensize (int *swp, int *shp)
  {
    int sz[2];

    _scrsize (sz);

    if (swp)
      *swp = sz[0];
    if (shp)
      *shp = sz[1];
  }
#endif

#if defined(__MINGW32__)
  void
  _win_get_screensize (int *swp, int *shp)
  {
    HANDLE hConOut;
    CONSOLE_SCREEN_BUFFER_INFO scr;

    hConOut = GetStdHandle (STD_OUTPUT_HANDLE);
    if (hConOut != INVALID_HANDLE_VALUE)
      {
        if (GetConsoleScreenBufferInfo (hConOut, &scr))
          {
            *swp = scr.dwSize.X;
            *shp = scr.srWindow.Bottom - scr.srWindow.Top + 1;
          }
      }
  }
#endif

  /* Write COUNT characters from STRING to the output stream. */
  inline void
  _rl_output_some_chars (const char *string, size_t count)
  {
    fwrite (string, 1, count, _rl_out_stream);
  }

  /* Move the cursor back. */
  inline void
  _rl_backspace (size_t count)
  {
    if (_rl_term_backspace)
      for (size_t i = 0; i < count; i++)
        tputs (_rl_term_backspace, 1, &_rl_output_character_function);
    else
      for (size_t i = 0; i < count; i++)
        putc ('\b', _rl_out_stream);
  }

  inline void
  _rl_cr ()
  {
    tputs (_rl_term_cr, 1, &_rl_output_character_function);
  }

#if defined(HANDLE_MULTIBYTE)
  size_t _rl_col_width (const char *str, size_t start, size_t end, int flags);
#else
  inline size_t
  _rl_col_width (string_view, size_t start, size_t end, int)
  {
    return (end <= start) ? 0 : (end - start);
  }
#endif

  /* text.cc */

  /* Fix up point so that it is within the line boundaries after killing
     text.  If FIX_MARK_TOO is true, the mark is forced within line
     boundaries also. */
  inline void
  _rl_fix_point (bool fix_mark_too)
  {
    if (rl_point > rl_end ())
      rl_point = rl_end ();

    if (fix_mark_too && rl_mark > rl_end ())
      rl_mark = rl_end ();
  }

  inline void
  _rl_fix_mark ()
  {
    if (rl_mark > rl_end ())
      rl_mark = rl_end ();
  }

  size_t _rl_replace_text (string_view, size_t, size_t);
  size_t _rl_forward_char_internal (int);
  size_t _rl_backward_char_internal (int);

  int _rl_insert_char (int, int);
  int _rl_overwrite_char (int, int);
  int _rl_overwrite_rubout (int, int);
  int _rl_rubout_char (int, int);

#if defined(HANDLE_MULTIBYTE)
  int _rl_char_search_internal (int, int, const char *, size_t);
#else
  int _rl_char_search_internal (int, int, int);
#endif

  /* Set the mark at POSITION. */
  inline int
  _rl_set_mark_at_pos (size_t position)
  {
    if (position > rl_line_buffer.size ())
      return 1;

    rl_mark = position;
    return 0;
  }

  /* The three kinds of things that we know how to do. */
  enum case_change_type
  {
    UpCase = 1,
    DownCase = 2,
    CapCase = 3
  };

  int rl_change_case (int, case_change_type);
  int _rl_char_search (int, int, int);

  int _rl_insert_next (int);

#if defined(READLINE_CALLBACKS)
  int _rl_insert_next_callback (_rl_callback_generic_arg *);
  int _rl_char_search_callback (_rl_callback_generic_arg *);
  void _rl_callback_newline ();
#endif

  /* undo.cc */

  /* util.cc */

  void _rl_ttymsg (const char *, ...)
      __attribute__ ((__format__ (printf, 2, 3)));
  void _rl_errmsg (const char *, ...)
      __attribute__ ((__format__ (printf, 2, 3)));

  int _rl_abort_internal ();
  int _rl_null_function (int, int);
  char *_rl_strindex (string_view, string_view);

  /* vi_mode.cc */

  inline void
  _rl_vi_initialize_line ()
  {
    int n = sizeof (vi_mark_chars) / sizeof (vi_mark_chars[0]);
    for (int i = 0; i < n; i++)
      vi_mark_chars[i] = -1;

    RL_UNSETSTATE (RL_STATE_VICMDONCE);
  }

  inline void
  _rl_vi_reset_last ()
  {
    _rl_vi_last_command = 'i';
    _rl_vi_last_repeat = 1;
    _rl_vi_last_arg_sign = 1;
    _rl_vi_last_motion = 0;
  }

  inline void
  _rl_vi_set_last (int key, int repeat, int sign)
  {
    _rl_vi_last_command = key;
    _rl_vi_last_repeat = repeat;
    _rl_vi_last_arg_sign = sign;
  }

  /* Is the command C a VI mode text modification command? */
  inline bool
  _rl_vi_textmod_command (int c)
  {
    return member (static_cast<char> (c), vi_textmod);
  }

  inline bool
  _rl_vi_motion_command (int c)
  {
    return member (static_cast<char> (c), vi_motion);
  }

  inline size_t
  _rl_vi_advance_point ()
  {
    size_t point = rl_point;

    if (rl_point < rl_end ())
#if defined(HANDLE_MULTIBYTE)
      {
        if (MB_CUR_MAX == 1 || rl_byte_oriented)
          rl_point++;
        else
          {
            point = rl_point;
            rl_point = _rl_forward_char_internal (1);
            if (point == rl_point || rl_point > rl_end ())
              rl_point = rl_end ();
          }
      }
#else
      rl_point++;
#endif

    return point;
  }

  /* Move the cursor back one character. */
  inline void
  _rl_vi_backup ()
  {
    if (MB_CUR_MAX > 1 && !rl_byte_oriented)
      rl_point = _rl_find_prev_mbchar (rl_line_buffer.c_str (), rl_point,
                                       MB_FIND_NONZERO);
    else
      rl_point--;
  }

  /* Move the point back one character, returning the starting value and not
     doing anything at the beginning of the line */
  inline size_t
  _rl_vi_backup_point ()
  {
    size_t point = rl_point;

    if (rl_point > 0)
#if defined(HANDLE_MULTIBYTE)
      {
        if (MB_CUR_MAX == 1 || rl_byte_oriented)
          rl_point--;
        else
          {
            point = rl_point;
            rl_point = _rl_backward_char_internal (1);
          }
      }
#else
      rl_point--;
#endif
    return point;
  }

  /* Move the cursor back one character if you're at the end of the line */
  inline void
  rl_vi_check ()
  {
    if (rl_point && rl_point == rl_line_buffer.size ())
      _rl_vi_backup ();
  }

  // A simplified loop for vi. Don't dispatch key at end. Don't recognize minus
  // sign? Should this do rl_save_prompt/rl_restore_prompt?
  inline int
  rl_digit_loop1 ()
  {
    while (1)
      {
        if (_rl_arg_overflow ())
          return 1;

        int c = _rl_arg_getchar ();
        int r = _rl_vi_arg_dispatch (c);
        if (r <= 0)
          break;
      }

    RL_UNSETSTATE (RL_STATE_NUMERICARG);
    return 0;
  }

  inline void
  _rl_vi_append_forward (int)
  {
    _rl_vi_advance_point ();
  }

  inline int
  rl_vi_domove_getchar (_rl_vimotion_cxt *)
  {
    return _rl_bracketed_read_key ();
  }

  inline int
  rl_vi_bracktype (int c)
  {
    switch (c)
      {
      case '(':
        return 1;
      case ')':
        return -1;
      case '[':
        return 2;
      case ']':
        return -2;
      case '{':
        return 3;
      case '}':
        return -3;
      default:
        return 0;
      }
  }

  inline int
  _rl_vi_callback_getchar (std::string &mb, int mlen)
  {
    return _rl_bracketed_read_mbstring (mb, mlen);
  }

  // Increment START to the next character in RL_LINE_BUFFER, handling
  // multibyte chars.
  inline void
  INCREMENT_POS (size_t &start)
  {
#if defined(HANDLE_MULTIBYTE)
    if (MB_CUR_MAX == 1 || rl_byte_oriented)
      start++;
    else
      start = _rl_find_next_mbchar (rl_line_buffer, start, 1, MB_FIND_ANY);
#else  /* !HANDLE_MULTIBYTE */
    start++;
#endif /* !HANDLE_MULTIBYTE */
  }

  void _rl_vi_done_inserting ();
  int _rl_vi_domove_motion_cleanup (int, _rl_vimotion_cxt *);
  void _rl_vi_replace_insert (int);
  void _rl_vi_stuff_insert (int);

  inline void
  vi_save_insert_buffer (size_t start, size_t len)
  {
    vi_insert_buffer = rl_line_buffer.substr (start, len - 1);
  }

  void _rl_vi_save_replace ();

  inline void
  _rl_vi_save_insert (UNDO_ENTRY &up)
  {
    if (up.what != UNDO_INSERT)
      {
        vi_insert_buffer.clear ();
        return;
      }

    size_t start = up.start;
    size_t end = up.end;
    size_t len = end - start + 1;

    vi_save_insert_buffer (start, len);
  }

#if defined(HANDLE_MULTIBYTE)
  void _rl_vi_change_mbchar_case (int);
#endif

  int _rl_vi_arg_dispatch (int);

  int rl_domove_motion_callback (_rl_vimotion_cxt *);

  int rl_domove_read_callback (_rl_vimotion_cxt *);

#if defined(READLINE_CALLBACKS)
  int _rl_vi_domove_callback (_rl_vimotion_cxt *);
  int _rl_vi_callback_char_search (_rl_callback_generic_arg *);
  int _rl_vi_callback_change_char (_rl_callback_generic_arg *);
  int _rl_vi_callback_set_mark (_rl_callback_generic_arg *);
  int _rl_vi_callback_goto_mark (_rl_callback_generic_arg *);
#endif

  int vi_domove_dispatch (_rl_vimotion_cxt *);

  int vi_delete_dispatch (_rl_vimotion_cxt *);

  int vi_change_dispatch (_rl_vimotion_cxt *);

  int vi_yank_dispatch (_rl_vimotion_cxt *);

  int _rl_vi_change_char (int, int, char *);

  int rl_vi_overstrike_kill_line (int, int);
  int rl_vi_overstrike_kill_word (int, int);
  int rl_vi_overstrike_yank (int, int);
  int rl_vi_overstrike_bracketed_paste (int, int);

  int _rl_vi_set_mark ();
  int _rl_vi_goto_mark ();

  inline size_t
  rl_end ()
  {
    return rl_line_buffer.size ();
  }

public:
  /* ************************************************************** */
  /*		Well Published Variables (ptr types)		    */
  /* ************************************************************** */

  /* The version of this incarnation of the readline library. */
  const char *rl_library_version; /* e.g., "4.2" */

  /* The name of the calling program.  You should initialize this to
     whatever was in argv[0].  It is used when parsing conditionals. */
  const char *rl_readline_name;

  /* The prompt readline uses.  This is set from the argument to
     readline (), and should not be assigned to directly. */
  std::string rl_prompt;

  /* The prompt string that is actually displayed by rl_redisplay.  Public so
     applications can more easily supply their own redisplay functions. */
  std::string rl_display_prompt;

  /* The line buffer that is in use. */
  std::string rl_line_buffer;

  /* The address of the last command function Readline executed. */
  rl_command_func_t rl_last_func;

  /* The name of the terminal to use. */
  const char *rl_terminal_name;

  FILE *rl_instream;  // The input stream.
  FILE *rl_outstream; // The output stream.

  /* If non-zero, then this is the address of a function to call just
     before readline_internal () prints the first prompt. */
  rl_hook_func_t rl_startup_hook;

  /* If non-zero, this is the address of a function to call just before
     readline_internal_setup () returns and readline_internal starts
     reading input characters. */
  rl_hook_func_t rl_pre_input_hook;

  /* The address of a function to call periodically while Readline is
     awaiting character input, or nullptr, for no event handling. */
  rl_hook_func_t rl_event_hook;

  /* The address of a function to call if a read is interrupted by a signal. */
  rl_hook_func_t rl_signal_event_hook;

  /* The address of a function to call if Readline needs to know whether or not
     there is data available from the current input source. */
  rl_hook_func_t rl_input_available_hook;

  /* The address of the function to call to fetch a character from the current
     Readline input stream */
  rl_getc_func_t rl_getc_function;

  rl_vintfunc_t rl_prep_term_function;
  rl_voidfunc_t rl_deprep_term_function;

  rl_vcpfunc_t rl_linefunc; /* user callback function */

  /* Dispatch variables. */

  Keymap rl_binding_keymap;

  std::string rl_executing_keyseq;

  /* Display variables (bools are moved to the end of the class). */

  /* The text of a currently-executing keyboard macro. */
  char *rl_executing_macro;

  /* Completion variables. */

  /* Pointer to the generator function for completion_matches ().
     nullptr means to use rl_filename_completion_function (), the default
     filename completer. */
  rl_compentry_func_t rl_completion_entry_function;

  /* Optional generator for menu completion.  Default is
     rl_completion_entry_function (rl_filename_completion_function). */
  rl_compentry_func_t rl_menu_completion_entry_function;

  /* If rl_ignore_some_completions_function is non-nullptr it is the address
     of a function to call after all of the possible matches have been
     generated, but before the actual completion is done to the input line.
     The function is called with one argument; a std::vector<std::string>. */
  rl_compignore_func_t rl_ignore_some_completions_function;

  /* Pointer to alternative function to create matches.
     Function is called with TEXT, START, and END.
     START and END are indices in RL_LINE_BUFFER saying what the boundaries
     of TEXT are.
     If this function exists and returns nullptr then call the value of
     rl_completion_entry_function to try to match, otherwise use the
     array of strings returned. */
  rl_completion_func_t rl_attempted_completion_function;

  /* The basic list of characters that signal a break between words for the
     completer routine.  The initial contents of this variable is what
     breaks words in the shell, i.e. "n\"\\'`@$>". */
  const char *rl_basic_word_break_characters;

  /* The list of characters that signal a break between words for
     rl_complete_internal.  The default list is the contents of
     rl_basic_word_break_characters.  */
  char *rl_completer_word_break_characters;

  /* Hook function to allow an application to set the completion word
     break characters before readline breaks up the line.  Allows
     position-dependent word break characters. */
  rl_cpvfunc_t rl_completion_word_break_hook;

  /* List of characters which can be used to quote a substring of the line.
     Completion occurs on the entire substring, and within the substring
     rl_completer_word_break_characters are treated as any other character,
     unless they also appear within this list. */
  const char *rl_completer_quote_characters;

  /* List of quote characters which cause a word break. */
  const char *rl_basic_quote_characters;

  // List of characters that need to be quoted in filenames by the completer.
  const char *rl_filename_quote_characters;

  /* List of characters that are word break characters, but should be left
     in TEXT when it is passed to the completion function.  The shell uses
     this to help determine what kind of completing to do. */
  const char *rl_special_prefixes;

  /* If non-zero, then this is the address of a function to call when
     completing on a directory name.  The function is called with
     the address of a string (the current directory name) as an arg.  It
     changes what is displayed when the possible completions are printed
     or inserted.  The directory completion hook should perform
     any necessary dequoting.  This function should return 1 if it modifies
     the directory name pointer passed as an argument.  If the directory
     completion hook returns 0, it should not modify the directory name
     pointer passed as an argument. */
  rl_icppfunc_t rl_directory_completion_hook;

  /* If non-zero, this is the address of a function to call when completing
     a directory name.  This function takes the address of the directory name
     to be modified as an argument.  Unlike rl_directory_completion_hook, it
     only modifies the directory name used in opendir(2), not what is displayed
     when the possible completions are printed or inserted.  If set, it takes
     precedence over rl_directory_completion_hook.  The directory rewrite
     hook should perform any necessary dequoting.  This function has the same
     return value properties as the directory_completion_hook.

     I'm not happy with how this works yet, so it's undocumented.  I'm trying
     it in bash to see how well it goes. */
  rl_icppfunc_t rl_directory_rewrite_hook;

  /* If non-zero, this is the address of a function for the completer to call
     before deciding which character to append to a completed name.  It should
     modify the directory name passed as an argument if appropriate, and return
     non-zero if it modifies the name.  This should not worry about dequoting
     the filename; that has already happened by the time it gets here. */
  rl_icppfunc_t rl_filename_stat_hook;

  /* If non-zero, this is the address of a function to call when reading
     directory entries from the filesystem for completion and comparing
     them to the partial word to be completed.  The function should
     either return its first argument (if no conversion takes place) or
     newly-allocated memory.  This can, for instance, convert filenames
     between character sets for comparison against what's typed at the
     keyboard.  The returned value is what is added to the list of
     matches. */
  rl_dequote_func_t rl_filename_rewrite_hook;

  /* If non-zero, then this is the address of a function to call when
     completing a word would normally display the list of possible matches.
     This function is called instead of actually doing the display.
     It takes three arguments: (std::vector<std::string> matches, int
     max_length) where MATCHES is the vector of strings that matched,
     and MAX_LENGTH is the length of the longest string in that array. */
  rl_compdisp_func_t rl_completion_display_matches_hook;

  /* Set to a function to quote a filename in an application-specific fashion.
     Called with the text to quote, the type of match found (single or
     multiple) and a pointer to the quoting character to be used, which the
     function can reset if desired. */
  rl_quote_func_t rl_filename_quoting_function;

  /* Function to call to remove quoting characters from a filename.  Called
     before completion is attempted, so the embedded quotes do not interfere
     with matching names in the file system. */
  rl_dequote_func_t rl_filename_dequoting_function;

  /* Function to call to decide whether or not a word break character is
     quoted.  If a character is quoted, it does not break words for the
     completer. */
  rl_linebuf_func_t rl_char_is_quoted_p;

  /* Incremental search terminators, used by isearch.cc */
  std::string _rl_isearch_terminators;

  _rl_search_cxt *_rl_iscxt;

  /* macro.cc */

  std::string _rl_executing_macro;

  /* Used by signal handlers to access the singleton. */
  static Readline *the_app;

private:
  /* ************************************************************** */
  /*	    Private Readline Variables (arrays / ptrs)		    */
  /* ************************************************************** */

  // character input buffer
  unsigned char _rl_ibuffer[512];

  /* The current undo list for RL_LINE_BUFFER. */
  UNDO_LIST *rl_undo_list;

  /* The list of funmap entries. */
  std::vector<FUNMAP *> funmap;

  /* Application-specific redisplay function. */
  rl_voidfunc_t rl_redisplay_function;

  /* Variables used to include the editing mode in the prompt. */

  std::string _rl_emacs_mode_str;

  std::string _rl_vi_ins_mode_str;

  std::string _rl_vi_cmd_mode_str;

  /* bind.cc */

  /* The last key bindings file read. */
  char *last_readline_init_file;

  /* The file we're currently reading key bindings from. */
  const char *current_readline_init_file;

  /* Stack of previous values of parsing_conditionalized_out. */
  std::vector<bool> if_stack;

  // Vector of boolean vars with pointers to member variables in this object.
  std::vector<boolean_var_t> boolean_varlist_;

  /* Vector of keymap names with pointers to lazy-inited keymaps. */
  std::vector<name_and_keymap> keymap_names_;

  struct _tc_string
  {
    _tc_string (const char *tc_var_, const char **tc_value_)
        : tc_var (tc_var_), tc_value (tc_value_)
    {
    }

    const char *tc_var;
    const char **tc_value;
  };

  /* Vector of termcap variables and their values. */
  std::vector<_tc_string> _tc_strings;

  /* callback.cc */

  /* Private data for callback registration functions.  See comments in
     rl_callback_read_char for more details. */
  _rl_callback_func_t _rl_callback_func;

  /* This may contain callback data for text.cc or vi_mode.c. */
  _rl_callback_generic_arg *_rl_callback_data;

  /* complete.cc */

  std::string _rl_ucmp_username;
  struct passwd *_rl_ucmp_pwentry;

  DIR *_rl_fcmp_directory;
  std::string _rl_fcmp_filename;
  std::string _rl_fcmp_dirname;
  std::string _rl_fcmp_users_dirname;

  char *_rl_omenu_orig_text;
  std::vector<std::string *> *_rl_omenu_matches;

  char *_rl_menu_orig_text;
  std::vector<std::string *> *_rl_menu_matches;

  /* isearch.cc */

  /* Last incremental search string. */
  std::string last_isearch_string;

  /* A buffer for `modeline' messages. */
  std::string msg_buf;

  Keymap emacs_standard_keymap_;
  Keymap emacs_meta_keymap_;
  Keymap emacs_ctlx_keymap_;

#if defined(VI_MODE)
  Keymap vi_insertion_keymap_;
  Keymap vi_movement_keymap_;
#endif

  /* kill.cc */

  /* Where to store killed text. */
  std::string rl_kill_ring[DEFAULT_MAX_KILLS];

  /* macro.cc */

  /* The current macro string being built.  Characters get stuffed
     in here by add_macro_char (). */
  std::string rl_current_macro;

  /* A structure used to save nested macro strings.
     It is a linked list of string/index for each saved macro. */
  struct saved_macro
  {
    struct saved_macro *next;
    char *string;
    size_t sindex;
  };

  /* The list of saved macros. */
  static struct saved_macro *rl_macro_list;

  /* misc.cc */

  rl_hook_func_t _rl_saved_internal_startup_hook;

  /* parse-colors.cc */

  /* new'd array of file type indicators (dir, sock, fifo, ...)
     Default value is initialized in parse-colors.c.
     It is then modified from the values of $LS_COLORS. */
  std::string *_rl_color_indicator;

  /* search.cc */

  std::string noninc_search_string;

  std::string history_search_string;

  /* signals.cc */

  sighandler_cxt old_int, old_term, old_hup, old_alrm, old_quit;

#if defined(SIGTSTP)
  sighandler_cxt old_tstp, old_ttou, old_ttin;
#endif

#if defined(HAVE_POSIX_SIGNALS)
  sigset_t sigint_set, sigint_oset;
  sigset_t sigwinch_set, sigwinch_oset;
#else /* !HAVE_POSIX_SIGNALS */
#if defined(HAVE_BSD_SIGNALS)
  int sigint_oldmask;
  int sigwinch_oldmask;
#endif /* HAVE_BSD_SIGNALS */
#endif /* !HAVE_POSIX_SIGNALS */

  /* terminal.cc */

  std::string term_buffer;
  std::string term_string_buffer;

  // Some strings to control terminal actions. These are output by tputs ().

  const char *_rl_term_clreol;
  const char *_rl_term_clrpag;
  const char *_rl_term_clrscroll;
  const char *_rl_term_cr;
  const char *_rl_term_backspace;
  const char *_rl_term_goto;
  const char *_rl_term_pc;

  /* How to insert characters. */

  const char *_rl_term_im;
  const char *_rl_term_ei;
  const char *_rl_term_ic;
  const char *_rl_term_ip;
  const char *_rl_term_IC;

  /* How to delete characters. */

  const char *_rl_term_dc;
  const char *_rl_term_DC;

  /* How to move forward a char, non-destructively */
  const char *_rl_term_forward_char;

  /* How to go up a line. */
  const char *_rl_term_up;

  /* A visible bell; char if the terminal can be made to flash the screen. */
  const char *_rl_visible_bell;

  /* The sequences to write to turn on and off the meta key, if this
     terminal has one. */

  const char *_rl_term_mm;
  const char *_rl_term_mo;

  /* The sequences to enter and exit standout mode. */

  const char *_rl_term_so;
  const char *_rl_term_se;

  /* The key sequences output by the arrow keys, if this terminal has any. */

  const char *_rl_term_ku;
  const char *_rl_term_kd;
  const char *_rl_term_kr;
  const char *_rl_term_kl;

  /* How to initialize and reset the arrow keys, if this terminal has any. */

  const char *_rl_term_ks;
  const char *_rl_term_ke;

  /* The key sequences sent by the Home and End keys, if any. */

  const char *_rl_term_kh;
  const char *_rl_term_kH;
  const char *_rl_term_at7; /* @7 */

  /* Delete key */
  const char *_rl_term_kD;

  /* Insert key */
  const char *_rl_term_kI;

  /* Cursor control */

  const char *_rl_term_vs; /* very visible */
  const char *_rl_term_ve; /* normal */

  // A pointer to the keymap that is currently in use. By default, it is the
  // standard emacs keymap.
  Keymap _rl_keymap;

  /* The input stream we interact with. */
  FILE *_rl_in_stream;

  // Any readline function can set this and have it run just before the user's
  // rl_startup_hook.
  rl_hook_func_t _rl_internal_startup_hook;

  /* String inserted into the line by rl_insert_comment (). */
  std::string _rl_comment_begin;

  /* Keymap holding the function currently being executed. */
  Keymap rl_executing_keymap;

  /* Keymap we're currently using to dispatch. */
  Keymap _rl_dispatching_keymap;

  /* Key sequence `contexts' */
  _rl_keyseq_cxt *_rl_kscxt;

  struct _rl_cmd _rl_pending_command;
  struct _rl_cmd *_rl_command_to_execute;

  /* The line display buffers.  One is the line currently displayed on
     the screen.  The other is the line about to be displayed. */
  line_state line_state_array[2];

  line_state *line_state_visible;
  line_state *line_state_invisible;

  /* Struct to hold the prompt and display information. */
  struct prompt_state
  {
    std::string prompt;
    std::string prefix;

    /* A vector of indexes into the prompt string where we will break physical
       screen lines.  It's easier to compute in expand_prompt and use later in
       rl_redisplay instead of having rl_redisplay try to guess about invisible
       characters in the prompt or use heuristics about where they are. */
    std::vector<size_t> newlines;

    /* Variables to keep track of the expanded prompt string, which may
       include invisible characters. */

    /* Number of chars in the buffer that contribute to visible chars on the
       screen. This might be different from the number of physical chars in the
       presence of multibyte characters */
    size_t visible_length;

    /* The index of the last invisible character in the prompt string. */
    size_t last_invisible;

    /* Number of invisible characters on the first physical line of the prompt.
       Only valid when the number of physical characters in the prompt exceeds
       (or is equal to) _rl_screenwidth. */
    size_t invis_chars_first_line;

    /* Number of physical chars in the prompt. */
    size_t physical_chars;

    /* Number of visible chars in the prompt prefix. */
    size_t prefix_length;
  };

  /* The current local prompt state. */
  prompt_state local_prompt;

  /* The saved local prompt state. */
  prompt_state saved_local_prompt;

  /* search.cc */

  _rl_search_cxt *_rl_nscxt;
  const std::string prev_line_found;

  /* signals.cc */

  _rl_sigcleanup_func_t _rl_sigcleanup;
  void *_rl_sigcleanarg;

  /* text.cc */

#if defined(HANDLE_MULTIBYTE)
  char _rl_vi_last_search_mbchar[MB_LEN_MAX];
  char _rl_pending_bytes[MB_LEN_MAX];
  mbstate_t _rl_pending_mbstate;
  int _rl_pending_bytes_length;
  int _rl_stored_count;
#endif

  /* undo.cc */

  // While we are editing the history, this is the saved version of the
  // original line.
  HIST_ENTRY *_rl_saved_line_for_history;

  /* vi_mode.cc */

  std::string vi_insert_buffer;

  _rl_vimotion_cxt *_rl_vimvcxt;

  // Keymap used for vi replace characters.  Created dynamically since it may
  // not be used.
  Keymap vi_replace_map;

  static const char *const vi_motion;
  static const char *const vi_textmod;

  /* ************************************************************** */
  /*    Private Readline Variables (32-bit or 64-bit aligned)	    */
  /* ************************************************************** */

  /* Length in characters of a common prefix replaced with an ellipsis (`...')
     when displaying completion matches.  Matches whose printable portion has
     more than this number of displaying characters in common will have the
     common display prefix replaced with an ellipsis. */
  size_t _rl_completion_prefix_display_length;

  /* complete.cc */

  /* The readline-private number of screen columns to use when displaying
     matches.  If < 0 or > _rl_screenwidth, it is ignored. */
  ssize_t _rl_completion_columns;

  size_t _rl_ucmp_first_char_loc;

  size_t _rl_omenu_orig_start;
  size_t _rl_omenu_orig_end;

  size_t _rl_menu_match_list_index;

  size_t _rl_menu_orig_start;
  size_t _rl_menu_orig_end;

  /* display.cc */

  /* The current offset in the current input line. */
  size_t rl_point;

  /* Mark in the current input line. */
  size_t rl_mark;

  /* The index into the line buffer corresponding to the cursor position */
  size_t cpos_buffer_position;

  /* Saved target point for when _rl_history_preserve_point is set.  Special
     value of (size_t)(-1) means that point is at the end of the line. */
  size_t _rl_history_saved_point;

  /* Saved logical offset, or (size_t)(-1) if not initialized. */
  size_t saved_history_logical_offset;

  /* Variables that hold the screen dimensions, used by the display code. */

  size_t _rl_screenwidth;
  size_t _rl_screenheight;
  size_t _rl_screenchars;

  /* The visible cursor position.  If you print some text, adjust this. */
  /* NOTE: _rl_last_c_pos is used as a buffer index when not in a locale
     supporting multibyte characters, and an absolute cursor position when
     in such a locale.  This is an artifact of the donated multibyte support.
     Care must be taken when modifying its value. */

  size_t _rl_last_c_pos;

  size_t _rl_last_v_pos;

  /* kill.cc */

  /* Where we are in the kill ring (index of last added entry). */
  size_t rl_kill_ring_index;

  /* Number of entries in the kill ring. */
  size_t rl_kill_ring_length;

  /* macro.cc */

  // The offset in `executing_macro` to the next character to be read.
  size_t executing_macro_index;

  /* Current visible prompt length. */
  size_t rl_visible_prompt_length;

  size_t noninc_history_pos;
  size_t rl_history_search_len;
  size_t rl_history_search_pos;

  /*************************************************************************
   *									   *
   * Global variables undocumented in texinfo manual and not in readline.hh *
   *									   *
   *************************************************************************/

  /* Number of physical lines consumed by the current line buffer currently
     on screen minus 1. */
  size_t _rl_vis_botlin;

  size_t _rl_inv_botlin;

  /* display.cc */

  /* The number of multibyte characters in the prompt, if any */
  size_t prompt_multibyte_chars;

  /* The last left edge of text that was displayed.  This is used when
     doing horizontal scrolling.  It shifts in thirds of a screenwidth. */
  size_t last_lmargin;

  /* The number of invisible characters in the line currently being
     displayed on the screen. */
  size_t visible_wrap_offset;

  /* The number of invisible characters in the prompt string. */
  size_t wrap_offset;

  /* The length (buffer offset) of the first line of the last (possibly
     multi-line) buffer displayed on the screen. */
  size_t visible_first_line_len;

  size_t prompt_last_screen_line;

  size_t _rl_omenu_match_list_index; // match list index may be negative.
  size_t _rl_omenu_match_list_size;

  size_t _rl_pop_index;
  size_t _rl_push_index;

  // the following definitions may be aligned differently on different systems

#if defined(SIGWINCH)
  sighandler_cxt _rl_old_winch;
#endif

#if defined(HAVE_POSIX_SIGNALS)
  sigset_t _rl_orig_sigset;
#endif /* !HAVE_POSIX_SIGNALS */

#if !defined(NO_TTY_DRIVER)
  TIOTYPE otio; // "struct termios" for POSIX termios.
#endif

public:
  /* ************************************************************** */
  /*	    Well Published Variables (32-bit int types)		    */
  /* ************************************************************** */

  // The readline version, as an int.
  int rl_readline_version; /* e.g., 0x0402 */

  /* If set to a character value, that will be the next keystroke read. */
  int rl_pending_input;

  /* The current value of the numeric argument specified by the user. */
  int rl_numeric_arg;

  int rl_executing_key;

  /* A non-zero value means to read only this many characters rather than
    up to a character bound to accept-line. */
  int rl_num_chars_to_read;

  /* Set to a character describing the type of completion being attempted by
     rl_complete_internal; available for use by application completion
     functions. */
  int rl_completion_type;

  /* Set to the last key used to invoke one of the completion functions */
  int rl_completion_invoking_key;

  /* Up to this many items will be displayed in response to a
     possible-completions call.  After that, we ask the user if she
     is sure she wants to see them all.  The default value is 100. */
  int rl_completion_query_items;

private:
  /* ************************************************************** */
  /*	    Private Readline Variables (32-bit types)		    */
  /* ************************************************************** */

  /* Flags word encapsulating the current readline state. */
  state_flags rl_readline_state;

  /* Says which editing mode readline is currently using. */
  editing_mode rl_editing_mode;

  /* The style of `bell' notification preferred.  This can be set to NO_BELL,
     AUDIBLE_BELL, or VISIBLE_BELL. */
  rl_bell_pref _rl_bell_preference;

  /* The character that can generate an EOF.  Really read from
     the terminal driver... just defaulted here. */
  int _rl_eof_char;

  /* bind.cc */

  int current_readline_init_include_level;
  int current_readline_init_lineno;

  // The current macro level.
  int rl_macro_level;

  /* complete.cc */

  /* Character appended to completed words when at the end of the line.  The
     default is a space.  Nothing is added if this is '\0'. */
  int rl_completion_append_character;

  /* Set to any quote character readline thinks it finds before any application
     completion function is called. */
  int rl_completion_quote_character;

  int _rl_omenu_delimiter;

  int _rl_menu_delimiter;

  /* misc.cc */

  _rl_arg_cxt _rl_argcxt;

  /* paren.cc */

  int _paren_blink_usec;

  /* Temporary value used while generating the argument. */
  int rl_arg_sign;

  /* Timeout (specified in milliseconds) when reading characters making up an
     ambiguous multiple-key sequence */
  int _rl_keyseq_timeout;

  int terminal_prepped; // XXX bool?

  /* search.cc */

  hist_search_flags rl_history_search_flags;

  /* signals.cc */

  int _rl_intr_char;
  int _rl_quit_char;
  int _rl_susp_char;

  /* How many unclosed undo groups we currently have. */
  int _rl_undo_group_level;

  int _keyboard_input_timeout; /* 0.1 seconds; it's in usec */

  /* vi_mode.cc */

  int _rl_vi_last_command; /* default `.' puts you in insert mode */

  int _rl_vi_last_repeat;

  int _rl_vi_last_arg_sign;

  /* The number of characters inserted in the last replace operation. */
  int vi_replace_count;

  int _rl_vi_last_motion;

  int _rl_vi_last_key_before_insert;

#if defined(HANDLE_MULTIBYTE)
  int _rl_vi_last_search_mblen;
#else
  int _rl_vi_last_search_char;
#endif

  int _rl_cs_dir;
  int _rl_cs_orig_dir;

  /* kill.cc */

  int _rl_yank_history_skip;
  int _rl_yank_count_passed;
  int _rl_yank_direction;

  /* Arrays for the saved marks. */
  int vi_mark_chars['z' - 'a' + 1];

  // private char arrays below

  /* rltty.cc */

  _RL_TTY_CHARS _rl_tty_chars;
  _RL_TTY_CHARS _rl_last_tty_chars;

public:
  /* ************************************************************** */
  /*	    Public Readline Variables (8-bit types)		    */
  /* ************************************************************** */

  /* True if this is real GNU readline as opposed to some stub substitute. */
  bool rl_gnu_readline_p;

  /* Insert or overwrite mode for emacs mode.  true means insert mode; false
     means overwrite mode.  Reset to insert mode on each input line. */
  bool rl_insert_mode;

  /* If this is non-zero, completion is (temporarily) inhibited, and the
     completion character will be inserted as any other. */
  bool rl_inhibit_completion;

  /* True if we called this function from _rl_dispatch().  It's present
     so functions can find out whether they were called from a key binding
     or directly from an application. */
  bool rl_dispatching;

  /* True if the user typed a numeric argument before executing the
     current function. */
  bool rl_explicit_arg;

  /* If true, readline will erase the entire line, including any prompt,
     if the only thing typed on an otherwise-blank line is something bound to
     rl_newline. */
  bool rl_erase_empty_line;

  /* If true, the application has already printed the prompt (rl_prompt)
     before calling readline, so readline should not output it the first time
     redisplay is done. */
  bool rl_already_prompted;

  /* Variables to control readline signal handling. */
  /* If true, readline will install its own signal handlers for
   SIGINT, SIGTERM, SIGQUIT, SIGALRM, SIGTSTP, SIGTTIN, and SIGTTOU. */
  bool rl_catch_signals;

  /* If true, readline will install a signal handler for SIGWINCH
     that also attempts to call any calling application's SIGWINCH signal
     handler.  Note that the terminal is not cleaned up before the
     application's signal handler is called; use rl_cleanup_after_signal()
     to do that. */
  bool rl_catch_sigwinch;

  /* If true, the readline SIGWINCH handler will modify LINES and
     COLUMNS in the environment. */
  bool rl_change_environment;

  /* True means that the results of the matches are to be treated
     as filenames.  This is ALWAYS zero on entry, and can only be changed
     within a completion entry finder function. */
  bool rl_filename_completion_desired;

  /* True means that the results of the matches are to be quoted using
     double quotes (or an application-specific quoting mechanism) if the
     filename contains any characters in rl_word_break_chars.  This is
     ALWAYS true on entry, and can only be changed within a completion
     entry finder function. */
  bool rl_filename_quoting_desired;

  /* Flag to indicate that readline has finished with the current input
     line and should return it. */
  bool rl_done;

  /* What YOU turn on when you have handled all redisplay yourself. */
  bool rl_display_fixed;

  /* If true, Readline gives values of LINES and COLUMNS from the environment
     greater precedence than values fetched from the kernel when computing the
     screen dimensions. */
  bool rl_prefer_env_winsize;

  /* True means to suppress normal filename completion after the
     user-specified completion function has been called. */
  bool rl_attempted_completion_over;

  /* If set to true by an application completion function,
     rl_completion_append_character will not be appended. */
  bool rl_completion_suppress_append;

  /* Set to true if readline found quoting anywhere in the word to
     be completed; set before any application completion function is called. */
  bool rl_completion_found_quote;

  /* If true, the completion functions don't append any closing quote.
     This is set to 0 by rl_complete_internal and may be changed by an
     application-specific completion function. */
  bool rl_completion_suppress_quote;

  /* If true, readline will sort the completion matches.  On by default. */
  bool rl_sort_completion_matches;

  /* If true, a slash will be appended to completed filenames that are
     symbolic links to directory names, subject to the value of the
     mark-directories variable (which is user-settable).  This exists so
     that application completion functions can override the user's preference
     (set via the mark-symlinked-directories variable) if appropriate.
     It's set to the value of _rl_complete_mark_symlink_dirs in
     rl_complete_internal before any application-specific completion
     function is called, so without that function doing anything, the user's
     preferences are honored. */
  bool rl_completion_mark_symlink_dirs;

  /* If true, then disallow duplicates in the matches. */
  bool rl_ignore_completion_duplicates;

  /* Applications can set this to true to have readline's signal handlers
     installed during the entire duration of reading a complete line, as in
     readline-6.2.  This should be used with care, because it can result in
     readline receiving signals and not handling them until it's called again
     via rl_callback_read_char, thereby stealing them from the application.
     By default, signal handlers are only active while readline is active. */
  bool rl_persistent_signal_handlers;

private:
  /* ************************************************************** */
  /*	    Private Readline Variables (8-bit types)		    */
  /* ************************************************************** */

  /* Set to true after Readline is initialized. */
  bool rl_initialized;

  /* Set to true after function map is initialized. */
  bool funmap_initialized;

  /* True if we determine that the terminal can do character insertion. */
  bool _rl_terminal_can_insert;

  /* True means that this terminal has a meta key. */
  bool term_has_meta;

  /* True means the user wants to enable the keypad. */
  bool _rl_enable_keypad;

  /* True means the terminal can auto-wrap lines (-1 means not initialized). */
  char _rl_term_autowrap;

  bool _rl_prefer_visible_bell;

  /* True means the user wants to enable a meta key. */
  bool _rl_enable_meta;

  /* True if the previous command was a kill command. */
  bool _rl_last_command_was_kill;

  /* True means echo characters as they are read.  Defaults to no echo;
     set to 1 if there is a controlling terminal, we can get its attributes,
     and the attributes include `echo'.  Look at
     rltty.c:prepare_terminal_settings for the code that sets it. */
  bool _rl_echoing_p;

  /* If true when readline_internal returns, it means we found EOF */
  bool _rl_eof_found;

  /* True means to always use horizontal scrolling in line display. */
  bool _rl_horizontal_scroll_mode;

  /* True means to display an asterisk at the starts of history lines
     which have been modified. */
  bool _rl_mark_modified_lines;

  /* How to print things in the "echo-area".  The prompt is treated as a
     mini-modeline. */
  bool msg_saved_prompt;

  /* bind.cc */
  bool currently_reading_init_file;

  bool _rl_emacs_mode_str_custom;

  bool _rl_vi_ins_mode_str_custom;

  bool _rl_vi_cmd_mode_str_custom;

  bool _rl_isearch_terminators_custom;

  /* complete.cc */

  /* If true, non-unique completions always show the list of matches. */
  bool _rl_complete_show_all;

  /* If true, non-unique completions show the list of matches, unless it
     is not possible to do partial completion and modify the line. */
  bool _rl_complete_show_unmodified;

  /* If true, completed directory names have a slash appended. */
  bool _rl_complete_mark_directories;

  /* If non-zero, the symlinked directory completion behavior introduced in
     readline-4.2a is disabled, and symlinks that point to directories have
     a slash appended (subject to the value of _rl_complete_mark_directories).
     This is user-settable via the mark-symlinked-directories variable. */
  bool _rl_complete_mark_symlink_dirs;

  /* If non-zero, completions are printed horizontally in alphabetical order,
     like `ls -x'. */
  bool _rl_print_completions_horizontally;

  /* Non-zero means that case is not significant in filename completion. */
  bool _rl_completion_case_fold;

  /* Non-zero means that `-' and `_' are equivalent when comparing filenames
     for completion. */
  bool _rl_completion_case_map;

  /* If zero, don't match hidden files (filenames beginning with a `.' on
     Unix) when doing filename completion. */
  bool _rl_match_hidden_files;

#if defined(COLOR_SUPPORT)
  /* Non-zero means to use colors to indicate file type when listing possible
     completions.  The colors used are taken from $LS_COLORS, if set. */
  bool _rl_colored_stats;

  /* Non-zero means to use a color (currently magenta) to indicate the common
     prefix of a set of possible word completions. */
  bool _rl_colored_completion_prefix;
#endif

  /* If non-zero, when completing in the middle of a word, don't insert
     characters from the match that match characters following point in
     the word.  This means, for instance, completing when the cursor is
     after the `e' in `Makefile' won't result in `Makefilefile'. */
  bool _rl_skip_completed_text;

  /* If non-zero, menu completion displays the common prefix first in the
     cycle of possible completions instead of the last. */
  bool _rl_menu_complete_prefix_first;

#if defined(VISIBLE_STATS)
  /* Non-zero means add an additional character to each filename displayed
     during listing completion iff rl_filename_completion_desired which helps
     to indicate the type of file being listed. */
  bool rl_visible_stats;
#endif /* VISIBLE_STATS */

  /* Non-zero means readline completion functions perform tilde expansion. */
  bool rl_complete_with_tilde_expansion;

  bool _rl_page_completions;

  /* Local variable states what happened during the last completion attempt. */
  bool _rl_cmpl_changed_buffer;

  bool last_completion_failed;

  bool _rl_complete_display_matches_interrupt;

  bool _rl_menu_nontrivial_lcd;
  bool _rl_menu_full_completion; // set to true if menu completion should
                                 // reinitialize on next call

  char _rl_ucmp_first_char;

  char _rl_omenu_quote_char;
  char _rl_menu_quote_char;

  /* display.cc */
  bool line_structures_initialized;

  bool _rl_quick_redisplay;

  /* This is a hint update_line gives to rl_redisplay that it has adjusted the
     value of _rl_last_c_pos *and* taken the presence of any invisible chars in
     the prompt into account.  rl_redisplay notes this and does not do the
     adjustment itself. */
  bool cpos_adjusted;

  /* A flag to note when we're displaying the first line of the prompt */
  bool displaying_prompt_first_line;

  /* True forces the redisplay even if we thought it was unnecessary. */
  bool forced_display;

  /* Set to true if horizontal scrolling has been enabled
     automatically because the terminal was resized to height 1. */
  bool horizontal_scrolling_autoset;

  /* set to true by rl_redisplay if we are marking modified history
     lines and the current line is so marked. */
  bool modmark;

  /* kill.cc */

  bool _rl_yank_explicit_arg;
  bool _rl_yank_undo_needed;

  /* misc.cc */

  /* If true, rl_get_previous_history and rl_get_next_history attempt
     to preserve the value of rl_point from line to line. */
  bool _rl_history_preserve_point;

  /* True means try to blink the matching open parenthesis when the
     close parenthesis is inserted. */
  bool rl_blink_matching_paren;

  /* True means do not parse any lines other than comments and
     parser directives. */
  bool _rl_parsing_conditionalized_out;

  /* True means to convert characters with the meta bit set to
     escape-prefixed characters so we can indirect through
     emacs_meta_keymap or vi_escape_keymap. */
  bool _rl_convert_meta_chars_to_ascii;

  /* True means to output characters with the meta bit set directly
     rather than as a meta-prefixed escape sequence. */
  bool _rl_output_meta_chars;

  /* True means to look at the termios special characters and bind
     them to equivalent readline functions at startup. */
  bool _rl_bind_stty_chars;

  /* True means to go through the history list at every newline (or
     whenever rl_done is set and readline returns) and revert each line to
     its initial state. */
  bool _rl_revert_all_at_newline;

  /* True means to honor the termios ECHOCTL bit and echo control
     characters corresponding to keyboard-generated signals. */
  bool _rl_echo_control_chars;

  /* True means to prefix the displayed prompt with a character indicating
     the editing mode: @ for emacs, : for vi-command, + for vi-insert. */
  bool _rl_show_mode_in_prompt;

  /* True means to attempt to put the terminal in `bracketed paste mode',
     where it will prefix pasted text with an escape sequence and send
     another to mark the end of the paste. */
  bool _rl_enable_bracketed_paste;
  bool _rl_enable_active_region;

  /* True means treat 0200 bit in terminal input as Meta bit. */
  bool _rl_meta_flag;

  bool _rl_echoctl;

  /* signals.cc */

  bool signals_set_flag;
  bool sigwinch_set_flag;

  bool sigint_blocked;
  bool sigwinch_blocked;

  /* terminal.cc */
  bool tcap_initialized;

  /* text.cc */

  /* Is the region active? */
  bool mark_active;

  /* Does the current command want the mark to remain active when it completes?
   */
  bool _rl_keep_mark_active;

  bool _rl_optimize_typeahead; /* rl_insert tries to read typeahead */

  /* Hints for other parts of readline to give to the display engine. */

  bool _rl_suppress_redisplay; // suppress redisplay

  bool _rl_want_redisplay; // want redisplay

  bool _rl_doing_an_undo;

  /* vi_mode.cc */

  /* True indicates we are redoing a vi-mode command with `.' */
  bool _rl_vi_redoing;

  /* True means enter insertion mode. */
  bool _rl_vi_doing_insert;

  /* If true, we have text inserted after a c[motion] command that put
     us implicitly into insert mode.  Some people want this text to be
     attached to the command so that it is `redoable' with `.'. */
  bool vi_continued_command;

  bool _rl_in_handler; /* terminal_prepped and signals set? */

  bool _rl_enabled_meta; /* flag indicating we enabled meta mode */

  /* Temp string storage for _rl_get_string_variable_value. */
  char _rl_numbuf[32];

  char _rl_vi_last_replacement[MB_LEN_MAX + 1]; /* reserve for trailing '\0' */
};

/* **************************************************************** */
/*								    */
/*	Functions for manipulating Keymaps (from keymap.c).	    */
/*								    */
/* **************************************************************** */

/* Return a new, empty keymap. Free it with free() when you are done. */
static inline Readline::Keymap
rl_make_bare_keymap ()
{
  Readline::Keymap keymap;

  keymap = new Readline::KEYMAP_ENTRY[KEYMAP_SIZE];
  for (int i = 0; i < KEYMAP_SIZE; i++)
    {
      keymap[i].type = ISFUNC;
      keymap[i].value.function = nullptr;
    }

  return keymap;
}

/* A convenience function that returns true if there are no keys bound to
   functions in KEYMAP */
static inline bool
rl_empty_keymap (Readline::Keymap keymap)
{
  for (int i = 0; i < ANYOTHERKEY; i++)
    {
      if (keymap[i].type != ISFUNC || keymap[i].value.function)
        return false;
    }
  return true;
}

/* Return a new keymap which is a copy of MAP.  Just copies pointers, does
   not copy text of macros or descend into child keymaps. */
static inline Readline::Keymap
rl_copy_keymap (Readline::Keymap map)
{
  Readline::Keymap temp;

  temp = rl_make_bare_keymap ();
  for (int i = 0; i < KEYMAP_SIZE; i++)
    {
      temp[i].type = map[i].type;
      temp[i].value = map[i].value;
    }
  return temp;
}

/* Free the storage associated with MAP. */
static inline void
rl_discard_keymap (Readline::Keymap map)
{
  if (map == nullptr)
    return;

  for (int i = 0; i < KEYMAP_SIZE; i++)
    {
      switch (map[i].type)
        {
        case ISFUNC:
          break;

        case ISKMAP:
          rl_discard_keymap (map[i].value.map);
          delete[] map[i].value.map;
          break;

        case ISMACR:
          delete[] map[i].value.macro;
          break;
        }
    }
}

/* Convenience function that discards, then frees, MAP. */
static inline void
rl_free_keymap (Readline::Keymap map)
{
  rl_discard_keymap (map);
  delete[] map;
}

} // namespace readline

#endif /* _READLINE_H_ */
