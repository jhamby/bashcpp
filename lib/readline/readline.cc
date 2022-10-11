/* readline.cc -- a general facility for reading lines of input
   with emacs style editing and completion. */

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

#include "readline.hh"
#include "history.hh"
#include "rlprivate.hh"

#include <sys/types.h>

#include "posixstat.hh"

#include <fcntl.h>
#if defined(HAVE_SYS_FILE_H)
#include <sys/file.h>
#endif /* HAVE_SYS_FILE_H */

#if defined(__EMX__)
#define INCL_DOSPROCESS
#include <os2.h>
#endif /* __EMX__ */

namespace readline
{

#ifndef RL_LIBRARY_VERSION
#define RL_LIBRARY_VERSION "9.0"
#endif

#ifndef RL_READLINE_VERSION
#define RL_READLINE_VERSION 0x0900
#endif

/* **************************************************************** */
/*								    */
/*			Top Level Functions			    */
/*								    */
/* **************************************************************** */

// This constructor has parameters for all of the defaults that bash overrides.
// We can add more params later, as needed by other applications.
Readline::Readline (const char *search_delimiter_chars,
                    const char *event_delimiter_chars,
                    rl_linebuf_func_t inhibit_expansion_function,
                    bool quotes_inhibit_expansion)
    : History (search_delimiter_chars, event_delimiter_chars,
               inhibit_expansion_function, quotes_inhibit_expansion),
      rl_library_version (RL_LIBRARY_VERSION),
      rl_prep_term_function (&Readline::rl_prep_terminal),
      rl_deprep_term_function (&Readline::rl_deprep_terminal),
      rl_basic_word_break_characters (" \t\n\"\\'`@$><=;|&{("),
      rl_completer_quote_characters ("'\""), rl_basic_quote_characters ("\"'"),
      rl_filename_quoting_function (&Readline::rl_quote_filename),
      rl_redisplay_function (&Readline::rl_redisplay),
      _rl_keymap (emacs_standard_keymap ()),
      line_state_visible (&line_state_array[0]),
      line_state_invisible (&line_state_array[1]), _rl_completion_columns (-1),
      cpos_buffer_position (static_cast<unsigned int> (-1)),
      _rl_history_saved_point (static_cast<unsigned int> (-1)),
      saved_history_logical_offset (static_cast<unsigned int> (-1)),
      rl_kill_ring_index (DEFAULT_MAX_KILLS - 1),
      rl_readline_version (RL_READLINE_VERSION), rl_numeric_arg (1),
      rl_completion_query_items (100), rl_readline_state (RL_STATE_NONE),
      rl_editing_mode (emacs_mode), _rl_bell_preference (AUDIBLE_BELL),
      _rl_eof_char (CTRL ('D')), rl_completion_append_character (' '),
      _paren_blink_usec (500000), rl_arg_sign (1), _rl_keyseq_timeout (500),
      _keyboard_input_timeout (100000), _rl_vi_last_command ('i'),
      _rl_vi_last_repeat (1), _rl_vi_last_arg_sign (1),
      _rl_yank_count_passed (1), _rl_yank_direction (1),
      rl_gnu_readline_p (true), rl_insert_mode (RL_IM_DEFAULT),
      rl_catch_signals (true),
#ifdef SIGWINCH
      rl_catch_sigwinch (true),
#endif
      rl_change_environment (true), rl_sort_completion_matches (true),
      _rl_term_autowrap (-1), _rl_prefer_visible_bell (true),
      _rl_enable_meta (true), _rl_complete_mark_directories (true),
#if (defined(__MSDOS__) && !defined(__DJGPP__))                               \
    || (defined(_WIN32) && !defined(__CYGWIN__))
      _rl_completion_case_fold (true),
#endif
      _rl_match_hidden_files (true), _rl_page_completions (true),
      _rl_convert_meta_chars_to_ascii (true), _rl_bind_stty_chars (true),
      _rl_echo_control_chars (true),
      _rl_enable_bracketed_paste (BRACKETED_PASTE_DEFAULT),
      _rl_enable_active_region (BRACKETED_PASTE_DEFAULT),
      _rl_optimize_typeahead (true)
{
  the_app = this;
}

// Virtual destructor deletes any lazily-allocated arrays.
Readline::~Readline ()
{
  delete[] emacs_standard_keymap_;
  delete[] emacs_meta_keymap_;
  delete[] emacs_ctlx_keymap_;

#if defined(VI_MODE)
  delete[] vi_insertion_keymap_;
  delete[] vi_movement_keymap_;
#endif

  delete[] _rl_color_indicator;

  // TODO: add anything else we need to delete
}

// Human-readable description for rl_exception, which also
// stops the class vtable from being emitted in every .o file.
const char *
rl_exception::what () const noexcept
{
  return "readline::rl_exception";
}

// Human-readable description for word_not_found exception,
// which is defined as a subclass of readline::rl_exception.
const char *
word_not_found::what () const noexcept
{
  return "readline::word_not_found";
}

/* Set up the prompt and expand it.  Called from readline() and
   rl_callback_handler_install (). */
void
Readline::rl_set_prompt (const std::string &prompt)
{
  rl_prompt = prompt;
  rl_display_prompt = rl_prompt;

  rl_visible_prompt_length
      = rl_expand_prompt (const_cast<char *> (rl_prompt.c_str ()));
}

/* Read a line of input.  Prompt with PROMPT.  An empty PROMPT means
   none.  A return value of nullptr means that EOF was encountered. */
std::string
Readline::readline (const std::string &prompt)
{
#if 0
  int in_callback;
#endif

  /* If we are at EOF return nullptr. */
  if (rl_pending_input == EOF)
    {
      rl_clear_pending_input ();
      return nullptr;
    }

#if 0
  /* If readline() is called after installing a callback handler, temporarily
     turn off the callback state to avoid ensuing messiness.  Patch supplied
     by the gdb folks.  XXX -- disabled.  This can be fooled and readline
     left in a strange state by a poorly-timed longjmp. */
  if (in_callback = RL_ISSTATE (RL_STATE_CALLBACK))
    RL_UNSETSTATE (RL_STATE_CALLBACK);
#endif

  rl_set_prompt (prompt);

  rl_initialize ();
  if (rl_prep_term_function)
    ((*this).*rl_prep_term_function) (_rl_meta_flag);

#if defined(HANDLE_SIGNALS)
  rl_set_signals ();
#endif

  std::string value = readline_internal ();

  if (rl_deprep_term_function)
    ((*this).*rl_deprep_term_function) ();

#if defined(HANDLE_SIGNALS)
  rl_clear_signals ();
#endif

  return value;
}

void
Readline::readline_internal_setup ()
{
  _rl_in_stream = rl_instream;
  _rl_out_stream = rl_outstream;

  /* Enable the meta key only for the duration of readline(), if this
     terminal has one and the terminal has been initialized */
  if (_rl_enable_meta & RL_ISSTATE (RL_STATE_TERMPREPPED))
    _rl_enable_meta_key ();

  if (rl_startup_hook)
    ((*this).*rl_startup_hook) ();

  if (_rl_internal_startup_hook)
    ((*this).*_rl_internal_startup_hook) ();

  rl_deactivate_mark ();

#if defined(VI_MODE)
  if (rl_editing_mode == vi_mode)
    rl_vi_insertion_mode (1, 'i'); /* don't want to reset last */
  else
#endif /* VI_MODE */
    if (_rl_show_mode_in_prompt)
      _rl_reset_prompt ();

  /* If we're not echoing, we still want to at least print a prompt, because
     rl_redisplay will not do it for us.  If the calling application has a
     custom redisplay function, though, let that function handle it. */
  if (!_rl_echoing_p && rl_redisplay_function == &Readline::rl_redisplay)
    {
      if (!rl_prompt.empty () && !rl_already_prompted)
        {
          std::string nprompt = _rl_strip_prompt (rl_prompt);
          std::fprintf (_rl_out_stream, "%s", nprompt.c_str ());
          std::fflush (_rl_out_stream);
        }
    }
  else
    {
      if (!rl_prompt.empty () && rl_already_prompted)
        rl_on_new_line_with_prompt ();
      else
        rl_on_new_line ();
      ((*this).*rl_redisplay_function) ();
    }

  if (rl_pre_input_hook)
    ((*this).*rl_pre_input_hook) ();

  RL_CHECK_SIGNALS ();
}

std::string
Readline::readline_internal_teardown (bool eof)
{
  char *temp;
  HIST_ENTRY *entry;

  RL_CHECK_SIGNALS ();

  /* Restore the original of this history line, iff the line that we
     are editing was originally in the history, AND the line has changed. */
  entry = current_history ();

  if (entry && rl_undo_list)
    {
      temp = savestring (rl_line_buffer);
      rl_revert_line (1, 0);
      entry
          = replace_history_entry (where_history (), rl_line_buffer, nullptr);
      delete entry;

      rl_line_buffer = temp;
      delete[] temp;
    }

  if (_rl_revert_all_at_newline)
    _rl_revert_all_lines ();

  /* At any rate, it is highly likely that this line has an undo list.  Get
     rid of it now. */
  if (rl_undo_list)
    rl_free_undo_list ();

  /* Disable the meta key, if this terminal has one and we were told to use it.
     The check whether or not we sent the enable string is in
     _rl_disable_meta_key(); the flag is set in _rl_enable_meta_key */
  _rl_disable_meta_key ();

  /* Restore normal cursor, if available. */
  _rl_set_insert_mode (RL_IM_INSERT, 0);

  return eof ? std::string () : rl_line_buffer;
}

void
Readline::_rl_internal_char_cleanup ()
{
#if defined(VI_MODE)
  /* In vi mode, when you exit insert mode, the cursor moves back
     over the previous character.  We explicitly check for that here. */
  if (rl_editing_mode == vi_mode && _rl_keymap == vi_movement_keymap ())
    rl_vi_check ();
#endif /* VI_MODE */

  int rl_end = static_cast<int> (rl_line_buffer.size ());
  if (rl_num_chars_to_read && rl_end >= rl_num_chars_to_read)
    {
      ((*this).*rl_redisplay_function) ();
      _rl_want_redisplay = 0;
      rl_newline (1, '\n');
    }

  if (rl_done == 0)
    {
      ((*this).*rl_redisplay_function) ();
      _rl_want_redisplay = 0;
    }

  /* If the application writer has told us to erase the entire line if
     the only character typed was something bound to rl_newline, do so. */
  if (rl_erase_empty_line && rl_done && rl_last_func == &Readline::rl_newline
      && rl_point == 0 && rl_line_buffer.empty ())
    _rl_erase_entire_line ();
}

int
#if defined(READLINE_CALLBACKS)
Readline::readline_internal_char ()
#else
Readline::readline_internal_charloop ()
#endif
{
  int lastc = EOF;

#if !defined(READLINE_CALLBACKS)
  bool eof_found = false;
  while (!rl_done)
    {
#endif
      bool lk = _rl_last_command_was_kill;

      try
        {
          if (rl_pending_input == 0)
            {
              /* Then initialize the argument and number of keys read. */
              _rl_reset_argument ();
              rl_executing_keyseq.clear ();
            }

          RL_SETSTATE (RL_STATE_READCMD);
          int c = rl_read_key ();
          RL_UNSETSTATE (RL_STATE_READCMD);

          /* look at input.c:rl_getc() for the circumstances under which this
             will be returned; punt immediately on read error without
             converting it to a newline; assume that rl_read_key has already
             called the signal handler. */
          if (c == READERR)
            {
#if defined(READLINE_CALLBACKS)
              RL_SETSTATE (RL_STATE_DONE);
              rl_done = true;
              return 1;
#else
          eof_found = true;
          break;
#endif
            }

          /* EOF typed to a non-blank line is ^D the first time, EOF the second
             time in a row.  This won't return any partial line read from the
             tty. If we want to change this, to force any existing line to be
             returned when read(2) reads EOF, for example, this is the place to
             change. */
          if (c == EOF && !rl_line_buffer.empty ())
            {
              if (RL_SIG_RECEIVED ())
                {
                  RL_CHECK_SIGNALS ();
                  if (rl_signal_event_hook)
                    ((*this).*rl_signal_event_hook) (); /* XXX */
                }

              /* XXX - reading two consecutive EOFs returns EOF */
              if (RL_ISSTATE (RL_STATE_TERMPREPPED))
                {
                  if (lastc == _rl_eof_char || lastc == EOF)
                    rl_line_buffer.clear ();
                  else
                    c = _rl_eof_char;
                }
              else
                c = NEWLINE;
            }

          /* The character _rl_eof_char typed to blank line, and not as the
             previous character is interpreted as EOF.  This doesn't work when
             READLINE_CALLBACKS is defined, so hitting a series of ^Ds will
             erase all the chars on the line and then return EOF. */
          if (((c == _rl_eof_char && lastc != c) || c == EOF)
              && rl_line_buffer.empty ())
            {
#if defined(READLINE_CALLBACKS)
              RL_SETSTATE (RL_STATE_DONE);
              return rl_done = 1;
#else
          eof_found = true;
          break;
#endif
            }

          lastc = c;
          (void)_rl_dispatch (static_cast<unsigned char> (c), _rl_keymap);
          RL_CHECK_SIGNALS ();

          if (_rl_command_to_execute)
            {
              ((*this).*rl_redisplay_function) ();

              rl_executing_keymap = _rl_command_to_execute->map;
              rl_executing_key = _rl_command_to_execute->key;

              rl_dispatching = 1;
              RL_SETSTATE (RL_STATE_DISPATCHING);
              // ignore return value
              (void)((*this).*(_rl_command_to_execute->func)) (
                  _rl_command_to_execute->count, _rl_command_to_execute->key);
              _rl_command_to_execute = nullptr;
              RL_UNSETSTATE (RL_STATE_DISPATCHING);
              rl_dispatching = 0;

              RL_CHECK_SIGNALS ();
            }

          /* If there was no change in _rl_last_command_was_kill, then no kill
             has taken place.  Note that if input is pending we are reading
             a prefix command, so nothing has changed yet. */
          if (rl_pending_input == 0 && lk == _rl_last_command_was_kill)
            _rl_last_command_was_kill = false;

          if (_rl_keep_mark_active)
            _rl_keep_mark_active = 0;
          else if (rl_mark_active_p ())
            rl_deactivate_mark ();

          _rl_internal_char_cleanup ();
        }
      catch (const std::exception &)
        {
          ((*this).*rl_redisplay_function) ();
          _rl_want_redisplay = false;

          if (RL_ISSTATE (RL_STATE_CALLBACK))
            return 0;

          // XXX the original setjmp code continues at the top of try block
          std::abort ();
        }

#if defined(READLINE_CALLBACKS)
      return 0;
#else
}

return eof_found;
#endif
    }

#if defined(READLINE_CALLBACKS)
  int Readline::_rl_dispatch_callback (_rl_keyseq_cxt * cxt)
  {
    int nkey, r;

    /* For now */
    /* The first time this context is used, we want to read input and dispatch
       on it.  When traversing the chain of contexts back `up', we want to use
       the value from the next context down.  We're simulating recursion using
       a chain of contexts. */
    if ((cxt->flags & KSEQ_DISPATCHED) == 0)
      {
        nkey = _rl_subseq_getchar (cxt->okey);
        if (nkey < 0)
          {
            _rl_abort_internal ();
            return -1;
          }
        r = _rl_dispatch_subseq (nkey, cxt->dmap, cxt->subseq_arg);
        cxt->flags |= KSEQ_DISPATCHED;
      }
    else
      r = cxt->childval;

    /* For now */
    if (r != -3) /* don't do this if we indicate there will be other matches */
      r = _rl_subseq_result (r, cxt->oldmap, cxt->okey,
                             (cxt->flags & KSEQ_SUBSEQ));

    RL_CHECK_SIGNALS ();
    /* We only treat values < 0 specially to simulate recursion. */
    if (r >= 0
        || (r == -1
            && (cxt->flags & KSEQ_SUBSEQ) == 0)) /* success! or failure! */
      {
        _rl_keyseq_chain_dispose ();
        RL_UNSETSTATE (RL_STATE_MULTIKEY);
        return r;
      }

    if (r != -3) /* magic value that says we added to the chain */
      _rl_kscxt = cxt->ocxt;
    if (_rl_kscxt)
      _rl_kscxt->childval = r;
    if (r != -3)
      delete cxt;

    return r;
  }
#endif /* READLINE_CALLBACKS */

  /* Do the command associated with KEY in MAP.
     If the associated command is really a keymap, then read
     another key, and dispatch into that map. */
  int Readline::_rl_dispatch (int key, Keymap map)
  {
    _rl_dispatching_keymap = map;
    return _rl_dispatch_subseq (key, map, 0);
  }

  int Readline::_rl_dispatch_subseq (int key, Keymap map, int got_subseq)
  {
    int r, newkey;
    char *macro;
    rl_command_func_t func;
#if defined(READLINE_CALLBACKS)
    _rl_keyseq_cxt *cxt;
#endif

    if (META_CHAR (key) && _rl_convert_meta_chars_to_ascii)
      {
        if (map[ESC].type == ISKMAP)
          {
            if (RL_ISSTATE (RL_STATE_MACRODEF))
              _rl_add_macro_char (ESC);
            rl_executing_keyseq.push_back (ESC);
            map = map[ESC].value.map;
            key = UNMETA (key);
            return _rl_dispatch (key, map);
          }
        else
          rl_ding ();
        return 0;
      }

    if (RL_ISSTATE (RL_STATE_MACRODEF))
      _rl_add_macro_char (static_cast<char> (key));

    r = 0;
    switch (map[key].type)
      {
      case ISFUNC:
        func = map[key].value.function;
        if (func)
          {
            /* Special case rl_do_lowercase_version (). */
            if (func == &Readline::rl_do_lowercase_version)
              /* Should we do anything special if key == ANYOTHERKEY? */
              return _rl_dispatch (_rl_to_lower (key), map);

            rl_executing_keymap = map;
            rl_executing_key = key;

            rl_executing_keyseq.push_back (static_cast<char> (key));

            rl_dispatching = 1;
            RL_SETSTATE (RL_STATE_DISPATCHING);
            r = ((*this).*func) (rl_numeric_arg * rl_arg_sign, key);
            RL_UNSETSTATE (RL_STATE_DISPATCHING);
            rl_dispatching = 0;

            /* If we have input pending, then the last command was a prefix
               command.  Don't change the state of rl_last_func.  Otherwise,
               remember the last command executed in this variable. */
#if defined(VI_MODE)
            if (rl_pending_input == 0
                && map[key].value.function != &Readline::rl_digit_argument
                && map[key].value.function != &Readline::rl_vi_arg_digit)
#else
          if (rl_pending_input == 0
              && map[key].value.function != &Readline::rl_digit_argument)
#endif
              rl_last_func = map[key].value.function;

            RL_CHECK_SIGNALS ();
          }
        else if (map[ANYOTHERKEY].value.function)
          {
            /* OK, there's no function bound in this map, but there is a
               shadow function that was overridden when the current keymap
               was created.  Return -2 to note  that. */
            if (RL_ISSTATE (RL_STATE_MACROINPUT))
              _rl_prev_macro_key ();
            else
              _rl_unget_char (key);
            if (!rl_executing_keyseq.empty ())
              rl_executing_keyseq.erase (rl_executing_keyseq.size () - 1, 1);
            return -2;
          }
        else if (got_subseq)
          {
            /* Return -1 to note that we're in a subsequence, but  we don't
               have a matching key, nor was one overridden.  This means
               we need to back up the recursion chain and find the last
               subsequence that is bound to a function. */
            if (RL_ISSTATE (RL_STATE_MACROINPUT))
              _rl_prev_macro_key ();
            else
              _rl_unget_char (key);
            if (!rl_executing_keyseq.empty ())
              rl_executing_keyseq.erase (rl_executing_keyseq.size () - 1, 1);
            return -1;
          }
        else
          {
#if defined(READLINE_CALLBACKS)
            RL_UNSETSTATE (RL_STATE_MULTIKEY);
            _rl_keyseq_chain_dispose ();
#endif
            _rl_abort_internal ();
            return -1;
          }
        break;

      case ISKMAP:
        if (map[key].value.map != nullptr)
          {
#if defined(VI_MODE)
            /* The only way this test will be true is if a subsequence has been
               bound starting with ESC, generally the arrow keys.  What we do
               is check whether there's input in the queue, which there
               generally will be if an arrow key has been pressed, and, if
               there's not, just dispatch to (what we assume is)
               rl_vi_movement_mode right away.  This is essentially an input
               test with a zero timeout (by default) or a timeout determined by
               the value of `keyseq-timeout' */
            /* _rl_keyseq_timeout specified in milliseconds; _rl_input_queued
               takes microseconds, so multiply by 1000 */
            if (rl_editing_mode == vi_mode && key == ESC
                && map == vi_insertion_keymap ()
                && (RL_ISSTATE (RL_STATE_INPUTPENDING | RL_STATE_MACROINPUT)
                    == 0)
                && _rl_pushed_input_available () == 0
                && _rl_input_queued ((_rl_keyseq_timeout > 0)
                                         ? _rl_keyseq_timeout * 1000
                                         : 0)
                       == 0)
              return _rl_dispatch (ANYOTHERKEY, map[key].value.map);
            /* This is a very specific test.  It can possibly be generalized in
               the future, but for now it handles a specific case of ESC being
               the last character in a keyboard macro. */
            if (rl_editing_mode == vi_mode && key == ESC
                && map == vi_insertion_keymap ()
                && (RL_ISSTATE (RL_STATE_INPUTPENDING) == 0)
                && (RL_ISSTATE (RL_STATE_MACROINPUT)
                    && _rl_peek_macro_key () == 0)
                && _rl_pushed_input_available () == 0
                && _rl_input_queued ((_rl_keyseq_timeout > 0)
                                         ? _rl_keyseq_timeout * 1000
                                         : 0)
                       == 0)
              return _rl_dispatch (ANYOTHERKEY, map[key].value.map);
#endif

            rl_executing_keyseq.push_back (static_cast<char> (key));
            _rl_dispatching_keymap = map[key].value.map;

            /* Allocate new context here.  Use linked contexts (linked through
               cxt->ocxt) to simulate recursion */
#if defined(READLINE_CALLBACKS)
#if defined(VI_MODE)
            /* If we're redoing a vi mode command and we know there is a
               shadowed function corresponding to this key, just call it -- all
               the redoable vi mode commands already have all the input they
               need, and rl_vi_redo assumes that one call to rl_dispatch is
               sufficient to complete the command. */
            if (_rl_vi_redoing && RL_ISSTATE (RL_STATE_CALLBACK)
                && map[ANYOTHERKEY].value.function != nullptr)
              return _rl_subseq_result (-2, map, key, got_subseq);
#endif
            if (RL_ISSTATE (RL_STATE_CALLBACK))
              {
                /* Return 0 only the first time, to indicate success to
                   _rl_callback_read_char.  The rest of the time, we're called
                   from _rl_dispatch_callback, so we return -3 to indicate
                   special handling is necessary. */
                r = RL_ISSTATE (RL_STATE_MULTIKEY) ? -3 : 0;
                cxt = _rl_keyseq_cxt_alloc ();

                if (got_subseq)
                  cxt->flags |= KSEQ_SUBSEQ;
                cxt->okey = key;
                cxt->oldmap = map;
                cxt->dmap = _rl_dispatching_keymap;
                cxt->subseq_arg
                    = got_subseq || cxt->dmap[ANYOTHERKEY].value.function;

                RL_SETSTATE (RL_STATE_MULTIKEY);
                _rl_kscxt = cxt;

                return r; /* don't indicate immediate success */
              }
#endif

            /* Tentative inter-character timeout for potential multi-key
               sequences?  If no input within timeout, abort sequence and
               act as if we got non-matching input. */
            /* _rl_keyseq_timeout specified in milliseconds; _rl_input_queued
               takes microseconds, so multiply by 1000 */
            if (_rl_keyseq_timeout > 0
                && (RL_ISSTATE (RL_STATE_INPUTPENDING | RL_STATE_MACROINPUT)
                    == 0)
                && _rl_pushed_input_available () == 0
                && _rl_dispatching_keymap[ANYOTHERKEY].value.function
                && _rl_input_queued (_rl_keyseq_timeout * 1000) == 0)
              {
                if (!rl_executing_keyseq.empty ())
                  rl_executing_keyseq.erase (rl_executing_keyseq.size () - 1,
                                             1);
                return _rl_subseq_result (-2, map, key, got_subseq);
              }

            newkey = _rl_subseq_getchar (key);
            if (newkey < 0)
              {
                _rl_abort_internal ();
                return -1;
              }

            r = _rl_dispatch_subseq (newkey, _rl_dispatching_keymap,
                                     got_subseq
                                         || map[ANYOTHERKEY].value.function);
            return _rl_subseq_result (r, map, key, got_subseq);
          }
        else
          {
            _rl_abort_internal (); /* XXX */
            return -1;
          }

#if !defined(__clang__)
        break; // clang knows that this is unreachable
#endif

      case ISMACR:
        if (map[key].value.macro != nullptr)
          {
            macro = savestring (map[key].value.macro);
            _rl_with_macro_input (macro);
            return 0;
          }
        break;
      }

#if defined(VI_MODE)
    if (rl_editing_mode == vi_mode && _rl_keymap == vi_movement_keymap ()
        && key != ANYOTHERKEY
        && _rl_dispatching_keymap == vi_movement_keymap ()
        && _rl_vi_textmod_command (static_cast<char> (key)))
      _rl_vi_set_last (key, rl_numeric_arg, rl_arg_sign);
#endif

    return r;
  }

  int Readline::_rl_subseq_result (int r, Keymap map, int key, int got_subseq)
  {
    if (r == -2)
      /* We didn't match anything, and the keymap we're indexed into
         shadowed a function previously bound to that prefix.  Call
         the function.  The recursive call to _rl_dispatch_subseq has
         already taken care of pushing any necessary input back onto
         the input queue with _rl_unget_char. */
      {
        Keymap m = _rl_dispatching_keymap;
        keymap_entry_type type = m[ANYOTHERKEY].type;
        rl_command_func_t func = m[ANYOTHERKEY].value.function;
        if (type == ISFUNC && func == &Readline::rl_do_lowercase_version)
          r = _rl_dispatch (_rl_to_lower (key), map);
        else if (type == ISFUNC)
          {
            /* If we shadowed a function, whatever it is, we somehow need a
               keymap with map[key].func == shadowed-function.
               Let's use this one.  Then we can dispatch using the original
               key, since there are commands (e.g., in vi mode) for which it
               matters. */
            keymap_entry_type nt = m[key].type;
            rl_command_func_t nf = m[key].value.function;

            m[key].type = type;
            m[key].value.function = func;
            /* Don't change _rl_dispatching_keymap, set it here */
            _rl_dispatching_keymap = map; /* previous map */
            r = _rl_dispatch_subseq (key, m, 0);
            m[key].type = nt;
            m[key].value.function = nf;
          }
        else
          /* We probably shadowed a keymap, so keep going. */
          r = _rl_dispatch (ANYOTHERKEY, m);
      }
    else if (r < 0 && map[ANYOTHERKEY].value.function)
      {
        /* We didn't match (r is probably -1), so return something to
           tell the caller that it should try ANYOTHERKEY for an
           overridden function. */
        if (RL_ISSTATE (RL_STATE_MACROINPUT))
          _rl_prev_macro_key ();
        else
          _rl_unget_char (key);
        if (!rl_executing_keyseq.empty ())
          rl_executing_keyseq.erase (rl_executing_keyseq.size () - 1, 1);
        _rl_dispatching_keymap = map;
        return -2;
      }
    else if (r < 0 && got_subseq) /* XXX */
      {
        /* OK, back up the chain. */
        if (RL_ISSTATE (RL_STATE_MACROINPUT))
          _rl_prev_macro_key ();
        else
          _rl_unget_char (key);
        if (!rl_executing_keyseq.empty ())
          rl_executing_keyseq.erase (rl_executing_keyseq.size () - 1, 1);
        _rl_dispatching_keymap = map;
        return -1;
      }

    return r;
  }

  /* **************************************************************** */
  /*								    */
  /*			Initializations 			    */
  /*								    */
  /* **************************************************************** */

  /* Initialize readline (and terminal if not already). */
  int Readline::rl_initialize ()
  {
    /* If we have never been called before, initialize the
       terminal and data structures. */
    if (!rl_initialized)
      {
        RL_SETSTATE (RL_STATE_INITIALIZING);
        readline_initialize_everything ();
        RL_UNSETSTATE (RL_STATE_INITIALIZING);
        rl_initialized = true;
        RL_SETSTATE (RL_STATE_INITIALIZED);
      }
    else
      (void)_rl_init_locale (); /* check current locale */

    /* Initialize the current line information. */
    _rl_init_line_state ();

    /* We aren't done yet.  We haven't even gotten started yet! */
    rl_done = false;
    RL_UNSETSTATE (RL_STATE_DONE);

    /* Tell the history routines what is going on. */
    _rl_start_using_history ();

    /* Make the display buffer match the state of the line. */
    rl_reset_line_state ();

    /* No such function typed yet. */
    rl_last_func = nullptr;

    /* Parsing of key-bindings begins in an enabled state. */
    _rl_parsing_conditionalized_out = false;

#if defined(VI_MODE)
    if (rl_editing_mode == vi_mode)
      _rl_vi_initialize_line ();
#endif

    /* Each line starts in insert mode (the default). */
    _rl_set_insert_mode (RL_IM_DEFAULT, 1);

    return 0;
  }

  /* Initialize the entire state of the world. */
  void Readline::readline_initialize_everything ()
  {
    /* Set up input and output if they are not already set up. */
    if (!rl_instream)
      rl_instream = stdin;

    if (!rl_outstream)
      rl_outstream = stdout;

    /* Bind _rl_in_stream and _rl_out_stream immediately.  These values
       may change, but they may also be used before readline_internal ()
       is called. */
    _rl_in_stream = rl_instream;
    _rl_out_stream = rl_outstream;

    /* Initialize the terminal interface. */
    if (rl_terminal_name == nullptr)
      rl_terminal_name = sh_get_env_value ("TERM");
    _rl_init_terminal_io (rl_terminal_name);

    /* Bind tty characters to readline functions. */
    readline_default_bindings ();

    /* Initialize the function names. */
    rl_initialize_funmap ();

    /* Decide whether we should automatically go into eight-bit mode. */
    _rl_init_eightbit ();

    /* Read in the init file. */
    rl_read_init_file (nullptr);

    /* XXX */
    if (_rl_horizontal_scroll_mode && _rl_term_autowrap)
      {
        _rl_screenwidth--;
        _rl_screenchars -= _rl_screenheight;
      }

    /* Override the effect of any `set keymap' assignments in the
       inputrc file. */
    rl_set_keymap_from_edit_mode ();

    /* Try to bind a common arrow key prefix, if not already bound. */
    bind_arrow_keys ();

    /* Bind the bracketed paste prefix assuming that the user will enable
       it on terminals that support it. */
    bind_bracketed_paste_prefix ();

    /* If the completion parser's default word break characters haven't
       been set yet, then do so now. */
    if (rl_completer_word_break_characters == nullptr)
      rl_completer_word_break_characters
          = const_cast<char *> (rl_basic_word_break_characters);

#if defined(COLOR_SUPPORT)
    if (_rl_colored_stats || _rl_colored_completion_prefix)
      _rl_parse_colors ();
#endif

    rl_executing_keyseq.clear ();
  }

  /* If this system allows us to look at the values of the regular
     input editing characters, then bind them to their readline
     equivalents, iff the characters are not bound to keymaps. */
  void Readline::readline_default_bindings ()
  {
    if (_rl_bind_stty_chars)
      rl_tty_set_default_bindings (_rl_keymap);
  }

  /* Reset the default bindings for the terminal special characters we're
     interested in back to rl_insert and read the new ones. */
  void Readline::reset_default_bindings ()
  {
    if (_rl_bind_stty_chars)
      {
        rl_tty_unset_default_bindings (_rl_keymap);
        rl_tty_set_default_bindings (_rl_keymap);
      }
  }

  /* Bind some common arrow key sequences in MAP. */
  void Readline::bind_arrow_keys_internal (Keymap map)
  {
    Keymap xkeymap;

    xkeymap = _rl_keymap;
    _rl_keymap = map;

    rl_bind_keyseq_if_unbound ("\033[A", &Readline::rl_get_previous_history);
    rl_bind_keyseq_if_unbound ("\033[B", &Readline::rl_get_next_history);
    rl_bind_keyseq_if_unbound ("\033[C", &Readline::rl_forward_char);
    rl_bind_keyseq_if_unbound ("\033[D", &Readline::rl_backward_char);
    rl_bind_keyseq_if_unbound ("\033[H", &Readline::rl_beg_of_line);
    rl_bind_keyseq_if_unbound ("\033[F", &Readline::rl_end_of_line);

    rl_bind_keyseq_if_unbound ("\033OA", &Readline::rl_get_previous_history);
    rl_bind_keyseq_if_unbound ("\033OB", &Readline::rl_get_next_history);
    rl_bind_keyseq_if_unbound ("\033OC", &Readline::rl_forward_char);
    rl_bind_keyseq_if_unbound ("\033OD", &Readline::rl_backward_char);
    rl_bind_keyseq_if_unbound ("\033OH", &Readline::rl_beg_of_line);
    rl_bind_keyseq_if_unbound ("\033OF", &Readline::rl_end_of_line);

    /* Key bindings for control-arrow keys */
    rl_bind_keyseq_if_unbound ("\033[1;5C", &Readline::rl_forward_word);
    rl_bind_keyseq_if_unbound ("\033[1;5D", &Readline::rl_backward_word);
    rl_bind_keyseq_if_unbound ("\033[3;5~", &Readline::rl_kill_word);

    /* Key bindings for alt-arrow keys */
    rl_bind_keyseq_if_unbound ("\033[1;3C", &Readline::rl_forward_word);
    rl_bind_keyseq_if_unbound ("\033[1;3D", &Readline::rl_backward_word);

#if defined(__MINGW32__)
    rl_bind_keyseq_if_unbound ("\340H", &Readline::rl_get_previous_history);
    rl_bind_keyseq_if_unbound ("\340P", &Readline::rl_get_next_history);
    rl_bind_keyseq_if_unbound ("\340M", &Readline::rl_forward_char);
    rl_bind_keyseq_if_unbound ("\340K", &Readline::rl_backward_char);
    rl_bind_keyseq_if_unbound ("\340G", &Readline::rl_beg_of_line);
    rl_bind_keyseq_if_unbound ("\340O", &Readline::rl_end_of_line);
    rl_bind_keyseq_if_unbound ("\340S", &Readline::rl_delete);
    rl_bind_keyseq_if_unbound ("\340R", &Readline::rl_overwrite_mode);

    /* These may or may not work because of the embedded NUL. */
    rl_bind_keyseq_if_unbound ("\\000H", &Readline::rl_get_previous_history);
    rl_bind_keyseq_if_unbound ("\\000P", &Readline::rl_get_next_history);
    rl_bind_keyseq_if_unbound ("\\000M", &Readline::rl_forward_char);
    rl_bind_keyseq_if_unbound ("\\000K", &Readline::rl_backward_char);
    rl_bind_keyseq_if_unbound ("\\000G", &Readline::rl_beg_of_line);
    rl_bind_keyseq_if_unbound ("\\000O", &Readline::rl_end_of_line);
    rl_bind_keyseq_if_unbound ("\\000S", &Readline::rl_delete);
    rl_bind_keyseq_if_unbound ("\\000R", &Readline::rl_overwrite_mode);
#endif

    _rl_keymap = xkeymap;
  }

  /* Try and bind the common arrow key prefixes after giving termcap and
     the inputrc file a chance to bind them and create `real' keymaps
     for the arrow key prefix. */
  void Readline::bind_arrow_keys ()
  {
    bind_arrow_keys_internal (emacs_standard_keymap ());

#if defined(VI_MODE)
    bind_arrow_keys_internal (vi_movement_keymap ());
    /* Unbind vi_movement_keymap[ESC] to allow users to repeatedly hit ESC
       in vi command mode while still allowing the arrow keys to work. */
    if (vi_movement_keymap ()[ESC].type == ISKMAP)
      rl_bind_keyseq_in_map ("\033", nullptr, vi_movement_keymap ());
    bind_arrow_keys_internal (vi_insertion_keymap ());
#endif
  }

  void Readline::bind_bracketed_paste_prefix ()
  {
    Keymap xkeymap;

    xkeymap = _rl_keymap;

    _rl_keymap = emacs_standard_keymap ();
    rl_bind_keyseq_if_unbound (BRACK_PASTE_PREF,
                               &Readline::rl_bracketed_paste_begin);

#if defined(VI_MODE)
    _rl_keymap = vi_insertion_keymap ();
    rl_bind_keyseq_if_unbound (BRACK_PASTE_PREF,
                               &Readline::rl_bracketed_paste_begin);
    /* XXX - is there a reason to do this in the vi command keymap? */
#endif

    _rl_keymap = xkeymap;
  }

  /* **************************************************************** */
  /*								    */
  /*		Saving and Restoring Readline's state		    */
  /*								    */
  /* **************************************************************** */

  int Readline::rl_save_state (struct readline_state * sp)
  {
    if (sp == nullptr)
      return -1;

    sp->point = rl_point;
    sp->mark = rl_mark;
    sp->buffer = rl_line_buffer;
    sp->ul = rl_undo_list;
    sp->prompt = rl_prompt;

    sp->rlstate = rl_readline_state;
    sp->done = rl_done;
    sp->kmap = _rl_keymap;

    sp->lastfunc = rl_last_func;
    sp->insmode = rl_insert_mode;
    sp->edmode = rl_editing_mode;
    sp->kseq = rl_executing_keyseq;
    sp->inf = rl_instream;
    sp->outf = rl_outstream;
    sp->pendingin = rl_pending_input;
    sp->macro = rl_executing_macro;

    sp->catchsigs = rl_catch_signals;
    sp->catchsigwinch = rl_catch_sigwinch;

    sp->entryfunc = rl_completion_entry_function;
    sp->menuentryfunc = rl_menu_completion_entry_function;
    sp->ignorefunc = rl_ignore_some_completions_function;
    sp->attemptfunc = rl_attempted_completion_function;
    sp->wordbreakchars = rl_completer_word_break_characters;

    return 0;
  }

  int Readline::rl_restore_state (struct readline_state * sp)
  {
    if (sp == nullptr)
      return -1;

    rl_point = sp->point;
    rl_mark = sp->mark;
    rl_line_buffer = sp->buffer;
    rl_undo_list = sp->ul;
    rl_prompt = sp->prompt;

    rl_readline_state = sp->rlstate;
    rl_done = sp->done;
    _rl_keymap = sp->kmap;

    rl_last_func = sp->lastfunc;
    rl_insert_mode = sp->insmode;
    rl_editing_mode = sp->edmode;
    rl_executing_keyseq = sp->kseq;
    rl_instream = sp->inf;
    rl_outstream = sp->outf;
    rl_pending_input = sp->pendingin;
    rl_executing_macro = sp->macro;

    rl_catch_signals = sp->catchsigs;
    rl_catch_sigwinch = sp->catchsigwinch;

    rl_completion_entry_function = sp->entryfunc;
    rl_menu_completion_entry_function = sp->menuentryfunc;
    rl_ignore_some_completions_function = sp->ignorefunc;
    rl_attempted_completion_function = sp->attemptfunc;
    rl_completer_word_break_characters = sp->wordbreakchars;

    rl_deactivate_mark ();

    return 0;
  }

} // namespace readline
