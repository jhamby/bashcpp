/* histexpand.c -- history expansion. */

/* Copyright (C) 1989-2018 Free Software Foundation, Inc.

   This file contains the GNU History Library (History), a set of
   routines for managing the text of previously typed lines.

   History is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   History is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with History.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "history.hh"

namespace readline
{

#define HISTORY_QUOTE_CHARACTERS "\"'`"

#define slashify_in_quotes "\\`\"$"

#define fielddelim(c) (whitespace (c) || (c) == '\n')

/* Possible history errors passed to hist_error. */
enum hist_error_type
{
  EVENT_NOT_FOUND,
  BAD_WORD_SPEC,
  SUBST_FAILED,
  BAD_MODIFIER,
  NO_PREV_SUBST
};

static char *history_substring (const char *, unsigned int, unsigned int);
static void freewords (char **, unsigned int);
static char *quote_breaks (const char *);
static char *hist_error (const char *, unsigned int, unsigned int,
                         hist_error_type);

/* **************************************************************** */
/*								    */
/*			History Expansion			    */
/*								    */
/* **************************************************************** */

/* Hairy history expansion on text, not tokens.  This is of general
   use, and thus belongs in this library. */

/* Return the event specified at TEXT + OFFSET modifying OFFSET to
   point to after the event specifier.  Just a pointer to the history
   line is returned; nullptr is returned in the event of a bad specifier.
   You pass STRING with *INDEX equal to the history_expansion_char that
   begins this specification.
   DELIMITING_QUOTE is a character that is allowed to end the string
   specification for what to search for in addition to the normal
   characters `:', ` ', `\t', `\n', and sometimes `?'.
   So you might call this function like:
   line = get_history_event ("!echo:p", &index, 0);  */
const char *
History::get_history_event (const char *string, unsigned int *caller_index,
                            char delimiting_quote)
{
  /* The event can be specified in a number of ways.

     !!   the previous command
     !n   command line N
     !-n  current command-line minus N
     !str the most recent command starting with STR
     !?str[?]
          the most recent command containing STR

     All values N are determined via HISTORY_BASE. */

  unsigned int i = *caller_index;

  if (string[i] != history_expansion_char)
    return nullptr;

  /* Move on to the specification. */
  i++;

  int sign = 1;
  bool substring_okay = false;
  HIST_ENTRY *entry;

#define RETURN_ENTRY(e, w)                                                    \
  return ((e = the_history[w]) ? e->line.c_str () : nullptr)

  /* Handle !! case. */
  if (string[i] == history_expansion_char)
    {
      i++;
      unsigned int which = history_base + (history_length () - 1);
      *caller_index = i;
      RETURN_ENTRY (entry, which);
    }

  /* Hack case of numeric line specification. */
  if (string[i] == '-' && _rl_digit_p (string[i + 1]))
    {
      sign = -1;
      i++;
    }

  if (_rl_digit_p (string[i]))
    {
      /* Get the extent of the digits and compute the value. */
      unsigned int which;
      for (which = 0; _rl_digit_p (string[i]); i++)
        which = (which * 10)
                + static_cast<unsigned int> (_rl_digit_value (string[i]));

      *caller_index = i;

      if (sign < 0)
        which = (history_length () + history_base) - which;

      RETURN_ENTRY (entry, which);
    }

  /* This must be something to search for.  If the spec begins with
     a '?', then the string may be anywhere on the line.  Otherwise,
     the string must be found at the start of a line. */
  if (string[i] == '?')
    {
      substring_okay = true;
      i++;
    }

  /* Only a closing `?' or a newline delimit a substring search string. */
  unsigned int local_index;
  char c;
  for (local_index = i; (c = string[i]); i++)
    {
#if defined(HANDLE_MULTIBYTE)
      if (MB_CUR_MAX > 1 && !rl_byte_oriented)
        {
          int v;
          mbstate_t ps;

          std::memset (&ps, 0, sizeof (mbstate_t));
          /* These produce warnings because we're passing a const string to a
             function that takes a non-const string. */
          _rl_adjust_point (string, i, &ps);
          if ((v = _rl_get_char_len (string + i, &ps)) > 1)
            {
              i += static_cast<unsigned int> (v - 1);
              continue;
            }
        }

#endif /* HANDLE_MULTIBYTE */
      if ((!substring_okay
           && (whitespace (c) || c == ':'
               || (i > local_index && history_event_delimiter_chars
                   && c == '-')
               || (c != '-' && history_event_delimiter_chars
                   && member (c, history_event_delimiter_chars))
               || (history_search_delimiter_chars
                   && member (c, history_search_delimiter_chars))
               || string[i] == delimiting_quote))
          || string[i] == '\n' || (substring_okay && string[i] == '?'))
        break;
    }

  unsigned int which = i - local_index;
  char *temp = new char[1 + which];
  std::strncpy (temp, string + local_index, which);
  temp[which] = '\0';

  if (substring_okay && string[i] == '?')
    i++;

  *caller_index = i;

#define FAIL_SEARCH()                                                         \
  do                                                                          \
    {                                                                         \
      history_offset = history_length ();                                     \
      delete[] temp;                                                          \
      return nullptr;                                                         \
    }                                                                         \
  while (0)

  /* If there is no search string, try to use the previous search string,
     if one exists.  If not, fail immediately. */
  if (*temp == '\0' && substring_okay)
    {
      if (history_search_string)
        {
          delete[] temp;
          temp = savestring (history_search_string);
        }
      else
        FAIL_SEARCH ();
    }

  rl_linebuf_func_t search_func = substring_okay
                                      ? &History::history_search
                                      : &History::history_search_prefix;
  while (1)
    {
      local_index = ((*this).*search_func) (temp, -1);

      if (static_cast<int> (local_index) < 0)
        FAIL_SEARCH ();

      if (local_index == 0 || substring_okay)
        {
          entry = current_history ();
          if (entry == nullptr)
            FAIL_SEARCH ();
          history_offset = history_length ();

          /* If this was a substring search, then remember the
             string that we matched for word substitution. */
          if (substring_okay)
            {
              delete[] history_search_string;
              history_search_string = temp;

              delete[] history_search_match;
              history_search_match = history_find_word (
                  entry->line.c_str (),
                  static_cast<unsigned int> (local_index));
            }
          else
            delete[] temp;

          return entry->line.c_str ();
        }

      if (history_offset)
        history_offset--;
      else
        FAIL_SEARCH ();
    }
#undef FAIL_SEARCH
#undef RETURN_ENTRY
}

/* Function for extracting single-quoted strings.  Used for inhibiting
   history expansion within single quotes. */

/* Extract the contents of STRING as if it is enclosed in single quotes.
   SINDEX, when passed in, is the offset of the character immediately
   following the opening single quote; on exit, SINDEX is left pointing
   to the closing single quote.  FLAGS currently used to allow backslash
   to escape a single quote (e.g., for bash $'...'). */
static inline void
hist_string_extract_single_quoted (const char *string, unsigned int *sindex,
                                   int flags)
{
  unsigned int i;
  for (i = *sindex; string[i] && string[i] != '\''; i++)
    {
      if ((flags & 1) && string[i] == '\\' && string[i + 1])
        i++;
    }

  *sindex = i;
}

static char *
quote_breaks (const char *s)
{
  const char *p;
  unsigned int len = 3;

  for (p = s; p && *p; p++, len++)
    {
      if (*p == '\'')
        len += 3;
      else if (whitespace (*p) || *p == '\n')
        len += 2;
    }

  char *r, *ret;
  r = ret = new char[len];
  *r++ = '\'';
  for (p = s; p && *p;)
    {
      if (*p == '\'')
        {
          *r++ = '\'';
          *r++ = '\\';
          *r++ = '\'';
          *r++ = '\'';
          p++;
        }
      else if (whitespace (*p) || *p == '\n')
        {
          *r++ = '\'';
          *r++ = *p++;
          *r++ = '\'';
        }
      else
        *r++ = *p++;
    }
  *r++ = '\'';
  *r = '\0';
  return ret;
}

static char *
hist_error (const char *s, unsigned int start, unsigned int current,
            hist_error_type errtype)
{
  char *temp;
  const char *emsg;
  unsigned int ll, elen;

  ll = current - start;

  switch (errtype)
    {
    case EVENT_NOT_FOUND:
      emsg = "event not found";
      elen = 15;
      break;
    case BAD_WORD_SPEC:
      emsg = "bad word specifier";
      elen = 18;
      break;
    case SUBST_FAILED:
      emsg = "substitution failed";
      elen = 19;
      break;
    case BAD_MODIFIER:
      emsg = "unrecognized history modifier";
      elen = 29;
      break;
    case NO_PREV_SUBST:
      emsg = "no previous substitution";
      elen = 24;
      break;
#if 0
    // not needed: switch covers all enumeration values
    default:
      emsg = "unknown expansion error";
      elen = 23;
      break;
#endif
    }

  temp = new char[ll + elen + 3];
  if (s[start])
    std::strncpy (temp, s + start, ll);
  else
    ll = 0;
  temp[ll] = ':';
  temp[ll + 1] = ' ';
  std::strcpy (temp + ll + 2, emsg);
  return temp;
}

/* Get a history substitution string from STR starting at *IPTR
   and return it.  The length is returned in LENPTR.

   A backslash can quote the delimiter.  If the string is the
   empty string, the previous pattern is used.  If there is
   no previous pattern for the lhs, the last history search
   string is used.

   If IS_RHS is 1, we ignore empty strings and set the pattern
   to "" anyway.  subst_lhs is not changed if the lhs is empty;
   subst_rhs is allowed to be set to the empty string. */

char *
History::get_subst_pattern (const char *str, unsigned int *iptr,
                            char delimiter, bool is_rhs, unsigned int *lenptr)
{
  char *s = nullptr;
  unsigned int i = *iptr;

#if defined(HANDLE_MULTIBYTE)
  mbstate_t ps;
  std::memset (&ps, 0, sizeof (mbstate_t));
  _rl_adjust_point (str, i, &ps);
#endif

  unsigned int si;
  for (si = i; str[si] && str[si] != delimiter; si++)
#if defined(HANDLE_MULTIBYTE)
    if (MB_CUR_MAX > 1 && !rl_byte_oriented)
      {
        int v;
        if ((v = _rl_get_char_len (str + si, &ps)) > 1)
          si += static_cast<unsigned int> (v - 1);
        else if (str[si] == '\\' && str[si + 1] == delimiter)
          si++;
      }
    else
#endif /* HANDLE_MULTIBYTE */
      if (str[si] == '\\' && str[si + 1] == delimiter)
        si++;

  if (si > i || is_rhs)
    {
      s = new char[si - i + 1];
      unsigned int j, k;
      for (j = 0, k = i; k < si; j++, k++)
        {
          /* Remove a backslash quoting the search string delimiter. */
          if (str[k] == '\\' && str[k + 1] == delimiter)
            k++;
          s[j] = str[k];
        }
      s[j] = '\0';
      if (lenptr)
        *lenptr = j;
    }

  i = si;
  if (str[i])
    i++;
  *iptr = i;

  return s;
}

void
History::postproc_subst_rhs ()
{
  std::string new_str;

  for (unsigned int i = 0; i < history_subst_rhs_len; i++)
    {
      if (history_subst_rhs[i] == '&')
        {
          new_str.append (history_subst_lhs);
        }
      else
        {
          /* a single backslash protects the `&' from lhs interpolation */
          if (history_subst_rhs[i] == '\\' && history_subst_rhs[i + 1] == '&')
            i++;

          new_str.push_back (history_subst_rhs[i]);
        }
    }

  delete[] history_subst_rhs;
  history_subst_rhs = savestring (new_str);
  history_subst_rhs_len = static_cast<unsigned int> (new_str.size ());
}

/* Expand the bulk of a history specifier starting at STRING[START].
   Returns 0 if everything is OK, -1 if an error occurred, and 1
   if the `p' modifier was supplied and the caller should just print
   the returned string.  Returns the new index into string in
   *END_INDEX_PTR, and the expanded specifier in *RET_STRING. */
/* need current line for !# */
int
History::history_expand_internal (const char *string, unsigned int start,
                                  char qc, unsigned int *end_index_ptr,
                                  char **ret_string, const char *current_line)
{
#if defined(HANDLE_MULTIBYTE)
  mbstate_t ps;

  std::memset (&ps, 0, sizeof (mbstate_t));
#endif

  unsigned int i = start;
  const char *event;

  /* If it is followed by something that starts a word specifier,
     then !! is implied as the event specifier. */

  if (member (string[i + 1], ":$*%^"))
    {
      char fake_s[3];
      unsigned int fake_i = 0;
      i++;
      fake_s[0] = fake_s[1] = history_expansion_char;
      fake_s[2] = '\0';
      event = get_history_event (fake_s, &fake_i, 0);
    }
  else if (string[i + 1] == '#')
    {
      i += 2;
      event = const_cast<char *> (current_line);
    }
  else
    event = get_history_event (string, &i, qc);

  if (event == nullptr)
    {
      *ret_string = hist_error (string, start, i, EVENT_NOT_FOUND);
      return -1;
    }

  /* If a word specifier is found, then do what that requires. */
  unsigned int starting_index = i;
  char *tstr = nullptr, *word_spec = nullptr;

  try
    {
      word_spec = get_history_word_specifier (string, event, &i);
    }
  catch (const word_not_found &)
    {
      /* There is no such thing as a `malformed word specifier'.  However,
         it is possible for a specifier that has no match.  In that case,
         we complain. */
      *ret_string = hist_error (string, starting_index, i, BAD_WORD_SPEC);
      return -1;
    }

  /* If no word specifier, than the thing of interest was the event. */
  char *temp = word_spec ? savestring (word_spec) : savestring (event);
  delete[] word_spec;

  /* Perhaps there are other modifiers involved.  Do what they say. */
  int want_quotes = 0;
  int substitute_globally = 0;
  bool subst_bywords = false;
  bool print_only = false;
  starting_index = i;

  while (string[i] == ':')
    {
      char c = string[i + 1];

      if (c == 'g' || c == 'a')
        {
          substitute_globally = 1;
          i++;
          c = string[i + 1];
        }
      else if (c == 'G')
        {
          subst_bywords = 1;
          i++;
          c = string[i + 1];
        }

      switch (c)
        {
        default:
          *ret_string = hist_error (string, i + 1, i + 2, BAD_MODIFIER);
          delete[] temp;
          return -1;

        case 'q':
          want_quotes = 'q';
          break;

        case 'x':
          want_quotes = 'x';
          break;

          /* :p means make this the last executed line.  So we
             return an error state after adding this line to the
             history. */
        case 'p':
          print_only = 1;
          break;

          /* :t discards all but the last part of the pathname. */
        case 't':
          tstr = std::strrchr (temp, '/');
          if (tstr)
            {
              tstr++;
              char *t = savestring (tstr);
              delete[] temp;
              temp = t;
            }
          break;

          /* :h discards the last part of a pathname. */
        case 'h':
          tstr = std::strrchr (temp, '/');
          if (tstr)
            *tstr = '\0';
          break;

          /* :r discards the suffix. */
        case 'r':
          tstr = std::strrchr (temp, '.');
          if (tstr)
            *tstr = '\0';
          break;

          /* :e discards everything but the suffix. */
        case 'e':
          tstr = std::strrchr (temp, '.');
          if (tstr)
            {
              char *t = savestring (tstr);
              delete[] temp;
              temp = t;
            }
          break;

          /* :s/this/that substitutes `that' for the first
             occurrence of `this'.  :gs/this/that substitutes `that'
             for each occurrence of `this'.  :& repeats the last
             substitution.  :g& repeats the last substitution
             globally. */

        case '&':
        case 's':
          {
            char *new_event;
            char delimiter;
            bool failed;
            unsigned int si, we;

            if (c == 's')
              {
                if (i + 2 < static_cast<unsigned int> (std::strlen (string)))
                  {
#if defined(HANDLE_MULTIBYTE)
                    if (MB_CUR_MAX > 1 && !rl_byte_oriented)
                      {
                        _rl_adjust_point (string, i + 2, &ps);
                        if (_rl_get_char_len (string + i + 2, &ps) > 1)
                          delimiter = 0;
                        else
                          delimiter = string[i + 2];
                      }
                    else
#endif /* HANDLE_MULTIBYTE */
                      delimiter = string[i + 2];
                  }
                else
                  break; /* no search delimiter */

                i += 3;

                char *t = get_subst_pattern (string, &i, delimiter, 0,
                                             &history_subst_lhs_len);
                /* An empty substitution lhs with no previous substitution
                   uses the last search string as the lhs. */
                if (t)
                  {
                    delete[] history_subst_lhs;
                    history_subst_lhs = t;
                  }
                else if (!history_subst_lhs)
                  {
                    if (history_search_string && *history_search_string)
                      {
                        history_subst_lhs = savestring (history_search_string);
                        history_subst_lhs_len = static_cast<unsigned int> (
                            std::strlen (history_subst_lhs));
                      }
                    else
                      {
                        history_subst_lhs = nullptr;
                        history_subst_lhs_len = 0;
                      }
                  }

                delete[] history_subst_rhs;
                history_subst_rhs = get_subst_pattern (
                    string, &i, delimiter, 1, &history_subst_rhs_len);

                /* If `&' appears in the rhs, it's supposed to be replaced
                   with the lhs. */
                if (member ('&', history_subst_rhs))
                  postproc_subst_rhs ();
              }
            else
              i += 2;

            /* If there is no lhs, the substitution can't succeed. */
            if (history_subst_lhs_len == 0)
              {
                *ret_string
                    = hist_error (string, starting_index, i, NO_PREV_SUBST);
                delete[] temp;
                return -1;
              }

            unsigned int l_temp
                = static_cast<unsigned int> (std::strlen (temp));
            /* Ignore impossible cases. */
            if (history_subst_lhs_len > l_temp)
              {
                *ret_string
                    = hist_error (string, starting_index, i, SUBST_FAILED);
                delete[] temp;
                return -1;
              }

            /* Find the first occurrence of THIS in TEMP. */
            /* Substitute SUBST_RHS for SUBST_LHS in TEMP.  There are three
               cases to consider:

                 1.  substitute_globally == subst_bywords == 0
                 2.  substitute_globally == 1 && subst_bywords == 0
                 3.  substitute_globally == 0 && subst_bywords == 1

               In the first case, we substitute for the first occurrence only.
               In the second case, we substitute for every occurrence.
               In the third case, we tokenize into words and substitute the
               first occurrence of each word. */

            si = we = 0;
            for (failed = true; (si + history_subst_lhs_len) <= l_temp; si++)
              {
                /* First skip whitespace and find word boundaries if
                   we're past the end of the word boundary we found
                   the last time. */
                if (subst_bywords && si > we)
                  {
                    for (; temp[si] && fielddelim (temp[si]); si++)
                      ;
                    we = history_tokenize_word (temp, si);
                  }

                if (STREQN (temp + si, history_subst_lhs,
                            history_subst_lhs_len))
                  {
                    unsigned int len = history_subst_rhs_len
                                       - history_subst_lhs_len + l_temp;
                    new_event = new char[1 + len];
                    std::strncpy (new_event, temp, si);
                    std::strncpy (new_event + si, history_subst_rhs,
                                  history_subst_rhs_len);
                    std::strncpy (new_event + si + history_subst_rhs_len,
                                  temp + si + history_subst_lhs_len,
                                  l_temp - (si + history_subst_lhs_len));
                    new_event[len] = '\0';
                    delete[] temp;
                    temp = new_event;

                    failed = false;

                    if (substitute_globally)
                      {
                        /* Reported to fix a bug that causes it to skip every
                           other match when matching a single character.  Was
                           si += subst_rhs_len previously. */
                        si += history_subst_rhs_len - 1;
                        l_temp
                            = static_cast<unsigned int> (std::strlen (temp));
                        substitute_globally++;
                        continue;
                      }
                    else if (subst_bywords)
                      {
                        si = we;
                        l_temp
                            = static_cast<unsigned int> (std::strlen (temp));
                        continue;
                      }
                    else
                      break;
                  }
              }

            if (substitute_globally > 1)
              {
                substitute_globally = 0;
                continue; /* don't want to increment i */
              }

            if (!failed)
              continue; /* don't want to increment i */

            *ret_string = hist_error (string, starting_index, i, SUBST_FAILED);
            delete[] temp;
            return -1;
          }
        }
      i += 2;
    }
  /* Done with modifiers. */
  /* Believe it or not, we have to back the pointer up by one. */
  --i;

  if (want_quotes)
    {
      char *x;

      if (want_quotes == 'q')
        x = sh_single_quote (temp);
      else if (want_quotes == 'x')
        x = quote_breaks (temp);
      else
        x = savestring (temp);

      delete[] temp;
      temp = x;
    }

  *end_index_ptr = i;
  *ret_string = temp;
  return print_only;
}

int
History::history_expand (const char *hstring, char **output)
{
#if defined(HANDLE_MULTIBYTE)
  char mb[MB_LEN_MAX];
  mbstate_t ps;
#endif

  if (output == nullptr)
    return 0;

  /* Setting the history expansion character to 0 inhibits all
     history expansion. */
  if (history_expansion_char == 0)
    {
      *output = savestring (hstring);
      return 0;
    }

  /* Prepare the buffer for printing error messages. */
  std::string result;

  unsigned int l = static_cast<unsigned int> (std::strlen (hstring));
  char *string;

  /* Grovel the string.  Only backslash and single quotes can quote the
     history escape character.  We also handle arg specifiers. */

  /* Before we grovel forever, see if the history_expansion_char appears
     anywhere within the text. */

  /* The quick substitution character is a history expansion all right.  That
     is to say, "^this^that^" is equivalent to "!!:s^this^that^", and in fact,
     that is the substitution that we do. */
  if (hstring[0] == history_subst_char)
    {
      string = new char[l + 5];

      string[0] = string[1] = history_expansion_char;
      string[2] = ':';
      string[3] = 's';
      std::strcpy (string + 4, hstring);
      l += 4;
    }
  else
    {
#if defined(HANDLE_MULTIBYTE)
      std::memset (&ps, 0, sizeof (mbstate_t));
#endif

      string = const_cast<char *> (hstring); /* will copy before returning */
      /* If not quick substitution, still maybe have to do expansion. */

      /* `!' followed by one of the characters in history_no_expand_chars
         is NOT an expansion. */
      bool dquote = history_quoting_state == '"';
      bool squote = history_quoting_state == '\'';

      /* If the calling application tells us we are already reading a
         single-quoted string, consume the rest of the string right now
         and then go on. */
      unsigned int i = 0;
      if (squote && history_quotes_inhibit_expansion)
        {
          hist_string_extract_single_quoted (string, &i, 0);
          squote = false;
          if (string[i])
            i++;
        }

      for (; string[i]; i++)
        {
#if defined(HANDLE_MULTIBYTE)
          if (MB_CUR_MAX > 1 && !rl_byte_oriented)
            {
              int v;
              v = _rl_get_char_len (string + i, &ps);
              if (v > 1)
                {
                  i += static_cast<unsigned int> (v - 1);
                  continue;
                }
            }
#endif /* HANDLE_MULTIBYTE */

          char cc = string[i + 1];
          /* The history_comment_char, if set, appearing at the beginning
             of a word signifies that the rest of the line should not have
             history expansion performed on it.
             Skip the rest of the line and break out of the loop. */
          if (history_comment_char && string[i] == history_comment_char
              && dquote == 0
              && (i == 0 || member (string[i - 1], history_word_delimiters)))
            {
              while (string[i])
                i++;
              break;
            }
          else if (string[i] == history_expansion_char)
            {
              if (cc == 0 || member (cc, history_no_expand_chars))
                continue;
              /* DQUOTE won't be set unless history_quotes_inhibit_expansion
                 is set.  The idea here is to treat double-quoted strings the
                 same as the word outside double quotes; in effect making the
                 double quote part of history_no_expand_chars when DQUOTE is
                 set. */
              else if (dquote && cc == '"')
                continue;
              /* If the calling application has set
                 history_inhibit_expansion_function to a function that checks
                 for special cases that should not be history expanded,
                 call the function and skip the expansion if it returns a
                 non-zero value. */
              else if (history_inhibit_expansion_function
                       && ((*this).*(history_inhibit_expansion_function)) (
                           string, static_cast<int> (i)))
                continue;
              else
                break;
            }
          /* Shell-like quoting: allow backslashes to quote double quotes
             inside a double-quoted string. */
          else if (dquote && string[i] == '\\' && cc == '"')
            i++;
          /* More shell-like quoting:  if we're paying attention to single
             quotes and letting them quote the history expansion character,
             then we need to pay attention to double quotes, because single
             quotes are not special inside double-quoted strings. */
          else if (history_quotes_inhibit_expansion && string[i] == '"')
            {
              dquote = 1 - dquote;
            }
          else if (dquote == 0 && history_quotes_inhibit_expansion
                   && string[i] == '\'')
            {
              /* If this is bash, single quotes inhibit history expansion. */
              int flag = (i > 0 && string[i - 1] == '$');
              i++;
              hist_string_extract_single_quoted (string, &i, flag);
            }
          else if (history_quotes_inhibit_expansion && string[i] == '\\')
            {
              /* If this is bash, allow backslashes to quote single
                 quotes and the history expansion character. */
              if (cc == '\'' || cc == history_expansion_char)
                i++;
            }
        }

      if (string[i] != history_expansion_char)
        {
          *output = savestring (string);
          return 0;
        }
    }

  /* Extract and perform the substitution. */
  bool dquote = history_quoting_state == '"';
  bool squote = history_quoting_state == '\'';

  /* If the calling application tells us we are already reading a
     single-quoted string, consume the rest of the string right now
     and then go on. */
  if (squote && history_quotes_inhibit_expansion)
    {
      unsigned int i;

      hist_string_extract_single_quoted (string, &i, 0);
      squote = 0;
      for (unsigned int c = 0; c < i; c++)
        result += string[c];
      if (string[i])
        {
          result += string[i];
          i++;
        }
    }

  bool passc = false, modified = false, only_printing = false;
  for (unsigned int i = 0; i < l; i++)
    {
      char qc, tchar = string[i];

      if (passc)
        {
          passc = false;
          result += tchar;
          continue;
        }

#if defined(HANDLE_MULTIBYTE)
      if (MB_CUR_MAX > 1 && !rl_byte_oriented)
        {
          int k, c;

          c = tchar;
          std::memset (mb, 0, sizeof (mb));
          for (k = 0; k < MB_LEN_MAX; k++)
            {
              mb[k] = static_cast<char> (c);
              std::memset (&ps, 0, sizeof (mbstate_t));
              if (_rl_get_char_len (mb, &ps) == -2)
                c = string[++i];
              else
                break;
            }
          if (std::strlen (mb) > 1)
            {
              result += mb;
              continue;
            }
        }
#endif /* HANDLE_MULTIBYTE */

      if (tchar == history_expansion_char)
        tchar = -3;
      else if (tchar == history_comment_char)
        tchar = -2;

      switch (tchar)
        {
        default:
          result += string[i];
          break;

        case '\\':
          passc = true;
          result += tchar;
          break;

        case '"':
          dquote = !dquote;
          result += tchar;
          break;

        case '\'':
          {
            /* If history_quotes_inhibit_expansion is set, single quotes
               inhibit history expansion, otherwise they are treated like
               double quotes. */
            if (squote)
              {
                squote = false;
                result += tchar;
              }
            else if (!dquote && history_quotes_inhibit_expansion)
              {
                unsigned int quote;

                int flag = (i > 0 && string[i - 1] == '$');
                quote = i++;
                hist_string_extract_single_quoted (string, &i, flag);
                result.append (string + quote, i - quote + 2);
              }
            else if (!dquote && !squote && !history_quotes_inhibit_expansion)
              {
                squote = 1;
                result += string[i];
              }
            else
              result += string[i];
            break;
          }

        case -2: /* history_comment_char */
          if ((!dquote || !history_quotes_inhibit_expansion)
              && (i == 0 || member (string[i - 1], history_word_delimiters)))
            {
              result += (string + i);
              i = l;
            }
          else
            result += string[i];
          break;

        case -3: /* history_expansion_char */
          char cc = string[i + 1];

          /* If the history_expansion_char is followed by one of the
             characters in history_no_expand_chars, then it is not a
             candidate for expansion of any kind. */
          if (cc == 0 || member (cc, history_no_expand_chars)
              || (dquote && cc == '"')
              || (history_inhibit_expansion_function
                  && ((*this).*(history_inhibit_expansion_function)) (
                      string, static_cast<int> (i))))
            {
              result += string[i];
              break;
            }

#if defined(NO_BANG_HASH_MODIFIERS)
          /* There is something that is listed as a `word specifier' in csh
             documentation which means `the expanded text to this point'.
             That is not a word specifier, it is an event specifier.  If we
             don't want to allow modifiers with `!#', just stick the current
             output line in again. */
          if (cc == '#')
            {
              if (result)
                {
                  result += result;
                }
              i++;
              break;
            }
#endif
          qc = squote ? '\'' : (dquote ? '"' : 0);
          unsigned int eindex;
          char *temp;
          int r = history_expand_internal (string, i, qc, &eindex, &temp,
                                           result.c_str ());
          if (r < 0)
            {
              *output = temp;
              if (string != hstring)
                delete[] string;
              return -1;
            }
          else
            {
              if (temp)
                {
                  modified = true;
                  if (*temp)
                    result += temp;
                  delete[] temp;
                }
              only_printing += r == 1;
              i = eindex;
            }
          break;
        }
    }

  *output = savestring (result);
  if (string != hstring)
    delete[] string;

  if (only_printing)
    {
      return 2;
    }

  return modified;
}

/* Return a consed string which is the word specified in SPEC, and found
   in FROM.  nullptr is returned if there is no spec.  The address of
   ERROR_POINTER is returned if the word specified cannot be found.
   CALLER_INDEX is the offset in SPEC to start looking; it is updated
   to point to just after the last character parsed. */
char *
History::get_history_word_specifier (const char *spec, const char *from,
                                     unsigned int *caller_index)
{
  unsigned int i = *caller_index;
  int first, last;
  int expecting_word_spec = 0;
  char *result;

  /* The range of words to return doesn't exist yet. */
  first = last = 0;
  result = nullptr;

  /* If we found a colon, then this *must* be a word specification.  If
     it isn't, then it is an error. */
  if (spec[i] == ':')
    {
      i++;
      expecting_word_spec++;
    }

  /* Handle special cases first. */

  /* `%' is the word last searched for. */
  if (spec[i] == '%')
    {
      *caller_index = i + 1;
      return history_search_match ? savestring (history_search_match)
                                  : savestring ("");
    }

  /* `*' matches all of the arguments, but not the command. */
  if (spec[i] == '*')
    {
      *caller_index = i + 1;
      result = history_arg_extract (1, '$', from);
      return result ? result : savestring ("");
    }

  /* `$' is last arg. */
  if (spec[i] == '$')
    {
      *caller_index = i + 1;
      return history_arg_extract ('$', '$', from);
    }

  /* Try to get FIRST and LAST figured out. */

  if (spec[i] == '-')
    first = 0;
  else if (spec[i] == '^')
    {
      first = 1;
      i++;
    }
  else if (_rl_digit_p (spec[i]) && expecting_word_spec)
    {
      for (first = 0; _rl_digit_p (spec[i]); i++)
        first = (first * 10) + _rl_digit_value (spec[i]);
    }
  else
    return nullptr; /* no valid `first' for word specifier */

  if (spec[i] == '^' || spec[i] == '*')
    {
      last = (spec[i] == '^') ? 1 : '$'; /* x* abbreviates x-$ */
      i++;
    }
  else if (spec[i] != '-')
    last = first;
  else
    {
      i++;

      if (_rl_digit_p (spec[i]))
        {
          for (last = 0; _rl_digit_p (spec[i]); i++)
            last = (last * 10) + _rl_digit_value (spec[i]);
        }
      else if (spec[i] == '$')
        {
          i++;
          last = '$';
        }
      else if (spec[i] == '^')
        {
          i++;
          last = 1;
        }
#if 0
      else if (!spec[i] || spec[i] == ':')
	/* check against `:' because there could be a modifier separator */
#else
      else
      /* csh seems to allow anything to terminate the word spec here,
         leaving it as an abbreviation. */
#endif
      last = -1; /* x- abbreviates x-$ omitting word `$' */
    }

  *caller_index = i;

  if (last >= first || last == '$' || last < 0)
    result = history_arg_extract (first, last, from);

  if (result)
    return result;
  else
    throw word_not_found (); // this replaces "error_pointer" return value
}

/* Extract the args specified, starting at FIRST, and ending at LAST.
   The args are taken from STRING.  If either FIRST or LAST is < 0,
   then make that arg count from the right (subtract from the number of
   tokens, so that FIRST = -1 means the next to last token on the line).
   If LAST is `$' the last arg from STRING is used. */
char *
History::history_arg_extract (int first, int last, const char *string)
{
  char **list;

  /* XXX - think about making history_tokenize return a struct array,
     each struct in array being a string and a length to avoid the
     calls to strlen below. */
  if ((list = history_tokenize (string)) == nullptr)
    return nullptr;

  int len;
  for (len = 0; list[len]; len++)
    ;

  if (last < 0)
    last = len + last - 1;

  if (first < 0)
    first = len + first - 1;

  if (last == '$')
    last = len - 1;

  if (first == '$')
    first = len - 1;

  last++;

  char *result;
  if (first >= len || last > len || first < 0 || last < 0 || first > last)
    result = (nullptr);
  else
    {
      unsigned int size = 0;
      for (int i = first; i < last; i++)
        size += std::strlen (list[i]) + 1;
      result = new char[size + 1];
      result[0] = '\0';

      for (int i = first, offset = 0; i < last; i++)
        {
          std::strcpy (result + offset, list[i]);
          offset += std::strlen (list[i]);
          if (i + 1 < last)
            {
              result[offset++] = ' ';
              result[offset] = 0;
            }
        }
    }

  for (int i = 0; i < len; i++)
    delete[] list[i];

  delete[] list;

  return result;
}

unsigned int
History::history_tokenize_word (const char *string, unsigned int ind)
{
  int delimiter, nestdelim, delimopen;

  unsigned int i = ind;
  delimiter = nestdelim = delimopen = 0;

  if (member (string[i],
              "()\n")) /* XXX - included \n, but why? been here forever */
    {
      i++;
      return i;
    }

  if (std::isdigit (string[i]))
    {
      unsigned int j = i;
      while (string[j] && std::isdigit (string[j]))
        j++;
      if (string[j] == 0)
        return j;
      if (string[j] == '<' || string[j] == '>')
        i = j; /* digit sequence is a file descriptor */
      else
        {
          i = j;
          goto get_word; /* digit sequence is part of a word */
        }
    }

  if (member (string[i], "<>;&|"))
    {
      int peek = string[i + 1];

      if (peek == string[i])
        {
          if (peek == '<' && string[i + 2] == '-')
            i++;
          else if (peek == '<' && string[i + 2] == '<')
            i++;
          i += 2;
          return i;
        }
      else if (peek == '&' && (string[i] == '>' || string[i] == '<'))
        {
          unsigned int j = i + 2;
          while (string[j] && std::isdigit (string[j])) /* file descriptor */
            j++;
          if (string[j] == '-') /* <&[digits]-, >&[digits]- */
            j++;
          return j;
        }
      else if ((peek == '>' && string[i] == '&')
               || (peek == '|' && string[i] == '>'))
        {
          i += 2;
          return i;
        }
      /* XXX - process substitution -- separated out for later -- bash-4.2 */
      else if (peek == '(' && (string[i] == '>' || string[i] == '<')) /*)*/
        {
          i += 2;
          delimopen = '(';
          delimiter = ')';
          nestdelim = 1;
          goto get_word;
        }

      i++;
      return i;
    }

get_word:
  /* Get word from string + i; */

  if (delimiter == 0 && member (string[i], HISTORY_QUOTE_CHARACTERS))
    delimiter = string[i++];

  for (; string[i]; i++)
    {
      if (string[i] == '\\' && string[i + 1] == '\n')
        {
          i++;
          continue;
        }

      if (string[i] == '\\' && delimiter != '\''
          && (delimiter != '"' || member (string[i], slashify_in_quotes)))
        {
          i++;
          continue;
        }

      /* delimiter must be set and set to something other than a quote if
         nestdelim is set, so these tests are safe. */
      if (nestdelim && string[i] == delimopen)
        {
          nestdelim++;
          continue;
        }
      if (nestdelim && string[i] == delimiter)
        {
          nestdelim--;
          if (nestdelim == 0)
            delimiter = 0;
          continue;
        }

      if (delimiter && string[i] == delimiter)
        {
          delimiter = 0;
          continue;
        }

      /* Command and process substitution; shell extended globbing patterns */
      if (nestdelim == 0 && delimiter == 0 && member (string[i], "<>$!@?+*")
          && string[i + 1] == '(') /*)*/
        {
          i += 2;
          delimopen = '(';
          delimiter = ')';
          nestdelim = 1;
          continue;
        }

      if (delimiter == 0 && (member (string[i], history_word_delimiters)))
        break;

      if (delimiter == 0 && member (string[i], HISTORY_QUOTE_CHARACTERS))
        delimiter = string[i];
    }

  return i;
}

static inline char *
history_substring (const char *string, unsigned int start, unsigned int end)
{
  unsigned int len = end - start;
  char *result = new char[len + 1];
  std::strncpy (result, string + start, len);
  result[len] = '\0';
  return result;
}

/* Parse STRING into tokens and return an array of strings.  If WIND is
   not -1 and INDP is not null, we also want the word surrounding index
   WIND.  The position in the returned array of strings is returned in
   *INDP. */
char **
History::history_tokenize_internal (const char *string, unsigned int wind,
                                    unsigned int *indp)
{
  std::vector<char *> result;

  /* If we're searching for a string that's not part of a word (e.g., " "),
     make sure we set *INDP to a reasonable value. */
  if (indp && wind != static_cast<unsigned int> (-1))
    *indp = static_cast<unsigned int> (-1);

  /* Get a token, and stuff it into RESULT.  The tokens are split
     exactly where the shell would split them. */
  for (unsigned int i = 0; string[i];)
    {
      /* Skip leading whitespace. */
      for (; string[i] && fielddelim (string[i]); i++)
        ;
      if (string[i] == 0 || string[i] == history_comment_char)
        goto return_result;

      unsigned int start = i;

      i = history_tokenize_word (string, start);

      /* If we have a non-whitespace delimiter character (which would not be
         skipped by the loop above), use it and any adjacent delimiters to
         make a separate field.  Any adjacent white space will be skipped the
         next time through the loop. */
      if (i == start && history_word_delimiters)
        {
          i++;
          while (string[i] && member (string[i], history_word_delimiters))
            i++;
        }

      /* If we are looking for the word in which the character at a
         particular index falls, remember it. */
      if (indp && wind != static_cast<unsigned int> (-1) && wind >= start
          && wind < i)
        *indp = static_cast<unsigned int> (
            result.size ()); // this will be the index of the new entry

      // add to the end of the vector
      result.push_back (history_substring (string, start, i));
    }

return_result:
  size_t result_size = result.size ();
  char **result_copy = new char *[result_size + 1];
  std::memcpy (result_copy, result.data (), sizeof (char *) * result_size);
  result_copy[result_size] = nullptr;

  return result_copy;
}

/* Free members of WORDS from START to an empty string */
static inline void
freewords (char **words, unsigned int start)
{
  for (unsigned int i = start; words[i]; i++)
    delete[] words[i];
}

/* Find and return the word which contains the character at index IND
   in the history line LINE.  Used to save the word matched by the
   last history !?string? search. */
char *
History::history_find_word (const char *line, unsigned int ind)
{
  char **words, *s;
  unsigned int wind;

  words = history_tokenize_internal (line, ind, &wind);
  if (wind == static_cast<unsigned int> (-1) || words == nullptr)
    {
      if (words)
        freewords (words, 0);

      delete[] words;
      return nullptr;
    }
  s = words[wind];

  for (unsigned int i = 0; i < wind; i++)
    delete[] words[i];

  freewords (words, wind + 1);

  delete[] words;
  return s;
}

} // namespace readline
