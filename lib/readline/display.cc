/* display.c -- readline redisplay facility. */

/* Copyright (C) 1987-2020 Free Software Foundation, Inc.

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

#include <sys/types.h>

#include "posixstat.hh"

/* Termcap library stuff. */
#include "tcap.hh"

namespace readline
{

/* Backwards-compatible names. */
#define inv_lbreaks (line_state_invisible->lbreaks)
#define vis_lbreaks (line_state_visible->lbreaks)

#define visible_line (line_state_visible->line)
#define vis_face (line_state_visible->lface)
#define invisible_line (line_state_invisible->line)
#define inv_face (line_state_invisible->lface)

/* Heuristic used to decide whether it is faster to move from CUR to NEW
   by backing up or outputting a carriage return and moving forward.  CUR
   and NEW are either both buffer positions or absolute screen positions. */
static inline bool
CR_FASTER (unsigned int newcol, unsigned int curcol)
{
  return (newcol + 1) < (curcol - newcol);
}

/* _rl_last_c_pos is an absolute cursor position in multibyte locales and a
   buffer index in others.  This macro is used when deciding whether the
   current cursor position is in the middle of a prompt string containing
   invisible characters.  XXX - might need to take `modmark' into account. */
/* XXX - only valid when tested against _rl_last_c_pos; buffer indices need
   to use prompt_last_invisible directly. */
#define PROMPT_ENDING_INDEX                                                   \
  ((MB_CUR_MAX > 1 && !rl_byte_oriented) ? local_prompt.physical_chars        \
                                         : local_prompt.last_invisible + 1)

/* **************************************************************** */
/*								    */
/*			Display stuff				    */
/*								    */
/* **************************************************************** */

/* This is the stuff that is hard for me.  I never seem to write good
   display routines in C.  Let's see how I do this time. */

/* (PWP) Well... Good for a simple line updater, but totally ignores
   the problems of input lines longer than the screen width.

   update_line and the code that calls it makes a multiple line,
   automatically wrapping line update.  Careful attention needs
   to be paid to the vertical position variables. */

/* Keep two buffers; one which reflects the current contents of the
   screen, and the other to draw what we think the new contents should
   be.  Then compare the buffers, and make whatever changes to the
   screen itself that we should.  Finally, make the buffer that we
   just drew into be the one which reflects the current contents of the
   screen, and place the cursor where it belongs.

   Commands that want to can fix the display themselves, and then let
   this function know that the display has been fixed by setting the
   RL_DISPLAY_FIXED variable.  This is good for efficiency. */

/* Return a string indicating the editing mode, for use in the prompt. */

const char *
Readline::prompt_modestr (unsigned int *lenp)
{
  if (rl_editing_mode == emacs_mode)
    {
      if (lenp)
        *lenp = _rl_emacs_mode_str ? _rl_emacs_modestr_len
                                   : RL_EMACS_MODESTR_DEFLEN;
      return _rl_emacs_mode_str ? _rl_emacs_mode_str
                                : RL_EMACS_MODESTR_DEFAULT;
    }
  else if (_rl_keymap == vi_insertion_keymap ())
    {
      if (lenp)
        *lenp = _rl_vi_ins_mode_str ? _rl_vi_ins_modestr_len
                                    : RL_VI_INS_MODESTR_DEFLEN;
      return _rl_vi_ins_mode_str
                 ? _rl_vi_ins_mode_str
                 : RL_VI_INS_MODESTR_DEFAULT; /* vi insert mode */
    }
  else
    {
      if (lenp)
        *lenp = _rl_vi_cmd_mode_str ? _rl_vi_cmd_modestr_len
                                    : RL_VI_CMD_MODESTR_DEFLEN;
      return _rl_vi_cmd_mode_str
                 ? _rl_vi_cmd_mode_str
                 : RL_VI_CMD_MODESTR_DEFAULT; /* vi command mode */
    }
}

/* Expand the prompt string S and return the number of visible
   characters in *LP, if LP is not null.  This is currently more-or-less
   a placeholder for expansion.  LIP, if non-null is a place to store the
   index of the last invisible character in the returned string. NIFLP,
   if non-zero, is a place to store the number of invisible characters in
   the first prompt line.  The previous are used as byte counts -- indexes
   into a character buffer.  *VLP gets the number of physical characters in
   the expanded prompt (visible length) */

/* Current implementation:
        \001 (^A) start non-visible characters
        \002 (^B) end non-visible characters
   all characters except \001 and \002 (following a \001) are copied to
   the returned string; all characters except those between \001 and
   \002 are assumed to be `visible'. */

/* Possible values for FLAGS:
        PMT_MULTILINE	caller indicates that this is part of a multiline
   prompt
*/

/* This approximates the number of lines the prompt will take when displayed */
#define APPROX_DIV(n, d) (((n) < (d)) ? 1 : ((n) / (d)) + 1)

std::string
Readline::expand_prompt (const std::string &pmt, expand_prompt_flags flags,
                         unsigned int *lp, unsigned int *lip,
                         unsigned int *niflp, unsigned int *vlp)
{
  unsigned int mlen = 0;
  std::string nprompt;

  /* We only expand the mode string for the last line of a multiline prompt
     (a prompt with embedded newlines). */
  const char *ms = (((pmt == rl_prompt) ^ (flags & PMT_MULTILINE))
                    && _rl_show_mode_in_prompt)
                       ? prompt_modestr (&mlen)
                       : nullptr;
  if (ms)
    {
      nprompt = ms;
      nprompt += pmt;
    }
  else
    nprompt = pmt;

  size_t mb_cur_max = MB_CUR_MAX;

  if (_rl_screenwidth == 0)
    _rl_get_screen_size (0, 0); /* avoid division by zero */

  /* Short-circuit if we can.  We can do this if we are treating the prompt as
     a sequence of bytes and there are no invisible characters in the prompt
     to deal with. Since we populate local_prompt_newlines, we have to run
     through the rest of the function if this prompt looks like it's going to
     be longer than one screen line. */
  if ((mb_cur_max <= 1 || rl_byte_oriented)
      && nprompt.find (RL_PROMPT_START_IGNORE) == nprompt.npos)
    {
      unsigned int l = static_cast<unsigned int> (nprompt.size ());
      if (l < (_rl_screenwidth > 0 ? _rl_screenwidth : 80))
        {
          if (lp)
            *lp = l;
          if (lip)
            *lip = 0;
          if (niflp)
            *niflp = 0;
          if (vlp)
            *vlp = l;

          local_prompt.newlines.resize (2);
          local_prompt.newlines[0] = 0;
          local_prompt.newlines[1] = static_cast<unsigned int> (-1);

          return nprompt;
        }
    }

  unsigned int l = static_cast<unsigned int> (nprompt.size ());
  std::string ret;

  /* Guess at how many screen lines the prompt will take to size the array that
     keeps track of where the line wraps happen */
  unsigned int newlines_guess = (_rl_screenwidth > 0)
                                    ? APPROX_DIV (l, _rl_screenwidth)
                                    : APPROX_DIV (l, 80);

  local_prompt.newlines.resize (newlines_guess + 1);
  unsigned int newlines = 0;
  local_prompt.newlines[0] = 0;

  for (unsigned int i = 1; i <= newlines_guess; i++)
    local_prompt.newlines[i] = static_cast<unsigned int> (-1);

  unsigned int rl, last, ninvis, invfl, ind, pind, physchars;
  std::string::iterator p; // source iterator
  std::string::iterator igstart;
  bool ignoring = false, invflset = false;

  rl = physchars = 0;       /* mode string now part of nprompt */
  invfl = 0;                /* invisible chars in first line of prompt */
  igstart = nprompt.end (); /* we're not ignoring any characters yet */

  for (last = ninvis = 0, p = nprompt.begin (); p != nprompt.end (); ++p)
    {
      /* This code strips the invisible character string markers
         RL_PROMPT_START_IGNORE and RL_PROMPT_END_IGNORE */
      if (!ignoring
          && *p == RL_PROMPT_START_IGNORE) /* XXX - check ignoring? */
        {
          ignoring = true;
          igstart = p;
          continue;
        }
      else if (ignoring && *p == RL_PROMPT_END_IGNORE)
        {
          ignoring = false;
          if (p != (igstart + 1))
            last = static_cast<unsigned int> (ret.size ()) - 1;
          continue;
        }
      else
        {
#if defined(HANDLE_MULTIBYTE)
          if (mb_cur_max > 1 && !rl_byte_oriented)
            {
              pind = static_cast<unsigned int> (p - nprompt.begin ());
              ind = _rl_find_next_mbchar (nprompt, pind, 1, MB_FIND_NONZERO);
              l = ind - pind;
              while (l--)
                ret += *p++;
              if (!ignoring)
                {
                  /* rl ends up being assigned to prompt_visible_length,
                     which is the number of characters in the buffer that
                     contribute to characters on the screen, which might
                     not be the same as the number of physical characters
                     on the screen in the presence of multibyte characters */
                  rl += ind - pind;
                  physchars += _rl_col_width (nprompt.data (), pind, ind, 0);
                }
              else
                ninvis += ind - pind;
              p--; /* compensate for later increment */
            }
          else
#endif
            {
              ret += *p;
              if (!ignoring)
                {
                  rl++; /* visible length byte counter */
                  physchars++;
                }
              else
                ninvis++; /* invisible chars byte counter */
            }

          if (!invflset && physchars >= _rl_screenwidth)
            {
              invfl = ninvis;
              invflset = true;
            }

          unsigned int bound;
          if (physchars >= (bound = (newlines + 1) * _rl_screenwidth)
              && local_prompt.newlines[newlines + 1]
                     == static_cast<unsigned int> (-1))
            {
              unsigned int new_;
              if (physchars > bound) /* should rarely happen */
                {
#if defined(HANDLE_MULTIBYTE)
                  if (mb_cur_max > 1 && !rl_byte_oriented)
                    new_ = _rl_find_prev_mbchar (
                        ret, static_cast<unsigned int> (ret.size ()),
                        MB_FIND_ANY);
                  else
#endif
                    new_ = static_cast<unsigned int> (ret.size ())
                           - (physchars - bound);
                }
              else
                new_ = static_cast<unsigned int> (ret.size ());
              local_prompt.newlines[++newlines] = new_;
            }
        }
    }

  if (rl < _rl_screenwidth)
    invfl = ninvis;

  if (lp)
    *lp = rl;
  if (lip)
    *lip = last;
  if (niflp)
    *niflp = invfl;
  if (vlp)
    *vlp = physchars;

  return ret;
}

/*
 * Expand the prompt string into the various display components, if
 * necessary.
 *
 * local_prompt = expanded last line of string in rl_display_prompt
 *		  (portion after the final newline)
 * local_prompt_prefix = portion before last newline of rl_display_prompt,
 *			 expanded via expand_prompt
 * prompt_visible_length = number of visible characters in local_prompt
 * prompt_prefix_length = number of visible characters in local_prompt_prefix
 *
 * It also tries to keep track of the number of invisible characters in the
 * prompt string, and where they are.
 *
 * This function is called once per call to readline().  It may also be
 * called arbitrarily to expand the primary prompt.
 *
 * The return value is the number of visible characters on the last line
 * of the (possibly multi-line) prompt.  In this case, multi-line means
 * there are embedded newlines in the prompt string itself, not that the
 * number of physical characters exceeds the screen width and the prompt
 * wraps.
 */
unsigned int
Readline::rl_expand_prompt (char *prompt)
{
  /* Clear out any saved values. */
  local_prompt.prompt.clear ();
  local_prompt.prefix.clear ();

  local_prompt.last_invisible = local_prompt.invis_chars_first_line = 0;
  local_prompt.visible_length = local_prompt.physical_chars = 0;

  if (prompt == nullptr || *prompt == '\0')
    return 0;

  char *p = std::strrchr (prompt, '\n');
  if (p == nullptr)
    {
      /* The prompt is only one logical line, though it might wrap. */
      local_prompt.prompt = expand_prompt (
          prompt, PMT_NONE, &local_prompt.visible_length,
          &local_prompt.last_invisible, &local_prompt.invis_chars_first_line,
          &local_prompt.physical_chars);
      local_prompt.prefix.clear ();
      return local_prompt.visible_length;
    }
  else
    {
      /* The prompt spans multiple lines. */
      char *t = ++p;
      char c = *t;
      *t = '\0';
      /* The portion of the prompt string up to and including the
         final newline is now null-terminated. */
      local_prompt.prefix
          = expand_prompt (prompt, PMT_MULTILINE, &local_prompt.prefix_length,
                           nullptr, nullptr, nullptr);
      *t = c;

      local_prompt.prefix = expand_prompt (
          p, PMT_MULTILINE, &local_prompt.visible_length,
          &local_prompt.last_invisible, &local_prompt.invis_chars_first_line,
          &local_prompt.physical_chars);
      return local_prompt.prefix_length;
    }
}

/* Basic redisplay algorithm.  See comments inline. */
void
Readline::rl_redisplay ()
{
  unsigned int in, linenum, cursor_linenum;
  unsigned int out;
  unsigned int inv_botlin, lb_botlin, lb_linenum;
  unsigned int o_cpos, lpos;
  unsigned int newlines;
  char cur_face;
  unsigned int hl_begin, hl_end;
  size_t mb_cur_max = MB_CUR_MAX;
#if defined(HANDLE_MULTIBYTE)
  wchar_t wc = 0;
  size_t wc_bytes;
  unsigned int wc_width = 0;
  mbstate_t ps;
  unsigned int _rl_wrapped_multicolumn = 0;
#endif

  if (!_rl_echoing_p)
    return;

  /* Block keyboard interrupts because this function manipulates global
     data structures. */
  _rl_block_sigint ();
  RL_SETSTATE (RL_STATE_REDISPLAYING);

  cur_face = FACE_NORMAL;
  /* Can turn this into an array for multiple highlighted objects in addition
     to the region */
  hl_begin = hl_end = static_cast<unsigned int> (-1);

  if (rl_mark_active_p ())
    set_active_region (&hl_begin,
                       &hl_end); // XXX: should we check for failure?

  if (!rl_display_prompt)
    rl_display_prompt = "";

  if (!line_structures_initialized)
    {
      line_structures_initialized = true;
      rl_on_new_line ();
    }

  /* Enable horizontal scrolling automatically for terminals of height 1
     where wrapping lines doesn't work. Disable it as soon as the terminal
     height is increased again if it was automatically enabled. */
  if (_rl_screenheight <= 1)
    {
      if (!_rl_horizontal_scroll_mode)
        horizontal_scrolling_autoset = true;

      _rl_horizontal_scroll_mode = true;
    }
  else if (horizontal_scrolling_autoset)
    _rl_horizontal_scroll_mode = false;

  /* Draw the line into the buffer. */
  cpos_buffer_position = static_cast<unsigned int> (-1);

  prompt_multibyte_chars
      = local_prompt.visible_length - local_prompt.physical_chars;

  out = inv_botlin = 0;

  /* Mark the line as modified or not.  We only do this for history
     lines. */
  modmark = false;
  if (_rl_mark_modified_lines && current_history () && rl_undo_list)
    {
      invis_addc ('*', cur_face);
      modmark = true;
    }

  /* If someone thought that the redisplay was handled, but the currently
     visible line has a different modification state than the one about
     to become visible, then correct the caller's misconception. */
  if (visible_line[0] != invisible_line[0])
    rl_display_fixed = false;

  /* If the prompt to be displayed is the `primary' readline prompt (the
     one passed to readline()), use the values we have already expanded.
     If not, use what's already in rl_display_prompt.  WRAP_OFFSET is the
     number of non-visible characters (bytes) in the prompt string. */
  /* This is where we output the characters in the prompt before the last
     newline, if any.  If there aren't any embedded newlines, we don't
     write anything. Copy the last line of the prompt string into the line in
     any case */
  if (rl_display_prompt == rl_prompt || !local_prompt.prompt.empty ())
    {
      if (!local_prompt.prefix.empty () && forced_display)
        _rl_output_some_chars (
            local_prompt.prefix.data (),
            static_cast<unsigned int> (local_prompt.prefix.size ()));

      if (!local_prompt.prompt.empty ())
        invis_adds (local_prompt.prompt, cur_face);

      wrap_offset = static_cast<unsigned int> (local_prompt.prompt.size ())
                    - local_prompt.visible_length;
    }
  else
    {
      unsigned int pmtlen;
      const char *prompt_this_line = std::strrchr (rl_display_prompt, '\n');
      if (!prompt_this_line)
        prompt_this_line = rl_display_prompt;
      else
        {
          prompt_this_line++;
          pmtlen = static_cast<unsigned int> (
              prompt_this_line - rl_display_prompt); /* temp var */
          if (forced_display)
            {
              _rl_output_some_chars (rl_display_prompt, pmtlen);
              /* Make sure we are at column zero even after a newline,
                 regardless of the state of terminal output processing. */
              if (pmtlen < 2 || prompt_this_line[-2] != '\r')
                cr ();
            }
        }

      local_prompt.physical_chars = pmtlen
          = static_cast<unsigned int> (std::strlen (prompt_this_line));
      invis_adds (prompt_this_line, pmtlen, cur_face);
      wrap_offset = local_prompt.invis_chars_first_line = 0;
    }

  /* inv_lbreaks[i] is where line i starts in the buffer. */
  inv_lbreaks[newlines = 0] = 0;

  /* lpos is a physical cursor position, so it needs to be adjusted by the
     number of invisible characters in the prompt, per line.  We compute
     the line breaks in the prompt string in expand_prompt, taking invisible
     characters into account, and if lpos exceeds the screen width, we copy
     the data in the loop below. */
  lpos = local_prompt.physical_chars + modmark;

#if defined(HANDLE_MULTIBYTE)
  std::memset (line_state_invisible->wrapped_line, 0,
               line_state_invisible->wbsize * sizeof (int));
#endif

  /* prompt_invis_chars_first_line is the number of invisible characters
     (bytes) in the first physical line of the prompt. wrap_offset -
     prompt_invis_chars_first_line is usually the number of invis chars on the
     second (or, more generally, last) line. */

  /* what if lpos is already >= _rl_screenwidth before we start drawing the
     contents of the command line? */
  if (lpos >= _rl_screenwidth)
    {
      unsigned int temp = 0;

      /* first copy the linebreaks array we computed in expand_prompt */
      while (local_prompt.newlines[newlines + 1]
             != static_cast<unsigned int> (-1))
        {
          temp = local_prompt.newlines[newlines + 1];
          inv_lbreaks[++newlines] = temp;
        }

      /* Now set lpos from the last newline */
      if (mb_cur_max > 1 && !rl_byte_oriented && prompt_multibyte_chars > 0)
        lpos = _rl_col_width (
                   local_prompt.prompt.data (), temp,
                   static_cast<unsigned int> (local_prompt.prompt.size ()), 1)
               - (wrap_offset - local_prompt.invis_chars_first_line);
      else
        lpos -= (_rl_screenwidth * newlines);
    }

  prompt_last_screen_line = newlines;

  /* Draw the rest of the line (after the prompt) into invisible_line, keeping
     track of where the cursor is (cpos_buffer_position), the number of the
     line containing the cursor (lb_linenum), the last line number (lb_botlin
     and inv_botlin).
     It maintains an array of line breaks for display (inv_lbreaks).
     This handles expanding tabs for display and displaying meta characters. */
  lb_linenum = 0;
  size_t rl_end = rl_line_buffer.size ();
#if defined(HANDLE_MULTIBYTE)
  in = 0;
  if (mb_cur_max > 1 && !rl_byte_oriented)
    {
      std::memset (&ps, 0, sizeof (mbstate_t));
      if (_rl_utf8locale && UTF8_SINGLEBYTE (rl_line_buffer[0]))
        {
          wc = static_cast<wchar_t> (rl_line_buffer[0]);
          wc_bytes = 1;
        }
      else
        wc_bytes = std::mbrtowc (&wc, rl_line_buffer.c_str (), rl_end, &ps);
    }
  else
    wc_bytes = 1;
  while (in < rl_end)
#else
  for (in = 0; in < rl_end; in++)
#endif
    {
      if (in == hl_begin)
        cur_face = FACE_STANDOUT;
      else if (in == hl_end)
        cur_face = FACE_NORMAL;

      unsigned char c = static_cast<unsigned char> (rl_line_buffer[in]);

#if defined(HANDLE_MULTIBYTE)
      if (mb_cur_max > 1 && !rl_byte_oriented)
        {
          if (MB_INVALIDCH (wc_bytes))
            {
              /* Byte sequence is invalid or shortened.  Assume that the
                 first byte represents a character. */
              wc_bytes = 1;
              /* Assume that a character occupies a single column. */
              wc_width = 1;
              std::memset (&ps, 0, sizeof (mbstate_t));
            }
          else if (MB_NULLWCH (wc_bytes))
            break; /* Found '\0' */
          else
            {
              int temp = WCWIDTH (wc);
              wc_width = (temp >= 0) ? static_cast<unsigned int> (temp) : 1;
            }
        }
#endif

      if (in == rl_point)
        {
          cpos_buffer_position = out;
          lb_linenum = newlines;
        }

#if defined(HANDLE_MULTIBYTE)
      if (META_CHAR (c) && _rl_output_meta_chars == 0) /* XXX - clean up */
#else
      if (META_CHAR (c))
#endif
        {
          if (_rl_output_meta_chars == 0)
            {
              char obuf[5];
              unsigned int olen;

              olen
                  = static_cast<unsigned int> (std::sprintf (obuf, "\\%o", c));

              if (lpos + olen >= _rl_screenwidth)
                {
                  unsigned int temp = _rl_screenwidth - lpos;
                  inv_lbreaks[++newlines] = out + temp;
#if defined(HANDLE_MULTIBYTE)
                  line_state_invisible->wrapped_line[newlines]
                      = _rl_wrapped_multicolumn;
#endif
                  lpos = olen - temp;
                }
              else
                lpos += olen;

              for (unsigned int temp = 0; temp < olen; temp++)
                {
                  invis_addc (obuf[temp], cur_face);
                }
            }
          else
            {
              invis_addc (static_cast<char> (c), cur_face);
            }
        }
#if defined(DISPLAY_TABS)
      else if (c == '\t')
        {
          unsigned int newout = out + 8 - lpos % 8;
          unsigned int temp = newout - out;
          if (lpos + temp >= _rl_screenwidth)
            {
              unsigned int temp2 = _rl_screenwidth - lpos;
              inv_lbreaks[++newlines] = out + temp2;
#if defined(HANDLE_MULTIBYTE)
              line_state_invisible->wrapped_line[newlines]
                  = _rl_wrapped_multicolumn;
#endif
              lpos = temp - temp2;
              while (out < newout)
                invis_addc (' ', cur_face);
            }
          else
            {
              while (out < newout)
                invis_addc (' ', cur_face);
              lpos += temp;
            }
        }
#endif
      else if (c == '\n' && !_rl_horizontal_scroll_mode && _rl_term_up
               && *_rl_term_up)
        {
          invis_addc ('\0', cur_face);
          inv_lbreaks[++newlines] = out;
#if defined(HANDLE_MULTIBYTE)
          line_state_invisible->wrapped_line[newlines]
              = _rl_wrapped_multicolumn;
#endif
          lpos = 0;
        }
      else if (CTRL_CHAR (c) || c == RUBOUT)
        {
          invis_addc ('^', cur_face);
          invis_addc (CTRL_CHAR (c) ? UNCTRL (c) : '?', cur_face);
        }
      else
        {
#if defined(HANDLE_MULTIBYTE)
          if (mb_cur_max > 1 && !rl_byte_oriented)
            {
              _rl_wrapped_multicolumn = 0;

              if (_rl_screenwidth < lpos + wc_width)
                for (unsigned int i = lpos; i < _rl_screenwidth; i++)
                  {
                    /* The space will be removed in update_line() */
                    invis_addc (' ', cur_face);
                    _rl_wrapped_multicolumn++;
                  }
              if (in == rl_point)
                {
                  cpos_buffer_position = out;
                  lb_linenum = newlines;
                }
              for (unsigned int i = in;
                   i < in + static_cast<unsigned int> (wc_bytes); i++)
                invis_addc (rl_line_buffer[i], cur_face);
            }
          else
            {
              invis_addc (static_cast<char> (c), cur_face);
            }
#else
          invis_addc (&out, static_cast<char> (c), cur_face);
#endif
        }

#if defined(HANDLE_MULTIBYTE)
      if (mb_cur_max > 1 && !rl_byte_oriented)
        {
          in += wc_bytes;
          if (_rl_utf8locale && UTF8_SINGLEBYTE (rl_line_buffer[in]))
            {
              wc = static_cast<wchar_t> (rl_line_buffer[in]);
              wc_bytes = 1;
              std::memset (&ps, 0, sizeof (mbstate_t)); /* re-init state */
            }
          else
            wc_bytes
                = std::mbrtowc (&wc, &rl_line_buffer[in], rl_end - in, &ps);
        }
      else
        in++;
#endif
    }

  if (cpos_buffer_position == static_cast<unsigned int> (-1))
    {
      cpos_buffer_position = out;
      lb_linenum = newlines;
    }

  /* If we are switching from one line to multiple wrapped lines, we don't
     want to do a dumb update (or we want to make it smarter). */
  if (_rl_quick_redisplay && newlines > 0)
    _rl_quick_redisplay = 0;

  inv_botlin = lb_botlin = _rl_inv_botlin = newlines;

  inv_lbreaks[newlines + 1] = out;

#if defined(HANDLE_MULTIBYTE)
  /* This should be 0 anyway */
  line_state_invisible->wrapped_line[newlines + 1] = _rl_wrapped_multicolumn;
#endif

  cursor_linenum = lb_linenum;

  /* CPOS_BUFFER_POSITION == position in buffer where cursor should be placed.
     CURSOR_LINENUM == line number where the cursor should be placed. */

  /* PWP: now is when things get a bit hairy.  The visible and invisible
     line buffers are really multiple lines, which would wrap every
     (screenwidth - 1) characters.  Go through each in turn, finding
     the changed region and updating it.  The line order is top to bottom. */

  /* If we can move the cursor up and down, then use multiple lines,
     otherwise, let long lines display in a single terminal line, and
     horizontally scroll it. */
  displaying_prompt_first_line = true;
  if (!_rl_horizontal_scroll_mode && _rl_term_up && *_rl_term_up)
    {
      unsigned int nleft, pos, changed_screen_line, tx;

      if (!rl_display_fixed || forced_display)
        {
          forced_display = false;

          /* If we have more than a screenful of material to display, then
             only display a screenful.  We should display the last screen,
             not the first.  */
          if (out >= _rl_screenchars)
            {
#if defined(HANDLE_MULTIBYTE)
              if (mb_cur_max > 1 && !rl_byte_oriented)
                out = _rl_find_prev_mbchar (invisible_line, _rl_screenchars,
                                            MB_FIND_ANY);
              else
#endif
                out = _rl_screenchars - 1;
            }

            /* The first line is at character position 0 in the buffer.  The
               second and subsequent lines start at inv_lbreaks[N], offset by
               OFFSET (which has already been calculated above).  */

#define INVIS_FIRST()                                                         \
  (local_prompt.physical_chars > _rl_screenwidth                              \
       ? local_prompt.invis_chars_first_line                                  \
       : wrap_offset)
#define WRAP_OFFSET(line, offset)                                             \
  ((line == 0) ? (offset ? INVIS_FIRST () : 0)                                \
               : ((line == prompt_last_screen_line)                           \
                      ? wrap_offset - local_prompt.invis_chars_first_line     \
                      : 0))
#define W_OFFSET(line, offset) ((line) == 0 ? offset : 0)
#define VIS_LLEN(l)                                                           \
  ((l) > _rl_vis_botlin ? 0                                                   \
                        : (vis_lbreaks[static_cast<size_t> (l + 1)]           \
                           - vis_lbreaks[static_cast<size_t> (l)]))
#define INV_LLEN(l)                                                           \
  (inv_lbreaks[static_cast<size_t> (l + 1)]                                   \
   - inv_lbreaks[static_cast<size_t> (l)])
#define VIS_CHARS(line)                                                       \
  (&visible_line[static_cast<size_t> (                                        \
      vis_lbreaks[static_cast<size_t> (line)])])
#define VIS_FACE(line)                                                        \
  (&vis_face[static_cast<size_t> (vis_lbreaks[static_cast<size_t> (line)])])
#define VIS_LINE(line) ((line) > _rl_vis_botlin) ? "" : VIS_CHARS (line)
#define VIS_LINE_FACE(line) ((line) > _rl_vis_botlin) ? "" : VIS_FACE (line)
#define INV_LINE(line)                                                        \
  (&invisible_line[static_cast<size_t> (                                      \
      inv_lbreaks[static_cast<size_t> (line)])])
#define INV_LINE_FACE(line)                                                   \
  (&inv_face[static_cast<size_t> (inv_lbreaks[static_cast<size_t> (line)])])

#define OLD_CPOS_IN_PROMPT()                                                  \
  (!cpos_adjusted && _rl_last_c_pos != o_cpos && _rl_last_c_pos > wrap_offset \
   && o_cpos < local_prompt.last_invisible)

          /* We don't want to highlight anything that's going to be off the top
             of the display; if the current line takes up more than an entire
            screen, just mark the lines that won't be displayed as having a
            `normal' face.
            It's imperfect, but better than display corruption. */
          if (rl_mark_active_p () && inv_botlin > _rl_screenheight)
            {
              unsigned int extra = inv_botlin - _rl_screenheight;
              for (linenum = 0; linenum <= extra; linenum++)
                std::memset (INV_LINE_FACE (linenum), FACE_NORMAL,
                             INV_LLEN (linenum));
            }

          /* For each line in the buffer, do the updating display. */
          for (linenum = 0; linenum <= inv_botlin; linenum++)
            {
              /* This can lead us astray if we execute a program that changes
                 the locale from a non-multibyte to a multibyte one. */
              o_cpos = _rl_last_c_pos;
              cpos_adjusted = 0;
              update_line (const_cast<char *> (VIS_LINE (linenum)),
                           const_cast<char *> (VIS_LINE_FACE (linenum)),
                           INV_LINE (linenum), INV_LINE_FACE (linenum),
                           linenum, VIS_LLEN (linenum), INV_LLEN (linenum),
                           inv_botlin);

              /* update_line potentially changes _rl_last_c_pos, but doesn't
                 take invisible characters into account, since _rl_last_c_pos
                 is an absolute cursor position in a multibyte locale.  We
                 choose to (mostly) compensate for that here, rather than
                 change update_line itself.  There are several cases in which
                 update_line adjusts _rl_last_c_pos itself (so it can pass
                 _rl_move_cursor_relative accurate values); it communicates
                 this back by setting cpos_adjusted.  If we assume that
                 _rl_last_c_pos is correct (an absolute cursor position) each
                 time update_line is called, then we can assume in our
                 calculations that o_cpos does not need to be adjusted by
                 wrap_offset. */
              if (linenum == 0 && (mb_cur_max > 1 && !rl_byte_oriented)
                  && OLD_CPOS_IN_PROMPT ())
                _rl_last_c_pos
                    -= local_prompt
                           .invis_chars_first_line; /* XXX - was wrap_offset */
              else if (!cpos_adjusted && linenum == prompt_last_screen_line
                       && local_prompt.physical_chars > _rl_screenwidth
                       && (mb_cur_max > 1 && !rl_byte_oriented)
                       && _rl_last_c_pos != o_cpos
                       && _rl_last_c_pos
                              > (local_prompt.last_invisible
                                 - (_rl_screenwidth
                                    - local_prompt
                                          .invis_chars_first_line))) /* XXX -
                                                                        rethink
                                                                        this
                                                                        last
                                                                        one */
                /* This assumes that all the invisible characters are split
                   between the first and last lines of the prompt, if the
                   prompt consumes more than two lines. It's usually right */
                /* XXX - not sure this is ever executed */
                _rl_last_c_pos
                    -= (wrap_offset - local_prompt.invis_chars_first_line);

              /* If this is the line with the prompt, we might need to
                 compensate for invisible characters in the new line. Do
                 this only if there is not more than one new line (which
                 implies that we completely overwrite the old visible line)
                 and the new line is shorter than the old.  Make sure we are
                 at the end of the new line before clearing. */
              if (linenum == 0 && inv_botlin == 0 && _rl_last_c_pos == out
                  && (wrap_offset > visible_wrap_offset)
                  && (_rl_last_c_pos < visible_first_line_len))
                {
                  if (mb_cur_max > 1 && !rl_byte_oriented)
                    nleft = _rl_screenwidth - _rl_last_c_pos;
                  else
                    nleft = _rl_screenwidth + wrap_offset - _rl_last_c_pos;
                  if (nleft)
                    _rl_clear_to_eol (nleft);
                }

              /* Since the new first line is now visible, save its length. */
              if (linenum == 0)
                visible_first_line_len
                    = (inv_botlin > 0) ? inv_lbreaks[1] : out - wrap_offset;
            }

          /* We may have deleted some lines.  If so, clear the left over
             blank ones at the bottom out. */
          if (_rl_vis_botlin > inv_botlin)
            {
              char *tt;
              for (; linenum <= _rl_vis_botlin; linenum++)
                {
                  tt = VIS_CHARS (linenum);
                  _rl_move_vert (linenum);
                  _rl_move_cursor_relative (0, tt, VIS_FACE (linenum));
                  _rl_clear_to_eol (
                      (linenum == _rl_vis_botlin)
                          ? static_cast<unsigned int> (std::strlen (tt))
                          : _rl_screenwidth);
                }
            }
          _rl_vis_botlin = inv_botlin;

          /* CHANGED_SCREEN_LINE is set to 1 if we have moved to a
             different screen line during this redisplay. */
          changed_screen_line = _rl_last_v_pos != cursor_linenum;
          if (changed_screen_line)
            {
              _rl_move_vert (cursor_linenum);
              /* If we moved up to the line with the prompt using _rl_term_up,
                 the physical cursor position on the screen stays the same,
                 but the buffer position needs to be adjusted to account
                 for invisible characters. */
              if ((mb_cur_max == 1 || rl_byte_oriented) && cursor_linenum == 0
                  && wrap_offset)
                _rl_last_c_pos += wrap_offset;
            }

          /* Now we move the cursor to where it needs to be.  First, make
             sure we are on the correct line (cursor_linenum). */

          /* We have to reprint the prompt if it contains invisible
             characters, since it's not generally OK to just reprint
             the characters from the current cursor position.  But we
             only need to reprint it if the cursor is before the last
             invisible character in the prompt string. */
          /* XXX - why not use local_prompt_len? */
          nleft = local_prompt.visible_length + wrap_offset;
          if (cursor_linenum == 0 && wrap_offset > 0 && _rl_last_c_pos > 0
              && _rl_last_c_pos < PROMPT_ENDING_INDEX
              && !local_prompt.prompt.empty ())
            {
              _rl_cr ();
              if (modmark)
                _rl_output_some_chars ("*", 1);

              _rl_output_some_chars (local_prompt.prompt.data (), nleft);
              if (mb_cur_max > 1 && !rl_byte_oriented)
                _rl_last_c_pos
                    = _rl_col_width (local_prompt.prompt.data (), 0, nleft, 1)
                      - wrap_offset + modmark;
              else
                _rl_last_c_pos = nleft + modmark;
            }

          /* Where on that line?  And where does that line start
             in the buffer? */
          pos = inv_lbreaks[cursor_linenum];
          /* nleft == number of characters (bytes) in the line buffer between
             the start of the line and the desired cursor position. */
          nleft = cpos_buffer_position - pos;

          /* NLEFT is now a number of characters in a buffer.  When in a
             multibyte locale, however, _rl_last_c_pos is an absolute cursor
             position that doesn't take invisible characters in the prompt
             into account.  We use a fudge factor to compensate. */

          /* Since _rl_backspace() doesn't know about invisible characters in
             the prompt, and there's no good way to tell it, we compensate for
             those characters here and call _rl_backspace() directly if
             necessary */
          if (wrap_offset && cursor_linenum == 0 && nleft < _rl_last_c_pos)
            {
              /* TX == new physical cursor position in multibyte locale. */
              if (mb_cur_max > 1 && !rl_byte_oriented)
                tx = _rl_col_width (&visible_line[pos], 0, nleft, 1)
                     - visible_wrap_offset;
              else
                tx = nleft;
              if (_rl_last_c_pos > tx)
                {
                  _rl_backspace (_rl_last_c_pos - tx); /* XXX */
                  _rl_last_c_pos = tx;
                }
            }

          /* We need to note that in a multibyte locale we are dealing with
             _rl_last_c_pos as an absolute cursor position, but moving to a
             point specified by a buffer position (NLEFT) that doesn't take
             invisible characters into account. */
          if (mb_cur_max > 1 && !rl_byte_oriented)
            _rl_move_cursor_relative (nleft, &invisible_line[pos],
                                      &inv_face[pos]);
          else if (nleft != _rl_last_c_pos)
            _rl_move_cursor_relative (nleft, &invisible_line[pos],
                                      &inv_face[pos]);
        }
    }
  else /* Do horizontal scrolling. Much simpler */
    {
#define M_OFFSET(margin, offset) ((margin) == 0 ? offset : 0)
      unsigned int lmargin, ndisp, nleft, phys_c_pos, t;

      /* Always at top line. */
      _rl_last_v_pos = 0;

      /* Compute where in the buffer the displayed line should start.  This
         will be LMARGIN. */

      /* The number of characters that will be displayed before the cursor. */
      ndisp = cpos_buffer_position - wrap_offset;
      nleft = local_prompt.visible_length + wrap_offset;
      /* Where the new cursor position will be on the screen.  This can be
         longer than SCREENWIDTH; if it is, lmargin will be adjusted. */
      phys_c_pos
          = cpos_buffer_position - (last_lmargin ? last_lmargin : wrap_offset);
      t = _rl_screenwidth / 3;

      /* If the number of characters had already exceeded the screenwidth,
         last_lmargin will be > 0. */

      /* If the number of characters to be displayed is more than the screen
         width, compute the starting offset so that the cursor is about
         two-thirds of the way across the screen. */
      if (phys_c_pos > _rl_screenwidth - 2)
        {
          if (cpos_buffer_position > (2 * t))
            lmargin = cpos_buffer_position - (2 * t);
          else
            lmargin = 0;
          /* If the left margin would be in the middle of a prompt with
             invisible characters, don't display the prompt at all. */
          if (wrap_offset && lmargin > 0 && lmargin < nleft)
            lmargin = nleft;
        }
      else if (ndisp < _rl_screenwidth - 2) /* XXX - was -1 */
        lmargin = 0;
      else if (phys_c_pos < 1)
        {
          /* If we are moving back towards the beginning of the line and
             the last margin is no longer correct, compute a new one. */
          lmargin = static_cast<unsigned int> (((cpos_buffer_position - 1) / t)
                                               * t); /* XXX */
          if (wrap_offset && lmargin > 0 && lmargin < nleft)
            lmargin = nleft;
        }
      else
        lmargin = last_lmargin;

      displaying_prompt_first_line = lmargin < nleft;

      /* If the first character on the screen isn't the first character
         in the display line, indicate this with a special character. */
      if (lmargin > 0)
        invisible_line[lmargin] = '<';

      /* If SCREENWIDTH characters starting at LMARGIN do not encompass
         the whole line, indicate that with a special character at the
         right edge of the screen.  If LMARGIN is 0, we need to take the
         wrap offset into account. */
      t = lmargin + M_OFFSET (lmargin, wrap_offset) + _rl_screenwidth;
      if (t > 0 && t < out)
        invisible_line[t - 1] = '>';

      if (!rl_display_fixed || forced_display || lmargin != last_lmargin)
        {
          forced_display = false;
          o_cpos = _rl_last_c_pos;
          cpos_adjusted = false;

          update_line (&visible_line[last_lmargin], &vis_face[last_lmargin],
                       &invisible_line[lmargin], &inv_face[lmargin], 0,
                       _rl_screenwidth + visible_wrap_offset,
                       _rl_screenwidth + (lmargin ? 0 : wrap_offset), 0);

          if ((mb_cur_max > 1 && !rl_byte_oriented)
              && displaying_prompt_first_line && OLD_CPOS_IN_PROMPT ())
            _rl_last_c_pos
                -= local_prompt
                       .invis_chars_first_line; /* XXX - was wrap_offset */

          /* If the visible new line is shorter than the old, but the number
             of invisible characters is greater, and we are at the end of
             the new line, we need to clear to eol. */
          t = _rl_last_c_pos - M_OFFSET (lmargin, wrap_offset);
          if ((M_OFFSET (lmargin, wrap_offset) > visible_wrap_offset)
              && (_rl_last_c_pos == out) && displaying_prompt_first_line
              && t < visible_first_line_len)
            {
              nleft = _rl_screenwidth - t;
              _rl_clear_to_eol (nleft);
            }
          visible_first_line_len
              = out - lmargin - M_OFFSET (lmargin, wrap_offset);
          if (visible_first_line_len > _rl_screenwidth)
            visible_first_line_len = _rl_screenwidth;

          _rl_move_cursor_relative (cpos_buffer_position - lmargin,
                                    &invisible_line[lmargin],
                                    &inv_face[lmargin]);
          last_lmargin = lmargin;
        }
    }
  std::fflush (rl_outstream);

  /* Swap visible and non-visible lines. */
  {
    line_state *vtemp = line_state_visible;

    line_state_visible = line_state_invisible;
    line_state_invisible = vtemp;

    rl_display_fixed = false;
    /* If we are displaying on a single line, and last_lmargin is > 0, we
       are not displaying any invisible characters, so set visible_wrap_offset
       to 0. */
    if (_rl_horizontal_scroll_mode && last_lmargin)
      visible_wrap_offset = 0;
    else
      visible_wrap_offset = wrap_offset;

    _rl_quick_redisplay = false;
  }

  RL_UNSETSTATE (RL_STATE_REDISPLAYING);
  _rl_release_sigint ();
}

void
Readline::putc_face (int c, char face, char *cur_face)
{
  char cf;
  cf = *cur_face;
  if (cf != face)
    {
      if (cf != FACE_NORMAL && cf != FACE_STANDOUT)
        return;
      if (face != FACE_NORMAL && face != FACE_STANDOUT)
        return;
      if (face == FACE_STANDOUT && cf == FACE_NORMAL)
        _rl_standout_on ();
      if (face == FACE_NORMAL && cf == FACE_STANDOUT)
        _rl_standout_off ();
      *cur_face = face;
    }
  if (c != EOF)
    std::putc (c, rl_outstream);
}

#define ADJUST_CPOS(x)                                                        \
  do                                                                          \
    {                                                                         \
      _rl_last_c_pos -= (x);                                                  \
      cpos_adjusted = true;                                                   \
    }                                                                         \
  while (0)

/* PWP: update_line() is based on finding the middle difference of each
   line on the screen; vis:

                             /old first difference
        /beginning of line   |              /old last same       /old EOL
        v 		     v              v                    v
old:	eddie> Oh, my little gruntle-buggy is to me, as lurgid as
new:	eddie> Oh, my little buggy says to me, as lurgid as
        ^		     ^        ^			   ^
        \beginning of line   |	      \new last same	   \new end of line
                             \new first difference

   All are character pointers for the sake of speed.  Special cases for
   no differences, as well as for end of line additions must be handled.

   Could be made even smarter, but this works well enough */
void
Readline::update_line (char *old, char *old_face, const char *new_,
                       const char *new_face, unsigned int current_line,
                       unsigned int omax, unsigned int nmax,
                       unsigned int inv_botlin)
{
  const char *ofd, *ols, *oe, *nfd, *nls, *ne;
  const char *ofdf, *nfdf, *olsf, *nlsf;
  unsigned int temp, wsatend, od, nd, o_cpos;
  unsigned int current_invis_chars;
  int lendiff, col_lendiff;
  unsigned int col_temp, bytes_to_insert;
  size_t mb_cur_max = MB_CUR_MAX;
#if defined(HANDLE_MULTIBYTE)
  mbstate_t ps_new, ps_old;
  unsigned int new_offset, old_offset;
#endif

  /* If we're at the right edge of a terminal that supports xn, we're
     ready to wrap around, so do so.  This fixes problems with knowing
     the exact cursor position and cut-and-paste with certain terminal
     emulators.  In this calculation, TEMP is the physical screen
     position of the cursor. */
  if (mb_cur_max > 1 && !rl_byte_oriented)
    temp = _rl_last_c_pos;
  else
    temp = _rl_last_c_pos - WRAP_OFFSET (_rl_last_v_pos, visible_wrap_offset);
  if (temp == _rl_screenwidth && _rl_term_autowrap
      && !_rl_horizontal_scroll_mode && _rl_last_v_pos == current_line - 1)
    {
      /* We're going to wrap around by writing the first character of NEW to
         the screen and dealing with changes to what's visible by modifying
         OLD to match it.  Complicated by the presence of multi-width
         characters at the end of the line or beginning of the new one. */
      /* old is always somewhere in visible_line; new is always somewhere in
         invisible_line.  These should always be null-terminated. */
#if defined(HANDLE_MULTIBYTE)
      if (mb_cur_max > 1 && !rl_byte_oriented)
        {
          wchar_t wc;
          mbstate_t ps;
          int oldwidth, newwidth;
          unsigned int oldbytes, newbytes;
          size_t ret;

          /* This fixes only double-column characters, but if the wrapped
             character consumes more than three columns, spaces will be
             inserted in the string buffer. */
          /* XXX remember that we are working on the invisible line right now;
             we don't swap visible and invisible until just before rl_redisplay
             returns */
          /* This will remove the extra placeholder space we added with
             _rl_wrapped_multicolumn */
          if (current_line < line_state_invisible->wbsize
              && line_state_invisible->wrapped_line[current_line] > 0)
            _rl_clear_to_eol (
                line_state_invisible->wrapped_line[current_line]);

          /* 1. how many screen positions does first char in old consume? */
          std::memset (&ps, 0, sizeof (mbstate_t));
          ret = std::mbrtowc (&wc, old, mb_cur_max, &ps);
          oldbytes = static_cast<unsigned int> (ret);
          if (MB_INVALIDCH (ret))
            {
              oldwidth = 1;
              oldbytes = 1;
            }
          else if (MB_NULLWCH (ret))
            oldwidth = 0;
          else
            oldwidth = WCWIDTH (wc);
          if (oldwidth < 0)
            oldwidth = 1;

          /* 2. how many screen positions does the first char in new consume?
           */
          std::memset (&ps, 0, sizeof (mbstate_t));
          ret = std::mbrtowc (&wc, new_, mb_cur_max, &ps);
          newbytes = static_cast<unsigned int> (ret);
          if (MB_INVALIDCH (ret))
            {
              newwidth = 1;
              newbytes = 1;
            }
          else if (MB_NULLWCH (ret))
            newwidth = 0;
          else
            newwidth = WCWIDTH (wc);
          if (newwidth < 0)
            newwidth = 1;

          /* 3. if the new width is less than the old width, we need to keep
             going in new until we have consumed at least that many screen
             positions, and figure out how many bytes that will take */
          while (newbytes < nmax && newwidth < oldwidth)
            {
              int t;

              ret = std::mbrtowc (&wc, new_ + newbytes, mb_cur_max, &ps);
              if (MB_INVALIDCH (ret))
                {
                  newwidth += 1;
                  newbytes += 1;
                }
              else if (MB_NULLWCH (ret))
                break;
              else
                {
                  t = WCWIDTH (wc);
                  newwidth += static_cast<unsigned int> ((t >= 0) ? t : 1);
                  newbytes += ret;
                }
            }
          /* 4. If the new width is more than the old width, keep going in old
             until we have consumed exactly that many screen positions, and
             figure out how many bytes that will take.  This is an optimization
           */
          while (oldbytes < omax && oldwidth < newwidth)
            {
              int t;

              ret = std::mbrtowc (&wc, old + oldbytes, mb_cur_max, &ps);
              if (MB_INVALIDCH (ret))
                {
                  oldwidth += 1;
                  oldbytes += 1;
                }
              else if (MB_NULLWCH (ret))
                break;
              else
                {
                  t = WCWIDTH (wc);
                  oldwidth += (t >= 0) ? t : 1;
                  oldbytes += ret;
                }
            }
          /* 5. write the first newbytes of new, which takes newwidth.  This is
             where the screen wrapping takes place, and we are now writing
             characters onto the new line. We need to fix up old so it
             accurately reflects what is on the screen after the
             _rl_output_some_chars below. */
          if (newwidth > 0)
            {
              unsigned int i, j;

              puts_face (new_, new_face, newbytes);
              _rl_last_c_pos = static_cast<unsigned int> (newwidth);
              _rl_last_v_pos++;

              /* 5a. If the number of screen positions doesn't match, punt
                 and do a dumb update.
                 5b. If the number of bytes is greater in the new line than
                 the old, do a dumb update, because there is no guarantee we
                 can extend the old line enough to fit the new bytes. */
              if (newwidth != oldwidth || newbytes > oldbytes)
                {
                  oe = old + omax;
                  ne = new_ + nmax;
                  nd = newbytes;
                  nfd = new_ + nd;
                  ofdf = old_face + oldbytes;
                  nfdf = new_face + newbytes;

                  goto dumb_update;
                }
              if (oldbytes != 0 && newbytes != 0)
                {
                  /* We have written as many bytes from new as we need to
                     consume the first character of old. Fix up `old' so it
                     reflects the new screen contents.  We use +1 in the
                     memmove call to copy the trailing NUL. */
                  /* (strlen(old+oldbytes) == (omax - oldbytes - 1)) */

                  /* Don't bother trying to fit the bytes if the number of
                     bytes doesn't change. */
                  if (oldbytes != newbytes)
                    {
                      std::memmove (old + newbytes, old + oldbytes,
                                    strlen (old + oldbytes) + 1);
                      std::memmove (old_face + newbytes, old_face + oldbytes,
                                    strlen (old + oldbytes) + 1);
                    }
                  std::memcpy (old, new_, newbytes);
                  std::memcpy (old_face, new_face, newbytes);
                  j = static_cast<unsigned int> (newbytes - oldbytes);
                  omax += j;
                  /* Fix up indices if we copy data from one line to another */
                  for (i = current_line + 1; j != 0 && i <= inv_botlin + 1
                                             && i <= _rl_vis_botlin + 1;
                       i++)
                    vis_lbreaks[i] += j;
                }
            }
          else
            {
              std::putc (' ', rl_outstream);
              _rl_last_c_pos = 1;
              _rl_last_v_pos++;
              if (old[0] && new_[0])
                {
                  old[0] = new_[0];
                  old_face[0] = new_face[0];
                }
            }
        }
      else
#endif
        {
          if (new_[0])
            puts_face (new_, new_face, 1);
          else
            std::putc (' ', rl_outstream);

          _rl_last_c_pos = 1;
          _rl_last_v_pos++;
          if (old[0] && new_[0])
            {
              old[0] = new_[0];
              old_face[0] = new_face[0];
            }
        }
    }

  /* We know that we are dealing with a single screen line here */
  if (_rl_quick_redisplay)
    {
      nfd = new_;
      nfdf = new_face;
      ofd = old;
      ofdf = old_face;
      for (od = 0, oe = ofd; od < omax && *oe; oe++, od++)
        ;
      for (nd = 0, ne = nfd; nd < nmax && *ne; ne++, nd++)
        ;
      od = nd = 0;
      _rl_move_cursor_relative (0, old, old_face);

      bytes_to_insert = static_cast<unsigned int> (ne - nfd);
      unsigned int local_prompt_len
          = static_cast<unsigned int> (local_prompt.prompt.size ());
      if (bytes_to_insert < local_prompt_len) /* ??? */
        goto dumb_update;

      /* output the prompt, output the line contents, clear the rest */
      _rl_output_some_chars (nfd, local_prompt_len);
      if (mb_cur_max > 1 && !rl_byte_oriented)
        _rl_last_c_pos = local_prompt.physical_chars;
      else
        _rl_last_c_pos = local_prompt_len;

      bytes_to_insert -= local_prompt_len;
      if (bytes_to_insert > 0)
        {
          puts_face (new_ + local_prompt_len, nfdf + local_prompt_len,
                     bytes_to_insert);
          if (mb_cur_max > 1 && rl_byte_oriented)
            _rl_last_c_pos
                += _rl_col_width (new_, local_prompt_len,
                                  static_cast<unsigned int> (ne - new_), 1);
          else
            _rl_last_c_pos += bytes_to_insert;
        }

      /* See comments at dumb_update: for an explanation of this heuristic */
      if (nmax < omax)
        goto clear_rest_of_line;
      else if ((nmax - W_OFFSET (current_line, wrap_offset))
               < (omax - W_OFFSET (current_line, visible_wrap_offset)))
        goto clear_rest_of_line;
      else
        return;
    }

    /* Find first difference. */
#if defined(HANDLE_MULTIBYTE)
  if (mb_cur_max > 1 && !rl_byte_oriented)
    {
      /* See if the old line is a subset of the new line, so that the
         only change is adding characters. */
      temp = (omax < nmax) ? omax : nmax;
      if (std::memcmp (old, new_, temp) == 0
          && std::memcmp (old_face, new_face, temp) == 0)
        {
          new_offset = old_offset = temp; /* adding at the end */
          ofd = old + temp;
          ofdf = old_face + temp;
          nfd = new_ + temp;
          nfdf = new_face + temp;
        }
      else
        {
          std::memset (&ps_new, 0, sizeof (mbstate_t));
          std::memset (&ps_old, 0, sizeof (mbstate_t));

          /* Are the old and new lines the same? */
          if (omax == nmax && std::memcmp (new_, old, omax) == 0
              && std::memcmp (new_face, old_face, omax) == 0)
            {
              old_offset = omax;
              new_offset = nmax;
              ofd = old + omax;
              ofdf = old_face + omax;
              nfd = new_ + nmax;
              nfdf = new_face + nmax;
            }
          else
            {
              /* Go through the line from the beginning and find the first
                 difference. We assume that faces change at (possibly multi-
                 byte) character boundaries. */
              new_offset = old_offset = 0;
              for (ofd = old, ofdf = old_face, nfd = new_, nfdf = new_face;
                   (ofd - old < omax) && *ofd
                   && _rl_compare_chars (old, old_offset, &ps_old, new_,
                                         new_offset, &ps_new)
                   && *ofdf == *nfdf;)
                {
                  old_offset
                      = _rl_find_next_mbchar (old, old_offset, 1, MB_FIND_ANY);
                  new_offset = _rl_find_next_mbchar (new_, new_offset, 1,
                                                     MB_FIND_ANY);

                  ofd = old + old_offset;
                  ofdf = old_face + old_offset;
                  nfd = new_ + new_offset;
                  nfdf = new_face + new_offset;
                }
            }
        }
    }
  else
#endif
    for (ofd = old, ofdf = old_face, nfd = new_, nfdf = new_face;
         (ofd - old < omax) && *ofd && (*ofd == *nfd) && (*ofdf == *nfdf);
         ofd++, nfd++, ofdf++, nfdf++)
      ;

  /* Move to the end of the screen line.  ND and OD are used to keep track
     of the distance between ne and new and oe and old, respectively, to
     move a subtraction out of each loop. */
  for (od = static_cast<unsigned int> (ofd - old), oe = ofd; od < omax && *oe;
       oe++, od++)
    ;
  for (nd = static_cast<unsigned int> (nfd - new_), ne = nfd; nd < nmax && *ne;
       ne++, nd++)
    ;

  /* If no difference, continue to next line. */
  if (ofd == oe && nfd == ne)
    return;

#if defined(HANDLE_MULTIBYTE)
  if (mb_cur_max > 1 && !rl_byte_oriented && _rl_utf8locale)
    {
      wchar_t wc;
      mbstate_t ps;
      std::memset (&ps, 0, sizeof (mbstate_t));

      /* If the first character in the difference is a zero-width character,
         assume it's a combining character and back one up so the two base
         characters no longer compare equivalently. */
      size_t t = std::mbrtowc (&wc, ofd, mb_cur_max, &ps);
      if (static_cast<ssize_t> (t) > 0 && UNICODE_COMBINING_CHAR (wc)
          && WCWIDTH (wc) == 0)
        {
          old_offset = _rl_find_prev_mbchar (
              old, static_cast<unsigned int> (ofd - old), MB_FIND_ANY);
          new_offset = _rl_find_prev_mbchar (
              new_, static_cast<unsigned int> (nfd - new_), MB_FIND_ANY);
          ofd = old + old_offset; /* equal by definition */
          ofdf = old_face + old_offset;
          nfd = new_ + new_offset;
          nfdf = new_face + new_offset;
        }
    }
#endif

  wsatend = 1; /* flag for trailing whitespace */

#if defined(HANDLE_MULTIBYTE)
  /* Find the last character that is the same between the two lines.  This
     bounds the region that needs to change. */
  if (mb_cur_max > 1 && !rl_byte_oriented)
    {
      ols = old
            + _rl_find_prev_mbchar (old, static_cast<unsigned int> (oe - old),
                                    MB_FIND_ANY);
      olsf = old_face + (ols - old);
      nls = new_
            + _rl_find_prev_mbchar (
                new_, static_cast<unsigned int> (ne - new_), MB_FIND_ANY);
      nlsf = new_face + (nls - new_);

      while ((ols > ofd) && (nls > nfd))
        {
          std::memset (&ps_old, 0, sizeof (mbstate_t));
          std::memset (&ps_new, 0, sizeof (mbstate_t));

          if (_rl_compare_chars (
                  old, static_cast<unsigned int> (ols - old), &ps_old, new_,
                  static_cast<unsigned int> (nls - new_), &ps_new)
                  == 0
              || *olsf != *nlsf)
            break;

          if (*ols == ' ')
            wsatend = 0;

          ols = old
                + _rl_find_prev_mbchar (
                    old, static_cast<unsigned int> (ols - old), MB_FIND_ANY);
          olsf = old_face + (ols - old);
          nls = new_
                + _rl_find_prev_mbchar (
                    new_, static_cast<unsigned int> (nls - new_), MB_FIND_ANY);
          nlsf = new_face + (nls - new_);
        }
    }
  else
    {
#endif              /* HANDLE_MULTIBYTE */
      ols = oe - 1; /* find last same */
      olsf = old_face + (ols - old);
      nls = ne - 1;
      nlsf = new_face + (nls - new_);
      while ((ols > ofd) && (nls > nfd) && (*ols == *nls) && (*olsf == *nlsf))
        {
          if (*ols != ' ')
            wsatend = 0;
          ols--;
          olsf--;
          nls--;
          nlsf--;
        }
#if defined(HANDLE_MULTIBYTE)
    }
#endif

  if (wsatend)
    {
      ols = oe;
      olsf = old_face + (ols - old);
      nls = ne;
      nlsf = new_face + (nls - new_);
    }
#if defined(HANDLE_MULTIBYTE)
  /* This may not work for stateful encoding, but who cares?  To handle
     stateful encoding properly, we have to scan each string from the
     beginning and compare. */
  else if (_rl_compare_chars (ols, 0, nullptr, nls, 0, nullptr) == false
           || *olsf != *nlsf)
#else
  else if (*ols != *nls || *olsf != *nlsf)
#endif
    {
      if (*ols) /* don't step past the NUL */
        {
          if (mb_cur_max > 1 && !rl_byte_oriented)
            ols = old
                  + _rl_find_next_mbchar (
                      old, static_cast<unsigned int> (ols - old), 1,
                      MB_FIND_ANY);
          else
            ols++;
        }
      if (*nls)
        {
          if (mb_cur_max > 1 && !rl_byte_oriented)
            nls = new_
                  + _rl_find_next_mbchar (
                      new_, static_cast<unsigned int> (nls - new_), 1,
                      MB_FIND_ANY);
          else
            nls++;
        }
      olsf = old_face + (ols - old);
      nlsf = new_face + (nls - new_);
    }

  /* count of invisible characters in the current invisible line. */
  current_invis_chars = W_OFFSET (current_line, wrap_offset);
  if (_rl_last_v_pos != current_line)
    {
      _rl_move_vert (current_line);
      /* We have moved up to a new screen line.  This line may or may not have
         invisible characters on it, but we do our best to recalculate
         visible_wrap_offset based on what we know. */
      if (current_line == 0)
        visible_wrap_offset = local_prompt.invis_chars_first_line; /* XXX */

      if ((mb_cur_max == 1 || rl_byte_oriented) && current_line == 0
          && visible_wrap_offset)
        _rl_last_c_pos += visible_wrap_offset;
    }

  /* If this is the first line and there are invisible characters in the
     prompt string, and the prompt string has not changed, and the current
     cursor position is before the last invisible character in the prompt,
     and the index of the character to move to is past the end of the prompt
     string, then redraw the entire prompt string.  We can only do this
     reliably if the terminal supports a `cr' capability.

     This can also happen if the prompt string has changed, and the first
     difference in the line is in the middle of the prompt string, after a
     sequence of invisible characters (worst case) and before the end of
     the prompt.  In this case, we have to redraw the entire prompt string
     so that the entire sequence of invisible characters is drawn.  We need
     to handle the worst case, when the difference is after (or in the middle
     of) a sequence of invisible characters that changes the text color and
     before the sequence that restores the text color to normal.  Then we have
     to make sure that the lines still differ -- if they don't, we can
     return immediately.

     This is not an efficiency hack -- there is a problem with redrawing
     portions of the prompt string if they contain terminal escape
     sequences (like drawing the `unbold' sequence without a corresponding
     `bold') that manifests itself on certain terminals. */

  lendiff = static_cast<int> (local_prompt.prompt.size ());
  if (lendiff > static_cast<int> (nmax))
    lendiff = static_cast<int> (nmax);
  od = static_cast<unsigned int> (
      ofd - old); /* index of first difference in visible line */
  nd = static_cast<unsigned int> (nfd - new_); /* nd, od are buffer indexes */
  if (current_line == 0 && !_rl_horizontal_scroll_mode && _rl_term_cr
      && lendiff > static_cast<int> (local_prompt.visible_length)
      && _rl_last_c_pos > 0
      && (((od > 0 || nd > 0)
           && (od <= local_prompt.last_invisible
               || nd <= local_prompt.last_invisible))
          || ((static_cast<int> (od) >= lendiff)
              && _rl_last_c_pos < PROMPT_ENDING_INDEX)))
    {
      _rl_cr ();
      if (modmark)
        _rl_output_some_chars ("*", 1);
      _rl_output_some_chars (local_prompt.prompt.data (),
                             static_cast<unsigned int> (lendiff));
      if (mb_cur_max > 1 && !rl_byte_oriented)
        {
          /* If we just output the entire prompt string we can take advantage
             of knowing the number of physical characters in the prompt. If
             the prompt wraps lines (lendiff clamped at nmax), we can't. */
          if (lendiff == static_cast<int> (local_prompt.prompt.size ()))
            _rl_last_c_pos = local_prompt.physical_chars + modmark;
          else
            /* We take wrap_offset into account here so we can pass correct
               information to _rl_move_cursor_relative. */
            _rl_last_c_pos
                = _rl_col_width (local_prompt.prompt.data (), 0,
                                 static_cast<unsigned int> (lendiff), 1)
                  - wrap_offset + modmark;
          cpos_adjusted = true;
        }
      else
        _rl_last_c_pos = static_cast<unsigned int> (lendiff + modmark);

      /* Now if we have printed the prompt string because the first difference
         was within the prompt, see if we need to recompute where the lines
         differ.  Check whether where we are now is past the last place where
         the old and new lines are the same and short-circuit now if we are. */
      if ((od <= local_prompt.last_invisible
           || nd <= local_prompt.last_invisible)
          && omax == nmax && lendiff > (ols - old) && lendiff > (nls - new_))
        return;

      /* XXX - we need to fix up our calculations if we are now past the
         old ofd/nfd and the prompt length (or line length) has changed.
         We punt on the problem and do a dumb update.  We'd like to be able
         to just output the prompt from the beginning of the line up to the
         first difference, but you don't know the number of invisible
         characters in that case.
         This needs a lot of work to be efficient, but it usually doesn't
         matter. */
      if ((od <= local_prompt.last_invisible
           || nd <= local_prompt.last_invisible))
        {
          nfd = new_ + lendiff; /* number of characters we output above */
          nfdf = new_face + lendiff;
          nd = static_cast<unsigned int> (lendiff);

          /* Do a dumb update and return */
        dumb_update:
          temp = static_cast<unsigned int> (ne - nfd);
          if (static_cast<int> (temp) > 0)
            {
              puts_face (nfd, nfdf, temp);
              if (mb_cur_max > 1 && !rl_byte_oriented)
                {
                  _rl_last_c_pos += _rl_col_width (
                      new_, nd, static_cast<unsigned int> (ne - new_), 1);
                  /* Need to adjust here based on wrap_offset. Guess that if
                     this is the line containing the last line of the prompt
                     we need to adjust by
                        wrap_offset-prompt_invis_chars_first_line
                     on the assumption that this is the number of invisible
                     characters in the last line of the prompt. */
                  if (wrap_offset > local_prompt.invis_chars_first_line
                      && current_line == prompt_last_screen_line
                      && local_prompt.physical_chars > _rl_screenwidth
                      && _rl_horizontal_scroll_mode == 0)
                    ADJUST_CPOS (wrap_offset
                                 - local_prompt.invis_chars_first_line);

                  /* If we just output a new line including the prompt, and
                     the prompt includes invisible characters, we need to
                     account for them in the _rl_last_c_pos calculation, since
                     _rl_col_width does not. This happens when other code does
                     a goto dumb_update; */
                  else if (current_line == 0 && nfd == new_
                           && local_prompt.invis_chars_first_line
                           && static_cast<unsigned int> (
                                  local_prompt.prompt.size ())
                                  <= temp
                           && wrap_offset
                                  >= local_prompt.invis_chars_first_line
                           && _rl_horizontal_scroll_mode == 0)
                    ADJUST_CPOS (local_prompt.invis_chars_first_line);
                }
              else
                _rl_last_c_pos += temp;
            }
          /* This is a useful heuristic, but what we really want is to clear
             if the new number of visible screen characters is less than the
             old number of visible screen characters. If the prompt has
             changed, we don't really have enough information about the visible
             line to know for sure, so we use another heuristic calclulation
             below. */
          if (nmax < omax)
            goto clear_rest_of_line; /* XXX */
          else if ((nmax - W_OFFSET (current_line, wrap_offset))
                   < (omax - W_OFFSET (current_line, visible_wrap_offset)))
            goto clear_rest_of_line;
          else
            return;
        }
    }

  o_cpos = _rl_last_c_pos;

  /* When this function returns, _rl_last_c_pos is correct, and an absolute
     cursor position in multibyte mode, but a buffer index when not in a
     multibyte locale. */
  _rl_move_cursor_relative (od, old, old_face);

#if defined(HANDLE_MULTIBYTE)
  /* We need to indicate that the cursor position is correct in the presence of
     invisible characters in the prompt string.  Let's see if setting this when
     we make sure we're at the end of the drawn prompt string works. */
  if (current_line == 0 && mb_cur_max > 1 && !rl_byte_oriented
      && (_rl_last_c_pos > 0 || o_cpos > 0)
      && _rl_last_c_pos == local_prompt.physical_chars)
    cpos_adjusted = 1;
#endif

  /* if (len (new_) > len (old))
     lendiff == difference in buffer (bytes)
     col_lendiff == difference on screen (columns)
     When not using multibyte characters, these are equal */
  lendiff = static_cast<int> ((nls - nfd) - (ols - ofd));
  if (mb_cur_max > 1 && !rl_byte_oriented)
    {
      unsigned int newchars, newwidth;
      unsigned int oldchars, oldwidth;

      newchars = static_cast<unsigned char> (nls - new_);
      oldchars = static_cast<unsigned char> (ols - old);

      /* If we can do it, try to adjust nls and ols so that nls-new will
         contain the entire new prompt string. That way we can use
         prompt_physical_chars and not have to recompute column widths.
         _rl_col_width adds wrap_offset and expects the caller to compensate,
         which we do below, so we do the same thing if we don't call
         _rl_col_width.
         We don't have to compare, since we know the characters are the same.
         The check of differing numbers of invisible chars may be extraneous.
         XXX - experimental */
      unsigned int local_prompt_len
          = static_cast<unsigned int> (local_prompt.prompt.size ());
      if (current_line == 0 && nfd == new_
          && newchars > local_prompt.last_invisible
          && newchars <= local_prompt_len && local_prompt_len <= nmax
          && current_invis_chars != visible_wrap_offset)
        {
          while (newchars < nmax && oldchars < omax
                 && newchars < local_prompt_len)
            {
#if defined(HANDLE_MULTIBYTE)
              size_t newind
                  = _rl_find_next_mbchar (new_, newchars, 1, MB_FIND_NONZERO);
              size_t oldind
                  = _rl_find_next_mbchar (old, oldchars, 1, MB_FIND_NONZERO);

              nls += newind - newchars;
              ols += oldind - oldchars;

              newchars = static_cast<unsigned int> (newind);
              oldchars = static_cast<unsigned int> (oldind);
#else
              nls++;
              ols++;
              newchars++;
              oldchars++;
#endif
            }
          newwidth
              = (newchars == local_prompt_len)
                    ? local_prompt.physical_chars + wrap_offset
                    : _rl_col_width (
                        new_, 0, static_cast<unsigned int> (nls - new_), 1);
          /* if we changed nls and ols, we need to recompute lendiff */
          lendiff = static_cast<int> ((nls - nfd) - (ols - ofd));

          nlsf = new_face + (nls - new_);
          olsf = old_face + (ols - old);
        }
      else
        newwidth = _rl_col_width (new_, static_cast<unsigned int> (nfd - new_),
                                  static_cast<unsigned int> (nls - new_), 1);

      oldwidth = _rl_col_width (old, static_cast<unsigned int> (ofd - old),
                                static_cast<unsigned int> (ols - old), 1);

      col_lendiff = static_cast<int> (newwidth) - static_cast<int> (oldwidth);
    }
  else
    col_lendiff = lendiff;

  /* col_lendiff uses _rl_col_width(), which doesn't know about whether or not
     the multibyte characters it counts are invisible, so unless we're printing
     the entire prompt string (in which case we can use prompt_physical_chars)
     the count is short by the number of bytes in the invisible multibyte
     characters - the number of multibyte characters.

     We don't have a good way to solve this without moving to something like
     a bitmap that indicates which characters are visible and which are
     invisible. We fix it up (imperfectly) in the caller and by trying to use
     the entire prompt string wherever we can. */

  /* If we are changing the number of invisible characters in a line, and
     the spot of first difference is before the end of the invisible chars,
     lendiff needs to be adjusted. */
  if (current_line == 0 && current_invis_chars != visible_wrap_offset)
    {
      if (mb_cur_max > 1 && !rl_byte_oriented)
        {
          lendiff += visible_wrap_offset - current_invis_chars;
          col_lendiff += visible_wrap_offset - current_invis_chars;
        }
      else
        {
          lendiff += visible_wrap_offset - current_invis_chars;
          col_lendiff = lendiff;
        }
    }

  /* We use temp as a count of the number of bytes from the first difference
     to the end of the new line.  col_temp is the corresponding number of
     screen columns.  A `dumb' update moves to the spot of first difference
     and writes TEMP bytes. */
  /* Insert (diff (len (old), len (new_)) ch. */
  temp = static_cast<unsigned int> (ne - nfd);
  if (mb_cur_max > 1 && !rl_byte_oriented)
    col_temp = _rl_col_width (new_, static_cast<unsigned int> (nfd - new_),
                              static_cast<unsigned int> (ne - new_), 1);
  else
    col_temp = temp;

  /* how many bytes from the new line buffer to write to the display */
  bytes_to_insert = static_cast<unsigned int> (nls - nfd);

  /* col_lendiff > 0 if we are adding characters to the line */
  if (col_lendiff > 0) /* XXX - was lendiff */
    {
      /* Non-zero if we're increasing the number of lines. */
      int gl = current_line >= _rl_vis_botlin && inv_botlin > _rl_vis_botlin;

      /* If col_lendiff is > 0, implying that the new string takes up more
         screen real estate than the old, but lendiff is < 0, meaning that it
         takes fewer bytes, we need to just output the characters starting
         from the first difference.  These will overwrite what is on the
         display, so there's no reason to do a smart update.  This can really
         only happen in a multibyte environment. */
      if (lendiff < 0)
        {
          puts_face (nfd, nfdf, temp);
          _rl_last_c_pos += col_temp;
          /* If nfd begins before any invisible characters in the prompt,
             adjust _rl_last_c_pos to account for wrap_offset and set
             cpos_adjusted to let the caller know. */
          if (current_line == 0 && displaying_prompt_first_line && wrap_offset
              && ((nfd - new_) <= local_prompt.last_invisible))
            ADJUST_CPOS (
                wrap_offset); /* XXX - prompt_invis_chars_first_line? */
          return;
        }
      /* Sometimes it is cheaper to print the characters rather than
         use the terminal's capabilities.  If we're growing the number
         of lines, make sure we actually cause the new line to wrap
         around on auto-wrapping terminals. */
      else if (_rl_terminal_can_insert
               && (static_cast<int> (2 * col_temp) >= col_lendiff
                   || _rl_term_IC)
               && (!_rl_term_autowrap || !gl))
        {
          /* If lendiff > local_prompt.visible_length and _rl_last_c_pos == 0
             and _rl_horizontal_scroll_mode == 1, inserting the characters with
             _rl_term_IC or _rl_term_ic will screw up the screen because of the
             invisible characters.  We need to just draw them. */
          /* The same thing happens if we're trying to draw before the last
             invisible character in the prompt string or we're increasing the
             number of invisible characters in the line and we're not drawing
             the entire prompt string. */
          if (*ols
              && ((_rl_horizontal_scroll_mode && _rl_last_c_pos == 0
                   && lendiff > static_cast<int> (local_prompt.visible_length)
                   && current_invis_chars > 0)
                  == 0)
              && (((mb_cur_max > 1 && !rl_byte_oriented) && current_line == 0
                   && wrap_offset
                   && ((nfd - new_) <= local_prompt.last_invisible)
                   && (col_lendiff
                       < static_cast<int> (local_prompt.visible_length)))
                  == 0)
              && (visible_wrap_offset >= current_invis_chars))
            {
              open_some_spaces (static_cast<unsigned int> (col_lendiff));
              puts_face (nfd, nfdf, bytes_to_insert);
              if (mb_cur_max > 1 && !rl_byte_oriented)
                _rl_last_c_pos += _rl_col_width (nfd, 0, bytes_to_insert, 1);
              else
                _rl_last_c_pos += bytes_to_insert;
            }
          else if ((mb_cur_max == 1 || rl_byte_oriented != 0) && *ols == 0
                   && lendiff > 0)
            {
              /* At the end of a line the characters do not have to
                 be "inserted".  They can just be placed on the screen. */
              puts_face (nfd, nfdf, temp);
              _rl_last_c_pos += col_temp;
              return;
            }
          else /* just write from first difference to end of new line */
            {
              puts_face (nfd, nfdf, temp);
              _rl_last_c_pos += col_temp;
              /* If nfd begins before the last invisible character in the
                 prompt, adjust _rl_last_c_pos to account for wrap_offset
                 and set cpos_adjusted to let the caller know. */
              if ((mb_cur_max > 1 && !rl_byte_oriented) && current_line == 0
                  && displaying_prompt_first_line && wrap_offset
                  && ((nfd - new_) <= local_prompt.last_invisible))
                ADJUST_CPOS (
                    wrap_offset); /* XXX - prompt_invis_chars_first_line? */
              return;
            }

          if (static_cast<int> (bytes_to_insert) > lendiff)
            {
              /* If nfd begins before the last invisible character in the
                 prompt, adjust _rl_last_c_pos to account for wrap_offset
                 and set cpos_adjusted to let the caller know. */
              if ((mb_cur_max > 1 && !rl_byte_oriented) && current_line == 0
                  && displaying_prompt_first_line && wrap_offset
                  && ((nfd - new_) <= local_prompt.last_invisible))
                ADJUST_CPOS (
                    wrap_offset); /* XXX - prompt_invis_chars_first_line? */
            }
        }
      else
        {
          /* cannot insert chars, write to EOL */
          puts_face (nfd, nfdf, temp);
          _rl_last_c_pos += col_temp;
          /* If we're in a multibyte locale and were before the last invisible
             char in the current line (which implies we just output some
             invisible characters) we need to adjust _rl_last_c_pos, since it
             represents a physical character position. */
          /* The current_line * rl_screenwidth +
             local_prompt.invis_chars_first_line is a crude attempt to compute
             how far into the new line buffer we are. It doesn't work well in
             the face of multibyte characters and needs to be rethought. XXX */
          if ((mb_cur_max > 1 && !rl_byte_oriented)
              && current_line == prompt_last_screen_line && wrap_offset
              && displaying_prompt_first_line
              && wrap_offset != local_prompt.invis_chars_first_line
              && ((nfd - new_) < (local_prompt.last_invisible
                                  - (current_line * _rl_screenwidth
                                     + local_prompt.invis_chars_first_line))))
            ADJUST_CPOS (wrap_offset - local_prompt.invis_chars_first_line);

          /* XXX - what happens if wrap_offset == prompt_invis_chars_first_line
             and we are drawing the first line (current_line == 0)? We should
             adjust by _rl_last_c_pos -= local_prompt.invis_chars_first_line */
        }
    }
  else /* Delete characters from line. */
    {
      /* If possible and inexpensive to use terminal deletion, then do so. */
      if (_rl_term_dc
          && (2 * col_temp) >= static_cast<unsigned int> (-col_lendiff))
        {
          /* If all we're doing is erasing the invisible characters in the
             prompt string, don't bother.  It screws up the assumptions
             about what's on the screen. */
          if (_rl_horizontal_scroll_mode && _rl_last_c_pos == 0
              && displaying_prompt_first_line
              && static_cast<unsigned int> (-lendiff) == visible_wrap_offset)
            col_lendiff = 0;

          /* If we have moved lmargin and we're shrinking the line, we've
             already moved the cursor to the first character of the new line,
             so deleting -col_lendiff characters will mess up the cursor
             position calculation */
          if (_rl_horizontal_scroll_mode && displaying_prompt_first_line == 0
              && col_lendiff
              && _rl_last_c_pos < static_cast<unsigned int> (-col_lendiff))
            col_lendiff = 0;

          if (col_lendiff)
            delete_chars (-col_lendiff); /* delete (diff) characters */

          /* Copy (new) chars to screen from first diff to last match,
             overwriting what is there. */
          if (bytes_to_insert > 0)
            {
              /* If nfd begins at the prompt, or before the invisible
                 characters in the prompt, we need to adjust _rl_last_c_pos
                 in a multibyte locale to account for the wrap offset and
                 set cpos_adjusted accordingly. */
              puts_face (nfd, nfdf, bytes_to_insert);
              if (mb_cur_max > 1 && !rl_byte_oriented)
                {
                  /* This still doesn't take into account whether or not the
                     characters that this counts are invisible. */
                  _rl_last_c_pos += _rl_col_width (nfd, 0, bytes_to_insert, 1);
                  if (current_line == 0 && wrap_offset
                      && displaying_prompt_first_line
                      && local_prompt.invis_chars_first_line
                      && _rl_last_c_pos >= local_prompt.invis_chars_first_line
                      && ((nfd - new_) <= local_prompt.last_invisible))
                    ADJUST_CPOS (local_prompt.invis_chars_first_line);

#if 1
#ifdef HANDLE_MULTIBYTE
                  /* If we write a non-space into the last screen column,
                     remove the note that we added a space to compensate for
                     a multibyte double-width character that didn't fit, since
                     it's only valid for what was previously there. */
                  /* XXX - watch this */
                  if (_rl_last_c_pos == _rl_screenwidth
                      && line_state_invisible->wrapped_line[current_line + 1]
                      && nfd[bytes_to_insert - 1] != ' ')
                    line_state_invisible->wrapped_line[current_line + 1] = 0;
#endif
#endif
                }
              else
                _rl_last_c_pos += bytes_to_insert;

              /* XXX - we only want to do this if we are at the end of the line
                 so we move there with _rl_move_cursor_relative */
              if (_rl_horizontal_scroll_mode && ((oe - old) > (ne - new_)))
                {
                  _rl_move_cursor_relative (
                      static_cast<unsigned int> (ne - new_), new_, new_face);
                  goto clear_rest_of_line;
                }
            }
        }
      /* Otherwise, print over the existing material. */
      else
        {
          if (temp > 0)
            {
              /* If nfd begins at the prompt, or before the invisible
                 characters in the prompt, we need to adjust _rl_last_c_pos
                 in a multibyte locale to account for the wrap offset and
                 set cpos_adjusted accordingly. */
              puts_face (nfd, nfdf, temp);
              _rl_last_c_pos += col_temp; /* XXX */
              if (mb_cur_max > 1 && !rl_byte_oriented)
                {
                  if (current_line == 0 && wrap_offset
                      && displaying_prompt_first_line
                      && _rl_last_c_pos > wrap_offset
                      && ((nfd - new_) <= local_prompt.last_invisible))
                    ADJUST_CPOS (
                        wrap_offset); /* XXX - prompt_invis_chars_first_line?
                                       */
                }
            }
        clear_rest_of_line:
          lendiff = static_cast<int> ((oe - old) - (ne - new_));
          if (mb_cur_max > 1 && !rl_byte_oriented)
            col_lendiff
                = static_cast<int> (_rl_col_width (
                      old, 0, static_cast<unsigned int> (oe - old), 1))
                  - static_cast<int> (_rl_col_width (
                      new_, 0, static_cast<unsigned int> (ne - new_), 1));
          else
            col_lendiff = lendiff;

          /* If we've already printed over the entire width of the screen,
             including the old material, then col_lendiff doesn't matter and
             space_to_eol will insert too many spaces.  XXX - maybe we should
             adjust col_lendiff based on the difference between _rl_last_c_pos
             and _rl_screenwidth */
          if (col_lendiff
              && ((mb_cur_max == 1 || rl_byte_oriented)
                  || (_rl_last_c_pos < _rl_screenwidth)))
            {
              if (_rl_term_autowrap && current_line < inv_botlin)
                space_to_eol (static_cast<unsigned int> (col_lendiff));
              else
                _rl_clear_to_eol (static_cast<unsigned int> (col_lendiff));
            }
        }
    }
}

/* Tell the update routines that we have moved onto a new line with the
   prompt already displayed.  Code originally from the version of readline
   distributed with CLISP.  rl_expand_prompt must have already been called
   (explicitly or implicitly).  This still doesn't work exactly right; it
   should use expand_prompt() */
int
Readline::rl_on_new_line_with_prompt ()
{
  /* Make sure the line structures hold the already-displayed prompt for
     redisplay. */
  const char *lprompt = !local_prompt.prompt.empty ()
                            ? local_prompt.prompt.c_str ()
                            : rl_prompt;
  visible_line = lprompt;
  invisible_line = lprompt;

  /* If the prompt contains newlines, take the last tail. */
  char *prompt_last_line = std::strrchr (rl_prompt, '\n');
  if (!prompt_last_line)
    prompt_last_line = rl_prompt;

  unsigned int l = static_cast<unsigned int> (std::strlen (prompt_last_line));
  if (MB_CUR_MAX > 1 && !rl_byte_oriented)
    _rl_last_c_pos = _rl_col_width (prompt_last_line, 0, l, 1); /* XXX */
  else
    _rl_last_c_pos = l;

  /* Dissect prompt_last_line into screen lines. Note that here we have
     to use the real screenwidth. Readline's notion of screenwidth might be
     one less, see terminal.c. */
  unsigned int real_screenwidth
      = _rl_screenwidth + (_rl_term_autowrap ? 0 : 1);
  _rl_last_v_pos = l / real_screenwidth;

  /* If the prompt length is a multiple of real_screenwidth, we don't know
     whether the cursor is at the end of the last line, or already at the
     beginning of the next line. Output a newline just to be safe. */
  if (l > 0 && (l % real_screenwidth) == 0)
    _rl_output_some_chars ("\n", 1);
  last_lmargin = 0;

  unsigned int newlines = 0, i = 0;
  while (i <= l)
    {
      _rl_vis_botlin = newlines;
      vis_lbreaks[newlines++] = i;
      i += real_screenwidth;
    }
  vis_lbreaks[newlines] = l;
  visible_wrap_offset = 0;

  rl_display_prompt = rl_prompt; /* XXX - make sure it's set */

  return 0;
}

/* Move the cursor from _rl_last_c_pos to NEW, which are buffer indices.
   (Well, when we don't have multibyte characters, _rl_last_c_pos is a
   buffer index.)
   DATA is the contents of the screen line of interest; i.e., where
   the movement is being done.
   DATA is always the visible line or the invisible line */
void
Readline::_rl_move_cursor_relative (unsigned int newcpos, const char *data,
                                    const char *dataf)
{
  unsigned int woff;       /* number of invisible chars on current line */
  unsigned int cpos, dpos; /* current and desired cursor positions */
  int adjust;
  size_t mb_cur_max = MB_CUR_MAX;

  woff = WRAP_OFFSET (_rl_last_v_pos, wrap_offset);
  cpos = _rl_last_c_pos;

  if (cpos == 0 && cpos == newcpos)
    return;

#if defined(HANDLE_MULTIBYTE)
  /* If we have multibyte characters, NEW is indexed by the buffer point in
     a multibyte string, but _rl_last_c_pos is the display position.  In
     this case, NEW's display position is not obvious and must be
     calculated.  We need to account for invisible characters in this line,
     as long as we are past them and they are counted by _rl_col_width. */
  if (mb_cur_max > 1 && !rl_byte_oriented)
    {
      adjust = 1;
      /* Try to short-circuit common cases and eliminate a bunch of multibyte
         character function calls. */
      /* 1.  prompt string */
      if (newcpos == local_prompt.prompt.size ()
          && local_prompt.prompt == data)
        {
          dpos = local_prompt.physical_chars;
          cpos_adjusted = true;
          adjust = 0;
        }
      /* 2.  prompt_string + line contents */
      else if (newcpos > local_prompt.prompt.size ()
               && !local_prompt.prompt.empty () && local_prompt.prompt == data)
        {
          dpos = local_prompt.physical_chars
                 + _rl_col_width (
                     data,
                     static_cast<unsigned int> (local_prompt.prompt.size ()),
                     newcpos, 1);
          cpos_adjusted = true;
          adjust = 0;
        }
      else
        dpos = _rl_col_width (data, 0, newcpos, 1);

      if (!displaying_prompt_first_line)
        adjust = 0;

      /* yet another special case: printing the last line of a prompt with
         multibyte characters and invisible characters whose printable length
         exceeds the screen width with the last invisible character
         (prompt_last_invisible) in the last line.  IN_INVISLINE is the
         offset of DATA in invisible_line */
      unsigned int in_invisline = 0;
      if (data > invisible_line
          && data < &invisible_line[static_cast<size_t> (
                 inv_lbreaks[static_cast<size_t> (_rl_inv_botlin + 1)])])
        in_invisline = static_cast<unsigned int> (data - &invisible_line[0]);

      /* Use NEW when comparing against the last invisible character in the
         prompt string, since they're both buffer indices and DPOS is a
         desired display position. */
      /* NEW is relative to the current displayed line, while
         PROMPT_LAST_INVISIBLE is relative to the entire (wrapped) line.
         Need a way to reconcile these two variables by turning NEW into a
         buffer position relative to the start of the line */
      if (adjust
          && ((newcpos > local_prompt.last_invisible)
              || /* XXX - don't use woff here */
              (newcpos + in_invisline > local_prompt.last_invisible)
              || /* invisible line */
              (local_prompt.physical_chars >= _rl_screenwidth
               && /* visible line */
               _rl_last_v_pos == prompt_last_screen_line && wrap_offset >= woff
               && dpos >= woff
               && newcpos
                      > (local_prompt.last_invisible
                         - (vis_lbreaks[static_cast<size_t> (_rl_last_v_pos)])
                         - wrap_offset))))
        /* XXX last comparison might need to be >= */
        {
          dpos -= woff;
          /* Since this will be assigned to _rl_last_c_pos at the end (more
             precisely, _rl_last_c_pos == dpos when this function returns),
             let the caller know. */
          cpos_adjusted = true;
        }
    }
  else
#endif
    dpos = newcpos;

  /* If we don't have to do anything, then return. */
  if (cpos == dpos)
    return;

  /* It may be faster to output a CR, and then move forwards instead
     of moving backwards. */
  /* i == current physical cursor position. */
  unsigned int i;
#if defined(HANDLE_MULTIBYTE)
  if (mb_cur_max > 1 && !rl_byte_oriented)
    i = _rl_last_c_pos;
  else
#endif
    i = _rl_last_c_pos - woff;
  if (dpos == 0 || CR_FASTER (dpos, _rl_last_c_pos)
      || (_rl_term_autowrap && i == _rl_screenwidth))
    {
      _rl_cr ();
      cpos = _rl_last_c_pos = 0;
    }

  if (cpos < dpos)
    {
      /* Move the cursor forward.  We do it by printing the command
         to move the cursor forward if there is one, else print that
         portion of the output buffer again.  Which is cheaper? */

      /* The above comment is left here for posterity.  It is faster
         to print one character (non-control) than to print a control
         sequence telling the terminal to move forward one character.
         That kind of control is for people who don't know what the
         data is underneath the cursor. */

      /* However, we need a handle on where the current display position is
         in the buffer for the immediately preceding comment to be true.
         In multibyte locales, we don't currently have that info available.
         Without it, we don't know where the data we have to display begins
         in the buffer and we have to go back to the beginning of the screen
         line.  In this case, we can use the terminal sequence to move forward
         if it's available. */
      if (mb_cur_max > 1 && !rl_byte_oriented)
        {
          if (_rl_term_forward_char)
            {
              for (i = cpos; i < dpos; i++)
                ::tputs (_rl_term_forward_char, 1,
                         &_rl_output_character_function);
            }
          else
            {
              _rl_cr ();
              puts_face (data, dataf, newcpos);
            }
        }
      else
        puts_face (data + cpos, dataf + cpos, newcpos - cpos);
    }

#if defined(HANDLE_MULTIBYTE)
    /* NEW points to the buffer point, but _rl_last_c_pos is the display point.
       The byte length of the string is probably bigger than the column width
       of the string, which means that if NEW == _rl_last_c_pos, then NEW's
       display point is less than _rl_last_c_pos. */
#endif
  else if (cpos > dpos)
    _rl_backspace (cpos - dpos);

  _rl_last_c_pos = dpos;
}

/* PWP: move the cursor up or down. */
void
Readline::_rl_move_vert (unsigned int to)
{
  if (_rl_last_v_pos == to || to > _rl_screenheight)
    return;

  int delta;
  if ((delta = static_cast<int> (to) - static_cast<int> (_rl_last_v_pos)) > 0)
    {
      for (int i = 0; i < delta; i++)
        std::putc ('\n', rl_outstream);
      _rl_cr ();
      _rl_last_c_pos = 0;
    }
  else
    { /* delta < 0 */
#ifdef __DJGPP__
      int row, col;

      std::fflush (rl_outstream);
      ScreenGetCursor (&row, &col);
      ScreenSetCursor (row + delta, col);
      i = -delta;
#else
      if (_rl_term_up && *_rl_term_up)
        for (int i = 0; i < -delta; i++)
          ::tputs (_rl_term_up, 1, &_rl_output_character_function);
#endif /* !__DJGPP__ */
    }

  _rl_last_v_pos = to; /* Now TO is here */
}

/* Physically print C on rl_outstream.  This is for functions which know
   how to optimize the display.  Return the number of characters output. */
int
Readline::rl_show_char (int c)
{
  int n = 1;
  if (META_CHAR (c) && (_rl_output_meta_chars == 0))
    {
      std::fprintf (rl_outstream, "M-");
      n += 2;
      c = UNMETA (c);
    }

#if defined(DISPLAY_TABS)
  if ((CTRL_CHAR (c) && c != '\t') || c == RUBOUT)
#else
  if (CTRL_CHAR (c) || c == RUBOUT)
#endif /* !DISPLAY_TABS */
    {
      std::fprintf (rl_outstream, "C-");
      n += 2;
      c = CTRL_CHAR (c) ? UNCTRL (c) : '?';
    }

  std::putc (c, rl_outstream);
  std::fflush (rl_outstream);
  return n;
}

/* How to print things in the "echo-area".  The prompt is treated as a
   mini-modeline. */

int
Readline::rl_message (const char *format, ...)
{
  va_list args;
  int bneed;

  va_start (args, format);

  bneed = std::vsnprintf (&msg_buf[0], msg_buf.size (), format, args);
  if (bneed > static_cast<int> (msg_buf.size ()))
    {
      msg_buf.resize (static_cast<size_t> (bneed + 1));
      bneed = std::vsnprintf (&msg_buf[0], msg_buf.size (), format, args);
    }
  msg_buf.resize (static_cast<size_t> (bneed));
  va_end (args);

  if (saved_local_prompt.prompt.empty ())
    {
      rl_save_prompt ();
      msg_saved_prompt = true;
    }
  else if (local_prompt.prompt != saved_local_prompt.prompt)
    {
      local_prompt.prompt.clear ();
      local_prompt.prefix.clear ();
    }
  rl_display_prompt = msg_buf.c_str ();
  local_prompt.prompt = expand_prompt (
      msg_buf, PMT_NONE, &local_prompt.visible_length,
      &local_prompt.last_invisible, &local_prompt.invis_chars_first_line,
      &local_prompt.physical_chars);

  local_prompt.prefix.clear ();
  ((*this).*rl_redisplay_function) ();

  return 0;
}

/* Save all of the variables associated with the prompt and its display. Most
   of the complexity is dealing with the invisible characters in the prompt
   string and where they are. There are enough of these that I should consider
   a struct. */
void
Readline::rl_save_prompt ()
{
  saved_local_prompt = local_prompt;

  local_prompt.prompt.clear ();
  local_prompt.prefix.clear ();
  local_prompt.newlines.clear ();

  local_prompt.last_invisible = local_prompt.visible_length = 0;
  local_prompt.invis_chars_first_line = local_prompt.physical_chars = 0;
}

void
Readline::rl_restore_prompt ()
{
  local_prompt = saved_local_prompt;

  /* can test saved_local_prompt to see if prompt info has been saved. */
  saved_local_prompt.prompt.clear ();
  saved_local_prompt.prefix.clear ();
  saved_local_prompt.newlines.clear ();

  saved_local_prompt.last_invisible = saved_local_prompt.visible_length = 0;
  saved_local_prompt.invis_chars_first_line = saved_local_prompt.physical_chars
      = 0;
}

char *
Readline::_rl_make_prompt_for_search (char pchar)
{
  char *pmt, *p;

  rl_save_prompt ();

  /* We've saved the prompt, and can do anything with the various prompt
     strings we need before they're restored.  We want the unexpanded
     portion of the prompt string after any final newline. */
  p = rl_prompt ? std::strrchr (rl_prompt, '\n') : nullptr;
  if (p == nullptr)
    {
      size_t len = (rl_prompt && *rl_prompt) ? std::strlen (rl_prompt) : 0;
      pmt = new char[len + 2];
      if (len)
        std::strcpy (pmt, rl_prompt);
      pmt[len] = pchar;
      pmt[len + 1] = '\0';
    }
  else
    {
      p++;
      size_t len = std::strlen (p);
      pmt = new char[len + 2];
      if (len)
        std::strcpy (pmt, p);
      pmt[len] = pchar;
      pmt[len + 1] = '\0';
    }

  /* will be overwritten by expand_prompt, called from rl_message */
  local_prompt.physical_chars = saved_local_prompt.physical_chars + 1;
  return pmt;
}

/* Insert COL spaces, keeping the cursor at the same position.  We follow the
   ncurses documentation and use either im/ei with explicit spaces, or IC/ic
   by itself.  We assume there will either be ei or we don't need to use it. */
void
Readline::open_some_spaces (unsigned int col)
{
#if !defined(__MSDOS__) && (!defined(__MINGW32__) || defined(NCURSES_VERSION))
  char *buffer;

  /* If IC is defined, then we do not have to "enter" insert mode. */
  if (_rl_term_IC)
    {
      buffer = ::tgoto (_rl_term_IC, 0, static_cast<int> (col));
      ::tputs (buffer, 1, &_rl_output_character_function);
    }
  else if (_rl_term_im && *_rl_term_im)
    {
      ::tputs (_rl_term_im, 1, &_rl_output_character_function);
      /* just output the desired number of spaces */
      for (unsigned int i = col; i--;)
        _rl_output_character_function (' ');
      /* If there is a string to turn off insert mode, use it now. */
      if (_rl_term_ei && *_rl_term_ei)
        ::tputs (_rl_term_ei, 1, &_rl_output_character_function);
      /* and move back the right number of spaces */
      _rl_backspace (col);
    }
  else if (_rl_term_ic && *_rl_term_ic)
    {
      /* If there is a special command for inserting characters, then
         use that first to open up the space. */
      for (unsigned int i = col; i--;)
        ::tputs (_rl_term_ic, 1, &_rl_output_character_function);
    }
#endif /* !__MSDOS__ && (!__MINGW32__ || NCURSES_VERSION)*/
}

/* Delete COUNT characters from the display line. */
void
Readline::delete_chars (int count)
{
  if (count <= 0 || count > static_cast<int> (_rl_screenwidth)) /* XXX */
    return;

#if !defined(__MSDOS__) && (!defined(__MINGW32__) || defined(NCURSES_VERSION))
  if (_rl_term_DC && *_rl_term_DC)
    {
      char *buffer;
      buffer = ::tgoto (_rl_term_DC, count, count);
      ::tputs (buffer, count, &_rl_output_character_function);
    }
  else
    {
      if (_rl_term_dc && *_rl_term_dc)
        while (count--)
          ::tputs (_rl_term_dc, 1, &_rl_output_character_function);
    }
#endif /* !__MSDOS__ && (!__MINGW32__ || NCURSES_VERSION)*/
}

void
Readline::_rl_update_final ()
{
  if (!line_structures_initialized)
    return;

  bool full_lines = false;
  /* If the cursor is the only thing on an otherwise-blank last line,
     compensate so we don't print an extra CRLF. */
  if (_rl_vis_botlin && _rl_last_c_pos == 0
      && visible_line[static_cast<size_t> (
             vis_lbreaks[static_cast<size_t> (_rl_vis_botlin)])]
             == 0)
    {
      _rl_vis_botlin--;
      full_lines = true;
    }
  _rl_move_vert (_rl_vis_botlin);
  unsigned int woff = W_OFFSET (_rl_vis_botlin, wrap_offset);
  unsigned int botline_length
      = static_cast<unsigned int> (VIS_LLEN (_rl_vis_botlin)) - woff;
  /* If we've wrapped lines, remove the final xterm line-wrap flag. */
  if (full_lines && _rl_term_autowrap && botline_length == _rl_screenwidth)
    {
      char *last_line, *last_face;

      /* LAST_LINE includes invisible characters, so if you want to get the
         last character of the first line, you have to take WOFF into account.
         This needs to be done for both calls to _rl_move_cursor_relative,
         which takes a buffer position as the first argument, and any direct
         subscripts of LAST_LINE. */
      size_t vis_botlin = static_cast<size_t> (_rl_vis_botlin);
      size_t line_index = static_cast<size_t> (vis_lbreaks[vis_botlin]);
      last_line = &visible_line[line_index]; /* = VIS_CHARS(_rl_vis_botlin); */
      last_face = &vis_face[line_index];     /* = VIS_CHARS(_rl_vis_botlin); */
      cpos_buffer_position = static_cast<unsigned int> (
          -1); /* don't know where we are in buffer */
      _rl_move_cursor_relative (_rl_screenwidth - 1 + woff, last_line,
                                last_face);
      _rl_clear_to_eol (0);
      puts_face (&last_line[_rl_screenwidth + woff - 1],
                 &last_face[_rl_screenwidth + woff - 1], 1);
    }
  _rl_vis_botlin = 0;
  if (botline_length > 0 || _rl_last_c_pos > 0)
    rl_crlf ();
  std::fflush (rl_outstream);
  rl_display_fixed = true;
}

/* Redraw the last line of a multi-line prompt that may possibly contain
   terminal escape sequences.  Called with the cursor at column 0 of the
   line to draw the prompt on. */
void
Readline::redraw_prompt (const char *t)
{
  const char *oldp;

  oldp = rl_display_prompt;
  rl_save_prompt ();

  rl_display_prompt = t;
  local_prompt.prompt = expand_prompt (
      t, PMT_MULTILINE, &local_prompt.visible_length,
      &local_prompt.last_invisible, &local_prompt.invis_chars_first_line,
      &local_prompt.physical_chars);

  local_prompt.prefix.clear ();

  rl_forced_update_display ();

  rl_display_prompt = oldp;
  rl_restore_prompt ();
}

/* Redisplay the current line after a SIGWINCH is received. */
void
Readline::_rl_redisplay_after_sigwinch ()
{
  /* Clear the last line (assuming that the screen size change will result in
     either more or fewer characters on that line only) and put the cursor at
     column 0.  Make sure the right thing happens if we have wrapped to a new
     screen line. */
  if (_rl_term_cr)
    {
      _rl_move_vert (_rl_vis_botlin);

      _rl_cr ();
      _rl_last_c_pos = 0;

#if !defined(__MSDOS__)
      if (_rl_term_clreol)
        ::tputs (_rl_term_clreol, 1, &_rl_output_character_function);
      else
#endif
        {
          space_to_eol (_rl_screenwidth);
          _rl_cr ();
        }

      if (_rl_last_v_pos > 0)
        _rl_move_vert (0);
    }
  else
    rl_crlf ();

  /* Redraw only the last line of a multi-line prompt. */
  const char *t = std::strrchr (rl_display_prompt, '\n');
  if (t)
    redraw_prompt (++t);
  else
    rl_forced_update_display ();
}

#if defined(HANDLE_MULTIBYTE)
/* Calculate the number of screen columns occupied by STR from START to END.
   In the case of multibyte characters with stateful encoding, we have to
   scan from the beginning of the string to take the state into account. */
unsigned int
Readline::_rl_col_width (const char *str, unsigned int start, unsigned int end,
                         int flags)
{
  wchar_t wc;
  mbstate_t ps;

  if (end <= start)
    return 0;

  if (MB_CUR_MAX == 1 || rl_byte_oriented)
    /* this can happen in some cases where it's inconvenient to check */
    return end - start;

  std::memset (&ps, 0, sizeof (mbstate_t));

  unsigned int point = 0;
  unsigned int max = end;

  /* Try to short-circuit common cases.  The adjustment to remove wrap_offset
     is done by the caller. */
  /* 1.  prompt string */
  if (flags && start == 0 && end == local_prompt.prompt.size ()
      && local_prompt.prompt == str)
    return local_prompt.physical_chars + wrap_offset;

  /* 2.  prompt string + line contents */
  else if (flags && start == 0 && !local_prompt.prompt.empty ()
           && end > local_prompt.prompt.size () && local_prompt.prompt == str)
    {
      unsigned int tmp = local_prompt.physical_chars + wrap_offset;
      /* XXX - try to call ourselves recursively with non-prompt portion */
      tmp += _rl_col_width (
          str, static_cast<unsigned int> (local_prompt.prompt.size ()), end,
          flags);
      return tmp;
    }

  while (point < start)
    {
      size_t tmp;
      if (_rl_utf8locale && UTF8_SINGLEBYTE (str[point]))
        {
          std::memset (&ps, 0, sizeof (mbstate_t));
          tmp = 1;
        }
      else
        tmp = std::mbrlen (str + point, max, &ps);
      if (MB_INVALIDCH (tmp))
        {
          /* In this case, the bytes are invalid or too short to compose a
             multibyte character, so we assume that the first byte represents
             a single character. */
          point++;
          max--;

          /* Clear the state of the byte sequence, because in this case the
             effect of mbstate is undefined. */
          std::memset (&ps, 0, sizeof (mbstate_t));
        }
      else if (MB_NULLWCH (tmp))
        break; /* Found '\0' */
      else
        {
          point += tmp;
          max -= tmp;
        }
    }

  /* If START is not a byte that starts a character, then POINT will be
     greater than START.  In this case, assume that (POINT - START) gives
     a byte count that is the number of columns of difference. */
  unsigned int width = point - start;

  while (point < end)
    {
      size_t tmp;
      if (_rl_utf8locale && UTF8_SINGLEBYTE (str[point]))
        {
          tmp = 1;
          wc = static_cast<wchar_t> (str[point]);
        }
      else
        tmp = std::mbrtowc (&wc, str + point, max, &ps);
      if (MB_INVALIDCH (tmp))
        {
          /* In this case, the bytes are invalid or too short to compose a
             multibyte character, so we assume that the first byte represents
             a single character. */
          point++;
          max--;

          /* and assume that the byte occupies a single column. */
          width++;

          /* Clear the state of the byte sequence, because in this case the
             effect of mbstate is undefined. */
          std::memset (&ps, 0, sizeof (mbstate_t));
        }
      else if (MB_NULLWCH (tmp))
        break; /* Found '\0' */
      else
        {
          point += tmp;
          max -= tmp;
          width += static_cast<unsigned int> (WCWIDTH (wc));
        }
    }

  width += point - end;

  return width;
}

#endif /* HANDLE_MULTIBYTE */

} // namespace readline
