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

#define CUSTOM_REDISPLAY_FUNC() (rl_redisplay_function != rl_redisplay)
#define CUSTOM_INPUT_FUNC() (rl_getc_function != rl_getc)

/* Callback data for reading numeric arguments */
#define NUM_SAWMINUS 0x01
#define NUM_SAWDIGITS 0x02
#define NUM_READONE 0x04

/* A context for reading key sequences longer than a single character when
   using the callback interface. */
#define KSEQ_DISPATCHED 0x01
#define KSEQ_SUBSEQ 0x02
#define KSEQ_RECURSIVE 0x04

/* vi-mode commands that use result of motion command to define boundaries */
#define VIM_DELETE 0x01
#define VIM_CHANGE 0x02
#define VIM_YANK 0x04

/* various states for vi-mode commands that use motion commands.  reflects
   RL_READLINE_STATE */
#define VMSTATE_READ 0x01
#define VMSTATE_NUMARG 0x02

#define BRACK_PASTE_PREF "\033[200~"
#define BRACK_PASTE_SUFF "\033[201~"

#define BRACK_PASTE_LAST '~'
#define BRACK_PASTE_SLEN 6

#define BRACK_PASTE_INIT "\033[?2004h"
#define BRACK_PASTE_FINI "\033[?2004l\r"

} // namespace readline

#endif /* _RL_PRIVATE_H_ */
