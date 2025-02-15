// This file is reserved_def.cc, in which the shell reserved words are defined.
// It has no direct C file production, but defines builtins for the Bash
// builtin help command.

// Copyright (C) 1987-2019 Free Software Foundation, Inc.

// This file is part of GNU Bash, the Bourne Again SHell.

// Bash is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Bash is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Bash.  If not, see <http://www.gnu.org/licenses/>.

// $BUILTIN for
// $SHORT_DOC for NAME [in WORDS ... ] ; do COMMANDS; done
// Execute commands for each member in a list.

// The `for' loop executes a sequence of commands for each member in a
// list of items.  If `in WORDS ...;' is not present, then `in "$@"' is
// assumed.  For each element in WORDS, NAME is set to that element, and
// the COMMANDS are executed.

// Exit Status:
// Returns the status of the last command executed.
// $END

// $BUILTIN for ((
// $DOCNAME arith_for
// $SHORT_DOC for (( exp1; exp2; exp3 )); do COMMANDS; done
// Arithmetic for loop.

// Equivalent to
// 	(( EXP1 ))
// 	while (( EXP2 )); do
// 		COMMANDS
// 		(( EXP3 ))
// 	done
// EXP1, EXP2, and EXP3 are arithmetic expressions.  If any expression is
// omitted, it behaves as if it evaluates to 1.

// Exit Status:
// Returns the status of the last command executed.
// $END

// $BUILTIN select
// $SHORT_DOC select NAME [in WORDS ... ;] do COMMANDS; done
// Select words from a list and execute commands.

// The WORDS are expanded, generating a list of words.  The
// set of expanded words is printed on the standard error, each
// preceded by a number.  If `in WORDS' is not present, `in "$@"'
// is assumed.  The PS3 prompt is then displayed and a line read
// from the standard input.  If the line consists of the number
// corresponding to one of the displayed words, then NAME is set
// to that word.  If the line is empty, WORDS and the prompt are
// redisplayed.  If EOF is read, the command completes.  Any other
// value read causes NAME to be set to null.  The line read is saved
// in the variable REPLY.  COMMANDS are executed after each selection
// until a break command is executed.

// Exit Status:
// Returns the status of the last command executed.
// $END

// $BUILTIN time
// $SHORT_DOC time [-p] pipeline
// Report time consumed by pipeline's execution.

// Execute PIPELINE and print a summary of the real time, user CPU time,
// and system CPU time spent executing PIPELINE when it terminates.

// Options:
//   -p	print the timing summary in the portable Posix format

// The value of the TIMEFORMAT variable is used as the output format.

// Exit Status:
// The return status is the return status of PIPELINE.
// $END

// $BUILTIN case
// $SHORT_DOC case WORD in [PATTERN [| PATTERN]...) COMMANDS ;;]... esac
// Execute commands based on pattern matching.

// Selectively execute COMMANDS based upon WORD matching PATTERN.  The
// `|' is used to separate multiple patterns.

// Exit Status:
// Returns the status of the last command executed.
// $END

// $BUILTIN if
// $SHORT_DOC if COMMANDS; then COMMANDS; [ elif COMMANDS; then COMMANDS; ]... [ else COMMANDS; ] fi
// Execute commands based on conditional.

// The `if COMMANDS' list is executed.  If its exit status is zero, then the
// `then COMMANDS' list is executed.  Otherwise, each `elif COMMANDS' list is
// executed in turn, and if its exit status is zero, the corresponding
// `then COMMANDS' list is executed and the if command completes.  Otherwise,
// the `else COMMANDS' list is executed, if present.  The exit status of the
// entire construct is the exit status of the last command executed, or zero
// if no condition tested true.

// Exit Status:
// Returns the status of the last command executed.
// $END

// $BUILTIN while
// $SHORT_DOC while COMMANDS; do COMMANDS; done
// Execute commands as long as a test succeeds.

// Expand and execute COMMANDS as long as the final command in the
// `while' COMMANDS has an exit status of zero.

// Exit Status:
// Returns the status of the last command executed.
// $END

// $BUILTIN until
// $SHORT_DOC until COMMANDS; do COMMANDS; done
// Execute commands as long as a test does not succeed.

// Expand and execute COMMANDS as long as the final command in the
// `until' COMMANDS has an exit status which is not zero.

// Exit Status:
// Returns the status of the last command executed.
// $END

// $BUILTIN coproc
// $SHORT_DOC coproc [NAME] command [redirections]
// Create a coprocess named NAME.

// Execute COMMAND asynchronously, with the standard output and standard
// input of the command connected via a pipe to file descriptors assigned
// to indices 0 and 1 of an array variable NAME in the executing shell.
// The default NAME is "COPROC".

// Exit Status:
// The coproc command returns an exit status of 0.
// $END

// $BUILTIN function
// $SHORT_DOC function name { COMMANDS ; } or name () { COMMANDS ; }
// Define shell function.

// Create a shell function named NAME.  When invoked as a simple command,
// NAME runs COMMANDs in the calling shell's context.  When NAME is invoked,
// the arguments are passed to the function as $1...$n, and the function's
// name is in $FUNCNAME.

// Exit Status:
// Returns success unless NAME is readonly.
// $END

// $BUILTIN { ... }
// $DOCNAME grouping_braces
// $SHORT_DOC { COMMANDS ; }
// Group commands as a unit.

// Run a set of commands in a group.  This is one way to redirect an
// entire set of commands.

// Exit Status:
// Returns the status of the last command executed.
// $END

// $BUILTIN %
// $DOCNAME fg_percent
// $SHORT_DOC job_spec [&]
// Resume job in foreground.

// Equivalent to the JOB_SPEC argument to the `fg' command.  Resume a
// stopped or background job.  JOB_SPEC can specify either a job name
// or a job number.  Following JOB_SPEC with a `&' places the job in
// the background, as if the job specification had been supplied as an
// argument to `bg'.

// Exit Status:
// Returns the status of the resumed job.
// $END

// $BUILTIN (( ... ))
// $DOCNAME arith
// $SHORT_DOC (( expression ))
// Evaluate arithmetic expression.

// The EXPRESSION is evaluated according to the rules for arithmetic
// evaluation.  Equivalent to `let "EXPRESSION"'.

// Exit Status:
// Returns 1 if EXPRESSION evaluates to 0; returns 0 otherwise.
// $END

// $BUILTIN [[ ... ]]
// $DOCNAME conditional
// $SHORT_DOC [[ expression ]]
// Execute conditional command.

// Returns a status of 0 or 1 depending on the evaluation of the conditional
// expression EXPRESSION.  Expressions are composed of the same primaries used
// by the `test' builtin, and may be combined using the following operators:

//   ( EXPRESSION )	Returns the value of EXPRESSION
//   ! EXPRESSION		True if EXPRESSION is false; else false
//   EXPR1 && EXPR2	True if both EXPR1 and EXPR2 are true; else false
//   EXPR1 || EXPR2	True if either EXPR1 or EXPR2 is true; else false

// When the `==' and `!=' operators are used, the string to the right of
// the operator is used as a pattern and pattern matching is performed.
// When the `=~' operator is used, the string to the right of the operator
// is matched as a regular expression.

// The && and || operators do not evaluate EXPR2 if EXPR1 is sufficient to
// determine the expression's value.

// Exit Status:
// 0 or 1 depending on value of EXPRESSION.
// $END

// $BUILTIN variables
// $DOCNAME variable_help
// $SHORT_DOC variables - Names and meanings of some shell variables
// Common shell variable names and usage.

// BASH_VERSION	Version information for this Bash.
// CDPATH	A colon-separated list of directories to search
// 		for directories given as arguments to `cd'.
// GLOBIGNORE	A colon-separated list of patterns describing filenames to
// 		be ignored by pathname expansion.
#if defined(HISTORY)
// HISTFILE	The name of the file where your command history is stored.
// HISTFILESIZE	The maximum number of lines this file can contain.
// HISTSIZE	The maximum number of history lines that a running
// 		shell can access.
#endif /* HISTORY */
// HOME	The complete pathname to your login directory.
// HOSTNAME	The name of the current host.
// HOSTTYPE	The type of CPU this version of Bash is running under.
// IGNOREEOF	Controls the action of the shell on receipt of an EOF
// 		character as the sole input.  If set, then the value
// 		of it is the number of EOF characters that can be seen
// 		in a row on an empty line before the shell will exit
// 		(default 10).  When unset, EOF signifies the end of input.
// MACHTYPE	A string describing the current system Bash is running on.
// MAILCHECK	How often, in seconds, Bash checks for new mail.
// MAILPATH	A colon-separated list of filenames which Bash checks
// 		for new mail.
// OSTYPE	The version of Unix this version of Bash is running on.
// PATH	A colon-separated list of directories to search when
// 		looking for commands.
// PROMPT_COMMAND	A command to be executed before the printing of each
// 		primary prompt.
// PS1		The primary prompt string.
// PS2		The secondary prompt string.
// PWD		The full pathname of the current directory.
// SHELLOPTS	A colon-separated list of enabled shell options.
// TERM	The name of the current terminal type.
// TIMEFORMAT	The output format for timing statistics displayed by the
// 		`time' reserved word.
// auto_resume	Non-null means a command word appearing on a line by
// 		itself is first looked for in the list of currently
// 		stopped jobs.  If found there, that job is foregrounded.
// 		A value of `exact' means that the command word must
// 		exactly match a command in the list of stopped jobs.  A
// 		value of `substring' means that the command word must
// 		match a substring of the job.  Any other value means that
// 		the command must be a prefix of a stopped job.
#if defined(HISTORY)
#if defined(BANG_HISTORY)
// histchars	Characters controlling history expansion and quick
// 		substitution.  The first character is the history
// 		substitution character, usually `!'.  The second is
// 		the `quick substitution' character, usually `^'.  The
// 		third is the `history comment' character, usually `#'.
#endif /* BANG_HISTORY */
// HISTIGNORE	A colon-separated list of patterns used to decide which
// 		commands should be saved on the history list.
#endif /* HISTORY */
// $END
