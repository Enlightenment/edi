#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__) || defined (__APPLE__) || defined(__OpenBSD__)
 #include <unistd.h>
 #include <sys/types.h>
 #include <sys/sysctl.h>
 #include <sys/user.h>
#endif
#if defined(__OpenBSD__)
 #include <sys/proc.h>
#endif

#include <Eo.h>
#include <Eina.h>
#include <Elementary.h>

#include "edi_debug.h"
#include "edi_config.h"
#include "edi_private.h"

Edi_Debug_Tool _debugger_tools[] = {
    { "gdb", "gdb", NULL, "run\n", "c\n", "set args %s" },
    { "lldb", "lldb", NULL, "run\n", "c\n", "settings set target.run-args %s" },
    { "pdb", "pdb", NULL, NULL, "c\n", "run %s" },
    { "memcheck", "valgrind", "--tool=memcheck", NULL, NULL, NULL },
    { "massif", "valgrind", "--tool=massif", NULL, NULL, NULL },
    { "callgrind", "valgrind", "--tool=callgrind", NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL, NULL },
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

#if defined(__FreeBSD__) || defined(__DragonFly__)
static long int
_sysctlfromname(const char *name, void *mib, int depth, size_t *len)
{
   long int result;

   if (sysctlnametomib(name, mib, len) < 0) return -1;
   *len = sizeof(result);
   if (sysctl(mib, depth, &result, len, NULL, 0) < 0) return -1;

   return result;
}
#endif

/* Get the process ID of the child process being debugged in *our* session */
int edi_debug_process_id(Edi_Debug *debugger)
{
   int my_pid, child_pid = -1;
#if defined(__FreeBSD__) || defined(__DragonFly__) || defined (__APPLE__) || defined(__OpenBSD__)
   struct kinfo_proc kp;
   int mib[6];
   size_t len;
   int max_pid, i;

   if (!debugger->program_name) return -1;
   if (!debugger->exe) return -1;

#if defined(__FreeBSD__) || defined(__DragonFly__)
   len = sizeof(max_pid);
   max_pid = _sysctlfromname("kern.lastpid", mib, 2, &len);
#elif defined(PID_MAX)
   max_pid = PID_MAX;
#else
   max_pid = 99999;
#endif
   my_pid = ecore_exe_pid_get(debugger->exe);

#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__APPLE__)
   if (sysctlnametomib("kern.proc.pid", mib, &len) < 0) return -1;
#elif defined(__OpenBSD__)
   mib[0] = CTL_KERN;
   mib[1] = KERN_PROC;
   mib[2] = KERN_PROC_PID;
   mib[4] = sizeof(struct kinfo_proc);
   mib[5] = 1;
#endif

   for (i = my_pid; i <= max_pid; i++)
     {
        mib[3] = i;
        len = sizeof(kp);
#if defined(__OpenBSD__)
        if (sysctl(mib, 6, &kp, &len, NULL, 0) == -1) continue;
#else
        if (sysctl(mib, 4, &kp, &len, NULL, 0) == -1) continue;
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__)
        if (kp.ki_ppid != my_pid) continue;
        if (strcmp(debugger->program_name, kp.ki_comm)) continue;
        child_pid = kp.ki_pid;
        if (kp.ki_stat == SRUN || kp.ki_stat == SSLEEP)
          debugger->state = EDI_DEBUG_PROCESS_ACTIVE;
        else
          debugger->state = EDI_DEBUG_PROCESS_SLEEPING;
#elif defined(__OpenBSD__)
        if (kp.p_ppid != my_pid) continue;
        if (strcmp(debugger->program_name, kp.p_comm)) continue;
        child_pid = kp.p_pid;

        if (kp.p_stat == SRUN || kp.p_stat == SSLEEP)
          debugger->state = EDI_DEBUG_PROCESS_ACTIVE;
        else
          debugger->state = EDI_DEBUG_PROCESS_SLEEPING;
#else /* APPLE */
        if (kp.kp_proc.p_oppid != my_pid) continue;
        if (strcmp(debugger->program_name, kp.kp_proc.p_comm)) continue;
        child_pid = kp.kp_proc.p_pid;
        if (kp.kp_proc.p_stat == SRUN || kp.kp_proc.p_stat == SSLEEP)
          debugger->state = EDI_DEBUG_PROCESS_ACTIVE;
        else
          debugger->state = EDI_DEBUG_PROCESS_SLEEPING;
#endif
        break;
     }
#else
   Eina_List *files, *l;
   const char *temp_name;
   char path[PATH_MAX];
   char buf[4096];
   char  *p, *name, *end;
   FILE *f;
   int count, parent_pid, pid;

   if (!debugger->program_name)
     return -1;

   if (!debugger->exe) return -1;

   my_pid = ecore_exe_pid_get(debugger->exe);

   files = ecore_file_ls("/proc");

   EINA_LIST_FOREACH(files, l, name)
     {
        pid = atoi(name);
        if (!pid) continue;
        snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
        f = fopen(path, "r");
        if (!f) continue;
        p = fgets(buf, sizeof(buf), f);
        fclose(f);
        if (!p) continue;
        temp_name = ecore_file_file_get(buf);
        if (!strcmp(temp_name, debugger->program_name))
          {
             parent_pid = 0;
             // Match success - program name with pid.
             child_pid = pid;
             snprintf(path, sizeof(path), "/proc/%d/stat", pid);
             f = fopen(path, "r");
             if (f)
               {
                  count = 0;
                  p = fgets(buf, sizeof(buf), f);
                  while (*p++ != '\0')
                    {
                       if (p[0] == ' ') { count++; p++; }
                       if (count == 2)
                         {
                            if (p[0] == 'S' || p[0] == 'R')
                              debugger->state = EDI_DEBUG_PROCESS_ACTIVE;
                            else
                              debugger->state = EDI_DEBUG_PROCESS_SLEEPING;
                         }
                       if (count == 3) break;
                    }
                  end = strchr(p, ' ');
                  if (end)
                    {
                       *end = '\0';
                       // parent pid matches - right process.
                       parent_pid = atoi(p);
                    }
                  fclose(f);
               }
             if (parent_pid == my_pid)
               break;
          }
     }

   if (files)
     eina_list_free(files);
#endif
   return child_pid;
}

