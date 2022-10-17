/* template - example template for loadable builtin */

/* See Makefile for compilation details. */

#include "config.h"

#include "loadables.hh"

namespace bash
{

// Loadable class for "template".
class ShellLoadable : public Shell
{
public:
  int template_builtin (WORD_LIST *);

private:
};

int
ShellLoadable::template_builtin (WORD_LIST *list)
{
  int opt, rval;

  rval = EXECUTION_SUCCESS;
  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "")) != -1)
    {
      switch (opt)
        {
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }
  list = loptend;

  return rval;
}

/* Called when `template' is enabled and loaded from the shared object.  If
   this function returns 0, the load fails. */
int
template_builtin_load (const char *name)
{
  return 1;
}

/* Called when `template' is disabled. */
void
template_builtin_unload (const char *name)
{
}

static const char *const template_doc[]
    = { "Short description.",
        ""
        "Longer description of builtin and usage.",
        nullptr };

Shell::builtin template_struct (
    static_cast<Shell::sh_builtin_func_t> (&ShellLoadable::template_builtin),
    template_doc, "template", nullptr, BUILTIN_ENABLED);

} // namespace bash
