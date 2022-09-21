/* rlprivate.h -- functions and variables global to the readline library,
                  but not intended for use by applications. */

/* Copyright (C) 1999-2020 Free Software Foundation, Inc.

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

#if !defined(_RL_PRIVATE_H_)
#define _RL_PRIVATE_H_

namespace readline
{

/*************************************************************************
 *									 *
 * Convenience definitions						 *
 *									 *
 *************************************************************************/

#define EMACS_MODE() (rl_editing_mode == emacs_mode)
#define VI_COMMAND_MODE()                                                     \
  (rl_editing_mode == vi_mode && _rl_keymap == vi_movement_keymap ())
#define VI_INSERT_MODE()                                                      \
  (rl_editing_mode == vi_mode && _rl_keymap == vi_insertion_keymap ())

#define RL_CHECK_SIGNALS()                                                    \
  do                                                                          \
    {                                                                         \
      if (_rl_caught_signal)                                                  \
        _rl_signal_handler (_rl_caught_signal);                               \
    }                                                                         \
  while (0)

#define RL_SIG_RECEIVED() (_rl_caught_signal != 0)
#define RL_SIGINT_RECEIVED() (_rl_caught_signal == SIGINT)
#define RL_SIGWINCH_RECEIVED() (_rl_caught_signal == SIGWINCH)

// Callback data for reading numeric arguments.
enum num_arg_flags
{
  NUM_NOFLAGS = 0,
  NUM_SAWMINUS = 0x01,
  NUM_SAWDIGITS = 0x02,
  NUM_READONE = 0x04
};

static inline num_arg_flags &
operator|= (num_arg_flags &a, const num_arg_flags &b)
{
  a = static_cast<num_arg_flags> (static_cast<uint32_t> (a)
                                  | static_cast<uint32_t> (b));
  return a;
}

static inline num_arg_flags &
operator&= (num_arg_flags &a, const num_arg_flags &b)
{
  a = static_cast<num_arg_flags> (static_cast<uint32_t> (a)
                                  & static_cast<uint32_t> (b));
  return a;
}

static inline num_arg_flags
operator~(const num_arg_flags &a)
{
  return static_cast<num_arg_flags> (~static_cast<uint32_t> (a));
}

// A context for reading key sequences longer than a single character when
// using the callback interface.
enum keyseq_flags
{
  KSEQ_NOFLAGS = 0,
  KSEQ_DISPATCHED = 0x01,
  KSEQ_SUBSEQ = 0x02,
  KSEQ_RECURSIVE = 0x04
};

static inline keyseq_flags &
operator|= (keyseq_flags &a, const keyseq_flags &b)
{
  a = static_cast<keyseq_flags> (static_cast<uint32_t> (a)
                                 | static_cast<uint32_t> (b));
  return a;
}

static inline keyseq_flags
operator| (const keyseq_flags &a, const keyseq_flags &b)
{
  return static_cast<keyseq_flags> (static_cast<uint32_t> (a)
                                    | static_cast<uint32_t> (b));
}

// vi-mode commands that use result of motion command to define boundaries.
enum vim_flags
{
  VIM_NOFLAGS = 0,
  VIM_DELETE = 0x01,
  VIM_CHANGE = 0x02,
  VIM_YANK = 0x04
};

#define BRACK_PASTE_PREF "\033[200~"
#define BRACK_PASTE_SUFF "\033[201~"

#define BRACK_PASTE_LAST '~'
#define BRACK_PASTE_SLEN 6

#define BRACK_PASTE_INIT "\033[?2004h"
#define BRACK_PASTE_FINI "\033[?2004l\r"

extern "C"
{
  // Stupid comparison routine for qsort () ing strings.
  int _rl_qsort_string_compare (const void *, const void *);
}

// Custom comparison function for sorting string vectors.
struct string_comp
{
  bool
  operator() (const std::string &s1, const std::string &s2)
  {
#if defined(HAVE_STRCOLL)
    return strcoll (s1.c_str (), s2.c_str ()) < 0;
#else
    return strcmp (s1.c_str (), s2.c_str ()) < 0;
#endif
  }
};

// Custom comparison function for sorting string pointer vectors.
struct string_ptr_comp
{
  bool
  operator() (const std::string *s1, const std::string *s2)
  {
#if defined(HAVE_STRCOLL)
    return strcoll (s1->c_str (), s2->c_str ()) < 0;
#else
    return strcmp (s1->c_str (), s2->c_str ()) < 0;
#endif
  }
};

} // namespace readline

#endif /* _RL_PRIVATE_H_ */
