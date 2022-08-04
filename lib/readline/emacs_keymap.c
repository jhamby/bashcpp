/* emacs_keymap.c -- the keymap for emacs_mode in readline (). */

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

#include "readline.h"

namespace readline
{

/* Definitions of arrays of pointers-to-method, one for each possible key.
   If the type byte is ISKMAP, then the pointer is the address of
   a keymap. */

// Return a new Emacs standard keymap. This will initialize
// emacs_ctlx_keymap_ and emacs_meta_keymap_, if necessary.
Readline::Keymap
Readline::new_emacs_standard_keymap ()
{
  if (!emacs_ctlx_keymap_)
    emacs_ctlx_keymap_ = new_emacs_ctlx_keymap ();
  if (!emacs_meta_keymap_)
    emacs_meta_keymap_ = new_emacs_meta_keymap ();

  Keymap ret = new KEYMAP_ENTRY[KEYMAP_SIZE];
  int i = 0;

  /* Control keys. */
  ret[i++] = KME (ISFUNC, &Readline::rl_set_mark);		/* Control-@ */
  ret[i++] = KME (ISFUNC, &Readline::rl_beg_of_line);		/* Control-a */
  ret[i++] = KME (ISFUNC, &Readline::rl_backward_char);		/* Control-b */
  i++;		/* zero-init */					/* Control-c */
  ret[i++] = KME (ISFUNC, &Readline::rl_delete);		/* Control-d */
  ret[i++] = KME (ISFUNC, &Readline::rl_end_of_line);		/* Control-e */
  ret[i++] = KME (ISFUNC, &Readline::rl_forward_char);		/* Control-f */
  ret[i++] = KME (ISFUNC, &Readline::rl_abort);			/* Control-g */
  ret[i++] = KME (ISFUNC, &Readline::rl_rubout);		/* Control-h */
  ret[i++] = KME (ISFUNC, &Readline::rl_complete);		/* Control-i */
  ret[i++] = KME (ISFUNC, &Readline::rl_newline);		/* Control-j */
  ret[i++] = KME (ISFUNC, &Readline::rl_kill_line);		/* Control-k */
  ret[i++] = KME (ISFUNC, &Readline::rl_clear_screen);		/* Control-l */
  ret[i++] = KME (ISFUNC, &Readline::rl_newline);		/* Control-m */
  ret[i++] = KME (ISFUNC, &Readline::rl_get_next_history);	/* Control-n */
  ret[i++] = KME (ISFUNC, &Readline::rl_operate_and_get_next);	/* Control-o */
  ret[i++] = KME (ISFUNC, &Readline::rl_get_previous_history);	/* Control-p */
  ret[i++] = KME (ISFUNC, &Readline::rl_quoted_insert);		/* Control-q */
  ret[i++] = KME (ISFUNC, &Readline::rl_reverse_search_history);	/* Control-r */
  ret[i++] = KME (ISFUNC, &Readline::rl_forward_search_history);	/* Control-s */
  ret[i++] = KME (ISFUNC, &Readline::rl_transpose_chars);	/* Control-t */
  ret[i++] = KME (ISFUNC, &Readline::rl_unix_line_discard);	/* Control-u */
  ret[i++] = KME (ISFUNC, &Readline::rl_quoted_insert);		/* Control-v */
  ret[i++] = KME (ISFUNC, &Readline::rl_unix_word_rubout);	/* Control-w */
  ret[i++] = KME (ISKMAP, emacs_ctlx_keymap_);			/* Control-x */
  ret[i++] = KME (ISFUNC, &Readline::rl_yank);			/* Control-y */
  i++;		/* zero-init */					/* Control-z */
  ret[i++] = KME (ISKMAP, emacs_meta_keymap_); 			/* Control-[ */
  i++;		/* zero-init */					/* Control-\ */
  ret[i++] = KME (ISFUNC, &Readline::rl_char_search);		/* Control-] */
  i++;		/* zero-init */					/* Control-^ */
  ret[i++] = KME (ISFUNC, &Readline::rl_undo_command);		/* Control-_ */

  /* The start of printing characters. */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* SPACE */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* ! */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* " */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* # */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* $ */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* % */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* & */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* ' */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* ( */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* ) */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* * */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* + */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* , */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* - */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* . */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* / */

	  /* Regular digits. */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* 0 */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* 1 */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* 2 */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* 3 */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* 4 */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* 5 */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* 6 */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* 7 */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* 8 */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* 9 */

  /* A little more punctuation. */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* : */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* ; */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* < */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* = */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* > */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* ? */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* @ */

  /* Uppercase alphabet. */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* A */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* B */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* C */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* D */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* E */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* F */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* G */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* H */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* I */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* J */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* K */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* L */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* M */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* N */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* O */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* P */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* Q */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* R */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* S */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* T */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* U */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* V */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* W */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* X */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* Y */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* Z */

  /* Some more punctuation. */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* [ */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* \ */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* ] */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* ^ */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* _ */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* ` */

  /* Lowercase alphabet. */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* a */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* b */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* c */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* d */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* e */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* f */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* g */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* h */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* i */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* j */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* k */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* l */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* m */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* n */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* o */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* p */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* q */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* r */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* s */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* t */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* u */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* v */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* w */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* x */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* y */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* z */

  /* Final punctuation. */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* { */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* | */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* } */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert);		/* ~ */
  ret[i++] = KME (ISFUNC, &Readline::rl_rubout);		/* RUBOUT */

#if KEYMAP_SIZE > 129
  /* Pure 8-bit characters (128 - 159). These might be used in some
     character sets. */
  /* ISO Latin-1 characters (160 - 255) */
  for (; i < 256; ++i)
    ret[i++] = KME (ISFUNC, &Readline::rl_insert);
#endif /* KEYMAP_SIZE > 129 */

  // last entry is zero-inited.
  return ret;
}

Readline::Keymap
Readline::new_emacs_meta_keymap ()
{
  Keymap ret = new KEYMAP_ENTRY[KEYMAP_SIZE];
  int i = 0;

  /* Meta keys.  Just like above, but the high bit is set. */
  i++;		/* zero-init */					/* Meta-Control-@ */
  i++;		/* zero-init */					/* Meta-Control-a */
  i++;		/* zero-init */					/* Meta-Control-b */
  i++;		/* zero-init */					/* Meta-Control-c */
  i++;		/* zero-init */					/* Meta-Control-d */
  i++;		/* zero-init */					/* Meta-Control-e */
  i++;		/* zero-init */					/* Meta-Control-f */
  ret[i++] = KME (ISFUNC, &Readline::rl_abort);			/* Meta-Control-g */
  ret[i++] = KME (ISFUNC, &Readline::rl_backward_kill_word);	/* Meta-Control-h */
  ret[i++] = KME (ISFUNC, &Readline::rl_tab_insert);		/* Meta-Control-i */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_editing_mode);	/* Meta-Control-j */
  i++;		/* zero-init */					/* Meta-Control-k */
  ret[i++] = KME (ISFUNC, &Readline::rl_clear_display);		/* Meta-Control-l */
  ret[i++] = KME (ISFUNC, &Readline::rl_vi_editing_mode); 	/* Meta-Control-m */
  i++;		/* zero-init */					/* Meta-Control-n */
  i++;		/* zero-init */					/* Meta-Control-o */
  i++;		/* zero-init */					/* Meta-Control-p */
  i++;		/* zero-init */					/* Meta-Control-q */
  ret[i++] = KME (ISFUNC, &Readline::rl_revert_line);		/* Meta-Control-r */
  i++;		/* zero-init */					/* Meta-Control-s */
  i++;		/* zero-init */					/* Meta-Control-t */
  i++;		/* zero-init */					/* Meta-Control-u */
  i++;		/* zero-init */					/* Meta-Control-v */
  i++;		/* zero-init */					/* Meta-Control-w */
  i++;		/* zero-init */					/* Meta-Control-x */
  ret[i++] = KME (ISFUNC, &Readline::rl_yank_nth_arg);		/* Meta-Control-y */
  i++;		/* zero-init */					/* Meta-Control-z */

  ret[i++] = KME (ISFUNC, &Readline::rl_complete);		/* Meta-Control-[ */
  i++;		/* zero-init */					/* Meta-Control-\ */
  ret[i++] = KME (ISFUNC, &Readline::rl_backward_char_search);	/* Meta-Control-] */
  i++;		/* zero-init */					/* Meta-Control-^ */
  i++;		/* zero-init */					/* Meta-Control-_ */

  /* The start of printing characters. */
  ret[i++] = KME (ISFUNC, &Readline::rl_set_mark);		/* Meta-SPACE */
  i++;		/* zero-init */					/* Meta-! */
  i++;		/* zero-init */					/* Meta-" */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert_comment);	/* Meta-# */
  i++;		/* zero-init */					/* Meta-$ */
  i++;		/* zero-init */					/* Meta-% */
  ret[i++] = KME (ISFUNC, &Readline::rl_tilde_expand);		/* Meta-& */
  i++;		/* zero-init */					/* Meta-' */
  i++;		/* zero-init */					/* Meta-( */
  i++;		/* zero-init */					/* Meta-) */
  ret[i++] = KME (ISFUNC, &Readline::rl_insert_completions);	/* Meta-* */
  i++;		/* zero-init */					/* Meta-+ */
  i++;		/* zero-init */					/* Meta-, */
  ret[i++] = KME (ISFUNC, &Readline::rl_digit_argument); 	/* Meta-- */
  ret[i++] = KME (ISFUNC, &Readline::rl_yank_last_arg);		/* Meta-. */
  i++;		/* zero-init */					/* Meta-/ */

  /* Regular digits. */
  ret[i++] = KME (ISFUNC, &Readline::rl_digit_argument); 	/* Meta-0 */
  ret[i++] = KME (ISFUNC, &Readline::rl_digit_argument); 	/* Meta-1 */
  ret[i++] = KME (ISFUNC, &Readline::rl_digit_argument); 	/* Meta-2 */
  ret[i++] = KME (ISFUNC, &Readline::rl_digit_argument); 	/* Meta-3 */
  ret[i++] = KME (ISFUNC, &Readline::rl_digit_argument); 	/* Meta-4 */
  ret[i++] = KME (ISFUNC, &Readline::rl_digit_argument); 	/* Meta-5 */
  ret[i++] = KME (ISFUNC, &Readline::rl_digit_argument); 	/* Meta-6 */
  ret[i++] = KME (ISFUNC, &Readline::rl_digit_argument); 	/* Meta-7 */
  ret[i++] = KME (ISFUNC, &Readline::rl_digit_argument); 	/* Meta-8 */
  ret[i++] = KME (ISFUNC, &Readline::rl_digit_argument); 	/* Meta-9 */

  /* A little more punctuation. */
  i++;		/* zero-init */					/* Meta-: */
  i++;		/* zero-init */					/* Meta-; */
  ret[i++] = KME (ISFUNC, &Readline::rl_beginning_of_history);	/* Meta-< */
  ret[i++] = KME (ISFUNC, &Readline::rl_possible_completions);	/* Meta-= */
  ret[i++] = KME (ISFUNC, &Readline::rl_end_of_history);	/* Meta-> */
  ret[i++] = KME (ISFUNC, &Readline::rl_possible_completions);	/* Meta-? */
  i++;		/* zero-init */					/* Meta-@ */

  /* Uppercase alphabet. */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-A */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-B */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-C */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-D */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-E */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-F */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-G */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-H */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-I */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-J */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-K */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-L */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-M */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-N */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-O */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-P */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-Q */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-R */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-S */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-T */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-U */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-V */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-W */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-X */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-Y */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Meta-Z */

  /* Some more punctuation. */
  i++;		/* zero-init */					/* Meta-[ */	/* was rl_arrow_keys */
  ret[i++] = KME (ISFUNC, &Readline::rl_delete_horizontal_space);	/* Meta-\ */
  i++;		/* zero-init */					/* Meta-] */
  i++;		/* zero-init */					/* Meta-^ */
  ret[i++] = KME (ISFUNC, &Readline::rl_yank_last_arg);		/* Meta-_ */
  i++;		/* zero-init */					/* Meta-` */

  /* Lowercase alphabet. */
  i++;		/* zero-init */					/* Meta-a */
  ret[i++] = KME (ISFUNC, &Readline::rl_backward_word);		/* Meta-b */
  ret[i++] = KME (ISFUNC, &Readline::rl_capitalize_word); 	/* Meta-c */
  ret[i++] = KME (ISFUNC, &Readline::rl_kill_word);		/* Meta-d */
  i++;		/* zero-init */					/* Meta-e */
  ret[i++] = KME (ISFUNC, &Readline::rl_forward_word);		/* Meta-f */
  i++;		/* zero-init */					/* Meta-g */
  i++;		/* zero-init */					/* Meta-h */
  i++;		/* zero-init */					/* Meta-i */
  i++;		/* zero-init */					/* Meta-j */
  i++;		/* zero-init */					/* Meta-k */
  ret[i++] = KME (ISFUNC, &Readline::rl_downcase_word);		/* Meta-l */
  i++;		/* zero-init */					/* Meta-m */
  ret[i++] = KME (ISFUNC, &Readline::rl_noninc_forward_search);	/* Meta-n */
  i++;		/* zero-init */					/* Meta-o */	/* was rl_arrow_keys */
  ret[i++] = KME (ISFUNC, &Readline::rl_noninc_reverse_search);	/* Meta-p */
  i++;		/* zero-init */					/* Meta-q */
  ret[i++] = KME (ISFUNC, &Readline::rl_revert_line);		/* Meta-r */
  i++;		/* zero-init */					/* Meta-s */
  ret[i++] = KME (ISFUNC, &Readline::rl_transpose_words); 	/* Meta-t */
  ret[i++] = KME (ISFUNC, &Readline::rl_upcase_word);		/* Meta-u */
  i++;		/* zero-init */					/* Meta-v */
  i++;		/* zero-init */					/* Meta-w */
  i++;		/* zero-init */					/* Meta-x */
  ret[i++] = KME (ISFUNC, &Readline::rl_yank_pop);		/* Meta-y */
  i++;		/* zero-init */					/* Meta-z */

  /* Final punctuation. */
  i++;		/* zero-init */					/* Meta-{ */
  i++;		/* zero-init */					/* Meta-| */
  i++;		/* zero-init */					/* Meta-} */
  ret[i++] = KME (ISFUNC, &Readline::rl_tilde_expand);		/* Meta-~ */
  ret[i] = KME (ISFUNC, &Readline::rl_backward_kill_word);	/* Meta-rubout */

  // the rest of the array is zero-inited.
  return ret;
}

Readline::Keymap
Readline::new_emacs_ctlx_keymap () {
  Keymap ret = new KEYMAP_ENTRY[KEYMAP_SIZE];
  int i = 0;

  /* Control keys. */
  i++;		/* zero-init */					/* Control-@ */
  i++;		/* zero-init */					/* Control-a */
  i++;		/* zero-init */					/* Control-b */
  i++;		/* zero-init */					/* Control-c */
  i++;		/* zero-init */					/* Control-d */
  i++;		/* zero-init */					/* Control-e */
  i++;		/* zero-init */					/* Control-f */
  ret[i++] = KME (ISFUNC, &Readline::rl_abort);			/* Control-g */
  i++;		/* zero-init */					/* Control-h */
  i++;		/* zero-init */					/* Control-i */
  i++;		/* zero-init */					/* Control-j */
  i++;		/* zero-init */					/* Control-k */
  i++;		/* zero-init */					/* Control-l */
  i++;		/* zero-init */					/* Control-m */
  i++;		/* zero-init */					/* Control-n */
  i++;		/* zero-init */					/* Control-o */
  i++;		/* zero-init */					/* Control-p */
  i++;		/* zero-init */					/* Control-q */
  ret[i++] = KME (ISFUNC, &Readline::rl_re_read_init_file);	/* Control-r */
  i++;		/* zero-init */					/* Control-s */
  i++;		/* zero-init */					/* Control-t */
  ret[i++] = KME (ISFUNC, &Readline::rl_undo_command);		/* Control-u */
  i++;		/* zero-init */					/* Control-v */
  i++;		/* zero-init */					/* Control-w */
  ret[i++] = KME (ISFUNC, &Readline::rl_exchange_point_and_mark);	/* Control-x */
  i++;		/* zero-init */					/* Control-y */
  i++;		/* zero-init */					/* Control-z */
  i++;		/* zero-init */					/* Control-[ */
  i++;		/* zero-init */					/* Control-\ */
  i++;		/* zero-init */					/* Control-] */
  i++;		/* zero-init */					/* Control-^ */
  i++;		/* zero-init */					/* Control-_ */

  /* The start of printing characters. */
  i++;		/* zero-init */					/* SPACE */
  i++;		/* zero-init */					/* ! */
  i++;		/* zero-init */					/* " */
  i++;		/* zero-init */					/* # */
  i++;		/* zero-init */					/* $ */
  i++;		/* zero-init */					/* % */
  i++;		/* zero-init */					/* & */
  i++;		/* zero-init */					/* ' */
  ret[i++] = KME (ISFUNC, &Readline::rl_start_kbd_macro);		/* ( */
  ret[i++] = KME (ISFUNC, &Readline::rl_end_kbd_macro );		/* ) */
  i++;		/* zero-init */					/* * */
  i++;		/* zero-init */					/* + */
  i++;		/* zero-init */					/* , */
  i++;		/* zero-init */					/* - */
  i++;		/* zero-init */					/* . */
  i++;		/* zero-init */					/* / */

  /* Regular digits. */
  i++;		/* zero-init */					/* 0 */
  i++;		/* zero-init */					/* 1 */
  i++;		/* zero-init */					/* 2 */
  i++;		/* zero-init */					/* 3 */
  i++;		/* zero-init */					/* 4 */
  i++;		/* zero-init */					/* 5 */
  i++;		/* zero-init */					/* 6 */
  i++;		/* zero-init */					/* 7 */
  i++;		/* zero-init */					/* 8 */
  i++;		/* zero-init */					/* 9 */

  /* A little more punctuation. */
  i++;		/* zero-init */					/* : */
  i++;		/* zero-init */					/* ; */
  i++;		/* zero-init */					/* < */
  i++;		/* zero-init */					/* = */
  i++;		/* zero-init */					/* > */
  i++;		/* zero-init */					/* ? */
  i++;		/* zero-init */					/* @ */

  /* Uppercase alphabet. */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* A */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* B */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* C */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* D */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* E */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* F */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* G */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* H */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* I */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* J */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* K */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* L */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* M */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* N */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* O */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* P */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Q */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* R */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* S */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* T */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* U */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* V */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* W */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* X */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Y */
  ret[i++] = KME (ISFUNC, &Readline::rl_do_lowercase_version);	/* Z */

  /* Some more punctuation. */
  i++;		/* zero-init */					/* [ */
  i++;		/* zero-init */					/* \ */
  i++;		/* zero-init */					/* ] */
  i++;		/* zero-init */					/* ^ */
  i++;		/* zero-init */					/* _ */
  i++;		/* zero-init */					/* ` */

  /* Lowercase alphabet. */
  i++;		/* zero-init */					/* a */
  i++;		/* zero-init */					/* b */
  i++;		/* zero-init */					/* c */
  i++;		/* zero-init */					/* d */
  ret[i++] = KME (ISFUNC, &Readline::rl_call_last_kbd_macro);	/* e */
  i++;		/* zero-init */					/* f */
  i++;		/* zero-init */					/* g */
  i++;		/* zero-init */					/* h */
  i++;		/* zero-init */					/* i */
  i++;		/* zero-init */					/* j */
  i++;		/* zero-init */					/* k */
  i++;		/* zero-init */					/* l */
  i++;		/* zero-init */					/* m */
  i++;		/* zero-init */					/* n */
  i++;		/* zero-init */					/* o */
  i++;		/* zero-init */					/* p */
  i++;		/* zero-init */					/* q */
  i++;		/* zero-init */					/* r */
  i++;		/* zero-init */					/* s */
  i++;		/* zero-init */					/* t */
  i++;		/* zero-init */					/* u */
  i++;		/* zero-init */					/* v */
  i++;		/* zero-init */					/* w */
  i++;		/* zero-init */					/* x */
  i++;		/* zero-init */					/* y */
  i++;		/* zero-init */					/* z */

  /* Final punctuation. */
  i++;		/* zero-init */					/* { */
  i++;		/* zero-init */					/* | */
  i++;		/* zero-init */					/* } */
  i++;		/* zero-init */					/* ~ */
  ret[i] = KME (ISFUNC, &Readline::rl_backward_kill_line);	/* RUBOUT */

  // the rest of the array is zero-inited.
  return ret;
}

}  // namespace readline
