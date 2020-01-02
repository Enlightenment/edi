#ifndef EDI_DEBUG_H_
# define EDI_DEBUG_H_

#include <Elementary.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for managing debugging features.
 */

typedef enum {
   EDI_DEBUG_PROCESS_SLEEPING = 0,
   EDI_DEBUG_PROCESS_ACTIVE
} Edi_Debug_Process_State;

typedef struct _Edi_Debug_Tool {
   const char *name;
   const char *exec;
   const char *arguments;
   const char *command_start;
   const char *command_continue;
   const char *command_arguments;
   const char *command_settings;
} Edi_Debug_Tool;

typedef struct _Edi_Debug {
   Edi_Debug_Tool *tool;
   const char *program_name;
   Ecore_Exe *exe;
   char cmd[1024];
   Edi_Debug_Process_State state;
} Edi_Debug;

/**
 * @brief Debug management functions.
 * @defgroup Debug
 *
 * @{
 *
 * Initialisation and management of debugging features.
 *
 */

/**
 * Initialise debugger internals.
 *
 * @return pointer to initialized debug instance.
 *
 * @ingroup Debug
 */
Edi_Debug *edi_debug_new(void);

/**
 * Obtain pointer to initialized debug instance.
 *
 * @ingroup Debug
 */
Edi_Debug *edi_debug_get(void);

/**
 * Obtain process information of debugged process.
 *
 * @param debugger Edi_Debug instance.
 *
 * @return process id of debugged process that is child of running debugger.
 *
 * @ingroup Debug
 */
int edi_debug_process_id(Edi_Debug *debugger);

/**
 * Obtain debugging info for given program name.
 *
 * @param name The name of the tool used to obtain helper data for given program.
 *
 * @return Pointer to debugging information instance associated with its name.
 *
 * @ingroup Debug
 */
Edi_Debug_Tool *edi_debug_tool_get(const char *name);

/**
 * Return a pointer to the list of available debugging tools.
 *
 * @return Pointer to debugging information for all available tools.
 *
 * @ingroup Debug
 */
Edi_Debug_Tool *edi_debug_tools_get(void);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* EDI_DEBUG_H_ */
