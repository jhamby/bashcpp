/*
 * perl builtin
 */
#include "config.h"

#include <fcntl.h>

#include "builtins.hh"
#include "shell.hh"

extern char **make_builtin_argv (WORD_LIST *, int *);
extern char **export_env;

extern void perl_close(void);
extern int perl_main(int, char **, char **);

int
bperl_builtin(WORD_LIST *list)
{
	char	**v;
	int	c, r;

	v = make_builtin_argv(list, &c);
	r = perl_main(c, v, export_env);
	free(v);

	return r;
}

void
bperl_builtin_unload (char *s)
{
	perl_close();
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
