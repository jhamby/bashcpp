/* mkbuiltins.c - Create builtext.hh and builtdoc.cc from
   a single source file called builtins.cc. */

/* Copyright (C) 1987-2020 Free Software Foundation, Inc.

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

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashtypes.hh"

#if defined(HAVE_SYS_FILE_H)
#include <sys/file.h>
#endif

#include "filecntl.hh"
#include "posixstat.hh"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include <string>
#include <vector>

using std::cerr;
using std::ofstream;
using std::ostringstream;
using std::string;

namespace bash
{

static const char *DOCFILE = "builtins.texi";

static inline bool
whitespace (char c)
{
  return (c == ' ') || (c == '\t');
}

/* Flag values that builtins can have. */
constexpr int BUILTIN_FLAG_SPECIAL = 0x01;
constexpr int BUILTIN_FLAG_ASSIGNMENT = 0x02;
constexpr int BUILTIN_FLAG_LOCALVAR = 0x04;
constexpr int BUILTIN_FLAG_POSIX_BUILTIN = 0x08;

constexpr int BASE_INDENT = 4;

/* If this stream descriptor is non-zero, then write
   texinfo documentation to it. */
static ofstream documentation_file;

/* Non-zero means to only produce documentation. */
static bool only_documentation = false;

/* Non-zero means to not do any productions. */
static bool inhibit_production = false;

/* Non-zero means to not add functions (xxx_builtin) to the members of the
   produced `bash::Builtin []' */
static bool inhibit_functions = false;

/* Non-zero means to produce separate help files for each builtin, named by
   the builtin name, in `./helpfiles'. */
static bool separate_helpfiles = false;

/* Non-zero means to create single C strings for each `longdoc', with
   embedded newlines, for ease of translation. */
static bool single_longdoc_strings = true;

/* The name of a directory into which the separate external help files will
   eventually be installed. */
static string helpfile_directory;

/* The name of a directory to precede the filename when reporting
   errors. */
static string source_directory;

/* The name of the structure file. */
static string struct_filename;

/* The name of the external declaration file. */
static string extern_filename;

/* This typedef replaces the old ARRAY implementation, for strings. */
typedef std::vector<string> StrArray;

/* Here is a structure defining a single BUILTIN. */
struct Builtin
{
  Builtin (const string &n) : name (n), flags (0) {}
  Builtin (const Builtin &rhs)
      : name (rhs.name), function (rhs.function), shortdoc (rhs.shortdoc),
        docname (rhs.docname), longdoc (rhs.longdoc),
        dependencies (rhs.dependencies), flags (rhs.flags)
  {
  }
  string name;           /* The name of this builtin. */
  string function;       /* The name of the function to call. */
  string shortdoc;       /* The short documentation for this builtin. */
  string docname;        /* Possible name for documentation string. */
  StrArray longdoc;      /* The long documentation for this builtin. */
  StrArray dependencies; /* std::vector of #define names. */
  int flags;             /* Flags for this builtin. */
};

/* This typedef replaces the old ARRAY implementation, for Builtins. */
typedef std::vector<Builtin *> BltArray;

/* Here is a structure which defines a DEF file. */
struct DefFile
{
  DefFile (const std::string &f, int lnum) : filename (f), line_number (lnum)
  {
  }
  string filename;   /* The name of the input def file. */
  StrArray lines;    /* The contents of the file. */
  string production; /* The name of the production file. */
  ofstream output;   /* Open file stream for PRODUCTION. */
  BltArray builtins; /* std::vector of Builtin *. */
  int line_number;   /* The current line number. */
};

/* The array of all builtins encountered during execution of this code. */
static BltArray saved_builtins;

/* The Posix.2 so-called `special' builtins. */
static const char *special_builtins[]
    = { ":",     ".",     "source", "break",    "continue", "eval",
        "exec",  "exit",  "export", "readonly", "return",   "set",
        "shift", "times", "trap",   "unset",    nullptr };

/* The builtin commands that take assignment statements as arguments. */
static const char *assignment_builtins[] = { "alias", "declare",  "export",
                                             "local", "readonly", "typeset",
                                             nullptr };

static const char *localvar_builtins[]
    = { "declare", "local", "typeset", nullptr };

/* The builtin commands that are special to the POSIX search order. */
static const char *posix_builtins[]
    = { "alias", "bg",      "cd",    "command", "false",  "fc",
        "fg",    "getopts", "jobs",  "kill",    "newgrp", "pwd",
        "read",  "true",    "umask", "unalias", "wait",   nullptr };

/* Forward declarations. */
static bool is_special_builtin (const string &name);
static bool is_assignment_builtin (const string &name);
static bool is_localvar_builtin (const string &name);
static bool is_posix_builtin (const string &name);

static void extract_info (const string &, ofstream &, ofstream &);

static void file_error (const string &) __attribute__ ((__noreturn__));

static void line_error (DefFile &defs, const string &msg);

static void write_file_headers (ofstream &structfile, ofstream &externfile);
static void write_file_footers (ofstream &structfile, ofstream &externfile);
static void write_ifdefs (ofstream &stream, const StrArray &defines);
static void write_endifs (ofstream &stream, const StrArray &defines);
static void write_documentation (ofstream &, const StrArray &, int, int);
static void write_longdocs (ofstream &stream, const BltArray &builtins);
static void write_builtins (DefFile &defs, ofstream &structfile,
                            ofstream &externfile);

static void add_documentation (DefFile &defs, const string &line);

static string get_arg (const string &for_whom, DefFile &defs,
                       const string &str);

static void must_be_building (const string &directive, DefFile &defs);

static Builtin *current_builtin (const string &directive, DefFile &defs);

static string strip_whitespace (const string &string);
static string remove_trailing_whitespace (const string &string);

static inline const char *
document_name (const Builtin &b)
{
  return !b.docname.empty () ? b.docname.c_str () : b.name.c_str ();
}

} /* namespace bash */

/* For each file mentioned on the command line, process it and
   write the information to STRUCTFILE and EXTERNFILE, while
   creating the production file if necessary. */
int
main (int argc, char **argv)
{
  string documentation_filename (bash::DOCFILE);

  int arg_index = 1;
  while (arg_index < argc && argv[arg_index][0] == '-')
    {
      char *arg = argv[arg_index++];

      if (std::strcmp (arg, "-externfile") == 0)
        bash::extern_filename = argv[arg_index++];
      else if (std::strcmp (arg, "-structfile") == 0)
        bash::struct_filename = argv[arg_index++];
      else if (std::strcmp (arg, "-noproduction") == 0)
        bash::inhibit_production = true;
      else if (std::strcmp (arg, "-nofunctions") == 0)
        bash::inhibit_functions = true;
      else if (std::strcmp (arg, "-document") == 0)
        {
          bash::documentation_file.open (documentation_filename.c_str ());
          if (!bash::documentation_file.is_open ())
            bash::file_error (documentation_filename);
        }
      else if (std::strcmp (arg, "-D") == 0)
        {
          bash::source_directory = argv[arg_index];

          string &tmp = bash::source_directory;
          if (tmp[tmp.size () - 1] != '/')
            tmp += '/';

          arg_index++;
        }
      else if (std::strcmp (arg, "-documentonly") == 0)
        {
          bash::only_documentation = true;
          bash::documentation_file.open (documentation_filename.c_str ());
          if (!bash::documentation_file.is_open ())
            bash::file_error (documentation_filename);
        }
      else if (std::strcmp (arg, "-H") == 0)
        {
          bash::separate_helpfiles = true;
          bash::helpfile_directory = argv[arg_index++];
        }
      else if (std::strcmp (arg, "-S") == 0)
        bash::single_longdoc_strings = false;
      else
        {
          cerr << argv[0] << ": Unknown flag " << arg << ".\n";
          std::exit (2);
        }
    }

  /* If there are no files to process, just quit now. */
  if (arg_index == argc)
    std::exit (0);

  ofstream structfile, externfile;
  std::string temp_struct_filename;

  if (!bash::only_documentation)
    {
      /* Open the files. */
      if (!bash::struct_filename.empty ())
        {
          std::ostringstream filename_sstream;
          filename_sstream << "mk-" << ::getpid ();
          temp_struct_filename = filename_sstream.str ();

          structfile.open (temp_struct_filename.c_str ());

          if (!structfile.is_open ())
            bash::file_error (temp_struct_filename);
        }

      if (!bash::extern_filename.empty ())
        {
          externfile.open (bash::extern_filename.c_str ());

          if (!externfile.is_open ())
            bash::file_error (bash::extern_filename);
        }

      /* Write out the headers. */
      bash::write_file_headers (structfile, externfile);
    }

  if (bash::documentation_file.is_open ())
    {
      bash::documentation_file << "@c Table of builtins created with "
                               << argv[0] << ".\n";
      bash::documentation_file << "@ftable @asis\n";
    }

  /* Process the .def files. */
  while (arg_index < argc)
    {
      char *arg;

      arg = argv[arg_index++];

      string fname (arg);
      if (fname[0] != '/' && !bash::source_directory.empty ())
        fname.insert (0, bash::source_directory);

      bash::extract_info (fname, structfile, externfile);
    }

  /* Close the files. */
  if (!bash::only_documentation)
    {
      /* Write the footers. */
      bash::write_file_footers (structfile, externfile);

      if (structfile)
        {
          bash::write_longdocs (structfile, bash::saved_builtins);
          structfile.close ();
          ::rename (temp_struct_filename.c_str (),
                    bash::struct_filename.c_str ());
        }
    }

  if (bash::documentation_file.is_open ())
    {
      bash::documentation_file << "@end ftable\n";
    }
}

namespace bash
{

/* **************************************************************** */
/*								    */
/*		       Processing a DEF File			    */
/*								    */
/* **************************************************************** */

/* The definition of a function. */
typedef void (*mk_handler_func_t) (const string &, DefFile &, const string &);

/* Structure handles processor directives. */
struct HandlerEntry
{
  const char *directive;
  mk_handler_func_t function;
};

static void builtin_handler (const string &, DefFile &, const string &);
static void function_handler (const string &, DefFile &, const string &);
static void short_doc_handler (const string &, DefFile &, const string &);
static void comment_handler (const string &, DefFile &, const string &);
static void depends_on_handler (const string &, DefFile &, const string &);
static void produces_handler (const string &, DefFile &, const string &);
static void end_handler (const string &, DefFile &, const string &);
static void docname_handler (const string &, DefFile &, const string &);

static HandlerEntry handlers[] = { { "BUILTIN", &builtin_handler },
                                   { "DOCNAME", &docname_handler },
                                   { "FUNCTION", &function_handler },
                                   { "SHORT_DOC", &short_doc_handler },
                                   { "$", &comment_handler },
                                   { "COMMENT", &comment_handler },
                                   { "DEPENDS_ON", &depends_on_handler },
                                   { "PRODUCES", &produces_handler },
                                   { "END", &end_handler },
                                   { nullptr, nullptr } };

/* Return the entry in the table of handlers for NAME. */
static HandlerEntry *
find_directive (const char *directive)
{
  for (int i = 0; handlers[i].directive; i++)
    if (std::strcmp (handlers[i].directive, directive) == 0)
      return &handlers[i];

  return nullptr;
}

/* Non-zero indicates that a $BUILTIN has been seen, but not
   the corresponding $END. */
static bool building_builtin = false;

/* Non-zero means to output cpp line and file information before
   printing the current line to the production file. */
static bool output_cpp_line_info = false;

/* The main function of this program.  Read FILENAME and act on what is
   found.  Lines not starting with a dollar sign are copied to the
   $PRODUCES target, if one is present.  Lines starting with a dollar sign
   are directives to this program, specifying the name of the builtin, the
   function to call, the short documentation and the long documentation
   strings.  FILENAME can contain multiple $BUILTINs, but only one $PRODUCES
   target.  After the file has been processed, write out the names of
   builtins found in each $BUILTIN.  Plain text found before the $PRODUCES
   is ignored, as is "$$ comment text". */
void
extract_info (const string &filename, ofstream &structfile,
              ofstream &externfile)
{
  struct stat finfo;
  int fd;
  ssize_t nr;

  if (::stat (filename.c_str (), &finfo) == -1)
    file_error (filename);

  fd = ::open (filename.c_str (), O_RDONLY,
               0644); /* fix insecure permissions */

  if (fd == -1)
    file_error (filename);

  size_t file_size = static_cast<size_t> (finfo.st_size);
  char *buffer = new char[1 + file_size];

  if ((nr = ::read (fd, buffer, file_size)) < 0)
    file_error (filename);

  /* This is needed on WIN32, and does not hurt on Unix. */
  if (static_cast<size_t> (nr) < file_size)
    file_size = static_cast<size_t> (nr);

  ::close (fd);

  if (nr == 0)
    {
      cerr << "mkbuiltins: " << filename << ": skipping zero-length file\n";
      delete[] buffer;
      return;
    }

  /* Create and fill in the initial structure describing this file. */
  DefFile defs (filename, 0); /* filename and line number */

  /* Build the array of lines. */
  for (size_t i = 0; i < file_size; ++i)
    {
      size_t start = i;
      while (i < file_size && buffer[i] != '\n')
        i++;

      buffer[i] = '\0';
      if (!std::strncmp (&buffer[start], "// ", 3)) // skip C++ comment prefix
        start += 3;
      defs.lines.push_back (&buffer[start]);
    }

  /* Begin processing the input file.  We don't write any output
     until we have a file to write output to. */
  output_cpp_line_info = true;

  /* Process each line in the array. */
  int i = 0;
  for (StrArray::iterator it = defs.lines.begin (); it != defs.lines.end ();
       ++it, ++i)
    {
      defs.line_number = i;
      string &line = *it;

      if (line[0] == '$')
        {
          size_t j;
          HandlerEntry *handler;

          /* Isolate the directive. */
          for (j = 0; line[j] && !whitespace (line[j]); j++)
            ;

          string directive (line, 1, j - 1);

          /* Get the function handler and call it. */
          handler = find_directive (directive.c_str ());

          if (!handler)
            {
              string tmp ("Unknown directive `");
              tmp += directive;
              tmp += '\'';
              line_error (defs, tmp);
              continue;
            }
          else
            {
              /* Advance to the first non-whitespace character. */
              while (whitespace (line[j]))
                j++;

              /* Call the directive handler with the FILE, and ARGS. */
              (*(handler->function)) (directive, defs, string (line, j));
            }
        }
      else
        {
          if (building_builtin)
            add_documentation (defs, line);
          else if (defs.output)
            {
              if (output_cpp_line_info)
                {
                  /* If we're handed an absolute pathname, don't prepend
                     the directory name. */
                  if (defs.filename[0] == '/')
                    defs.output << "#line " << (defs.line_number + 1) << " \""
                                << defs.filename << "\"\n";
                  else
                    defs.output << "#line " << (defs.line_number + 1) << " \""
                                << (source_directory.empty ()
                                        ? "./"
                                        : source_directory.c_str ())
                                << defs.filename << "\"\n";

                  output_cpp_line_info = false;
                }

              defs.output << line << '\n';
            }
        }
    }

  /* The file has been processed.  Write the extern definitions to the
     builtext.h file. */
  write_builtins (defs, structfile, externfile);

  for (BltArray::iterator it = defs.builtins.begin ();
       it != defs.builtins.end (); ++it)
    {
      delete *it;
    }

  delete[] buffer;
}

/* **************************************************************** */
/*								    */
/*		     The Handler Functions Themselves		    */
/*								    */
/* **************************************************************** */

/* Strip surrounding whitespace from STRING and return a new string. */
string
strip_whitespace (const string &str)
{
  size_t pos = 0;
  while (pos < str.size () && whitespace (str[pos]))
    pos++;

  size_t count = str.size () - pos;
  while (count > 0 && whitespace (str[pos + count - 1]))
    count--;

  return str.substr (pos, count);
}

/* Remove only the trailing whitespace from STRING. */
string
remove_trailing_whitespace (const string &str)
{
  size_t count = str.size ();
  while (count > 0 && whitespace (str[count - 1]))
    count--;

  return str.substr (0, count);
}

/* Ensure that there is a argument in STRING and return it.
   FOR_WHOM is the name of the directive which needs the argument.
   DEFS is the DEF_FILE in which the directive is found.
   If there is no argument, produce an error. */
string
get_arg (const string &for_whom, DefFile &defs, const string &str)
{
  string arg = strip_whitespace (str);

  if (arg.empty ())
    {
      string tmp (for_whom);
      tmp += " requires an argument";
      line_error (defs, tmp);
    }

  return arg;
}

/* Error if not building a builtin. */
void
must_be_building (const string &directive, DefFile &defs)
{
  if (!building_builtin)
    {
      string tmp (directive);
      tmp += " must be inside of a $BUILTIN block";
      line_error (defs, tmp);
    }
}

/* Return the current builtin. */
Builtin *
current_builtin (const string &directive, DefFile &defs)
{
  must_be_building (directive, defs);

  return defs.builtins.back ();
}

/* Add LINE to the long documentation for the current builtin.
   Ignore blank lines until the first non-blank line has been seen. */
void
add_documentation (DefFile &defs, const string &line)
{
  Builtin *builtin = current_builtin ("(implied LONGDOC)", defs);

  string stripped_line = remove_trailing_whitespace (line);

  if (line.empty ())
    return;

  builtin->longdoc.push_back (line);
}

/* How to handle the $BUILTIN directive. */
void
builtin_handler (const string &self, DefFile &defs, const string &arg)
{
  /* If we are already building a builtin, we cannot start a new one. */
  if (building_builtin)
    {
      string tmp (self);
      tmp += " found before $END";
      line_error (defs, tmp);
      return;
    }

  output_cpp_line_info = true;

  /* Get the name of this builtin, and stick it in the array. */
  string name = get_arg (self, defs, arg);

  Builtin *new_builtin = new Builtin (name);

  if (is_special_builtin (name))
    new_builtin->flags |= BUILTIN_FLAG_SPECIAL;
  if (is_assignment_builtin (name))
    new_builtin->flags |= BUILTIN_FLAG_ASSIGNMENT;
  if (is_localvar_builtin (name))
    new_builtin->flags |= BUILTIN_FLAG_LOCALVAR;
  if (is_posix_builtin (name))
    new_builtin->flags |= BUILTIN_FLAG_POSIX_BUILTIN;

  defs.builtins.push_back (new_builtin);
  building_builtin = true;
}

/* How to handle the $FUNCTION directive. */
void
function_handler (const string &self, DefFile &defs, const string &arg)
{
  Builtin *builtin = current_builtin (self, defs);

  if (builtin == nullptr)
    {
      line_error (defs,
                  "syntax error: no current builtin for $FUNCTION directive");
      std::exit (1);
    }
  if (!builtin->function.empty ())
    {
      string tmp (builtin->name);
      tmp += " already has a function (";
      tmp += builtin->function;
      tmp += ')';
      line_error (defs, tmp);
    }
  else
    builtin->function = get_arg (self, defs, arg);
}

/* How to handle the $DOCNAME directive. */
void
docname_handler (const string &self, DefFile &defs, const string &arg)
{
  Builtin *builtin = current_builtin (self, defs);

  if (!builtin->docname.empty ())
    {
      string tmp (builtin->name);
      tmp += " already had a docname (";
      tmp += builtin->docname;
      tmp += ')';
      line_error (defs, tmp);
    }
  else
    builtin->docname = get_arg (self, defs, arg);
}

/* How to handle the $SHORT_DOC directive. */
void
short_doc_handler (const string &self, DefFile &defs, const string &arg)
{
  Builtin *builtin = current_builtin (self, defs);

  if (!builtin->shortdoc.empty ())
    {
      string tmp (builtin->name);
      tmp += " already has short documentation (";
      tmp += builtin->shortdoc;
      tmp += ')';
      line_error (defs, tmp);
    }
  else
    builtin->shortdoc = get_arg (self, defs, arg);
}

/* How to handle the $COMMENT directive. */
void
comment_handler (const string &, DefFile &, const string &)
{
}

/* How to handle the $DEPENDS_ON directive. */
void
depends_on_handler (const string &self, DefFile &defs, const string &arg)
{
  Builtin *builtin = current_builtin (self, defs);
  string dependent = get_arg (self, defs, arg);

  builtin->dependencies.push_back (dependent);
}

/* How to handle the $PRODUCES directive. */
void
produces_handler (const string &self, DefFile &defs, const string &arg)
{
  /* If just hacking documentation, don't change any of the production
     files. */
  if (only_documentation)
    return;

  output_cpp_line_info = true;

  if (!defs.production.empty ())
    {
      string tmp (defs.filename);
      tmp += " already has a ";
      tmp += self;
      tmp += " definition";
      line_error (defs, tmp);
    }
  else
    {
      defs.production = get_arg (self, defs, arg);

      if (inhibit_production)
        return;

      defs.output.open (defs.production.c_str ());

      if (!defs.output.is_open ())
        file_error (defs.production);

      defs.output << "/* " << defs.production << ", created from "
                  << defs.filename << ". */\n";
    }
}

/* How to handle the $END directive. */
void
end_handler (const string &self, DefFile &defs, const string &)
{
  must_be_building (self, defs);
  building_builtin = false;
}

/* **************************************************************** */
/*								    */
/*		    Error Handling Functions			    */
/*								    */
/* **************************************************************** */

/* Produce an error for DEFS with MSG. */
void
line_error (DefFile &defs, const string &msg)
{
  if (defs.filename[0] != '/')
    cerr << (source_directory.empty () ? "./" : source_directory.c_str ());

  cerr << defs.filename << ':' << (defs.line_number + 1) << ':';
  cerr << msg << '\n';
  /* no need to flush(): cerr should be unbuffered. */
}

/* Print error message for FILENAME. */
static void
file_error (const string &filename)
{
  std::perror (filename.c_str ());
  std::exit (2);
}

/* **************************************************************** */
/*								    */
/*		  Creating the Struct and Extern Files		    */
/*								    */
/* **************************************************************** */

/* Flags that mean something to write_documentation (). */
constexpr int STRING_ARRAY = 0x01;
constexpr int TEXINFO = 0x02;
// constexpr int PLAINTEXT =	0x04;
constexpr int HELPFILE = 0x08;

static const char *structfile_header[] = {
  "/* builtins.cc -- the built in shell commands. */",
  "",
  "/* This file is manufactured by ./mkbuiltins, and should not be",
  "   edited by hand.  See the source to mkbuiltins for details. */",
  "",
  "/* Copyright (C) 1987-2015 Free Software Foundation, Inc.",
  "",
  "   This file is part of GNU Bash, the Bourne Again SHell.",
  "",
  "   Bash is free software: you can redistribute it and/or modify",
  "   it under the terms of the GNU General Public License as published by",
  "   the Free Software Foundation, either version 3 of the License, or",
  "   (at your option) any later version.",
  "",
  "   Bash is distributed in the hope that it will be useful,",
  "   but WITHOUT ANY WARRANTY; without even the implied warranty of",
  "   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the",
  "   GNU General Public License for more details.",
  "",
  "   You should have received a copy of the GNU General Public License",
  "   along with Bash.  If not, see <http://www.gnu.org/licenses/>.",
  "*/",
  "",
  "/* The list of shell builtins.  Each element is name, function, flags,",
  "   long-doc, short-doc.  The long-doc field contains a pointer to an array",
  "   of help lines.  The function takes a WORD_LIST *; the first word in the",
  "   list is the first arg to the command.  The list has already had word",
  "   expansion performed.",
  "",
  "   Functions which need to look at only the simple commands (e.g.",
  "   the enable_builtin ()), should ignore entries where",
  "   (array[i].function == nullptr).  Such entries are for",
  "   the list of shell reserved control structures, like `if' and `while'.",
  "   The end of the list is denoted with a nullptr name field. */",
  "",
  "/* TRANSLATORS: Please do not translate command names in descriptions */",
  "",
  "#include \"builtins.hh\"",
  nullptr
};

static const char *structfile_footer[] = { "}", "", "}", nullptr };

/* Write out any necessary opening information for
   STRUCTFILE and EXTERNFILE. */
void
write_file_headers (ofstream &structfile, ofstream &externfile)
{
  if (structfile)
    {
      for (int i = 0; structfile_header[i]; i++)
        structfile << structfile_header[i] << '\n';

      structfile << "#include \""
                 << (extern_filename.empty () ? "builtext.hh"
                                              : extern_filename.c_str ())
                 << "\"\n#include \"shell.hh\"\n"
                 << "\nnamespace bash {\n"
                 << "\n#if __cplusplus < 201103L"
                 << "\n#define emplace(x) insert (x)"
                 << "\n#endif"
                 << "\nvoid\nShell::initialize_shell_builtins ()\n{\n";
    }

  if (externfile.is_open ())
    externfile << "/* "
               << (extern_filename.empty () ? "builtext.hh"
                                            : extern_filename.c_str ())
               << " - The list of builtins found in builtins/_def.cc. */\n";
}

/* Write out any necessary closing information for
   STRUCTFILE and EXTERNFILE. */
void
write_file_footers (ofstream &structfile, ofstream &)
{
  /* Write out the footers. */
  if (structfile)
    {
      for (int i = 0; structfile_footer[i]; i++)
        structfile << structfile_footer[i] << '\n';
    }
}

/* Write out the information accumulated in DEFS to
   STRUCTFILE and EXTERNFILE. */
void
write_builtins (DefFile &defs, ofstream &structfile, ofstream &externfile)
{
  /* Write out the information. */
  if (!defs.builtins.empty ())
    {
      for (BltArray::iterator it = defs.builtins.begin ();
           it != defs.builtins.end (); ++it)
        {
          Builtin *builtin = *it;

          /* Write out any #ifdefs that may be there. */
          if (!only_documentation)
            {
              if (!builtin->dependencies.empty ())
                {
                  write_ifdefs (externfile, builtin->dependencies);
                  write_ifdefs (structfile, builtin->dependencies);
                }

              /* Write the extern definition. */
              if (externfile.is_open ())
                {
                  // these are now method pointers declared in shell.h
                  if (!builtin->function.empty ())
                    externfile << "// extern int Shell::" << builtin->function
                               << " (bash::WORD_LIST *);\n";

                  externfile << "extern char * const "
                             << document_name (*builtin) << "_doc[];\n";
                }

              /* Write the structure definition. */
              if (structfile.is_open ())
                {
                  structfile << "shell_builtins.emplace (std::make_pair (\""
                             << builtin->name << "\", Builtin (";

                  if (!builtin->function.empty () && !inhibit_functions)
                    structfile << "&Shell::" << builtin->function << ", ";
                  else
                    structfile << "nullptr, ";

                  structfile << document_name (*builtin) << "_doc,\n";

                  /* Don't translate short document summaries that are
                     identical to command names */
                  if (!builtin->shortdoc.empty ()
                      && builtin->name == builtin->shortdoc)
                    {
                      if (inhibit_functions)
                        structfile
                            << "     \""
                            << (!builtin->shortdoc.empty () ? builtin->shortdoc
                                                            : builtin->name)
                            << "\", \"" << document_name (*builtin)
                            << "\",\n";
                      else
                        structfile
                            << "     \""
                            << (!builtin->shortdoc.empty () ? builtin->shortdoc
                                                            : builtin->name)
                            << "\", nullptr,\n";
                    }
                  else
                    {
                      if (inhibit_functions)
                        structfile
                            << "     N_(\""
                            << (!builtin->shortdoc.empty () ? builtin->shortdoc
                                                            : builtin->name)
                            << "\"), \"" << document_name (*builtin)
                            << "\",\n";
                      else
                        structfile
                            << "     N_(\""
                            << (!builtin->shortdoc.empty () ? builtin->shortdoc
                                                            : builtin->name)
                            << "\"), nullptr,\n";
                    }
                }

                // Moved flags to the end of the struct, to reduce padding.
                structfile << "BUILTIN_ENABLED | STATIC_BUILTIN"
                             << ((builtin->flags & BUILTIN_FLAG_SPECIAL)
                                     ? " | SPECIAL_BUILTIN"
                                     : "")
                             << ((builtin->flags & BUILTIN_FLAG_ASSIGNMENT)
                                     ? " | ASSIGNMENT_BUILTIN"
                                     : "")
                             << ((builtin->flags & BUILTIN_FLAG_LOCALVAR)
                                     ? " | LOCALVAR_BUILTIN"
                                     : "")
                             << ((builtin->flags & BUILTIN_FLAG_POSIX_BUILTIN)
                                     ? " | POSIX_BUILTIN"
                                     : "");

              structfile << " )));\n";

              if (structfile.is_open () || separate_helpfiles)
                {
                  /* Save away a copy of this builtin for later writing of the
                     long documentation strings. */
                  Builtin *saved_copy = new Builtin (*builtin);
                  saved_builtins.push_back (saved_copy);
                }

              /* Write out the matching #endif, if necessary. */
              if (!builtin->dependencies.empty ())
                {
                  if (externfile.is_open ())
                    write_endifs (externfile, builtin->dependencies);

                  if (structfile.is_open ())
                    write_endifs (structfile, builtin->dependencies);
                }
            }

          if (documentation_file.is_open ())
            {
              documentation_file << "@item " << builtin->name << '\n';
              write_documentation (documentation_file, builtin->longdoc, 0,
                                   TEXINFO);
            }
        }
    }
}

/* Write out the long documentation strings in BUILTINS to STREAM. */
void
write_longdocs (ofstream &stream, const BltArray &builtins)
{
  for (BltArray::const_iterator it = builtins.begin (); it != builtins.end ();
       ++it)
    {
      const Builtin *builtin = *it;

      if (!builtin->dependencies.empty ())
        write_ifdefs (stream, builtin->dependencies);

      /* Write the long documentation strings. */
      const char *dname = document_name (*builtin);
      stream << "char * const " << dname << "_doc[] =";

      if (separate_helpfiles)
        {
          StrArray sarray;
          string tmp (helpfile_directory);
          tmp += '/';
          tmp += dname;
          sarray.push_back (tmp);

          write_documentation (stream, sarray, 0, STRING_ARRAY | HELPFILE);
        }
      else
        write_documentation (stream, builtin->longdoc, 0, STRING_ARRAY);

      if (!builtin->dependencies.empty ())
        write_endifs (stream, builtin->dependencies);

      /* delete the builtin now that we're done with it. */
      delete builtin;
    }
}

/* Write an #ifdef string saying what needs to be defined (or not defined)
   in order to allow compilation of the code that will follow.
   STREAM is the stream to write the information to,
   DEFINES is a null terminated array of define names.
   If a define is preceded by an `!', then the sense of the test is
   reversed. */
void
write_ifdefs (ofstream &stream, const StrArray &defines)
{
  if (!stream.is_open ())
    return;

  stream << "#if ";

  for (StrArray::const_iterator it = defines.begin (); it != defines.end ();
       ++it)
    {
      if ((*it)[0] == '!')
        stream << "!defined (" << it->substr (1) << ')';
      else
        stream << "defined (" << *it << ')';

      if ((it + 1) != defines.end ())
        stream << " && ";
    }

  stream << '\n';
}

/* Write an #endif string saying what defines controlled the compilation
   of the immediately preceding code.
   STREAM is the stream to write the information to.
   DEFINES is a null terminated array of define names. */
void
write_endifs (ofstream &stream, const StrArray &defines)
{
  if (!stream.is_open ())
    return;

  stream << "#endif /* ";

  for (StrArray::const_iterator it = defines.begin (); it != defines.end ();
       ++it)
    {
      stream << *it;

      if ((it + 1) != defines.end ())
        stream << " && ";
    }

  stream << " */\n";
}

/* Write DOCUMENTATION to STREAM, perhaps surrounding it with double-quotes
   and quoting special characters in the string.  Handle special things for
   internationalization (gettext) and the single-string vs. multiple-strings
   issues. */
void
write_documentation (ofstream &stream, const StrArray &documentation,
                     int indentation, int flags)
{
  if (!stream.is_open ())
    return;

  bool string_array = flags & STRING_ARRAY;
  bool filename_p = flags & HELPFILE;

  if (string_array)
    {
      stream << " {\n#if defined (HELP_BUILTIN)\n"; /* } */
      if (single_longdoc_strings)
        {
          if (!filename_p)
            {
              if (!documentation.empty () && !documentation[0].empty ())
                stream << "N_(\"";
              else
                stream
                    << "N_(\" "; /* the empty string translates specially. */
            }
          else
            stream << "\"";
        }
    }

  int base_indent = (string_array && single_longdoc_strings && !filename_p)
                        ? BASE_INDENT
                        : 0;

  bool texinfo = flags & TEXINFO;

  for (StrArray::const_iterator it = documentation.begin ();
       it != documentation.end (); ++it)
    {
      const string &line = *it;

      /* Allow #ifdef's to be written out verbatim, but don't put them into
         separate help files. */
      if (!line.empty () && line[0] == '#')
        {
          if (string_array && !filename_p && !single_longdoc_strings)
            stream << line << '\n';
          continue;
        }

      /* prefix with N_( for gettext */
      if (string_array && !single_longdoc_strings)
        {
          if (!filename_p)
            {
              if (!line.empty ())
                stream << "  N_(\"";
              else
                stream
                    << "  N_(\" "; /* the empty string translates specially. */
            }
          else
            stream << "  \"";
        }

      if (indentation)
        for (int j = 0; j < indentation; j++)
          stream << ' ';

      /* Don't indent the first line, because of how the help builtin works. */
      if (it == documentation.begin ())
        indentation += base_indent;

      if (string_array)
        {
          for (size_t j = 0; j < line.size (); j++)
            {
              switch (line[j])
                {
                case '\\':
                case '"':
                  stream << "\\" << line[j];
                  break;

                default:
                  stream << line[j];
                }
            }

          /* closing right paren for gettext */
          if (!single_longdoc_strings)
            {
              if (!filename_p)
                stream << "\"),\n";
              else
                stream << "\",\n";
            }
          else if ((it + 1) != documentation.end ())
            /* don't add extra newline after last line */
            stream << "\\n\\\n";
        }
      else if (texinfo)
        {
          for (size_t j = 0; j < line.size (); j++)
            {
              switch (line[j])
                {
                case '@':
                case '{':
                case '}':
                  stream << "@" << line[j];
                  break;

                default:
                  stream << line[j];
                }
            }
          stream << '\n';
        }
      else
        stream << line << '\n';
    }

  /* closing right paren for gettext */
  if (string_array && single_longdoc_strings)
    {
      if (!filename_p)
        stream << "\"),\n";
      else
        stream << "\",\n";
    }

  if (string_array)
    stream << "#endif /* HELP_BUILTIN */\n  nullptr\n};\n";
}

static bool
_find_in_table (const char *name, const char *name_table[])
{
  for (int i = 0; name_table[i]; i++)
    if (std::strcmp (name, name_table[i]) == 0)
      return true;

  return false;
}

static bool
is_special_builtin (const string &name)
{
  return _find_in_table (name.c_str (), special_builtins);
}

static bool
is_assignment_builtin (const string &name)
{
  return _find_in_table (name.c_str (), assignment_builtins);
}

static bool
is_localvar_builtin (const string &name)
{
  return _find_in_table (name.c_str (), localvar_builtins);
}

static bool
is_posix_builtin (const string &name)
{
  return _find_in_table (name.c_str (), posix_builtins);
}

} // namespace bash
