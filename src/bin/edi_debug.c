#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__OpenBSD__)
 #include <unistd.h>
 #include <sys/types.h>
 #include <sys/sysctl.h>
 #include <sys/user.h>
#endif

#if defined(__OpenBSD__)
# include <kvm.h>
# include <limits.h>
# include <sys/proc.h>
# include <sys/param.h>
# include <sys/resource.h>
#endif

#if defined(__linux__)
# include <sys/types.h>
# include <dirent.h>
# include <ctype.h>
#endif

#include <Eo.h>
#include <Eina.h>
#include <Elementary.h>

#include "edi_debug.h"
#include "edi_process.h"
#include "edi_config.h"
#include "edi_private.h"

Edi_Debug_Tool _debugger_tools[] = {
    { "gdb", "gdb", NULL, "run\n", "c\n", "set args %s\n", "set follow-fork-mode child\n"},
    { "lldb", "lldb", NULL, "run\n", "c\n", "settings set target.run-args %s", NULL },
    { "pdb", "pdb", NULL, NULL, "c\n", "run %s", NULL },
    { "memcheck", "valgrind", "--tool=memcheck", NULL, NULL, NULL, NULL },
    { "massif", "valgrind", "--tool=massif", NULL, NULL, NULL, NULL },
    { "callgrind", "valgrind", "--tool=callgrind", NULL, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL },
};

Edi_Debug *_debugger = NULL;

Edi_Debug *edi_debug_new(void)
{
   _debugger = calloc(1, sizeof(Edi_Debug));
   return _debugger;
}

Edi_Debug *edi_debug_get(void)
{
   return _debugger;
}

Edi_Debug_Tool *edi_debug_tools_get(void)
{
   return _debugger_tools;
}

Edi_Debug_Tool *edi_debug_tool_get(const char *name)
{
   int i;

   for (i = 0; _debugger_tools[i].name && name; i++)
      {
         if (!strcmp(_debugger_tools[i].name, name) &&
             ecore_file_app_installed(_debugger_tools[i].exec))
           return &_debugger_tools[i];
      }

    // Fallback, but not installed.
    if (name || !ecore_file_app_installed(_debugger_tools[0].exec))
      return NULL;

    // Fallback to first.
    return &_debugger_tools[0];
}

static int
_system_pid_max_get(void)
{
   static int pid_max = 0;

   if (pid_max > 0)
    return pid_max;

#if defined(__linux__)
   FILE *f;
   char buf[128];
   size_t n;

   f = fopen("/proc/sys/kernel/pid_max", "r");
   if (f)
     {
        n = fread(buf, 1, sizeof(buf) - 1, f);
        buf[n] = 0x00;
        fclose(f);
        pid_max = atoi(buf);
     }
#elif defined(PID_MAX)
   pid_max = PID_MAX;
#endif
   if (pid_max <= 0)
     pid_max = 99999;

   return pid_max;
}

/* Get the process ID of the child process being debugged in *our* session */
int edi_debug_process_id(Edi_Debug *debugger)
{
   Edi_Proc_Stats *p;
   int pid_max, debugger_pid, child_pid = -1;

   if (!debugger) return -1;
   if (!debugger->program_name) return -1;
   if (!debugger->exe) return -1;

   debugger_pid = ecore_exe_pid_get(debugger->exe);
#if defined(__linux__)
   DIR *dir;
   struct dirent *dh;

   dir = opendir("/proc");
   if (!dir) return -1;

   while ((dh = readdir(dir)))
     {
        const char *c = dh->d_name;
        while (*c)
          {
             if (!isdigit(*c)) break;
             ++c;
          }

        if (*c) continue;

        p = edi_process_stats_by_pid(atoi(dh->d_name));
        if (!p) continue;

        if (p->ppid == debugger_pid)
          {
             if (!strcmp(debugger->program_name, p->command))
               {
                  child_pid = p->pid;
                  if (!strcmp(p->state, "RUN") ||!strcmp(p->state, "SLEEP"))
                    debugger->state = EDI_DEBUG_PROCESS_ACTIVE;
                  else
                    debugger->state = EDI_DEBUG_PROCESS_SLEEPING;
               }
          }

        free(p);

        if (child_pid != -1)
          break;
     }

   closedir(dir);

   return child_pid;
#elif defined(__OpenBSD__)
   kvm_t *kern;
   struct kinfo_proc *kp;
   char errbuf[4096];
   int pid_count;

   kern = kvm_openfiles(NULL, NULL, NULL, KVM_NO_FILES, errbuf);
   if (!kern) return -1;

   kp = kvm_getprocs(kern, KERN_PROC_ALL, 0, sizeof(*kp), &pid_count);
   if (!kp) return -1;

   for (int i = 0; i < pid_count; i++)
     {
        p = edi_process_stats_by_pid(kp[i].p_pid);
        if (!p) continue;

        if (p->ppid == debugger_pid)
          {
             if (!strcmp(debugger->program_name, p->command))
               {
                  child_pid = p->pid;
                  if (!strcmp(p->state, "RUN") ||!strcmp(p->state, "SLEEP"))
                    debugger->state = EDI_DEBUG_PROCESS_ACTIVE;
                  else
                    debugger->state = EDI_DEBUG_PROCESS_SLEEPING;
               }
          }

        free(p);

        if (child_pid != -1)
          break;
     }

   kvm_close(kern);

   return child_pid;
#endif

   pid_max = _system_pid_max_get();

   for (int i = 1; i <= pid_max; i++)
     {
        p = edi_process_stats_by_pid(i);
        if (!p) continue;

        if (p->ppid == debugger_pid)
          {
             if (!strcmp(debugger->program_name, p->command))
               {
                  child_pid = p->pid;
                  if (!strcmp(p->state, "RUN") ||!strcmp(p->state, "SLEEP"))
                    debugger->state = EDI_DEBUG_PROCESS_ACTIVE;
                  else
                    debugger->state = EDI_DEBUG_PROCESS_SLEEPING;
               }
          }

        free(p);

        if (child_pid != -1)
          break;
     }

   return child_pid;
}

