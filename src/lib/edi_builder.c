#ifdef HAVE_CONFIG
# include "config.h"
#endif

#include "Edi.h"
#include "edi_builder.h"

#include "edi_private.h"

EAPI Eina_Bool
edi_builder_can_build(void)
{
   return edi_project_file_exists("Makefile") ||
          edi_project_file_exists("configure") ||
          edi_project_file_exists("autogen.sh");
}

EAPI void
_edi_builder_build_make(void)
{
   chdir(edi_project_get());
   ecore_exe_pipe_run("make", ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR, NULL);
}

EAPI void
_edi_builder_build_configure(void)
{
   chdir(edi_project_get());
   ecore_exe_pipe_run("./configure && make", ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR, NULL);
}

EAPI void
_edi_builder_build_autogen(void)
{
   chdir(edi_project_get());
   ecore_exe_pipe_run("./autogen.sh && make", ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR, NULL);
}

EAPI void
edi_builder_build(void)
{
   if (edi_project_file_exists("Makefile"))
     _edi_builder_build_make();
   else if (edi_project_file_exists("configure"))
     _edi_builder_build_configure();
   else if (edi_project_file_exists("autogen.sh"))
     _edi_builder_build_autogen();
}

EAPI void
edi_builder_test(void)
{
   chdir(edi_project_get());
   ecore_exe_pipe_run("make check", ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR, NULL);
}

EAPI void
edi_builder_clean(void)
{
   chdir(edi_project_get());
   ecore_exe_pipe_run("make clean", ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_READ |
                              ECORE_EXE_PIPE_ERROR_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR, NULL);
}


