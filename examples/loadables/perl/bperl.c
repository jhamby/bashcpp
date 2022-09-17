/*
 * perl builtin
 */
#include "config.hh"

#include <fcntl.h>

#include "builtins.hh"
#include "shell.hh"

extern char **make_builtin_argv ();
extern char **export_env;

extern int perl_main();

bperl_builtin(list)
WORD_LIST *list;
{
	char	**v;
	int	c, r;

	v = make_builtin_argv(list, &c);
	r = perl_main(c, v, export_env);
	free(v);

	return r;
}

char *bperl_doc[] = {
	"An interface to a perl5 interpreter.",
	(char *)0
};

struct builtin bperl_struct = {
	"bperl",
	bperl_builtin,
	BUILTIN_ENABLED,
	bperl_doc,
	"bperl [perl options] [file ...]",
	0
};
