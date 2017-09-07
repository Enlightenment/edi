#ifndef EDI_EXE_H_
# define EDI_EXE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for Edi executable management.
 */

/**
 * @brief Executable helpers
 * @defgroup Exe
 *
 * @{
 *
 * Functions of executable management.
 *
 */

/**
 * Run an executable command and wait for it to return.
 *
 * @param command The command to execute in a child process.
 * @return The return code of the executable.
 *
 * @ingroup Exe
 */
EAPI int edi_exe_wait(const char *command);

/**
 * Run an executable command and return command string.
 *
 * @param command The command to execute in a child process.
 * @return The output string of the command.
 *
 * @ingroup Exe
 */
EAPI char *edi_exe_response(const char *command);

/**
 * Run an executable command with notifcation enabled.
 *
 * @param name The name of the resource used to identify the notification.
 * @param command The command to execute in a child process.
 *
 * @ingroup Exe
 */
EAPI void edi_exe_notify(const char *name, const char *command);

/**
 * This function is used to set a callback that will execute when an
 * edi_exe_response has terminated and supplied its exit status.
 *
 * @param name The name of the resource used to identify the notification.
 * @param func Function that will execute upon receiving exit code of exe.
 * @param data Additional data to pass to the callback.
 *
 * @ingroup Exe
 */
EAPI Eina_Bool edi_exe_notify_handle(const char *name, void ((*func)(int, void *)), void *data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_EXE_H_ */
