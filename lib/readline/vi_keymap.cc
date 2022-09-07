/* vi_keymap.c -- the keymap for vi_mode in readline (). */

/* Copyright (C) 1987-2017 Free Software Foundation, Inc.

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

/* The keymap arrays for handling vi mode. */
Readline::Keymap
Readline::new_vi_movement_keymap ()
{
  Keymap ret = new KEYMAP_ENTRY[KEYMAP_SIZE];
  int i = 0;

  /* The regular control keys come first. */
  i++;		/* zero-init */						/* Control-@ */
  i++;		/* zero-init */						/* Control-a */
  i++;		/* zero-init */						/* Control-b */
  i++;		/* zero-init */						/* Control-c */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_eof_maybe);			/* Control-d */
  ret[i++] = KME (ISFUNC, &Readline::rl_emacs_editing_mode);		/* Control-e */
  i++;		/* zero-init */						/* Control-f */
  ret[i++] = KME (ISFUNC, &Readline::rl_abort);				/* Control-g */
  ret[i++] = KME (ISFUNC, &Readline::rl_backward_char);			/* Control-h */
  i++;		/* zero-init */						/* Control-i */
  ret[i++] = KME (ISFUNC, &Readline::rl_newline);			/* Control-j */
  ret[i++] = KME (ISFUNC, &Readline::rl_kill_line);			/* Control-k */
  ret[i++] = KME (ISFUNC, &Readline::rl_clear_screen);			/* Control-l */
  ret[i++] = KME (ISFUNC, &Readline::rl_newline);			/* Control-m */
  ret[i++] = KME (ISFUNC, &Readline::rl_get_next_history);		/* Control-n */
  i++;		/* zero-init */						/* Control-o */
  ret[i++] = KME (ISFUNC, &Readline::rl_get_previous_history);		/* Control-p */
  ret[i++] = KME (ISFUNC, &Readline::rl_quoted_insert);			/* Control-q */
  ret[i++] = KME (ISFUNC, &Readline::rl_reverse_search_history);	/* Control-r */
  ret[i++] = KME (ISFUNC, &Readline::rl_forward_search_history);	/* Control-s */
  ret[i++] = KME (ISFUNC, &Readline::rl_transpose_chars);		/* Control-t */
  ret[i++] = KME (ISFUNC, &Readline::rl_unix_line_discard);		/* Control-u */
  ret[i++] = KME (ISFUNC, &Readline::rl_quoted_insert);			/* Control-v */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_unix_word_rubout);		/* Control-w */
  i++;		/* zero-init */						/* Control-x */
  ret[i++] = KME (ISFUNC, &Readline::rl_yank);				/* Control-y */
  i++;		/* zero-init */						/* Control-z */

  i++;		/* zero-init */						/* Control-[ */	/* vi_escape_keymap */
  i++;		/* zero-init */						/* Control-\ */
  i++;		/* zero-init */						/* Control-] */
  i++;		/* zero-init */						/* Control-^ */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_undo);			/* Control-_ */

  /* The start of printing characters. */
  ret[i++] = KME (ISFUNC, &Readline::rl_forward_char);			/* SPACE */
  i++;		/* zero-init */						/* ! */
  i++;		/* zero-init */						/* " */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert_comment);		/* # */
  ret[i++] = KME (ISFUNC, &Readline::rl_end_of_line);			/* $ */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_match);			/* % */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_tilde_expand);		/* & */
  i++;		/* zero-init */						/* ' */
  i++;		/* zero-init */						/* ( */
  i++;		/* zero-init */						/* ) */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_complete);			/* * */
  ret[i++] = KME (ISFUNC, &Readline::rl_get_next_history);		/* + */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_char_search);		/* , */
  ret[i++] = KME (ISFUNC, &Readline::rl_get_previous_history);		/* - */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_redo);			/* . */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_search);			/* / */

  /* Regular digits. */
  ret[i++] = KME (ISFUNC, &Readline::rl_beg_of_line);			/* 0 */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_arg_digit);			/* 1 */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_arg_digit);			/* 2 */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_arg_digit);			/* 3 */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_arg_digit);			/* 4 */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_arg_digit);			/* 5 */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_arg_digit);			/* 6 */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_arg_digit);			/* 7 */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_arg_digit);			/* 8 */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_arg_digit);			/* 9 */

  /* A little more punctuation. */
  i++;		/* zero-init */						/* : */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_char_search);		/* ; */
  i++;		/* zero-init */						/* < */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_complete);			/* = */
  i++;		/* zero-init */						/* > */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_search);			/* ? */
  i++;		/* zero-init */						/* @ */

  /* Uppercase alphabet. */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_append_eol);			/* A */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_prev_word);			/* B */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_change_to);			/* C */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_delete_to);			/* D */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_end_word);			/* E */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_char_search);		/* F */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_fetch_history);		/* G */
  i++;		/* zero-init */						/* H */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_insert_beg);			/* I */
  i++;		/* zero-init */						/* J */
  i++;		/* zero-init */						/* K */
  i++;		/* zero-init */						/* L */
  i++;		/* zero-init */						/* M */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_search_again);		/* N */
  i++;		/* zero-init */						/* O */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_put);			/* P */
  i++;		/* zero-init */						/* Q */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_replace);			/* R */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_subst);			/* S */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_char_search);		/* T */
  ret[i++] = KME (ISFUNC, &Readline::rl_revert_line);			/* U */
  i++;		/* zero-init */						/* V */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_next_word);			/* W */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_rubout);			/* X */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_yank_to);			/* Y */
  i++;		/* zero-init */						/* Z */

  /* Some more punctuation. */
  i++;		/* zero-init */						/* [ */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_complete);			/* \ */
  i++;		/* zero-init */						/* ] */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_first_print);		/* ^ */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_yank_arg);			/* _ */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_goto_mark);			/* ` */

  /* Lowercase alphabet. */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_append_mode);		/* a */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_prev_word);			/* b */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_change_to);			/* c */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_delete_to);			/* d */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_end_word);			/* e */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_char_search);		/* f */
  i++;		/* zero-init */						/* g */
  ret[i++] = KME (ISFUNC, &Readline::rl_backward_char);			/* h */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_insert_mode);		/* i */
  ret[i++] = KME (ISFUNC, &Readline::rl_get_next_history);		/* j */
  ret[i++] = KME (ISFUNC, &Readline::rl_get_previous_history);		/* k */
  ret[i++] = KME (ISFUNC, &Readline::rl_forward_char);			/* l */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_set_mark);			/* m */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_search_again);		/* n */
  i++;		/* zero-init */						/* o */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_put);			/* p */
  i++;		/* zero-init */						/* q */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_change_char);		/* r */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_subst);			/* s */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_char_search);		/* t */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_undo);			/* u */
  i++;		/* zero-init */						/* v */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_next_word);			/* w */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_delete);			/* x */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_yank_to);			/* y */
  i++;		/* zero-init */						/* z */

  /* Final punctuation. */
  i++;		/* zero-init */						/* { */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_column);			/* | */
  i++;		/* zero-init */						/* } */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_change_case);		/* ~ */
  i++;		/* zero-init */						/* RUBOUT */

  // the rest of the array is zero-inited.
  return ret;
}

Readline::Keymap
Readline::new_vi_insertion_keymap ()
{
  Keymap ret = new KEYMAP_ENTRY[KEYMAP_SIZE];
  int i = 0;

  /* The regular control keys come first. */
  i++;		/* zero-init */						/* Control-@ */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* Control-a */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* Control-b */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* Control-c */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_eof_maybe);			/* Control-d */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* Control-e */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* Control-f */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* Control-g */
  ret[i++] = KME (ISFUNC, &Readline::rl_rubout);			/* Control-h */
  ret[i++] = KME (ISFUNC, &Readline::rl_complete);			/* Control-i */
  ret[i++] = KME (ISFUNC, &Readline::rl_newline);			/* Control-j */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* Control-k */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* Control-l */
  ret[i++] = KME (ISFUNC, &Readline::rl_newline);			/* Control-m */
  ret[i++] = KME (ISFUNC, &Readline::rl_menu_complete);			/* Control-n */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* Control-o */
  ret[i++] = KME (ISFUNC, &Readline::rl_backward_menu_complete);	/* Control-p */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* Control-q */
  ret[i++] = KME (ISFUNC, &Readline::rl_reverse_search_history);	/* Control-r */
  ret[i++] = KME (ISFUNC, &Readline::rl_forward_search_history);	/* Control-s */
  ret[i++] = KME (ISFUNC, &Readline::rl_transpose_chars);		/* Control-t */
  ret[i++] = KME (ISFUNC, &Readline::rl_unix_line_discard);		/* Control-u */
  ret[i++] = KME (ISFUNC, &Readline::rl_quoted_insert);			/* Control-v */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_unix_word_rubout);		/* Control-w */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* Control-x */
  ret[i++] = KME (ISFUNC, &Readline::rl_yank);				/* Control-y */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* Control-z */

  ret[i++] = KME (ISFUNC, &Readline::rl_vi_movement_mode);		/* Control-[ */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* Control-\ */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* Control-] */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* Control-^ */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_undo);			/* Control-_ */

  /* The start of printing characters. */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* SPACE */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* ! */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* " */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* # */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* $ */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* % */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* & */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* ' */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* ( */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* ) */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* * */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* + */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* , */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* - */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* . */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* / */

  /* Regular digits. */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* 0 */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* 1 */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* 2 */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* 3 */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* 4 */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* 5 */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* 6 */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* 7 */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* 8 */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* 9 */

  /* A little more punctuation. */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* : */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* ; */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* < */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* = */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* > */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* ? */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* @ */

  /* Uppercase alphabet. */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* A */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* B */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* C */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* D */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* E */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* F */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* G */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* H */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* I */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* J */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* K */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* L */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* M */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* N */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* O */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* P */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* Q */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* R */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* S */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* T */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* U */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* V */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* W */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* X */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* Y */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* Z */

  /* Some more punctuation. */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* [ */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* \ */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* ] */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* ^ */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* _ */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* ` */

  /* Lowercase alphabet. */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* a */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* b */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* c */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* d */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* e */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* f */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* g */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* h */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* i */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* j */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* k */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* l */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* m */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* n */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* o */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* p */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* q */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* r */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* s */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* t */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* u */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* v */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* w */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* x */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* y */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* z */

  /* Final punctuation. */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* { */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* | */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* } */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);			/* ~ */
  ret[i++] = KME (ISFUNC, &Readline::rl_rubout);			/* RUBOUT */

#if KEYMAP_SIZE > 129
  /* Pure 8-bit characters (128 - 159). These might be used in some
     character sets. */
  /* ISO Latin-1 characters (160 - 255) */
  for (; i <= 255; ++i)
    ret[i++] = KME (ISFUNC, &Readline::rl_insert);
#endif /* KEYMAP_SIZE > 129 */

  // last entry is zero-inited.
  return ret;
}

}  // namespace readline
