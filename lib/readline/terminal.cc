/* terminal.c -- controlling the terminal with termcap. */

/* Copyright (C) 1996-2017 Free Software Foundation, Inc.

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

#include "history.hh"
#include "readline.hh"
#include "rlprivate.hh"

#include <fcntl.h>
#include <sys/types.h>
#if defined(HAVE_SYS_FILE_H)
#include <sys/file.h>
#endif /* HAVE_SYS_FILE_H */

#include <clocale>

#include "rltty.hh"

#if defined(HAVE_SYS_IOCTL_H)
#include <sys/ioctl.h> /* include for declaration of ioctl */
#endif

#include "tcap.hh"

#include "posixstat.hh"

#if defined(__MINGW32__)
#include <wincon.h>
#include <windows.h>

static void _win_get_screensize (int *, int *);
#endif

#if defined(__EMX__)
static void _emx_get_screensize (int *, int *);
#endif

namespace readline
{

/* **************************************************************** */
/*								    */
/*			Terminal and Termcap			    */
/*								    */
/* **************************************************************** */

#if !defined(__linux__) && !defined(NCURSES_VERSION)
// Note: for C++, this should always be extern "C" if not declared already
extern "C" char PC, *BC, *UP;
#endif /* !__linux__ && !NCURSES_VERSION */

/* It's not clear how HPUX is so broken here. */
#ifdef TGETENT_BROKEN
#define TGETENT_SUCCESS 0
#else
#define TGETENT_SUCCESS 1
#endif
#ifdef TGETFLAG_BROKEN
#define TGETFLAG_SUCCESS 0
#else
#define TGETFLAG_SUCCESS 1
#endif
#define TGETFLAG(cap) (::tgetflag (cap) == TGETFLAG_SUCCESS)

/* Get readline's idea of the screen size.  TTY is a file descriptor open
   to the terminal.  If IGNORE_ENV is true, we do not pay attention to the
   values of $LINES and $COLUMNS.  The tests for TERM_STRING_BUFFER being
   non-null serve to check whether or not we have initialized termcap. */
void
Readline::_rl_get_screen_size (int tty, bool ignore_env)
{
  char *ss;
#if defined(TIOCGWINSZ)
  struct winsize window_size;
#endif /* TIOCGWINSZ */
  unsigned int wr = 0, wc = 0;
#if defined(TIOCGWINSZ)
  if (::ioctl (tty, TIOCGWINSZ, &window_size) == 0)
    {
      wc = window_size.ws_col;
      wr = window_size.ws_row;
    }
#endif /* TIOCGWINSZ */

#if defined(__EMX__)
  _emx_get_screensize (&wc, &wr);
#elif defined(__MINGW32__)
  _win_get_screensize (&wc, &wr);
#endif

  if (ignore_env || !rl_prefer_env_winsize)
    {
      _rl_screenwidth = wc;
      _rl_screenheight = wr;
    }
  else
    _rl_screenwidth = _rl_screenheight = 0;

  /* Environment variable COLUMNS overrides setting of "co" if IGNORE_ENV
     is unset.  If we prefer the environment, check it first before
     assigning the value returned by the kernel. */
  if (_rl_screenwidth == 0)
    {
      if (!ignore_env && (ss = sh_get_env_value ("COLUMNS")))
        _rl_screenwidth = static_cast<unsigned int> (std::atoi (ss));

      if (_rl_screenwidth == 0)
        _rl_screenwidth = wc;

      if (_rl_screenwidth == 0 && term_string_buffer)
        _rl_screenwidth = static_cast<unsigned int> (::tgetnum ("co"));
    }

  /* Environment variable LINES overrides setting of "li" if IGNORE_ENV
     is unset. */
  if (_rl_screenheight == 0)
    {
      if (!ignore_env && (ss = sh_get_env_value ("LINES")))
        _rl_screenheight = static_cast<unsigned int> (std::atoi (ss));

      if (_rl_screenheight == 0)
        _rl_screenheight = wr;

      if (_rl_screenheight == 0 && term_string_buffer)
        _rl_screenheight = static_cast<unsigned int> (::tgetnum ("li"));
    }

  /* If all else fails, default to 80x24 terminal. */
  if (static_cast<int> (_rl_screenwidth) <= 1) // tgetnum() may return -1
    _rl_screenwidth = 80;

  if (static_cast<int> (_rl_screenheight) <= 0) // tgetnum() may return -1
    _rl_screenheight = 24;

  /* If we're being compiled as part of bash, set the environment
     variables $LINES and $COLUMNS to new values.  Otherwise, just
     do a pair of putenv () or setenv () calls. */
  if (rl_change_environment)
    sh_set_lines_and_columns (_rl_screenheight, _rl_screenwidth);

  if (_rl_term_autowrap == 0)
    _rl_screenwidth--;

  _rl_screenchars = _rl_screenwidth * _rl_screenheight;
}

void
Readline::_rl_set_screen_size (unsigned int rows, unsigned int cols)
{
  if (_rl_term_autowrap == -1)
    _rl_init_terminal_io (rl_terminal_name);

  if (rows > 0)
    _rl_screenheight = rows;
  if (cols > 0)
    {
      _rl_screenwidth = cols;

      if (!_rl_term_autowrap)
        _rl_screenwidth--;
    }

  if (rows > 0 || cols > 0)
    _rl_screenchars = _rl_screenwidth * _rl_screenheight;
}

void
Readline::_tc_strings_init ()
{
  std::vector<_tc_string> &tcs = _tc_strings;

  // Note: keep this sorted in case-sensitive order, for binary searching.
  tcs.push_back (_tc_string ("@7", &_rl_term_at7));
  tcs.push_back (_tc_string ("DC", &_rl_term_DC));
  tcs.push_back (_tc_string ("E3", &_rl_term_clrscroll));
  tcs.push_back (_tc_string ("IC", &_rl_term_IC));
  tcs.push_back (_tc_string ("ce", &_rl_term_clreol));
  tcs.push_back (_tc_string ("cl", &_rl_term_clrpag));
  tcs.push_back (_tc_string ("cr", &_rl_term_cr));
  tcs.push_back (_tc_string ("dc", &_rl_term_dc));
  tcs.push_back (_tc_string ("ei", &_rl_term_ei));
  tcs.push_back (_tc_string ("ic", &_rl_term_ic));
  tcs.push_back (_tc_string ("im", &_rl_term_im));
  tcs.push_back (_tc_string ("kD", &_rl_term_kD)); // delete
  tcs.push_back (_tc_string ("kH", &_rl_term_kH)); // home down ??
  tcs.push_back (_tc_string ("kI", &_rl_term_kI)); // insert
  tcs.push_back (_tc_string ("kd", &_rl_term_kd));
  tcs.push_back (_tc_string ("ke", &_rl_term_ke)); // end keypad mode
  tcs.push_back (_tc_string ("kh", &_rl_term_kh)); // home
  tcs.push_back (_tc_string ("kl", &_rl_term_kl));
  tcs.push_back (_tc_string ("kr", &_rl_term_kr));
  tcs.push_back (_tc_string ("ks", &_rl_term_ks)); // start keypad mode
  tcs.push_back (_tc_string ("ku", &_rl_term_ku));
  tcs.push_back (_tc_string ("le", &_rl_term_backspace));
  tcs.push_back (_tc_string ("mm", &_rl_term_mm));
  tcs.push_back (_tc_string ("mo", &_rl_term_mo));
  tcs.push_back (_tc_string ("nd", &_rl_term_forward_char));
  tcs.push_back (_tc_string ("pc", &_rl_term_pc));
  tcs.push_back (_tc_string ("se", &_rl_term_se));
  tcs.push_back (_tc_string ("so", &_rl_term_so));
  tcs.push_back (_tc_string ("up", &_rl_term_up));
  tcs.push_back (_tc_string ("vb", &_rl_visible_bell));
  tcs.push_back (_tc_string ("ve", &_rl_term_ve));
  tcs.push_back (_tc_string ("vs", &_rl_term_vs));
}

/* Read the desired terminal capability strings into BP.  The capabilities
   are described in the TC_STRINGS table. */
void
Readline::get_term_capabilities (char **bp)
{
  if (!tcap_initialized)
    _tc_strings_init ();

  for (std::vector<_tc_string>::iterator it = _tc_strings.begin ();
       it != _tc_strings.end (); ++it)
    *((*it).tc_value) = ::tgetstr (const_cast<char *> ((*it).tc_var), bp);

  tcap_initialized = true;
}

void
Readline::_rl_init_terminal_io (const char *terminal_name)
{
  const char *term;
  char *buffer;
  int tty, tgetent_ret;

  term = terminal_name ? terminal_name : sh_get_env_value ("TERM");
  _rl_term_clrpag = _rl_term_cr = _rl_term_clreol = _rl_term_clrscroll
      = nullptr;
  tty = rl_instream ? ::fileno (rl_instream) : 0;

  if (term == nullptr)
    term = "dumb";

  bool dumbterm = STREQ (term, "dumb");

  /* I've separated this out for later work on not calling tgetent at all
     if the calling application has supplied a custom redisplay function,
     (and possibly if the application has supplied a custom input function). */
  if (rl_redisplay_function != &Readline::rl_redisplay)
    {
      tgetent_ret = -1;
    }
  else
    {
      if (term_string_buffer == nullptr)
        term_string_buffer = new char[2032];

      if (term_buffer == nullptr)
        term_buffer = new char[4080];

      buffer = term_string_buffer;

      tgetent_ret = ::tgetent (term_buffer, term);
    }

  if (tgetent_ret != TGETENT_SUCCESS)
    {
      delete[] term_string_buffer;
      delete[] term_buffer;
      buffer = term_buffer = term_string_buffer = nullptr;

      _rl_term_autowrap = 0; /* used by _rl_get_screen_size */

      /* Allow calling application to set default height and width, using
         rl_set_screen_size */
      if (_rl_screenwidth <= 0 || _rl_screenheight <= 0)
        {
#if defined(__EMX__)
          _emx_get_screensize (&_rl_screenwidth, &_rl_screenheight);
          _rl_screenwidth--;
#else  /* !__EMX__ */
          _rl_get_screen_size (tty, 0);
#endif /* !__EMX__ */
        }

      /* Defaults. */
      if (_rl_screenwidth <= 0 || _rl_screenheight <= 0)
        {
          _rl_screenwidth = 79;
          _rl_screenheight = 24;
        }

      /* Everything below here is used by the redisplay code (tputs). */
      _rl_screenchars = _rl_screenwidth * _rl_screenheight;
      _rl_term_cr = "\r";
      _rl_term_im = _rl_term_ei = _rl_term_ic = _rl_term_IC = nullptr;
      _rl_term_up = _rl_term_dc = _rl_term_DC = _rl_visible_bell = nullptr;
      _rl_term_ku = _rl_term_kd = _rl_term_kl = _rl_term_kr = nullptr;
      _rl_term_kh = _rl_term_kH = _rl_term_kI = _rl_term_kD = nullptr;
      _rl_term_ks = _rl_term_ke = _rl_term_at7 = nullptr;
      _rl_term_mm = _rl_term_mo = nullptr;
      _rl_term_ve = _rl_term_vs = nullptr;
      _rl_term_forward_char = nullptr;
      _rl_term_so = _rl_term_se = nullptr;
      _rl_terminal_can_insert = term_has_meta = false;

      /* Assume generic unknown terminal can't handle the enable/disable
         escape sequences */
      _rl_enable_bracketed_paste = false;

      /* Reasonable defaults for tgoto().  Readline currently only uses
         tgoto if _rl_term_IC or _rl_term_DC is defined, but just in case we
         change that later... */
      PC = '\0';
      BC = const_cast<char *> (_rl_term_backspace = "\b");
      UP = const_cast<char *> (_rl_term_up);

      return;
    }

  get_term_capabilities (&buffer);

  /* Set up the variables that the termcap library expects the application
     to provide. */
  PC = _rl_term_pc ? *_rl_term_pc : 0;
  BC = const_cast<char *> (_rl_term_backspace);
  UP = const_cast<char *> (_rl_term_up);

  if (_rl_term_cr == nullptr)
    _rl_term_cr = "\r";

  _rl_term_autowrap = TGETFLAG ("am") && TGETFLAG ("xn");

  /* Allow calling application to set default height and width, using
     rl_set_screen_size */
  if (_rl_screenwidth <= 0 || _rl_screenheight <= 0)
    _rl_get_screen_size (tty, 0);

  /* "An application program can assume that the terminal can do
      character insertion if *any one of* the capabilities `IC',
      `im', `ic' or `ip' is provided."  But we can't do anything if
      only `ip' is provided, so... */
  _rl_terminal_can_insert = (_rl_term_IC || _rl_term_im || _rl_term_ic);

  /* Check to see if this terminal has a meta key and clear the capability
     variables if there is none. */
  term_has_meta = TGETFLAG ("km");
  if (!term_has_meta)
    _rl_term_mm = _rl_term_mo = nullptr;

  /* Attempt to find and bind the arrow keys.  Do not override already
     bound keys in an overzealous attempt, however. */

  bind_termcap_arrow_keys (emacs_standard_keymap ());

#if defined(VI_MODE)
  bind_termcap_arrow_keys (vi_movement_keymap ());
  bind_termcap_arrow_keys (vi_insertion_keymap ());
#endif /* VI_MODE */

  /* There's no way to determine whether or not a given terminal supports
     bracketed paste mode, so we assume a terminal named "dumb" does not. */
  if (dumbterm)
    _rl_enable_bracketed_paste = false;
}

/* Bind the arrow key sequences from the termcap description in MAP. */
void
Readline::bind_termcap_arrow_keys (Keymap map)
{
  Keymap xkeymap;

  xkeymap = _rl_keymap;
  _rl_keymap = map;

  rl_bind_keyseq_if_unbound (_rl_term_ku, &Readline::rl_get_previous_history);
  rl_bind_keyseq_if_unbound (_rl_term_kd, &Readline::rl_get_next_history);
  rl_bind_keyseq_if_unbound (_rl_term_kr, &Readline::rl_forward_char);
  rl_bind_keyseq_if_unbound (_rl_term_kl, &Readline::rl_backward_char);

  rl_bind_keyseq_if_unbound (_rl_term_kh,
                             &Readline::rl_beg_of_line); /* Home */
  rl_bind_keyseq_if_unbound (_rl_term_at7,
                             &Readline::rl_end_of_line); /* End */

  rl_bind_keyseq_if_unbound (_rl_term_kD, &Readline::rl_delete);
  rl_bind_keyseq_if_unbound (_rl_term_kI,
                             &Readline::rl_overwrite_mode); /* Insert */

  _rl_keymap = xkeymap;
}

const char *
Readline::rl_get_termcap (const char *cap)
{
  if (!tcap_initialized)
    return nullptr;

  size_t low = 0;
  size_t high = _tc_strings.size () - 1;
  size_t mid = high >> 1;
  int direction;

  while ((direction = std::strcmp (cap, _tc_strings[mid].tc_var)))
    {
      if (direction > 0)
        if (mid == high)
          return nullptr;
        else
          low = mid + 1;
      else if (mid == 0)
        return nullptr;
      else
        high = mid - 1;

      mid = (low + high) >> 1;
    }

  return direction ? nullptr : *(_tc_strings[mid].tc_value);
}

/* An extern "C" callback function for the use of tputs () */
int
_rl_output_character_function (int c)
{
  return std::putc (c, _rl_out_stream);
}

/* Ring the terminal bell. */
int
Readline::rl_ding ()
{
  if (_rl_echoing_p)
    {
      switch (_rl_bell_preference)
        {
        case NO_BELL:
          break;
        case VISIBLE_BELL:
          if (_rl_visible_bell)
            {
              ::tputs (_rl_visible_bell, 1, &_rl_output_character_function);
              break;
            }
          /* FALLTHROUGH */
          __attribute__ ((fallthrough));
        case AUDIBLE_BELL:
          std::fprintf (stderr, "\007");
          std::fflush (stderr);
          break;
        }
      return 0;
    }
  return -1;
}

} // namespace readline
