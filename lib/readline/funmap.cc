/* funmap.c -- attach names to functions. */

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

namespace readline
{

static const Readline::FUNMAP default_funmap[] = {
  { "abort", &Readline::rl_abort },
  { "accept-line", &Readline::rl_newline },
  { "arrow-key-prefix", &Readline::rl_arrow_keys },
  { "backward-byte", &Readline::rl_backward_byte },
  { "backward-char", &Readline::rl_backward_char },
  { "backward-delete-char", &Readline::rl_rubout },
  { "backward-kill-line", &Readline::rl_backward_kill_line },
  { "backward-kill-word", &Readline::rl_backward_kill_word },
  { "backward-word", &Readline::rl_backward_word },
  { "beginning-of-history", &Readline::rl_beginning_of_history },
  { "beginning-of-line", &Readline::rl_beg_of_line },
  { "bracketed-paste-begin", &Readline::rl_bracketed_paste_begin },
  { "call-last-kbd-macro", &Readline::rl_call_last_kbd_macro },
  { "capitalize-word", &Readline::rl_capitalize_word },
  { "character-search", &Readline::rl_char_search },
  { "character-search-backward", &Readline::rl_backward_char_search },
  { "clear-display", &Readline::rl_clear_display },
  { "clear-screen", &Readline::rl_clear_screen },
  { "complete", &Readline::rl_complete },
  { "copy-backward-word", &Readline::rl_copy_backward_word },
  { "copy-forward-word", &Readline::rl_copy_forward_word },
  { "copy-region-as-kill", &Readline::rl_copy_region_to_kill },
  { "delete-char", &Readline::rl_delete },
  { "delete-char-or-list", &Readline::rl_delete_or_show_completions },
  { "delete-horizontal-space", &Readline::rl_delete_horizontal_space },
  { "digit-argument", &Readline::rl_digit_argument },
  { "do-lowercase-version", &Readline::rl_do_lowercase_version },
  { "downcase-word", &Readline::rl_downcase_word },
  { "dump-functions", &Readline::rl_dump_functions },
  { "dump-macros", &Readline::rl_dump_macros },
  { "dump-variables", &Readline::rl_dump_variables },
  { "emacs-editing-mode", &Readline::rl_emacs_editing_mode },
  { "end-kbd-macro", &Readline::rl_end_kbd_macro },
  { "end-of-history", &Readline::rl_end_of_history },
  { "end-of-line", &Readline::rl_end_of_line },
  { "exchange-point-and-mark", &Readline::rl_exchange_point_and_mark },
  { "forward-backward-delete-char", &Readline::rl_rubout_or_delete },
  { "forward-byte", &Readline::rl_forward_byte },
  { "forward-char", &Readline::rl_forward_char },
  { "forward-search-history", &Readline::rl_forward_search_history },
  { "forward-word", &Readline::rl_forward_word },
  { "history-search-backward", &Readline::rl_history_search_backward },
  { "history-search-forward", &Readline::rl_history_search_forward },
  { "history-substring-search-backward",
    &Readline::rl_history_substr_search_backward },
  { "history-substring-search-forward",
    &Readline::rl_history_substr_search_forward },
  { "insert-comment", &Readline::rl_insert_comment },
  { "insert-completions", &Readline::rl_insert_completions },
  { "kill-whole-line", &Readline::rl_kill_full_line },
  { "kill-line", &Readline::rl_kill_line },
  { "kill-region", &Readline::rl_kill_region },
  { "kill-word", &Readline::rl_kill_word },
  { "menu-complete", &Readline::rl_menu_complete },
  { "menu-complete-backward", &Readline::rl_backward_menu_complete },
  { "next-history", &Readline::rl_get_next_history },
  { "next-screen-line", &Readline::rl_next_screen_line },
  { "non-incremental-forward-search-history",
    &Readline::rl_noninc_forward_search },
  { "non-incremental-reverse-search-history",
    &Readline::rl_noninc_reverse_search },
  { "non-incremental-forward-search-history-again",
    &Readline::rl_noninc_forward_search_again },
  { "non-incremental-reverse-search-history-again",
    &Readline::rl_noninc_reverse_search_again },
  { "old-menu-complete", &Readline::rl_old_menu_complete },
  { "operate-and-get-next", &Readline::rl_operate_and_get_next },
  { "overwrite-mode", &Readline::rl_overwrite_mode },
#if defined(_WIN32)
  { "paste-from-clipboard", &Readline::rl_paste_from_clipboard },
#endif
  { "possible-completions", &Readline::rl_possible_completions },
  { "previous-history", &Readline::rl_get_previous_history },
  { "previous-screen-line", &Readline::rl_previous_screen_line },
  { "print-last-kbd-macro", &Readline::rl_print_last_kbd_macro },
  { "quoted-insert", &Readline::rl_quoted_insert },
  { "re-read-init-file", &Readline::rl_re_read_init_file },
  { "redraw-current-line", &Readline::rl_refresh_line },
  { "reverse-search-history", &Readline::rl_reverse_search_history },
  { "revert-line", &Readline::rl_revert_line },
  { "self-insert", &Readline::rl_insert },
  { "set-mark", &Readline::rl_set_mark },
  { "skip-csi-sequence", &Readline::rl_skip_csi_sequence },
  { "start-kbd-macro", &Readline::rl_start_kbd_macro },
  { "tab-insert", &Readline::rl_tab_insert },
  { "tilde-expand", &Readline::rl_tilde_expand },
  { "transpose-chars", &Readline::rl_transpose_chars },
  { "transpose-words", &Readline::rl_transpose_words },
  { "tty-status", &Readline::rl_tty_status },
  { "undo", &Readline::rl_undo_command },
  { "universal-argument", &Readline::rl_universal_argument },
  { "unix-filename-rubout", &Readline::rl_unix_filename_rubout },
  { "unix-line-discard", &Readline::rl_unix_line_discard },
  { "unix-word-rubout", &Readline::rl_unix_word_rubout },
  { "upcase-word", &Readline::rl_upcase_word },
  { "yank", &Readline::rl_yank },
  { "yank-last-arg", &Readline::rl_yank_last_arg },
  { "yank-nth-arg", &Readline::rl_yank_nth_arg },
  { "yank-pop", &Readline::rl_yank_pop },

#if defined(VI_MODE)
  { "vi-append-eol", &Readline::rl_vi_append_eol },
  { "vi-append-mode", &Readline::rl_vi_append_mode },
  { "vi-arg-digit", &Readline::rl_vi_arg_digit },
  { "vi-back-to-indent", &Readline::rl_vi_back_to_indent },
  { "vi-backward-bigword", &Readline::rl_vi_bWord },
  { "vi-backward-word", &Readline::rl_vi_bword },
  { "vi-bWord", &Readline::rl_vi_bWord },
  { "vi-bword",
    &Readline::rl_vi_bword }, /* BEWARE: name matching is case insensitive */
  { "vi-change-case", &Readline::rl_vi_change_case },
  { "vi-change-char", &Readline::rl_vi_change_char },
  { "vi-change-to", &Readline::rl_vi_change_to },
  { "vi-char-search", &Readline::rl_vi_char_search },
  { "vi-column", &Readline::rl_vi_column },
  { "vi-complete", &Readline::rl_vi_complete },
  { "vi-delete", &Readline::rl_vi_delete },
  { "vi-delete-to", &Readline::rl_vi_delete_to },
  { "vi-eWord", &Readline::rl_vi_eWord },
  { "vi-editing-mode", &Readline::rl_vi_editing_mode },
  { "vi-end-bigword", &Readline::rl_vi_eWord },
  { "vi-end-word", &Readline::rl_vi_end_word },
  { "vi-eof-maybe", &Readline::rl_vi_eof_maybe },
  { "vi-eword",
    &Readline::rl_vi_eword }, /* BEWARE: name matching is case insensitive */
  { "vi-fWord", &Readline::rl_vi_fWord },
  { "vi-fetch-history", &Readline::rl_vi_fetch_history },
  { "vi-first-print", &Readline::rl_vi_first_print },
  { "vi-forward-bigword", &Readline::rl_vi_fWord },
  { "vi-forward-word", &Readline::rl_vi_fword },
  { "vi-fword",
    &Readline::rl_vi_fword }, /* BEWARE: name matching is case insensitive */
  { "vi-goto-mark", &Readline::rl_vi_goto_mark },
  { "vi-insert-beg", &Readline::rl_vi_insert_beg },
  { "vi-insertion-mode", &Readline::rl_vi_insert_mode },
  { "vi-match", &Readline::rl_vi_match },
  { "vi-movement-mode", &Readline::rl_vi_movement_mode },
  { "vi-next-word", &Readline::rl_vi_next_word },
  { "vi-overstrike", &Readline::rl_vi_overstrike },
  { "vi-overstrike-delete", &Readline::rl_vi_overstrike_delete },
  { "vi-prev-word", &Readline::rl_vi_prev_word },
  { "vi-put", &Readline::rl_vi_put },
  { "vi-redo", &Readline::rl_vi_redo },
  { "vi-replace", &Readline::rl_vi_replace },
  { "vi-rubout", &Readline::rl_vi_rubout },
  { "vi-search", &Readline::rl_vi_search },
  { "vi-search-again", &Readline::rl_vi_search_again },
  { "vi-set-mark", &Readline::rl_vi_set_mark },
  { "vi-subst", &Readline::rl_vi_subst },
  { "vi-tilde-expand", &Readline::rl_vi_tilde_expand },
  { "vi-unix-word-rubout", &Readline::rl_vi_unix_word_rubout },
  { "vi-yank-arg", &Readline::rl_vi_yank_arg },
  { "vi-yank-pop", &Readline::rl_vi_yank_pop },
  { "vi-yank-to", &Readline::rl_vi_yank_to },
#endif /* VI_MODE */

  { nullptr, nullptr }
};

/* Make the funmap contain all of the default entries. */
void
Readline::rl_initialize_funmap ()
{
  if (funmap_initialized)
    return;

  for (size_t i = 0; default_funmap[i].name; i++)
    rl_add_funmap_entry (default_funmap[i].name, default_funmap[i].function);

  funmap_initialized = true;
}

/* Produce a nullptr-terminated array of known function names.  The array
   is sorted.  The array itself is allocated, but not the strings inside.
   You should delete[] the array when you done, but not the pointers. */
const char **
Readline::rl_funmap_names ()
{
  std::vector<const char *> result;

  /* Make sure that the function map has been initialized. */
  rl_initialize_funmap ();

  for (size_t result_index = 0; funmap[result_index]; result_index++)
    {
      result.push_back (funmap[result_index]->name);
    }

  result.push_back (nullptr);
  size_t result_size = result.size ();

  const char **result_copy = new const char *[result_size];
  std::copy (result.begin (), result.end (), result_copy);

  std::qsort (result_copy, result_size, sizeof (char *),
              &_rl_qsort_string_compare);
  return result_copy;
}

} // namespace readline
