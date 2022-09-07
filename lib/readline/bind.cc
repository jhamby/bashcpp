/* bind.c -- key binding and startup file support for the readline library. */

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
#include <fcntl.h>

#if defined (HAVE_SYS_FILE_H)
#  include <sys/file.h>
#endif /* HAVE_SYS_FILE_H */

#include "posixstat.hh"

namespace readline
{

#define OP_EQ	1
#define OP_NE	2
#define OP_GT	3
#define OP_GE	4
#define OP_LT	5
#define OP_LE	6

#define OPSTART(c)	((c) == '=' || (c) == '!' || (c) == '<' || (c) == '>')
#define CMPSTART(c)	((c) == '=' || (c) == '!')

static int find_string_var (const char *);
static const char *string_varname (int);
static int glean_key_from_name (const char *);

/* **************************************************************** */
/*								    */
/*			Binding keys				    */
/*								    */
/* **************************************************************** */

/* Bind KEY to FUNCTION.  Returns non-zero if KEY is out of range. */
int
Readline::rl_bind_key (int key, rl_command_func_t function)
{
  char keyseq[4];
  int l;

  if (key < 0 || key > largest_char)
    return key;

  /* Want to make this a multi-character key sequence with an ESC prefix */
  if (META_CHAR (key) && _rl_convert_meta_chars_to_ascii)
    {
      if (_rl_keymap[ESC].type == ISKMAP)
	{
	  Keymap escmap;

	  escmap = _rl_keymap[ESC].value.map;
	  key = UNMETA (key);
	  escmap[key].type = ISFUNC;
	  escmap[key].value.function = function;
	  return 0;
	}

      /* Otherwise, let's just let rl_generic_bind handle the key sequence.
	 We start it off with ESC here and let the code below add the rest
	 of the sequence. */
      keyseq[0] = ESC;
      l = 1;
      key = UNMETA(key);
      goto bind_keyseq;
    }

  /* If it's bound to a function or macro, just overwrite.  Otherwise we have
     to treat it as a key sequence so rl_generic_bind handles shadow keymaps
     for us.  If we are binding '\' or \C-@ (NUL) make sure to escape it so
     it makes it through the call to rl_translate_keyseq. */
  if (_rl_keymap[key].type != ISKMAP)
    {
      if (_rl_keymap[key].type == ISMACR)
	delete[] _rl_keymap[key].value.macro;

      _rl_keymap[key].type = ISFUNC;
      _rl_keymap[key].value.function = function;
    }
  else
    {
      l = 0;
bind_keyseq:
      if (key == '\\')
	{
	  keyseq[l++] = '\\';
	  keyseq[l++] = '\\';
	}
      else if (key == '\0')
	{
	  keyseq[l++] = '\\';
	  keyseq[l++] = '0';
	}
      else
	keyseq[l++] = static_cast<char> (key);
      keyseq[l] = '\0';
      rl_bind_keyseq (keyseq, function);
    }
  rl_binding_keymap = _rl_keymap;
  return 0;
}

/* Unbind all keys bound to FUNCTION in MAP. */
int
Readline::rl_unbind_function_in_map (rl_command_func_t func, Keymap map)
{
  int rval = 0;

  for (int i = 0; i < KEYMAP_SIZE; i++)
    {
      if (map[i].type == ISFUNC && map[i].value.function == func)
	{
	  map[i].value.function = nullptr;
	  rval = 1;
	}
      else if (map[i].type == ISKMAP)		/* TAG:readline-8.1 */
	{
	  int r;
	  r = rl_unbind_function_in_map (func, map[i].value.map);
	  if (r == 1)
	    rval = 1;
	}
    }
  return rval;
}

/* Bind key sequence KEYSEQ to DEFAULT_FUNC if KEYSEQ is unbound.  Right
   now, this is always used to attempt to bind the arrow keys, hence the
   check for rl_vi_movement_mode. */
int
Readline::rl_bind_keyseq_if_unbound_in_map (const char *keyseq,
					    rl_command_func_t default_func,
					    Keymap kmap)
{
  if (keyseq)
    {
      /* Handle key sequences that require translations and `raw' ones that
	 don't. This might be a problem with backslashes. */
      char *keys = new char[1 + (2 * std::strlen (keyseq))];
      unsigned int keys_len;
      if (rl_translate_keyseq (keyseq, keys, &keys_len))
	{
	  delete[] keys;
	  return -1;
	}
      rl_command_func_t func = rl_function_of_keyseq_len (keys, keys_len,
							  kmap, nullptr);
      delete[] keys;

#if defined (VI_MODE)
      if (!func || func == &Readline::rl_do_lowercase_version ||
	  func == &Readline::rl_vi_movement_mode)
#else
      if (!func || func == &Readline::rl_do_lowercase_version)
#endif
	return rl_bind_keyseq_in_map (keyseq, default_func, kmap);
      else
	return 1;
    }
  return 0;
}

/* Bind the key sequence represented by the string KEYSEQ to
   the arbitrary pointer DATA.  The type and data are passed as
   a KEYMAP_ENTRY ref.  This makes new keymaps as necessary.
   The initial place to do bindings is in MAP. */
int
Readline::rl_generic_bind (const char *keyseq, KEYMAP_ENTRY &k, Keymap map)
{
  /* If no keys to bind to, exit right away. */
  if (keyseq == nullptr || *keyseq == '\0')
    {
      if (k.type == ISMACR)
	delete[] k.value.macro;
      return -1;
    }

  char *keys = new char[1 + (2 * std::strlen (keyseq))];
  unsigned int keys_len;

  /* Translate the ASCII representation of KEYSEQ into an array of
     characters.  Stuff the characters into KEYS, and the length of
     KEYS into KEYS_LEN. */
  if (rl_translate_keyseq (keyseq, keys, &keys_len))
    {
      delete[] keys;
      return -1;
    }

  Keymap prevmap = map;
  int prevkey = static_cast<unsigned char> (keys[0]);
  int ic = 0;	// must be able to hold 256 == ANYOTHERKEY

  /* Bind keys, making new keymaps as necessary. */
  for (unsigned int i = 0; i < keys_len; i++)
    {
      unsigned char uc = static_cast<unsigned char> (keys[i]);

      if (i > 0)
	prevkey = ic;

      ic = uc;
      if (ic < 0 || ic >= KEYMAP_SIZE)
        {
          delete[] keys;
	  return -1;
        }

      if ((i + 1) < keys_len)
	{
	  if (map[ic].type != ISKMAP)
	    {
	      /* We allow subsequences of keys.  If a keymap is being
		 created that will `shadow' an existing function or macro
		 key binding, we save that keybinding into the ANYOTHERKEY
		 index in the new map.  The dispatch code will look there
		 to find the function to execute if the subsequence is not
		 matched.  ANYOTHERKEY was chosen to be greater than
		 UCHAR_MAX. */
	      k = map[ic];

	      map[ic].type = ISKMAP;
	      map[ic].value.map = rl_make_bare_keymap();
	    }

	  prevmap = map;
	  map = map[ic].value.map;

	  /* The dispatch code will return this function if no matching
	     key sequence is found in the keymap.  This (with a little
	     help from the dispatch code in readline.c) allows `a' to be
	     mapped to something, `abc' to be mapped to something else,
	     and the function bound  to `a' to be executed when the user
	     types `abx', leaving `bx' in the input queue. */
	  if (k.type == ISFUNC && k.value.function != nullptr &&
	      k.value.function != &Readline::rl_do_lowercase_version)
	    {
	      map[ANYOTHERKEY] = k;
	      k.value.function = nullptr;
	    }
	  else if (k.type == ISMACR && k.value.macro != nullptr)
	    {
	      map[ANYOTHERKEY] = k;
	      k.value.macro = nullptr;
	    }
	}
      else
	{
	  if (map[ic].type == ISKMAP)
	    {
	      prevmap = map;
	      map = map[ic].value.map;
	      ic = ANYOTHERKEY;
	      /* If we're trying to override a keymap with a null function
		 (e.g., trying to unbind it), we can't use a null pointer
		 here because that's indistinguishable from having not been
		 overridden.  We use a special bindable function that does
		 nothing. */
	      if (k.type == ISFUNC && k.value.function == nullptr)
		k.value.function = &Readline::_rl_null_function;
	    }
	  if (map[ic].type == ISMACR)
	    delete[] map[ic].value.macro;

	  map[ic] = k;
	}

      rl_binding_keymap = map;
    }

  /* If we unbound a key (type == ISFUNC, data == 0), and the prev keymap
     points to the keymap where we unbound the key (sanity check), and the
     current binding keymap is empty (rl_empty_keymap() returns non-zero),
     and the binding keymap has ANYOTHERKEY set with type == ISFUNC
     (overridden function), delete the now-empty keymap, take the previously-
     overridden function and remove the override. */
  /* Right now, this only works one level back. */
  if (k.type == ISFUNC && k.value.function == nullptr &&
      prevmap[prevkey].type == ISKMAP &&
      (prevmap[prevkey].value.map == rl_binding_keymap) &&
      rl_binding_keymap[ANYOTHERKEY].type == ISFUNC &&
      rl_empty_keymap (rl_binding_keymap))
    {
      prevmap[prevkey] = rl_binding_keymap[ANYOTHERKEY];
      rl_discard_keymap (rl_binding_keymap);
      rl_binding_keymap = prevmap;
    }

  delete[] keys;
  return 0;
}

/* Translate the ASCII representation of SEQ, stuffing the values into ARRAY,
   an array of characters.  LEN gets the final length of ARRAY.  Return
   non-zero if there was an error parsing SEQ. */
int
Readline::rl_translate_keyseq (const char *seq, char *array, unsigned int *len)
{
  int l = 0;
  int c;

  bool has_control = false;
  bool has_meta = false;

  /* When there are incomplete prefixes \C- or \M- (has_control || has_meta)
     without base character at the end of SEQ, they are processed as the
     prefixes for '\0'.
  */
  for (int i = 0; (c = seq[i]) || has_control || has_meta; i++)
    {
      /* Only backslashes followed by a non-null character are handled
	 specially.  Trailing backslash (backslash followed by '\0') is
	 processed as a normal character.
      */
      if (c == '\\' && seq[i + 1] != '\0')
	{
	  c = seq[++i];

	  /* Handle \C- and \M- prefixes. */
	  if (c == 'C' && seq[i + 1] == '-')
	    {
	      i++;
	      has_control = true;
	      continue;
	    }
	  else if (c == 'M' && seq[i + 1] == '-')
	    {
	      i++;
	      has_meta = true;
	      continue;
	    }

	  /* Translate other backslash-escaped characters.  These are the
	     same escape sequences that bash's `echo' and `printf' builtins
	     handle, with the addition of \d -> RUBOUT.  A backslash
	     preceding a character that is not special is stripped. */
	  int temp;
	  switch (c)
	    {
	    case 'a':
	      c = '\007';
	      break;
	    case 'b':
	      c = '\b';
	      break;
	    case 'd':
	      c = RUBOUT;	/* readline-specific */
	      break;
	    case 'e':
	      c = ESC;
	      break;
	    case 'f':
	      c = '\f';
	      break;
	    case 'n':
	      c = NEWLINE;
	      break;
	    case 'r':
	      c = RETURN;
	      break;
	    case 't':
	      c = TAB;
	      break;
	    case 'v':
	      c = 0x0B;
	      break;
	    case '\\':
	      c = '\\';
	      break;
	    case '0': case '1': case '2': case '3':
	    case '4': case '5': case '6': case '7':
	      i++;
	      for (temp = 2, c -= '0'; ISOCTAL (seq[i]) && temp--; i++)
	        c = (c * 8) + OCTVALUE (seq[i]);
	      i--;	/* auto-increment in for loop */
	      c &= largest_char;
	      break;
	    case 'x':
	      i++;
	      for (temp = 2, c = 0; ISXDIGIT (seq[i]) && temp--; i++)
	        c = (c * 16) + HEXVALUE (seq[i]);
	      if (temp == 2)
	        c = 'x';
	      i--;	/* auto-increment in for loop */
	      c &= largest_char;
	      break;
	    default:	/* backslashes before non-special chars just add the char */
	      c &= largest_char;
	      break;	/* the backslash is stripped */
	    }
	}

      /* Process \C- and \M- flags */
      if (has_control)
	{
	  /* Special treatment for C-? */
	  c = (c == '?') ? RUBOUT : CTRL (_rl_to_upper (c));
	  has_control = false;
	}
      if (has_meta)
	{
	  c = META (c);
	  has_meta = false;
	}

      /* If convert-meta is turned on, convert a meta char to a key sequence  */
      if (META_CHAR (c) && _rl_convert_meta_chars_to_ascii)
	{
	  array[l++] = ESC;	/* ESC is meta-prefix */
	  array[l++] = static_cast<char> (UNMETA (c));
	}
      else
	array[l++] = static_cast<char> (c);

      /* Null characters may be processed for incomplete prefixes at the end of
	 sequence */
      if (seq[i] == '\0')
	break;
    }

  *len = static_cast<unsigned int> (l);
  array[l] = '\0';
  return 0;
}

static inline bool
_rl_isescape (int c)
{
  switch (c)
    {
    case '\007':
    case '\b':
    case '\f':
    case '\n':
    case '\r':
    case TAB:
    case 0x0b:  return true;
    default: return false;
    }
}

static inline int
_rl_escchar (int c)
{
  switch (c)
    {
    case '\007':  return 'a';
    case '\b':  return 'b';
    case '\f':  return 'f';
    case '\n':  return 'n';
    case '\r':  return 'r';
    case TAB:  return 't';
    case 0x0b:  return 'v';
    default: return c;
    }
}

char *
Readline::rl_untranslate_keyseq (int seq)
{
  static char kseq[16];
  int i, c;

  i = 0;
  c = seq;
  if (META_CHAR (c))
    {
      kseq[i++] = '\\';
      kseq[i++] = 'M';
      kseq[i++] = '-';
      c = UNMETA (c);
    }
  else if (c == ESC)
    {
      kseq[i++] = '\\';
      c = 'e';
    }
  else if (CTRL_CHAR (c))
    {
      kseq[i++] = '\\';
      kseq[i++] = 'C';
      kseq[i++] = '-';
      c = _rl_to_lower (UNCTRL (c));
    }
  else if (c == RUBOUT)
    {
      kseq[i++] = '\\';
      kseq[i++] = 'C';
      kseq[i++] = '-';
      c = '?';
    }

  if (c == ESC)
    {
      kseq[i++] = '\\';
      c = 'e';
    }
  else if (c == '\\' || c == '"')
    {
      kseq[i++] = '\\';
    }

  kseq[i++] = static_cast<char> (c);
  kseq[i] = '\0';
  return kseq;
}

char *
Readline::_rl_untranslate_macro_value (const char *seq, bool use_escapes)
{
  char *ret, *r;

  r = ret = new char[7 * std::strlen (seq) + 1];
  for (const char *s = seq; *s; s++)
    {
      int c = *s;
      if (META_CHAR (c))
	{
	  *r++ = '\\';
	  *r++ = 'M';
	  *r++ = '-';
	  c = UNMETA (c);
	}
      else if (c == ESC)
	{
	  *r++ = '\\';
	  c = 'e';
	}
      else if (CTRL_CHAR (c))
	{
	  *r++ = '\\';
	  if (use_escapes && _rl_isescape (c))
	    c = _rl_escchar (c);
	  else
	    {
	      *r++ = 'C';
	      *r++ = '-';
	      c = _rl_to_lower (UNCTRL (c));
	    }
	}
      else if (c == RUBOUT)
 	{
 	  *r++ = '\\';
 	  *r++ = 'C';
 	  *r++ = '-';
 	  c = '?';
 	}

      if (c == ESC)
	{
	  *r++ = '\\';
	  c = 'e';
	}
      else if (c == '\\' || c == '"')
	*r++ = '\\';

      *r++ = static_cast<char> (c);
    }
  *r = '\0';
  return ret;
}

/* Return the function (or macro) definition which would be invoked via
   KEYSEQ if executed in MAP.  If MAP is NULL, then the current keymap is
   used.  TYPE, if non-NULL, is a pointer to an int which will receive the
   type of the object pointed to.  One of ISFUNC (function), ISKMAP (keymap),
   or ISMACR (macro). */
Readline::rl_command_func_t
Readline::_rl_function_of_keyseq_internal (const char *keyseq, unsigned int len, Keymap map,
					   int *type)
{
  if (map == nullptr)
    map = _rl_keymap;

  for (unsigned int i = 0; keyseq && i < len; i++)
    {
      int ic = static_cast<unsigned char> (keyseq[i]);

      if (META_CHAR (ic) && _rl_convert_meta_chars_to_ascii)
	{
	  if (map[ESC].type == ISKMAP)
	    {
	      map = map[ESC].value.map;
	      ic = UNMETA (ic);
	    }
	  /* XXX - should we just return NULL here, since this obviously
	     doesn't match? */
	  else
	    {
	      if (type)
		*type = map[ESC].type;

	      return map[ESC].value.function;
	    }
	}

      if (map[ic].type == ISKMAP)
	{
	  /* If this is the last key in the key sequence, return the
	     map. */
	  if (i + 1 == len)
	    {
	      if (type)
		*type = ISKMAP;

	      return map[ic].value.function;
	    }
	  else
	    map = map[ic].value.map;
	}
      /* If we're not at the end of the key sequence, and the current key
	 is bound to something other than a keymap, then the entire key
	 sequence is not bound. */
      else if (map[ic].type != ISKMAP && i + 1 < len)
	return nullptr;
      else	/* map[ic].type != ISKMAP && i + 1 == len */
	{
	  if (type)
	    *type = map[ic].type;

	  return map[ic].value.function;
	}
    }
  return nullptr;
}

/* Read FILENAME into a locally-allocated buffer and return the buffer.
   The size of the buffer is returned in *SIZEP.  Returns nullptr if any
   errors were encountered. */
char *
Readline::_rl_read_file (char *filename, size_t *sizep)
{
  struct stat finfo;
  char *buffer;
  int file;

  file = -1;
  if (((file = ::open (filename, O_RDONLY, 0644)) < 0) || (::fstat (file, &finfo) < 0))
    {
      if (file >= 0)
	::close (file);
      return nullptr;
    }

  size_t file_size = static_cast<size_t> (finfo.st_size);
  ssize_t sfile_size = static_cast<ssize_t> (file_size);

  /* check for overflow on very large files using signed file size */
  if (sfile_size != finfo.st_size || sfile_size < 0)
    {
      if (file >= 0)
	::close (file);
#if defined (EFBIG)
      errno = EFBIG;
#endif
      return nullptr;
    }

  /* Read the file into BUFFER. */
  buffer = new char[file_size + 1];
  ssize_t i = ::read (file, buffer, file_size);
  ::close (file);

  if (i < 0)
    {
      delete[] buffer;
      return nullptr;
    }

  RL_CHECK_SIGNALS ();

  buffer[i] = '\0';
  if (sizep)
    *sizep = static_cast<size_t> (i);

  return buffer;
}

/* Re-read the current keybindings file. */
int
Readline::rl_re_read_init_file (int, int)
{
  int r = rl_read_init_file (nullptr);
  rl_set_keymap_from_edit_mode ();
  return r;
}

/* Do key bindings from a file.  If FILENAME is NULL it defaults
   to the first non-null filename from this list:
     1. the filename used for the previous call
     2. the value of the shell variable `INPUTRC'
     3. ~/.inputrc
     4. /etc/inputrc
   If the file existed and could be opened and read, 0 is returned,
   otherwise errno is returned. */
int
Readline::rl_read_init_file (const char *filename)
{
  /* Default the filename. */
  if (filename == nullptr)
    filename = last_readline_init_file;
  if (filename == nullptr)
    filename = sh_get_env_value ("INPUTRC");
  if (filename == nullptr || *filename == '\0')
    {
      filename = DEFAULT_INPUTRC;
      /* Try to read DEFAULT_INPUTRC; fall back to SYS_INPUTRC on failure */
      if (_rl_read_init_file (filename, 0) == 0)
	return 0;
      filename = SYS_INPUTRC;
    }

#if defined (__MSDOS__)
  if (_rl_read_init_file (filename, 0) == 0)
    return 0;
  filename = "~/_inputrc";
#endif
  return _rl_read_init_file (filename, 0);
}

int
Readline::_rl_read_init_file (const char *filename, int include_level)
{
  char *buffer, *openname, *line, *end;
  size_t file_size;

  current_readline_init_file = filename;
  current_readline_init_include_level = include_level;

  openname = tilde_expand (filename);
  buffer = _rl_read_file (openname, &file_size);
  delete[] openname;

  RL_CHECK_SIGNALS ();
  if (buffer == nullptr)
    return errno;

  if (include_level == 0 && filename != last_readline_init_file)
    {
      delete[] last_readline_init_file;
      last_readline_init_file = savestring (filename);
    }

  currently_reading_init_file = true;

  /* Loop over the lines in the file.  Lines that start with `#' are
     comments; all other lines are commands for readline initialization. */
  current_readline_init_lineno = 1;
  line = buffer;
  end = buffer + file_size;
  while (line < end)
    {
      /* Find the end of this line. */
      int i;
      for (i = 0; line + i != end && line[i] != '\n'; i++);

#if defined (__CYGWIN__)
      /* ``Be liberal in what you accept.'' */
      if (line[i] == '\n' && line[i-1] == '\r')
	line[i - 1] = '\0';
#endif

      /* Mark end of line. */
      line[i] = '\0';

      /* Skip leading whitespace. */
      while (*line && whitespace (*line))
        {
	  line++;
	  i--;
        }

      /* If the line is not a comment, then parse it. */
      if (*line && *line != '#')
	rl_parse_and_bind (line);

      /* Move to the next line. */
      line += i + 1;
      current_readline_init_lineno++;
    }

  delete[] buffer;
  currently_reading_init_file = false;
  return 0;
}

void
Readline::_rl_init_file_error (const char *format, ...)
{
  va_list args;
  va_start (args, format);

  std::fprintf (stderr, "readline: ");
  if (currently_reading_init_file)
    std::fprintf (stderr, "%s: line %d: ", current_readline_init_file,
		     current_readline_init_lineno);

  std::vfprintf (stderr, format, args);
  std::fprintf (stderr, "\n");
  std::fflush (stderr);

  va_end (args);
}

/* **************************************************************** */
/*								    */
/*			Parser Helper Functions       		    */
/*								    */
/* **************************************************************** */

static inline int
parse_comparison_op (const char *s, size_t *indp)
{
  size_t i;
  int op;
  char peekc;

  if (OPSTART (s[*indp]) == 0)
    return -1;
  i = *indp;
  peekc = s[i] ? s[i+1] : 0;
  op = -1;

  if (s[i] == '=')
    {
      op = OP_EQ;
      if (peekc == '=')
        i++;
      i++;
    }
  else if (s[i] == '!' && peekc == '=')
    {
      op = OP_NE;
      i += 2;
    }
  else if (s[i] == '<' && peekc == '=')
    {
      op = OP_LE;
      i += 2;
    }
  else if (s[i] == '>' && peekc == '=')
    {
      op = OP_GE;
      i += 2;
    }
  else if (s[i] == '<')
    {
      op = OP_LT;
      i += 1;
    }
  else if (s[i] == '>')
    {
      op = OP_GT;
      i += 1;
    }

  *indp = i;
  return op;
}

/* **************************************************************** */
/*								    */
/*			Parser Directives       		    */
/*								    */
/* **************************************************************** */

/* Things that mean `Control'. */
static const char * const _rl_possible_control_prefixes[] = {
  "Control-", "C-", "CTRL-", nullptr
};

static const char * const _rl_possible_meta_prefixes[] = {
  "Meta", "M-", nullptr
};

/* Conditionals. */

/* Push _rl_parsing_conditionalized_out, and set parser state based
   on ARGS. */
int
Readline::parser_if (char *args)
{
  int boolvar = -1, strvar = -1;

  /* Push parser state. */
  if_stack.push_back (_rl_parsing_conditionalized_out);

  /* If parsing is turned off, then nothing can turn it back on except
     for finding the matching endif.  In that case, return right now. */
  if (_rl_parsing_conditionalized_out)
    return 0;

  size_t llen = std::strlen (args);

  /* Isolate first argument. */
  size_t i;
  for (i = 0; args[i] && !whitespace (args[i]); i++);

  if (args[i])
    args[i++] = '\0';

  /* Handle "$if term=foo" and "$if mode=emacs" constructs.  If this
     isn't term=foo, or mode=emacs, then check to see if the first
     word in ARGS is the same as the value stored in rl_readline_name. */
  if (rl_terminal_name && _rl_strnicmp (args, "term=", 5) == 0)
    {
      char *tem, *tname;

      /* Terminals like "aaa-60" are equivalent to "aaa". */
      tname = savestring (rl_terminal_name);
      tem = std::strchr (tname, '-');
      if (tem)
	*tem = '\0';

      /* Test the `long' and `short' forms of the terminal name so that
	 if someone has a `sun-cmd' and does not want to have bindings
	 that will be executed if the terminal is a `sun', they can put
	 `$if term=sun-cmd' into their .inputrc. */
      _rl_parsing_conditionalized_out = _rl_stricmp (args + 5, tname) &&
					_rl_stricmp (args + 5, rl_terminal_name);
      delete[] tname;
    }
#if defined (VI_MODE)
  else if (_rl_strnicmp (args, "mode=", 5) == 0)
    {
      editing_mode mode;

      if (_rl_stricmp (args + 5, "emacs") == 0)
	mode = emacs_mode;
      else if (_rl_stricmp (args + 5, "vi") == 0)
	mode = vi_mode;
      else
	mode = no_mode;

      _rl_parsing_conditionalized_out = (mode != rl_editing_mode);
    }
#endif /* VI_MODE */
  else if (_rl_strnicmp (args, "version", 7) == 0)
    {
      _rl_parsing_conditionalized_out = true;
      int rlversion = RL_VERSION_MAJOR * 10 + RL_VERSION_MINOR;

      /* if "version" is separated from the operator by whitespace, or the
         operand is separated from the operator by whitespace, restore it.
         We're more liberal with allowed whitespace for this variable. */
      if (i > 0 && i <= llen && args[i - 1] == '\0')
        args[i - 1] = ' ';
      args[llen] = '\0';		/* just in case */

      for (i = 7; whitespace (args[i]); i++)
	;
      if (OPSTART(args[i]) == 0)
	{
	  _rl_init_file_error ("comparison operator expected, found `%s'",
				args[i] ? args + i : "end-of-line");
	  return 0;
	}

      size_t previ = i;
      int op = parse_comparison_op (args, &i);
      if (op <= 0)
	{
	  _rl_init_file_error ("comparison operator expected, found `%s'", args + previ);
	  return 0;
	}
      for ( ; args[i] && whitespace (args[i]); i++)
	;
      if (args[i] == 0 || _rl_digit_p (args[i]) == 0)
	{
	  _rl_init_file_error ("numeric argument expected, found `%s'", args + i);
	  return 0;
	}

      int major = 0, minor = 0;
      previ = i;
      for ( ; args[i] && _rl_digit_p (args[i]); i++)
	major = major * 10 + _rl_digit_value (args[i]);

      if (args[i] == '.')
	{
	  if (args[i + 1] && _rl_digit_p (args [i + 1]) == 0)
	    {
	      _rl_init_file_error ("numeric argument expected, found `%s'", args+previ);
	      return 0;
	    }
	  for (++i; args[i] && _rl_digit_p (args[i]); i++)
	    minor = minor * 10 + _rl_digit_value (args[i]);
	}

      /* optional - check for trailing garbage on the line, allow whitespace
	 and a trailing comment */
      previ = i;
      for ( ; args[i] && whitespace (args[i]); i++)
	;
      if (args[i] && args[i] != '#')
	{
	  _rl_init_file_error ("trailing garbage on line: `%s'", args+previ);
	  return 0;
	}

      int versionarg = major * 10 + minor;

      bool opresult;
      switch (op)
	{
	case OP_EQ:
 	  opresult = rlversion == versionarg;
	  break;
	case OP_NE:
	  opresult = rlversion != versionarg;
	  break;
	case OP_GT:
	  opresult = rlversion > versionarg;
	  break;
	case OP_GE:
	  opresult = rlversion >= versionarg;
	  break;
	case OP_LT:
	  opresult = rlversion < versionarg;
	  break;
	case OP_LE:
	  opresult = rlversion <= versionarg;
	  break;
	default:
	  opresult = false;
	  break;
	}
      _rl_parsing_conditionalized_out = !opresult;
    }
  /* Check to see if the first word in ARGS is the same as the
     value stored in rl_readline_name. */
  else if (_rl_stricmp (args, rl_readline_name) == 0)
    _rl_parsing_conditionalized_out = 0;
  else if ((boolvar = find_boolean_var (args)) >= 0 || (strvar = find_string_var (args)) >= 0)
    {
      int op;
      size_t previ;
      size_t vlen;
      const char *vname;
      char *valuearg, prevc;

      _rl_parsing_conditionalized_out = 1;
      vname = (boolvar >= 0) ? boolean_varname (boolvar) : string_varname (strvar);
      vlen = std::strlen (vname);
      if (i > 0 && i <= llen && args[i - 1] == '\0')
        args[i - 1] = ' ';
      args[llen] = '\0';		/* just in case */

      for (i = vlen; whitespace (args[i]); i++)
	;
      if (CMPSTART(args[i]) == 0)
	{
	  _rl_init_file_error ("equality comparison operator expected, found `%s'", args[i] ? args + i : "end-of-line");
	  return 0;
	}

      previ = i;
      op = parse_comparison_op (args, &i);
      if (op != OP_EQ && op != OP_NE)
	{
	  _rl_init_file_error ("equality comparison operator expected, found `%s'", args+previ);
	  return 0;
	}

      for ( ; args[i] && whitespace (args[i]); i++)
	;
      if (args[i] == 0)
	{
	  _rl_init_file_error ("argument expected, found `%s'", args+i);
	  return 0;
	}

      previ = i;
      valuearg = args + i;
      for ( ; args[i] && whitespace (args[i]) == 0; i++)
	;

      prevc = args[i];

      args[i] = '\0';		/* null-terminate valuearg */
      const char *vval = rl_variable_value (vname);
      if (op == OP_EQ)
        _rl_parsing_conditionalized_out = _rl_stricmp (vval, valuearg) != 0;
      else if (op == OP_NE)
        _rl_parsing_conditionalized_out = _rl_stricmp (vval, valuearg) == 0;

      args[i] = prevc;
    }
  else
    _rl_parsing_conditionalized_out = true;
  return 0;
}

/* Invert the current parser state if there is anything on the stack. */
int
Readline::parser_else (char *)
{
  if (if_stack.empty ())
    {
      _rl_init_file_error ("$else found without matching $if");
      return 0;
    }

  /* Check the previous (n) levels of the stack to make sure that
     we haven't previously turned off parsing. */
  for (std::vector<bool>::iterator it = if_stack.begin (); it != if_stack.end (); ++it)
    if (*it)
      return 0;

  /* Invert the state of parsing if at top level. */
  _rl_parsing_conditionalized_out = !_rl_parsing_conditionalized_out;

  return 0;
}

/* Terminate a conditional, popping the value of
   _rl_parsing_conditionalized_out from the stack. */
int
Readline::parser_endif (char *)
{
  if (!if_stack.empty ())
    {
      _rl_parsing_conditionalized_out = if_stack.back ();
      if_stack.pop_back ();
    }
  else
    _rl_init_file_error ("$endif without matching $if");

  return 0;
}

int
Readline::parser_include (char *args)
{
  const char *old_init_file;
  char *e;
  int old_line_number, old_include_level, r;

  if (_rl_parsing_conditionalized_out)
    return 0;

  old_init_file = current_readline_init_file;
  old_line_number = current_readline_init_lineno;
  old_include_level = current_readline_init_include_level;

  e = std::strchr (args, '\n');
  if (e)
    *e = '\0';
  r = _rl_read_init_file (args, old_include_level + 1);

  current_readline_init_file = old_init_file;
  current_readline_init_lineno = old_line_number;
  current_readline_init_include_level = old_include_level;

  return r;
}

/* Associate textual names with actual functions. */
static const struct {
  const char *name;
  Readline::_rl_parser_func_t function;
} parser_directives[] = {
  { "if", &Readline::parser_if },
  { "endif", &Readline::parser_endif },
  { "else", &Readline::parser_else },
  { "include", &Readline::parser_include },
  { nullptr, nullptr }
};

/* Handle a parser directive.  STATEMENT is the line of the directive
   without any leading `$'. */
int
Readline::handle_parser_directive (char *statement)
{
  int i;
  char *directive, *args;

  /* Isolate the actual directive. */

  /* Skip whitespace. */
  for (i = 0; whitespace (statement[i]); i++);

  directive = &statement[i];

  for (; statement[i] && !whitespace (statement[i]); i++);

  if (statement[i])
    statement[i++] = '\0';

  for (; statement[i] && whitespace (statement[i]); i++);

  args = &statement[i];

  /* Lookup the command, and act on it. */
  for (i = 0; parser_directives[i].name; i++)
    if (_rl_stricmp (directive, parser_directives[i].name) == 0)
      {
	((*this).*parser_directives[i].function) (args);
	return 0;
      }

  /* display an error message about the unknown parser directive */
  _rl_init_file_error ("%s: unknown parser directive", directive);
  return 1;
}

/* Start at STRING[START] and look for DELIM.  Return I where STRING[I] ==
   DELIM or STRING[I] == 0.  DELIM is usually a double quote. */
int
Readline::_rl_skip_to_delim (char *string, int start, int delim)
{
  int i, c, passc;

  for (i = start,passc = 0; (c = string[i]); i++)
    {
      if (passc)
	{
	  passc = 0;
	  if (c == 0)
	    break;
	  continue;
	}

      if (c == '\\')
	{
	  passc = 1;
	  continue;
	}

      if (c == delim)
	break;
    }

  return i;
}

/* Read the binding command from STRING and perform it.
   A key binding command looks like: Keyname: function-name\0,
   a variable binding command looks like: set variable value.
   A new-style keybinding looks like "\C-x\C-x": exchange-point-and-mark. */
int
Readline::rl_parse_and_bind (char *string)
{
  while (string && whitespace (*string))
    string++;

  if (string == nullptr || *string == '\0' || *string == '#')
    return 0;

  /* If this is a parser directive, act on it. */
  if (*string == '$')
    {
      handle_parser_directive (&string[1]);
      return 0;
    }

  /* If we aren't supposed to be parsing right now, then we're done. */
  if (_rl_parsing_conditionalized_out)
    return 0;

  int i = 0;
  /* If this keyname is a complex key expression surrounded by quotes,
     advance to after the matching close quote.  This code allows the
     backslash to quote characters in the key expression. */
  if (*string == '"')
    {
      i = _rl_skip_to_delim (string, 1, '"');

      /* If we didn't find a closing quote, abort the line. */
      if (string[i] == '\0')
        {
          _rl_init_file_error ("%s: no closing `\"' in key binding", string);
          return 1;
        }
      else
        i++;	/* skip past closing double quote */
    }

  /* Advance to the colon (:) or whitespace which separates the two objects. */
  int c;
  for (; (c = string[i]) && c != ':' && c != ' ' && c != '\t'; i++ );

  if (i == 0)
    {
      _rl_init_file_error ("`%s': invalid key binding: missing key sequence", string);
      return 1;
    }

  bool equivalency = (c == ':' && string[i + 1] == '=');

  bool foundsep = c != 0;

  /* Mark the end of the command (or keyname). */
  if (string[i])
    string[i++] = '\0';

  /* If doing assignment, skip the '=' sign as well. */
  if (equivalency)
    string[i++] = '\0';

  /* If this is a command to set a variable, then do that. */
  if (_rl_stricmp (string, "set") == 0)
    {
      char *var, *value, *e;

      var = string + i;
      /* Make VAR point to start of variable name. */
      while (*var && whitespace (*var)) var++;

      /* Make VALUE point to start of value string. */
      value = var;
      while (*value && whitespace (*value) == 0) value++;
      if (*value)
	*value++ = '\0';
      while (*value && whitespace (*value)) value++;

      /* Strip trailing whitespace from values of boolean variables. */
      if (find_boolean_var (var) >= 0)
	{
	  /* just read a whitespace-delimited word or empty string */
	  for (e = value; *e && whitespace (*e) == 0; e++)
	    ;
	  if (e > value)
	    *e = '\0';		/* cut off everything trailing */
	}
      else if ((i = find_string_var (var)) >= 0)
	{
	  /* Allow quoted strings in variable values */
	  if (*value == '"')
	    {
	      i = _rl_skip_to_delim (value, 1, *value);
	      value[i] = '\0';
	      value++;	/* skip past the quote */
	    }
	  else
	    {
	      /* remove trailing whitespace */
	      e = value + std::strlen (value) - 1;
	      while (e >= value && whitespace (*e))
		e--;
	      e++;		/* skip back to whitespace or EOS */

	      if (*e && e >= value)
		*e = '\0';
	    }
	}
      else
	{
	  /* avoid calling rl_variable_bind just to find this out */
	  _rl_init_file_error ("%s: unknown variable name", var);
	  return 1;
	}

      rl_variable_bind (var, value);
      return 0;
    }

  /* Skip any whitespace between keyname and funname. */
  for (; string[i] && whitespace (string[i]); i++) ;

  char *funname = &string[i];

  /* Now isolate funname.
     For straight function names just look for whitespace, since
     that will signify the end of the string.  But this could be a
     macro definition.  In that case, the string is quoted, so skip
     to the matching delimiter.  We allow the backslash to quote the
     delimiter characters in the macro body. */
  /* This code exists to allow whitespace in macro expansions, which
     would otherwise be gobbled up by the next `for' loop.*/
  /* XXX - it may be desirable to allow backslash quoting only if " is
     the quoted string delimiter, like the shell. */
  if (*funname == '\'' || *funname == '"')
    {
      i = _rl_skip_to_delim (string, i+1, *funname);
      if (string[i])
	i++;
      else
	{
	  _rl_init_file_error ("`%s': missing closing quote for macro", funname);
	  return 1;
	}
    }

  /* Advance to the end of the string.  */
  for (; string[i] && whitespace (string[i]) == 0; i++) ;

  /* No extra whitespace at the end of the string. */
  string[i] = '\0';

  /* Handle equivalency bindings here.  Make the left-hand side be exactly
     whatever the right-hand evaluates to, including keymaps. */
  if (equivalency)
    {
      return 0;
    }

  if (!foundsep)
    {
      _rl_init_file_error ("%s: no key sequence terminator", string);
      return 1;
    }

  /* If this is a new-style key-binding, then do the binding with
     rl_bind_keyseq ().  Otherwise, let the older code deal with it. */
  if (*string == '"')
    {
      char *seq = new char[1 + std::strlen (string)];

      int j, k, passc;
      for (j = 1, k = passc = 0; string[j]; j++)
	{
	  /* Allow backslash to quote characters, but leave them in place.
	     This allows a string to end with a backslash quoting another
	     backslash, or with a backslash quoting a double quote.  The
	     backslashes are left in place for rl_translate_keyseq (). */
	  if (passc || (string[j] == '\\'))
	    {
	      seq[k++] = string[j];
	      passc = !passc;
	      continue;
	    }

	  if (string[j] == '"')
	    break;

	  seq[k++] = string[j];
	}
      seq[k] = '\0';

      /* Binding macro? */
      if (*funname == '\'' || *funname == '"')
	{
	  size_t fl = std::strlen (funname);

	  /* Remove the delimiting quotes from each end of FUNNAME. */
	  if (fl && funname[fl - 1] == *funname)
	    funname[fl - 1] = '\0';

	  rl_macro_bind (seq, &funname[1], _rl_keymap);
	}
      else
	rl_bind_keyseq (seq, rl_named_function (funname));

      delete[] seq;
      return 0;
    }

  /* Get the actual character we want to deal with. */
  char *kname = std::strrchr (string, '-');
  if (kname == nullptr)
    kname = string;
  else
    kname++;

  int key = glean_key_from_name (kname);

  /* Add in control and meta bits. */
  bool foundmod = false;
  if (substring_member_of_array (string, _rl_possible_control_prefixes))
    {
      key = CTRL (_rl_to_upper (key));
      foundmod = true;
    }

  if (substring_member_of_array (string, _rl_possible_meta_prefixes))
    {
      key = META (key);
      foundmod = true;
    }

  if (!foundmod && kname != string)
    {
      _rl_init_file_error ("%s: unknown key modifier", string);
      return 1;
    }

  /* Temporary.  Handle old-style keyname with macro-binding. */
  if (*funname == '\'' || *funname == '"')
    {
      char useq[2];
      size_t fl = std::strlen (funname);

      useq[0] = static_cast<char> (key);
      useq[1] = '\0';
      if (fl && funname[fl - 1] == *funname)
	funname[fl - 1] = '\0';

      rl_macro_bind (useq, &funname[1], _rl_keymap);
    }
#if defined (PREFIX_META_HACK)
  /* Ugly, but working hack to keep prefix-meta around. */
  else if (_rl_stricmp (funname, "prefix-meta") == 0)
    {
      char seq[2];

      seq[0] = static_cast<char> (key);
      seq[1] = '\0';
      KEYMAP_ENTRY entry (ISKMAP, emacs_meta_keymap ());
      rl_generic_bind (seq, entry, _rl_keymap);
    }
#endif /* PREFIX_META_HACK */
  else
    rl_bind_key (key, rl_named_function (funname));

  return 0;
}

/* Simple structure for boolean readline variables (i.e., those that can
   have one of two values; either "On" or 1 for truth, or "Off" or 0 for
   false. */

#define V_SPECIAL	0x1

void
Readline::init_boolean_varlist ()
{
  std::vector<bvt> &bvl = boolean_varlist_;

  // Note: keep this list sorted in case-insensitive order, for binary search by name.
  bvl.push_back (bvt ( "bind-tty-special-chars",	&_rl_bind_stty_chars,		0 ));
  bvl.push_back (bvt ( "blink-matching-paren",		&rl_blink_matching_paren,	V_SPECIAL ));
  bvl.push_back (bvt ( "byte-oriented",			&rl_byte_oriented,		0 ));
#if defined (COLOR_SUPPORT)
  bvl.push_back (bvt ( "colored-completion-prefix",	&_rl_colored_completion_prefix,	0 ));
  bvl.push_back (bvt ( "colored-stats",			&_rl_colored_stats,		0 ));
#endif /* COLOR_SUPPORT */
  bvl.push_back (bvt ( "completion-ignore-case",	&_rl_completion_case_fold,	0 ));
  bvl.push_back (bvt ( "completion-map-case",		&_rl_completion_case_map,	0 ));
  bvl.push_back (bvt ( "convert-meta",			&_rl_convert_meta_chars_to_ascii, 0 ));
  bvl.push_back (bvt ( "disable-completion",		&rl_inhibit_completion,		0 ));
  bvl.push_back (bvt ( "echo-control-characters",	&_rl_echo_control_chars,	0 ));
  bvl.push_back (bvt ( "enable-bracketed-paste",	&_rl_enable_bracketed_paste,	V_SPECIAL ));
  bvl.push_back (bvt ( "enable-keypad",			&_rl_enable_keypad,		0 ));
  bvl.push_back (bvt ( "enable-meta-key",		&_rl_enable_meta,		0 ));
  bvl.push_back (bvt ( "expand-tilde",			&rl_complete_with_tilde_expansion, 0 ));
  bvl.push_back (bvt ( "history-preserve-point",	&_rl_history_preserve_point,	0 ));
  bvl.push_back (bvt ( "horizontal-scroll-mode",	&_rl_horizontal_scroll_mode,	0 ));
  bvl.push_back (bvt ( "input-meta",			&_rl_meta_flag,			0 ));
  bvl.push_back (bvt ( "mark-directories",		&_rl_complete_mark_directories,	0 ));
  bvl.push_back (bvt ( "mark-modified-lines",		&_rl_mark_modified_lines,	0 ));
  bvl.push_back (bvt ( "mark-symlinked-directories",	&_rl_complete_mark_symlink_dirs, 0 ));
  bvl.push_back (bvt ( "match-hidden-files",		&_rl_match_hidden_files,	0 ));
  bvl.push_back (bvt ( "menu-complete-display-prefix", &_rl_menu_complete_prefix_first, 0 ));
  bvl.push_back (bvt ( "meta-flag",			&_rl_meta_flag,			0 ));
  bvl.push_back (bvt ( "output-meta",			&_rl_output_meta_chars,		0 ));
  bvl.push_back (bvt ( "page-completions",		&_rl_page_completions,		0 ));
  bvl.push_back (bvt ( "prefer-visible-bell",		&_rl_prefer_visible_bell,	V_SPECIAL ));
  bvl.push_back (bvt ( "print-completions-horizontally", &_rl_print_completions_horizontally, 0 ));
  bvl.push_back (bvt ( "revert-all-at-newline",		&_rl_revert_all_at_newline,	0 ));
  bvl.push_back (bvt ( "show-all-if-ambiguous",		&_rl_complete_show_all,		0 ));
  bvl.push_back (bvt ( "show-all-if-unmodified",	&_rl_complete_show_unmodified,	0 ));
  bvl.push_back (bvt ( "show-mode-in-prompt",		&_rl_show_mode_in_prompt,	0 ));
  bvl.push_back (bvt ( "skip-completed-text",		&_rl_skip_completed_text,	0 ));
#if defined (VISIBLE_STATS)
  bvl.push_back (bvt ( "visible-stats",			&rl_visible_stats,		0 ));
#endif /* VISIBLE_STATS */
}

// binary search the sorted string array.
int
Readline::find_boolean_var (const char *name)
{
  std::vector<boolean_var_t> *bvl = boolean_varlist ();
  size_t low = 0;
  size_t high = bvl->size () - 1;
  size_t mid = high >> 1;
  int dir;

  while ((dir = _rl_stricmp (name, (*bvl)[mid].name)))
    {
      if (dir > 0)
	if (mid == high)
	  return -1;
	else
	  low = mid + 1;
      else
	if (mid == 0)
	  return -1;
	else
	  high = mid - 1;

      mid = (low + high) >> 1;
    }

  return dir ? -1 : static_cast<int> (mid);
}

/* Hooks for handling special boolean variables, where a
   function needs to be called or another variable needs
   to be changed when they're changed. */
void
Readline::hack_special_boolean_var (int i)
{
  const char *name;

  // assume the array has been set up if we have an index.
  name = boolean_varlist_[static_cast<size_t> (i)].name;

  if (_rl_stricmp (name, "blink-matching-paren") == 0)
    _rl_enable_paren_matching (rl_blink_matching_paren);
  else if (_rl_stricmp (name, "prefer-visible-bell") == 0)
    {
      if (_rl_prefer_visible_bell)
	_rl_bell_preference = VISIBLE_BELL;
      else
	_rl_bell_preference = AUDIBLE_BELL;
    }
  else if (_rl_stricmp (name, "show-mode-in-prompt") == 0)
    _rl_reset_prompt ();
  else if (_rl_stricmp (name, "enable-bracketed-paste") == 0)
    _rl_enable_active_region = _rl_enable_bracketed_paste;
}

#define	V_STRING	1
#define V_INT		2

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"

static const struct {
  const char *name;
  int flags;
  Readline::_rl_sv_func_t set_func;
} string_varlist[] = {
  { "bell-style",		V_STRING,	&Readline::sv_bell_style },
  { "comment-begin",		V_STRING,	&Readline::sv_combegin },
  { "completion-display-width", V_INT,		&Readline::sv_compwidth },
  { "completion-prefix-display-length", V_INT,	&Readline::sv_dispprefix },
  { "completion-query-items",	V_INT,		&Readline::sv_compquery },
  { "editing-mode",		V_STRING,	&Readline::sv_editmode },
  { "emacs-mode-string", 	V_STRING,	&Readline::sv_emacs_modestr },
  { "history-size",		V_INT,		&Readline::sv_histsize },
  { "isearch-terminators", 	V_STRING,	&Readline::sv_isrchterm },
  { "keymap",			V_STRING,	&Readline::sv_keymap },
  { "keyseq-timeout",		V_INT,		&Readline::sv_seqtimeout },
  { "vi-cmd-mode-string", 	V_STRING,	&Readline::sv_vicmd_modestr },
  { "vi-ins-mode-string", 	V_STRING,	&Readline::sv_viins_modestr },
};

#pragma clang diagnostic pop

static int
find_string_var (const char *name)
{
  size_t low = 0;
  size_t high = (sizeof (string_varlist) / sizeof (string_varlist[0])) - 1;
  size_t mid = high >> 1;
  int dir;

  while ((dir = _rl_stricmp (name, string_varlist[mid].name)))
    {
      if (dir > 0)
	if (mid == high)
	  return -1;
	else
	  low = mid + 1;
      else
	if (mid == 0)
	  return -1;
	else
	  high = mid - 1;

      mid = (low + high) >> 1;
    }

  return dir ? -1 : static_cast<int> (mid);
}

static inline const char *
string_varname (int i)
{
  return (i >= 0) ? string_varlist[i].name : nullptr;
}

/* A boolean value that can appear in a `set variable' command is true if
   the value is null or empty, `on' (case-insensitive), or "1".  All other
   values result in 0 (false). */
static inline bool
str_to_bool (const char *value)
{
  return (value == nullptr || *value == '\0' ||
		(_rl_stricmp (value, "on") == 0) ||
		(value[0] == '1' && value[1] == '\0'));
}

const char *
Readline::rl_variable_value (const char *name)
{
  /* Check for simple variables first. */
  int i = find_boolean_var (name);
  if (i != -1)
    return *boolean_varlist_[static_cast<size_t> (i)].value ? "on" : "off";

  i = find_string_var (name);
  if (i != -1)
    return _rl_get_string_variable_value (string_varlist[i].name);

  /* Unknown variable names return nullptr. */
  return nullptr;
}

int
Readline::rl_variable_bind (const char *name, const char *value)
{
  /* Check for simple variables first. */
  int i = find_boolean_var (name);
  if (i != -1)
    {
      size_t ind = static_cast<size_t> (i);
      *boolean_varlist_[ind].value = str_to_bool (value);
      if (boolean_varlist_[ind].flags & V_SPECIAL)
	hack_special_boolean_var (i);
      return 0;
    }

  i = find_string_var (name);

  /* For the time being, string names without a handler function are simply
     ignored. */
  if (i == -1 || string_varlist[i].set_func == nullptr)
    {
      if (i == -1)
	_rl_init_file_error ("%s: unknown variable name", name);
      return 0;
    }

  int v = ((*this).*string_varlist[i].set_func) (value);
  if (v != 0)
    _rl_init_file_error ("%s: could not set value to `%s'", name, value);
  return v;
}

int
Readline::sv_editmode (const char *value)
{
  if (_rl_strnicmp (value, "vi", 2) == 0)
    {
#if defined (VI_MODE)
      _rl_keymap = vi_insertion_keymap ();
      rl_editing_mode = vi_mode;
#endif /* VI_MODE */
      return 0;
    }
  else if (_rl_strnicmp (value, "emacs", 5) == 0)
    {
      _rl_keymap = emacs_standard_keymap ();
      rl_editing_mode = emacs_mode;
      return 0;
    }
  return 1;
}

int
Readline::sv_combegin (const char *value)
{
  if (value && *value)
    {
      delete[] _rl_comment_begin;
      _rl_comment_begin = savestring (value);
      return 0;
    }
  return 1;
}

int
Readline::sv_dispprefix (const char *value)
{
  int nval = 0;

  if (value && *value)
    {
      nval = std::atoi (value);
      if (nval < 0)
	nval = 0;
    }
  _rl_completion_prefix_display_length = static_cast<unsigned int> (nval);
  return 0;
}

int
Readline::sv_compquery (const char *value)
{
  int nval = 100;

  if (value && *value)
    {
      nval = std::atoi (value);
      if (nval < 0)
	nval = 0;
    }
  rl_completion_query_items = nval;
  return 0;
}

int
Readline::sv_compwidth (const char *value)
{
  int nval = -1;

  if (value && *value)
    nval = std::atoi (value);

  _rl_completion_columns = nval;
  return 0;
}

int
Readline::sv_histsize (const char *value)
{
  int nval;

  nval = 500;
  if (value && *value)
    {
      nval = std::atoi (value);
      if (nval < 0)
	{
	  unstifle_history ();
	  return 0;
	}
    }
  stifle_history (static_cast<unsigned int> (nval));
  return 0;
}

int
Readline::sv_keymap (const char *value)
{
  Keymap kmap;

  kmap = rl_get_keymap_by_name (value);
  if (kmap)
    {
      rl_set_keymap (kmap);
      return 0;
    }
  return 1;
}

int
Readline::sv_seqtimeout (const char *value)
{
  int nval;

  nval = 0;
  if (value && *value)
    {
      nval = std::atoi (value);
      if (nval < 0)
	nval = 0;
    }
  _rl_keyseq_timeout = nval;
  return 0;
}

int
Readline::sv_bell_style (const char *value)
{
  if (value == nullptr || *value == '\0')
    _rl_bell_preference = AUDIBLE_BELL;
  else if (_rl_stricmp (value, "none") == 0 || _rl_stricmp (value, "off") == 0)
    _rl_bell_preference = NO_BELL;
  else if (_rl_stricmp (value, "audible") == 0 || _rl_stricmp (value, "on") == 0)
    _rl_bell_preference = AUDIBLE_BELL;
  else if (_rl_stricmp (value, "visible") == 0)
    _rl_bell_preference = VISIBLE_BELL;
  else
    return 1;
  return 0;
}

int
Readline::sv_isrchterm (const char *value)
{
  unsigned int beg, end;
  char *v;

  if (value == nullptr)
    return 1;

  /* Isolate the value and translate it into a character string. */
  v = savestring (value);
  delete[] _rl_isearch_terminators;
  if (v[0] == '"' || v[0] == '\'')
    {
      char delim = v[0];
      for (beg = end = 1; v[end] && v[end] != delim; end++)
	;
    }
  else
    {
      for (beg = end = 0; v[end] && whitespace (v[end]) == 0; end++)
	;
    }

  v[end] = '\0';

  /* The value starts at v + beg.  Translate it into a character string. */
  _rl_isearch_terminators = new char[2 * std::strlen (v) + 1];
  rl_translate_keyseq (v + beg, _rl_isearch_terminators, &end);
  _rl_isearch_terminators[end] = '\0';

  delete[] v;
  return 0;
}

int
Readline::sv_emacs_modestr (const char *value)
{
  if (value && *value)
    {
      delete[] _rl_emacs_mode_str;
      _rl_emacs_mode_str = new char[2 * std::strlen (value) + 1];
      rl_translate_keyseq (value, _rl_emacs_mode_str, &_rl_emacs_modestr_len);
      _rl_emacs_mode_str[_rl_emacs_modestr_len] = '\0';
      return 0;
    }
  else if (value)
    {
      delete[] _rl_emacs_mode_str;
      _rl_emacs_mode_str = new char[1];
      _rl_emacs_mode_str[_rl_emacs_modestr_len = 0] = '\0';
      return 0;
    }
  else if (value == nullptr)
    {
      delete[] _rl_emacs_mode_str;
      _rl_emacs_mode_str = nullptr;	/* prompt_modestr does the right thing */
      _rl_emacs_modestr_len = 0;
      return 0;
    }
  return 1;
}

int
Readline::sv_viins_modestr (const char *value)
{
  if (value && *value)
    {
      delete[] _rl_vi_ins_mode_str;
      _rl_vi_ins_mode_str = new char[2 * std::strlen (value) + 1];
      rl_translate_keyseq (value, _rl_vi_ins_mode_str, &_rl_vi_ins_modestr_len);
      _rl_vi_ins_mode_str[_rl_vi_ins_modestr_len] = '\0';
      return 0;
    }
  else if (value)
    {
      delete[] _rl_vi_ins_mode_str;
      _rl_vi_ins_mode_str = new char[1];
      _rl_vi_ins_mode_str[_rl_vi_ins_modestr_len = 0] = '\0';
      return 0;
    }
  else if (value == nullptr)
    {
      delete[] _rl_vi_ins_mode_str;
      _rl_vi_ins_mode_str = nullptr;	/* prompt_modestr does the right thing */
      _rl_vi_ins_modestr_len = 0;
      return 0;
    }
  return 1;
}

int
Readline::sv_vicmd_modestr (const char *value)
{
  if (value && *value)
    {
      delete[] _rl_vi_cmd_mode_str;
      _rl_vi_cmd_mode_str = new char[2 * std::strlen (value) + 1];
      rl_translate_keyseq (value, _rl_vi_cmd_mode_str, &_rl_vi_cmd_modestr_len);
      _rl_vi_cmd_mode_str[_rl_vi_cmd_modestr_len] = '\0';
      return 0;
    }
  else if (value)
    {
      delete[] _rl_vi_cmd_mode_str;
      _rl_vi_cmd_mode_str = new char[1];
      _rl_vi_cmd_mode_str[_rl_vi_cmd_modestr_len = 0] = '\0';
      return 0;
    }
  else if (value == nullptr)
    {
      delete[] _rl_vi_cmd_mode_str;
      _rl_vi_cmd_mode_str = nullptr;	/* prompt_modestr does the right thing */
      _rl_vi_cmd_modestr_len = 0;
      return 0;
    }
  return 1;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"

/* Return the character which matches NAME.
   For example, `Space' returns ' '. */

struct assoc_list {
  const char *name;
  int value;
};

#pragma clang diagnostic pop

// Keep this list sorted in case-insensitive order for binary search.
static const assoc_list name_key_alist[] = {
  { "DEL",	0x7f },
  { "ESC",	'\033' },
  { "Escape",	'\033' },
  { "LFD",	'\n' },
  { "Newline",	'\n' },
  { "RET",	'\r' },
  { "Return",	'\r' },
  { "Rubout",	0x7f },
  { "Space",	' ' },
  { "SPC",	' ' },
  { "Tab",	0x09 },
};

static inline int
glean_key_from_name (const char *name)
{
  size_t low = 0;
  size_t high = (sizeof (name_key_alist) / sizeof (name_key_alist[0])) - 1;
  size_t mid = high >> 1;
  int dir;

  while ((dir = _rl_stricmp (name, name_key_alist[mid].name)))
    {
      if (dir > 0)
	if (mid == high)
	  break;
	else
	  low = mid + 1;
      else
	if (mid == 0)
	  break;
	else
	  high = mid - 1;

      mid = (low + high) >> 1;
    }

  if (dir == 0)
    return name_key_alist[mid].value;

  return static_cast<unsigned char> (*name);	/* XXX was return (*name) */
}

/* Auxiliary functions to manage keymaps. */
struct name_and_keymap {
  const char *name;
  Readline::Keymap map;
};

void
Readline::init_keymap_names ()
{
  std::vector<nmk> &nml = keymap_names_;

  // Push these in a specific order for reverse lookup name returns.
  nml.push_back (nmk ("emacs", emacs_standard_keymap ()));
  nml.push_back (nmk ("emacs-standard", emacs_standard_keymap ()));
  nml.push_back (nmk ("emacs-meta", emacs_meta_keymap ()));
  nml.push_back (nmk ("emacs-ctlx", emacs_ctlx_keymap ()));
#if defined (VI_MODE)
  nml.push_back (nmk ("vi", vi_movement_keymap ()));
  nml.push_back (nmk ("vi-move", vi_movement_keymap ()));
  nml.push_back (nmk ("vi-command", vi_movement_keymap ()));
  nml.push_back (nmk ("vi-insert", vi_insertion_keymap ()));
#endif /* VI_MODE */
}

#if defined (VI_MODE)
#define NUM_BUILTIN_KEYMAPS	8
#else
#define NUM_BUILTIN_KEYMAPS	4
#endif

int
Readline::rl_set_keymap_name (const char *name, Keymap map)
{
  int ni, mi;

  /* First check whether or not we're trying to rename a builtin keymap */
  mi = _rl_get_keymap_by_map (map);
  if (mi >= 0 && mi < NUM_BUILTIN_KEYMAPS)
    return -1;

  /* Then reject attempts to set one of the builtin names to a new map */
  ni = _rl_get_keymap_by_name (name);
  if (ni >= 0 && ni < NUM_BUILTIN_KEYMAPS)
    return -1;

  /* Renaming a keymap we already added */
  if (mi >= 0)	/* XXX - could be >= NUM_BUILTIN_KEYMAPS */
    {
      size_t smi = static_cast<size_t> (mi);
      delete[] const_cast<char*> (keymap_names_[smi].name);
      keymap_names_[smi].name = savestring (name);
      return mi;
    }

  /* Associating new keymap with existing name */
  if (ni >= 0)
    {
      size_t sni = static_cast<size_t> (ni);
      delete[] keymap_names_[sni].map;	// delete previous user keymap
      keymap_names_[sni].map = map;
      return ni;
    }

  keymap_names_.push_back (nmk (savestring (name), map));
  return static_cast<int> (keymap_names_.size () - 1);
}

/* **************************************************************** */
/*								    */
/*		  Key Binding and Function Information		    */
/*								    */
/* **************************************************************** */

/* Each of the following functions produces information about the
   state of keybindings and functions known to Readline.  The info
   is always printed to rl_outstream, and in such a way that it can
   be read back in (i.e., passed to rl_parse_and_bind ()). */

/* Print the names of functions known to Readline. */
void
Readline::rl_list_funmap_names ()
{
  const char **funmap_names = rl_funmap_names ();

  if (!funmap_names)
    return;

  for (int i = 0; funmap_names[i]; i++)
    std::fprintf (rl_outstream, "%s\n", funmap_names[i]);

  delete[] funmap_names;
}

static char *
_rl_get_keyname (int key)
{
  int i, c;

  char *keyname = new char[8];

  c = key;
  /* Since this is going to be used to write out keysequence-function
     pairs for possible inclusion in an inputrc file, we don't want to
     do any special meta processing on KEY. */

#if 1
  /* XXX - Experimental */
  /* We might want to do this, but the old version of the code did not. */

  /* If this is an escape character, we don't want to do any more processing.
     Just add the special ESC key sequence and return. */
  if (c == ESC)
    {
      keyname[0] = '\\';
      keyname[1] = 'e';
      keyname[2] = '\0';
      return keyname;
    }
#endif

  /* RUBOUT is translated directly into \C-? */
  if (key == RUBOUT)
    {
      keyname[0] = '\\';
      keyname[1] = 'C';
      keyname[2] = '-';
      keyname[3] = '?';
      keyname[4] = '\0';
      return keyname;
    }

  i = 0;
  /* Now add special prefixes needed for control characters.  This can
     potentially change C. */
  if (CTRL_CHAR (c))
    {
      keyname[i++] = '\\';
      keyname[i++] = 'C';
      keyname[i++] = '-';
      c = _rl_to_lower (UNCTRL (c));
    }

  /* XXX experimental code.  Turn the characters that are not ASCII or
     ISO Latin 1 (128 - 159) into octal escape sequences (\200 - \237).
     This changes C. */
  if (c >= 128 && c <= 159)
    {
      keyname[i++] = '\\';
      keyname[i++] = '2';
      c -= 128;
      keyname[i++] = static_cast<char> (c / 8) + '0';
      c = (c % 8) + '0';
    }

  /* Now, if the character needs to be quoted with a backslash, do that. */
  if (c == '\\' || c == '"')
    keyname[i++] = '\\';

  /* Now add the key, terminate the string, and return it. */
  keyname[i++] = static_cast<char> (c);
  keyname[i] = '\0';

  return keyname;
}

/* Return a NULL terminated array of strings which represent the key
   sequences that are used to invoke FUNCTION in MAP. */
char **
Readline::rl_invoking_keyseqs_in_map (rl_command_func_t function, Keymap map)
{
  std::vector<char*> result;

  for (int key = 0; key < KEYMAP_SIZE; key++)
    {
      switch (map[key].type)
	{
	case ISMACR:
	  /* Macros match, if, and only if, the pointers are identical.
	     Thus, they are treated exactly like functions in here. */
	case ISFUNC:
	  /* If the function in the keymap is the one we are looking for,
	     then add the current KEY to the list of invoking keys. */
	  if (map[key].value.function == function)
	    {
	      char *keyname = _rl_get_keyname (key);
	      result.push_back (keyname);
	    }
	  break;

	case ISKMAP:
	  {
	    char **seqs;

	    /* Find the list of keyseqs in this map which have FUNCTION as
	       their target.  Add the key sequences found to RESULT. */
	    if (map[key].value.map)
	      seqs =
	        rl_invoking_keyseqs_in_map (function, map[key].value.map);
	    else
	      break;

	    if (seqs == nullptr)
	      break;

	    for (int i = 0; seqs[i]; i++)
	      {
		char *keyname = new char[6 + std::strlen (seqs[i])];

		if (key == ESC)
		  {
		    /* If ESC is the meta prefix and we're converting chars
		       with the eighth bit set to ESC-prefixed sequences, then
		       we can use \M-.  Otherwise we need to use the sequence
		       for ESC. */
		    if (_rl_convert_meta_chars_to_ascii && map[ESC].type == ISKMAP)
		      std::sprintf (keyname, "\\M-");
		    else
		      std::sprintf (keyname, "\\e");
		  }
		else
		  {
		    int c = key, l = 0;
		    if (CTRL_CHAR (c) || c == RUBOUT)
		      {
			keyname[l++] = '\\';
			keyname[l++] = 'C';
			keyname[l++] = '-';
			c = (c == RUBOUT) ? '?' : _rl_to_lower (UNCTRL (c));
		      }

		    if (c == '\\' || c == '"')
		      keyname[l++] = '\\';

		    keyname[l++] = static_cast<char> (c);
		    keyname[l++] = '\0';
		  }

		std::strcat (keyname, seqs[i]);
		delete[] seqs[i];

		result.push_back (keyname);
	      }

	    delete[] seqs;
	  }
	  break;
	}
    }

  if (result.empty ())
    return nullptr;

  size_t size = result.size ();
  char **result_array = new char*[size + 1];
  std::memcpy(result_array, &result[0], size * sizeof (char *));
  result_array[size] = nullptr;

  return result_array;
}

/* Print all of the functions and their bindings to rl_outstream.  If
   PRINT_READABLY is non-zero, then print the output in such a way
   that it can be read back in. */
void
Readline::rl_function_dumper (bool print_readably)
{
  const char **names;
  const char *name;

  names = rl_funmap_names ();

  std::fprintf (rl_outstream, "\n");

  for (int i = 0; (name = names[i]); i++)
    {
      rl_command_func_t function = rl_named_function (name);
      char **invokers = rl_invoking_keyseqs_in_map (function, _rl_keymap);

      if (print_readably)
	{
	  if (!invokers)
	    std::fprintf (rl_outstream, "# %s (not bound)\n", name);
	  else
	    {
	      for (int j = 0; invokers[j]; j++)
		{
		  std::fprintf (rl_outstream, "\"%s\": %s\n",
				invokers[j], name);
		  delete[] invokers[j];
		}

	      delete[] invokers;
	    }
	}
      else
	{
	  if (!invokers)
	    std::fprintf (rl_outstream, "%s is not bound to any keys\n",
		     name);
	  else
	    {
	      std::fprintf (rl_outstream, "%s can be found on ", name);

	      int j;
	      for (j = 0; invokers[j] && j < 5; j++)
		{
		  std::fprintf (rl_outstream, "\"%s\"%s", invokers[j],
				invokers[j + 1] ? ", " : ".\n");
		}

	      if (j == 5 && invokers[j])
		std::fprintf (rl_outstream, "...\n");

	      for (j = 0; invokers[j]; j++)
		delete[] invokers[j];

	      delete[] invokers;
	    }
	}
    }

  delete[] names;
}

/* Print all of the current functions and their bindings to
   rl_outstream.  If an explicit argument is given, then print
   the output in such a way that it can be read back in. */
int
Readline::rl_dump_functions (int, int)
{
  if (rl_dispatching)
    std::fprintf (rl_outstream, "\r\n");

  rl_function_dumper (rl_explicit_arg);
  rl_on_new_line ();
  return 0;
}

void
Readline::_rl_macro_dumper_internal (bool print_readably, Keymap map, const char *prefix)
{
  char *keyname, *out;

  for (int key = 0; key < KEYMAP_SIZE; key++)
    {
      switch (map[key].type)
	{
	case ISMACR:
	  keyname = _rl_get_keyname (key);
	  out = _rl_untranslate_macro_value (map[key].value.macro, false);

	  if (print_readably)
	    std::fprintf (rl_outstream, "\"%s%s\": \"%s\"\n", prefix ? prefix : "",
						         keyname,
						         out ? out : "");
	  else
	    std::fprintf (rl_outstream, "%s%s outputs %s\n", prefix ? prefix : "",
							keyname,
							out ? out : "");
	  delete[] keyname;
	  delete[] out;
	  break;

	case ISFUNC:
	  break;

	case ISKMAP:
	  size_t prefix_len = prefix ? std::strlen (prefix) : 0;
	  if (key == ESC)
	    {
	      keyname = new char[3 + prefix_len];
	      if (prefix)
		std::strcpy (keyname, prefix);
	      keyname[prefix_len] = '\\';
	      keyname[prefix_len + 1] = 'e';
	      keyname[prefix_len + 2] = '\0';
	    }
	  else
	    {
	      keyname = _rl_get_keyname (key);
	      if (prefix)
		{
		  out = new char[std::strlen (keyname) + prefix_len + 1];
		  std::strcpy (out, prefix);
		  std::strcpy (out + prefix_len, keyname);
		  delete[] keyname;
		  keyname = out;
		}
	    }

	  _rl_macro_dumper_internal (print_readably, map[key].value.map, keyname);
	  delete[] keyname;
	  break;
	}
    }
}

int
Readline::rl_dump_macros (int, int)
{
  if (rl_dispatching)
    std::fprintf (rl_outstream, "\r\n");

  rl_macro_dumper (rl_explicit_arg);
  rl_on_new_line ();
  return 0;
}

const char *
Readline::_rl_get_string_variable_value (const char *name)
{
  if (_rl_stricmp (name, "bell-style") == 0)
    {
      switch (_rl_bell_preference)
	{
	  case NO_BELL:
	    return "none";
	  case VISIBLE_BELL:
	    return "visible";
	  case AUDIBLE_BELL:
	    return "audible";
	}
    }
  else if (_rl_stricmp (name, "comment-begin") == 0)
    return _rl_comment_begin ? _rl_comment_begin : RL_COMMENT_BEGIN_DEFAULT;
  else if (_rl_stricmp (name, "completion-display-width") == 0)
    {
      std::snprintf (_rl_numbuf, sizeof (_rl_numbuf), "%d", _rl_completion_columns);
      return _rl_numbuf;
    }
  else if (_rl_stricmp (name, "completion-prefix-display-length") == 0)
    {
      std::snprintf (_rl_numbuf, sizeof (_rl_numbuf), "%d", _rl_completion_prefix_display_length);
      return _rl_numbuf;
    }
  else if (_rl_stricmp (name, "completion-query-items") == 0)
    {
      std::snprintf (_rl_numbuf, sizeof (_rl_numbuf), "%d", rl_completion_query_items);
      return _rl_numbuf;
    }
  else if (_rl_stricmp (name, "editing-mode") == 0)
    return rl_get_keymap_name_from_edit_mode ();
  else if (_rl_stricmp (name, "history-size") == 0)
    {
      std::snprintf (_rl_numbuf, sizeof (_rl_numbuf), "%d",
		     history_is_stifled() ? static_cast<int> (history_max_entries) : 0);
      return _rl_numbuf;
    }
  else if (_rl_stricmp (name, "isearch-terminators") == 0)
    {
      if (_rl_isearch_terminators == nullptr)
	return nullptr;
      char *ret = _rl_untranslate_macro_value (_rl_isearch_terminators, 0);
      if (ret)
	{
	  std::strncpy (_rl_numbuf, ret, sizeof (_rl_numbuf) - 1);
	  delete[] ret;
	  _rl_numbuf[sizeof(_rl_numbuf) - 1] = '\0';
	}
      else
	_rl_numbuf[0] = '\0';
      return _rl_numbuf;
    }
  else if (_rl_stricmp (name, "keymap") == 0)
    {
      const char *ret = rl_get_keymap_name (_rl_keymap);
      if (ret == nullptr)
	ret = rl_get_keymap_name_from_edit_mode ();
      return ret ? ret : "none";
    }
  else if (_rl_stricmp (name, "keyseq-timeout") == 0)
    {
      std::snprintf (_rl_numbuf, sizeof (_rl_numbuf), "%d", _rl_keyseq_timeout);
      return _rl_numbuf;
    }
  else if (_rl_stricmp (name, "emacs-mode-string") == 0)
    return _rl_emacs_mode_str ? _rl_emacs_mode_str : RL_EMACS_MODESTR_DEFAULT;
  else if (_rl_stricmp (name, "vi-cmd-mode-string") == 0)
    return _rl_vi_cmd_mode_str ? _rl_vi_cmd_mode_str : RL_VI_CMD_MODESTR_DEFAULT;
  else if (_rl_stricmp (name, "vi-ins-mode-string") == 0)
    return _rl_vi_ins_mode_str ? _rl_vi_ins_mode_str : RL_VI_INS_MODESTR_DEFAULT;
  else
    return nullptr;
}

void
Readline::rl_variable_dumper (bool print_readably)
{
  for (std::vector<bvt>::iterator it = boolean_varlist_.begin (); it != boolean_varlist_.end (); ++it)
    {
      if (print_readably)
        std::fprintf (rl_outstream, "set %s %s\n", it->name, *(it->value) ? "on" : "off");
      else
        std::fprintf (rl_outstream, "%s is set to `%s'\n", it->name,
			*(it->value) ? "on" : "off");
    }

  for (int i = 0; string_varlist[i].name; i++)
    {
      const char *v = _rl_get_string_variable_value (string_varlist[i].name);
      if (v == nullptr)		// _rl_isearch_terminators can be nullptr
	continue;

      if (print_readably)
        std::fprintf (rl_outstream, "set %s %s\n", string_varlist[i].name, v);
      else
        std::fprintf (rl_outstream, "%s is set to `%s'\n", string_varlist[i].name, v);
    }
}

/* Print all of the current variables and their values to
   rl_outstream.  If an explicit argument is given, then print
   the output in such a way that it can be read back in. */
int
Readline::rl_dump_variables (int, int)
{
  if (rl_dispatching)
    std::fprintf (rl_outstream, "\r\n");

  rl_variable_dumper (rl_explicit_arg);
  rl_on_new_line ();
  return 0;
}

}  // namespace readline
