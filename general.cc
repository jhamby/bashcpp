/* general.cc -- Stuff that is used by all files. */

/* Copyright (C) 1987-2021 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.hh"

#include "bashtypes.hh"

#if defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif

#include "posixstat.hh"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "shell.hh"

#include "builtins/common.hh"

#if defined(HAVE_MBSTR_H) && defined(HAVE_MBSCHR)
#include <mbstr.h> /* mbschr */
#endif

#include "tilde.hh"

#ifdef __CYGWIN__
#include <sys/cygwin.h>
#endif

namespace bash
{

// Return the type of bash_exception as a string.
const char *
bash_exception::what () const noexcept
{
  switch (type)
    {
    case FORCE_EOF:
      return "FORCE_EOF";

    case DISCARD:
      return "DISCARD";

    case EXITPROG:
      return "EXITPROG";

    case ERREXIT:
      return "ERREXIT";

    case SIGEXIT:
      return "SIGEXIT";

    case EXITBLTIN:
      return "EXITBLTIN";

    case NOEXCEPTION:
    default:
      return "NOEXCEPTION";
    }
}

// Return the type of the subshell_child_start exception.
const char *
subshell_child_start::what () const noexcept
{
  return "subshell_child_start";
}

// Return the type of the sigalarm_interrupt exception.
const char *
sigalarm_interrupt::what () const noexcept
{
  return "sigalarm_interrupt";
}

// Return the type of the wait_interrupt exception.
const char *
wait_interrupt::what () const noexcept
{
  return "wait_interrupt";
}

// Return the type of the return_exception exception.
const char *
return_exception::what () const noexcept
{
  return "return_exception";
}

// Return the type of the subst_expand_error exception.
const char *
subst_expand_error::what () const noexcept
{
  return "subst_expand_error";
}

// Return the type of the subst_expand_fatal exception.
const char *
subst_expand_fatal::what () const noexcept
{
  return "subst_expand_fatal";
}

// Return the type of the extract_string_error exception.
const char *
extract_string_error::what () const noexcept
{
  return "extract_string_error";
}

// Return the type of the extract_string_fatal exception.
const char *
extract_string_fatal::what () const noexcept
{
  return "extract_string_fatal";
}

// Return the type of the matched_pair_error exception.
const char *
matched_pair_error::what () const noexcept
{
  return "matched_pair_error";
}

// Return the type of the parse_error exception.
const char *
parse_error::what () const noexcept
{
  return "parse_error";
}

#if defined(ALIAS)
// Return the type of the read_again_exception exception.
const char *
read_again_exception::what () const noexcept
{
  return "read_again_exception";
}
#endif

/* Do whatever is necessary to initialize `Posix mode'.  This currently
   modifies the following variables which are controlled via shopt:
      interactive_comments
      source_uses_path
      expand_aliases
      inherit_errexit
      print_shift_error

   and the following variables which cannot be user-modified:

      source_searches_cwd

  If we add to the first list, we need to change the table and functions
  below */
void
Shell::posix_initialize (bool on)
{
  /* Things that should be turned on when posix mode is enabled. */
  if (on)
    {
      interactive_comments = source_uses_path = expand_aliases = 1;
      inherit_errexit = 1;
      source_searches_cwd = false;
      print_shift_error = 1;
    }

  /* Things that should be turned on when posix mode is disabled. */
  else if (saved_posix_vars) /* on == 0, restore saved settings */
    {
      set_posix_options (saved_posix_vars);
      delete[] saved_posix_vars;
      saved_posix_vars = nullptr;
    }
  else /* on == 0, restore a default set of settings */
    {
      source_searches_cwd = true;
      expand_aliases = interactive_shell;
      print_shift_error = 0;
    }
}

/* **************************************************************** */
/*								    */
/*  Functions to convert to and from and display non-standard types */
/*								    */
/* **************************************************************** */

#if defined(RLIMTYPE)
RLIMTYPE
string_to_rlimtype (char *s)
{
  RLIMTYPE ret;
  int neg;

  ret = 0;
  neg = 0;
  while (s && *s && whitespace (*s))
    s++;
  if (s && (*s == '-' || *s == '+'))
    {
      neg = *s == '-';
      s++;
    }
  for (; s && *s && std::isdigit (*s); s++)
    ret = (ret * 10) + static_cast<RLIMTYPE> (todigit (*s));
  return neg ? -ret : ret;
}

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-unsigned-zero-compare"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

void
print_rlimtype (RLIMTYPE n, int addnl)
{
  char s[INT_STRLEN_BOUND (RLIMTYPE) + 1], *p;

  p = s + sizeof (s);
  *--p = '\0';

  // Note: RLIMTYPE may be unsigned
  if (n < 0)
    {
      do
        *--p = '0' - n % 10;
      while ((n /= 10) != 0);

      *--p = '-';
    }
  else
    {
      do
        *--p = '0' + n % 10;
      while ((n /= 10) != 0);
    }

  printf ("%s%s", p, addnl ? "\n" : "");
}

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif /* RLIMTYPE */

/* **************************************************************** */
/*								    */
/*		       Input Validation Functions		    */
/*								    */
/* **************************************************************** */

/* Return non-zero if the characters pointed to by STRING constitute a
   valid number.  Stuff the converted number into RESULT if RESULT is
   not null. */
bool
legal_number (const char *string, int64_t *result)
{
  int64_t value;
  char *ep;

  if (result)
    *result = 0;

  errno = 0;
  value = strtoimax (string, &ep, 10);
  if (errno || ep == string)
    return false; /* errno is set on overflow or underflow */

  /* Skip any trailing whitespace, since strtoimax does not. */
  while (whitespace (*ep))
    ep++;

  /* If *string is not '\0' but *ep is '\0' on return, the entire string
     is valid. */
  if (*string && *ep == '\0')
    {
      if (result)
        *result = value;
      /* The SunOS4 implementation of strtol() will happily ignore
         overflow conditions, so this cannot do overflow correctly
         on those systems. */
      return true;
    }

  return false;
}

/* Return true if NAME is a valid value that can be assigned to a nameref
   variable.  FLAGS can be 2, in which case the name is going to be used
   to create a variable.  Other values are currently unused, but could
   be used to allow values to be stored and indirectly referenced, but
   not used in assignments. */
bool
Shell::valid_nameref_value (string_view name, valid_array_flags flags)
{
  if (name.empty ())
    return false;

    /* valid identifier */
#if defined(ARRAY_VARS)
  if (legal_identifier (name)
      || (flags != 2 && valid_array_reference (name, VA_NOFLAGS)))
#else
  if (legal_identifier (name))
#endif
    return true;

  return false;
}

bool
Shell::check_selfref (string_view name, string_view value)
{
  if (name == value)
    return true;

#if defined(ARRAY_VARS)
  if (valid_array_reference (value, VA_NOFLAGS))
    {
      std::string t = array_variable_name (value, 0, nullptr);
      if (name == t)
        return true;
    }
#endif

  return false; /* not a self reference */
}

/* Make sure that WORD is a valid shell identifier, i.e. it doesn't contain a
   dollar sign, nor is quoted in any way.  If CHECK_WORD is true, the word is
   checked to ensure that it consists of only letters, digits, and underscores,
   and doesn't consist of all digits.
 */
bool
Shell::check_identifier (WORD_DESC *word, bool check_word)
{
  if (word->flags & (W_HASDOLLAR | W_QUOTED)) /* XXX - HASDOLLAR? */
    {
      internal_error (_ ("`%s': not a valid identifier"), word->word.c_str ());
      return false;
    }
  else if (check_word
           && (all_digits (word->word.c_str ())
               || !legal_identifier (word->word.c_str ())))
    {
      internal_error (_ ("`%s': not a valid identifier"), word->word.c_str ());
      return false;
    }
  else
    return true;
}

/* Return true if STRING is a valid alias name.  The shell accepts
   essentially all characters except those which must be quoted to the
   parser (which disqualifies them from alias expansion anyway) and `/'. */
bool
Shell::legal_alias_name (string_view string)
{
  string_view::iterator it;
  for (it = string.begin (); it != string.end (); ++it)
    if (shellbreak (*it) || shellxquote (*it) || shellexp (*it)
        || (*it == '/'))
      return false;
  return true;
}

/* Returns non-zero if STRING is an assignment statement.  The returned value
   is the index of the `=' sign.  If FLAGS & 1 we are expecting a compound
   assignment and require an array subscript before the `=' to denote an
   assignment statement. */
size_t
Shell::assignment (string_view string, int flags)
{
  if (string.empty ())
    return 0;

  unsigned char c = static_cast<unsigned char> (string[0]);

#if defined(ARRAY_VARS)
  /* If parser_state includes PST_COMPASSIGN, FLAGS will include 1, so we are
     parsing the contents of a compound assignment. If parser_state includes
     PST_REPARSE, we are in the middle of an assignment statement and breaking
     the words between the parens into words and assignment statements, but
     we don't need to check for that right now. Within a compound assignment,
     the subscript is required to make the word an assignment statement. If
     we don't have a subscript, even if the word is a valid assignment
     statement otherwise, we don't want to treat it as one. */
  if ((flags & 1) && c != '[') /* ] */
    return 0;
  else if ((flags & 1) == 0 && !legal_variable_starter (c))
#else
  if (!legal_variable_starter (c))
#endif
    return 0;

  string_view::iterator it;
  for (it = string.begin (); it != string.end (); ++it)
    {
      c = static_cast<unsigned char> (*it);

      /* The following is safe.  Note that '=' at the start of a word
         is not an assignment statement. */
      if (c == '=')
        return static_cast<size_t> (it - string.begin ());

#if defined(ARRAY_VARS)
      if (c == '[')
        {
          string_view::iterator newit
              = skipsubscript (string, it, (flags & 2) ? 1 : 0);
          /* XXX - why not check for blank subscripts here, if we do in
             valid_array_reference? */
          if (newit == string.end () || *(newit++) != ']')
            return 0;
          if (*newit == '+' && (newit + 1 != string.end ())
              && *(newit + 1) == '=')
            return static_cast<size_t> (newit + 1 - string.begin ());

          return (*newit == '=')
                     ? static_cast<size_t> (newit - string.begin ())
                     : 0;
        }
#endif /* ARRAY_VARS */

      /* Check for `+=' */
      if (c == '+' && (it + 1 != string.end ()) && *(it + 1) == '=')
        return static_cast<size_t> (it + 1 - string.begin ());

      /* Variable names in assignment statements may contain only letters,
         digits, and `_'. */
      if (!legal_variable_char (c))
        return 0;
    }
  return 0;
}

/* **************************************************************** */
/*								    */
/*	     Functions to manage files and file descriptors	    */
/*								    */
/* **************************************************************** */

/* A function to unset no-delay mode on a file descriptor.  Used in shell.c
   to unset it on the fd passed as stdin.  Should be called on stdin if
   readline gets an EAGAIN or EWOULDBLOCK when trying to read input. */

#if !defined(O_NDELAY)
#if defined(FNDELAY)
#define O_NDELAY FNDELAY
#endif
#endif /* O_NDELAY */

/* Make sure no-delay mode is not set on file descriptor FD. */
int
sh_unset_nodelay_mode (int fd)
{
  int flags, bflags;

  if ((flags = ::fcntl (fd, F_GETFL, 0)) < 0)
    return -1;

  bflags = 0;

  /* This is defined to O_NDELAY in filecntl.h if O_NONBLOCK is not present
     and O_NDELAY is defined. */
#ifdef O_NONBLOCK
  bflags |= O_NONBLOCK;
#endif

#ifdef O_NDELAY
  bflags |= O_NDELAY;
#endif

  if (flags & bflags)
    {
      flags &= ~bflags;
      return ::fcntl (fd, F_SETFL, flags);
    }

  return 0;
}

/* Return 1 if PATH1 and PATH2 are the same file.  This is kind of
   expensive.  If non-nullptr STP1 and STP2 point to stat structures
   corresponding to PATH1 and PATH2, respectively. */
bool
same_file (const char *path1, const char *path2, struct stat *stp1,
           struct stat *stp2)
{
  struct stat st1, st2;

  if (stp1 == nullptr)
    {
      if (::stat (path1, &st1) != 0)
        return false;
      stp1 = &st1;
    }

  if (stp2 == nullptr)
    {
      if (::stat (path2, &st2) != 0)
        return false;
      stp2 = &st2;
    }

  return (stp1->st_dev == stp2->st_dev) && (stp1->st_ino == stp2->st_ino);
}

/* Move FD to a number close to the maximum number of file descriptors
   allowed in the shell process, to avoid the user stepping on it with
   redirection and causing us extra work.  If CHECK_NEW is non-zero,
   we check whether or not the file descriptors are in use before
   duplicating FD onto them.  MAXFD says where to start checking the
   file descriptors.  If it's less than 20, we get the maximum value
   available from getdtablesize(2). */
int
move_to_high_fd (int fd, int check_new, int maxfd)
{
  int script_fd, nfds, ignore;

  if (maxfd < 20)
    {
      nfds = getdtablesize ();
      if (nfds <= 0)
        nfds = 20;
      if (nfds > HIGH_FD_MAX)
        nfds = HIGH_FD_MAX; /* reasonable maximum */
    }
  else
    nfds = maxfd;

  for (nfds--; check_new && nfds > 3; nfds--)
    if (fcntl (nfds, F_GETFD, &ignore) == -1)
      break;

  if (nfds > 3 && fd != nfds && (script_fd = dup2 (fd, nfds)) != -1)
    {
      if (check_new == 0 || fd != fileno (stderr)) /* don't close stderr */
        close (fd);
      return script_fd;
    }

  /* OK, we didn't find one less than our artificial maximum; return the
     original file descriptor. */
  return fd;
}

/* **************************************************************** */
/*								    */
/*		    Functions to manipulate pathnames		    */
/*								    */
/* **************************************************************** */

/* Turn STRING (a pathname) into an absolute pathname, assuming that
   DOT_PATH contains the symbolic location of `.'.  This always
   returns a new string, even if STRING was an absolute pathname to
   begin with. */
char *
Shell::make_absolute (const char *string, const char *dot_path)
{
  char *result;

  if (dot_path == nullptr || ABSPATH (string))
#ifdef __CYGWIN__
    {
      char pathbuf[PATH_MAX + 1];

      /* WAS cygwin_conv_to_full_posix_path (string, pathbuf); */
      cygwin_conv_path (CCP_WIN_A_TO_POSIX, string, pathbuf, PATH_MAX);
      result = savestring (pathbuf);
    }
#else
    result = savestring (string);
#endif
  else
    result = sh_makepath (dot_path, string, MP_NOFLAGS);

  return result;
}

/* Return the full pathname of FILE.  Easy.  Filenames that begin
   with a '/' are returned as themselves.  Other filenames have
   the current working directory prepended.  A new string is
   returned in either case. */
char *
Shell::full_pathname (const char *file)
{
  char *ret;

  ret = (*file == '~') ? bash_tilde_expand (file, 0) : savestring (file);

  if (ABSPATH (ret))
    return ret;

  char *ret2 = sh_makepath (nullptr, ret, (MP_DOCWD | MP_RMDOT));
  delete[] ret;

  return ret2;
}

/* A slightly related function.  Get the prettiest name of this
   directory possible. */

/* Return a pretty pathname.  If the first part of the pathname is
   the same as $HOME, then replace that with `~'.  */
std::string
Shell::polite_directory_format (string_view name)
{
  const char *home = get_string_value ("HOME");
  size_t l = home ? strlen (home) : 0;

  if (l > 1 && name.compare (0, l, home) == 0
      && (name.size () == l || name[l] == '/'))
    {
      std::string tdir_buf;
      tdir_buf.reserve (name.size () - l + 1);
      tdir_buf.push_back ('~');
      tdir_buf.append (name.begin() + l, name.end ());
      return tdir_buf;
    }
  else
    return to_string (name);
}

/* Given a string containing units of information separated by colons,
   return the next one pointed to by (P_INDEX), or nullptr if there are no
   more. Advance (P_INDEX) to the character after the colon. */
char *
extract_colon_unit (const char *string, size_t *p_index)
{
  size_t i, start, len;
  char *value;

  if (string == nullptr)
    return nullptr;

  len = std::strlen (string);
  if (*p_index >= len)
    return nullptr;

  i = *p_index;

  /* Each call to this routine leaves the index pointing at a colon if
     there is more to the path.  If I is > 0, then increment past the
     `:'.  If I is 0, then the path has a leading colon.  Trailing colons
     are handled OK by the `else' part of the if statement; an empty
     string is returned in that case. */
  if (i && string[i] == ':')
    i++;

  for (start = i; string[i] && string[i] != ':'; i++)
    ;

  *p_index = i;

  if (i == start)
    {
      if (string[i])
        (*p_index)++;
      /* Return "" in the case of a trailing `:'. */
      value = new char[1];
      value[0] = '\0';
    }
  else
    value = substring (string, start, i);

  return value;
}

/* **************************************************************** */
/*								    */
/*		    Tilde Initialization and Expansion		    */
/*								    */
/* **************************************************************** */

/* If tilde_expand hasn't been able to expand the text, perhaps it
   is a special shell expansion.  This function is installed as the
   tilde_expansion_preexpansion_hook.  It knows how to expand ~- and ~+.
   If PUSHD_AND_POPD is defined, ~[+-]N expands to directories from the
   directory stack. */
char *
Shell::bash_special_tilde_expansions (const char *text)
{
  const char *result = nullptr;

  if (text[0] == '+' && text[1] == '\0')
    result = get_string_value ("PWD");
  else if (text[0] == '-' && text[1] == '\0')
    result = get_string_value ("OLDPWD");
#if defined(PUSHD_AND_POPD)
  else if (std::isdigit (*text)
           || ((*text == '+' || *text == '-') && std::isdigit (text[1])))
    result = get_dirstack_from_string (text);
#endif

  return result ? savestring (result) : nullptr;
}

/* Initialize the tilde expander.  In Bash, we handle `~-' and `~+', as
   well as handling special tilde prefixes; `:~" and `=~' are indications
   that we should do tilde expansion. */
void
Shell::tilde_initialize ()
{
  /* Tell the tilde expander that we want a crack first. */
  tilde_expansion_preexpansion_hook = &Shell::bash_special_tilde_expansions;

  /* Tell the tilde expander about special strings which start a tilde
     expansion, and the special strings that end one.  Only do this once.
     tilde_initialize () is called from within bashline_reinitialize (). */
  if (!tilde_strings_initialized)
    {
      tilde_strings_initialized = true;

      bash_tilde_prefixes = new char *[3];
      bash_tilde_prefixes[0] = savestring ("=~");
      bash_tilde_prefixes[1] = savestring (":~");
      bash_tilde_prefixes[2] = nullptr;

      bash_tilde_prefixes2 = new char *[2];
      bash_tilde_prefixes2[0] = savestring (":~");
      bash_tilde_prefixes2[1] = nullptr;

      tilde_additional_prefixes = bash_tilde_prefixes;

      bash_tilde_suffixes = new char *[3];
      bash_tilde_suffixes[0] = savestring (":");
      bash_tilde_suffixes[1] = savestring ("=~"); /* XXX - ?? */
      bash_tilde_suffixes[2] = nullptr;

      tilde_additional_suffixes = bash_tilde_suffixes;

      bash_tilde_suffixes2 = new char *[2];
      bash_tilde_suffixes2[0] = savestring (":");
      bash_tilde_suffixes2[1] = nullptr;
    }
}

/* POSIX.2, 3.6.1:  A tilde-prefix consists of an unquoted tilde character
   at the beginning of the word, followed by all of the characters preceding
   the first unquoted slash in the word, or all the characters in the word
   if there is no slash...If none of the characters in the tilde-prefix are
   quoted, the characters in the tilde-prefix following the tilde shell be
   treated as a possible login name. */

#define TILDE_END(c) ((c) == '\0' || (c) == '/' || (c) == ':')

static bool
unquoted_tilde_word (const char *s)
{
  const char *r;

  for (r = s; TILDE_END (*r) == 0; r++)
    {
      switch (*r)
        {
        case '\\':
        case '\'':
        case '"':
          return 0;
        }
    }
  return 1;
}

/* Find the end of the tilde-prefix starting at S, and return the tilde
   prefix in newly-allocated memory. FLAGS tells whether or not we're in an
   assignment context -- if so, `:' delimits the end of the tilde prefix as
   well. */
char *
bash_tilde_find_word (const char *s, int flags, size_t *lenp)
{
  const char *r;
  char *ret;

  for (r = s; *r && *r != '/'; r++)
    {
      /* Short-circuit immediately if we see a quote character.  Even though
         POSIX says that `the first unquoted slash' (or `:') terminates the
         tilde-prefix, in practice, any quoted portion of the tilde prefix
         will cause it to not be expanded. */
      if (*r == '\\' || *r == '\'' || *r == '"')
        {
          ret = savestring (s);
          if (lenp)
            *lenp = 0;
          return ret;
        }
      else if (flags && *r == ':')
        break;
    }
  size_t l = static_cast<size_t> (r - s);
  ret = new char[l + 1];
  std::strncpy (ret, s, l);
  ret[l] = '\0';
  if (lenp)
    *lenp = l;
  return ret;
}

/* Tilde-expand S by running it through the tilde expansion library.
   ASSIGN_P is 1 if this is a variable assignment, so the alternate
   tilde prefixes should be enabled (`=~' and `:~', see above).  If
   ASSIGN_P is 2, we are expanding the rhs of an assignment statement,
   so `=~' is not valid. */
char *
Shell::bash_tilde_expand (const char *s, int assign_p)
{
  int r;
  char *ret;

  tilde_additional_prefixes
      = assign_p == 0
            ? nullptr
            : (assign_p == 2 ? bash_tilde_prefixes2 : bash_tilde_prefixes);

  if (assign_p == 2)
    tilde_additional_suffixes = bash_tilde_suffixes2;

  r = (*s == '~') ? unquoted_tilde_word (s) : 1;
  ret = r ? tilde_expand (s) : savestring (s);

  QUIT;

  return ret;
}

/* **************************************************************** */
/*								    */
/*	  Functions to manipulate and search the group list	    */
/*								    */
/* **************************************************************** */

void
Shell::initialize_group_array ()
{
  int i;

  if (maxgroups == 0)
    maxgroups = getmaxgroups ();

  ngroups = 0;
  delete[] group_array;
  group_array = new GETGROUPS_T[static_cast<size_t> (maxgroups)];

#if defined(HAVE_GETGROUPS)
  ngroups = ::getgroups (maxgroups, group_array);
#endif

  /* If getgroups returns nothing, or the OS does not support getgroups(),
     make sure the groups array includes at least the current gid. */
  if (ngroups == 0)
    {
      group_array[0] = current_user.gid;
      ngroups = 1;
    }

  /* If the primary group is not in the groups array, add it as group_array[0]
     and shuffle everything else up 1, if there's room. */
  for (i = 0; i < ngroups; i++)
    if (current_user.gid == group_array[i])
      break;
  if (i == ngroups && ngroups < maxgroups)
    {
      for (i = ngroups; i > 0; i--)
        group_array[i] = group_array[i - 1];
      group_array[0] = current_user.gid;
      ngroups++;
    }

  /* If the primary group is not group_array[0], swap group_array[0] and
     whatever the current group is.  The vast majority of systems should
     not need this; a notable exception is Linux. */
  if (group_array[0] != current_user.gid)
    {
      for (i = 0; i < ngroups; i++)
        if (group_array[i] == current_user.gid)
          break;
      if (i < ngroups)
        {
          group_array[i] = group_array[0];
          group_array[0] = current_user.gid;
        }
    }
}

/* Return non-zero if GID is one that we have in our groups list. */
#if !defined(HAVE_GROUP_MEMBER)
int
Shell::group_member (gid_t gid)
{
#if defined(HAVE_GETGROUPS)
  int i;
#endif

  /* Short-circuit if possible, maybe saving a call to getgroups(). */
  if (gid == current_user.gid || gid == current_user.egid)
    return 1;

#if defined(HAVE_GETGROUPS)
  if (ngroups == 0)
    initialize_group_array ();

  /* In case of error, the user loses. */
  if (ngroups <= 0)
    return 0;

  /* Search through the list looking for GID. */
  for (i = 0; i < ngroups; i++)
    if (gid == static_cast<gid_t> (group_array[i]))
      return 1;
#endif

  return 0;
}
#endif /* !HAVE_GROUP_MEMBER */

STRINGLIST *
Shell::get_group_list ()
{
  if (group_vector)
    return group_vector;

  if (ngroups == 0)
    initialize_group_array ();

  if (ngroups <= 0)
    return nullptr;

  group_vector = new STRINGLIST ();

  for (int i = 0; i < ngroups; i++)
    group_vector->push_back (savestring (itos (group_array[i])));

  return group_vector;
}

std::vector<gid_t> *
Shell::get_group_array ()
{
  if (group_iarray)
    return group_iarray;

  if (ngroups == 0)
    initialize_group_array ();

  if (ngroups <= 0)
    return nullptr;

  group_iarray = new std::vector<gid_t> ();

  for (int i = 0; i < ngroups; i++)
    group_iarray->push_back (group_array[i]);

  return group_iarray;
}

/* **************************************************************** */
/*								    */
/*	  Miscellaneous functions				    */
/*								    */
/* **************************************************************** */

/* Return a value for PATH that is guaranteed to find all of the standard
   utilities.  This uses Posix.2 configuration variables, if present.  It
   uses a value defined in config.h as a last resort. */
std::string
conf_standard_path ()
{
#if defined(_CS_PATH) && defined(HAVE_CONFSTR)
  char *p;
  size_t len;

  len = ::confstr (_CS_PATH, nullptr, 0);
  if (len > 0)
    {
      p = new char[len + 2];
      *p = '\0';
      ::confstr (_CS_PATH, p, len);
      return p;
    }
  else
    return savestring (STANDARD_UTILS_PATH);
#else /* !_CS_PATH || !HAVE_CONFSTR  */
#if defined(CS_PATH)
  return savestring (CS_PATH);
#else
  return savestring (STANDARD_UTILS_PATH);
#endif /* !CS_PATH */
#endif /* !_CS_PATH || !HAVE_CONFSTR */
}

int
Shell::default_columns ()
{
  int c = -1;
  const char *v = get_string_value ("COLUMNS");
  if (v && *v)
    {
      c = atoi (v);
      if (c > 0)
        return c;
    }

  if (check_window_size)
    get_new_window_size (0, nullptr, &c);

  return c > 0 ? c : 80;
}

} // namespace bash
