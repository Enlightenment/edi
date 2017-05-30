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
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_EXE_H_ */
