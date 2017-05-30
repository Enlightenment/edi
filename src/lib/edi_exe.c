#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/wait.h>

#include <Ecore.h>

#include "Edi.h"
#include "edi_private.h"

EAPI int
edi_exe_wait(const char *command)
{
   pid_t pid;
   Ecore_Exe *exe;
   int exit;

   ecore_thread_main_loop_begin();
   exe = ecore_exe_pipe_run(command, ECORE_EXE_USE_SH |
                            ECORE_EXE_PIPE_WRITE, NULL);
   pid = ecore_exe_pid_get(exe);
   ecore_thread_main_loop_end();

   waitpid(pid, &exit, 0);

   ecore_thread_main_loop_begin();
   ecore_exe_free(exe);
   ecore_thread_main_loop_end();

   return exit;
}

