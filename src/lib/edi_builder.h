#ifndef EDI_BUILDER_H_
# define EDI_BUILDER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for Edi building.
 */

/**
 * @brief Main builder management
 * @defgroup Builder
 *
 * @{
 *
 * Functions of build management and execution.
 *
 */

/**
 * Check if Edi can build the current project.
 *
 * @return Whether or not the current project can be built.
 *
 * @see edi_builder_build().
 *
 * @ingroup Builder
 */
EAPI Eina_Bool
edi_builder_can_build(void);

/**
 * Check if Edi can run the current project.
 * This may depend on user configuration which is passed into the method.
 *
 * @return Whether or not the current project has a runnable executable.
 *
 * @see edi_builder_build().
 *
 * @ingroup Builder
 */
EAPI Eina_Bool
edi_builder_can_run(const char *runpath);

/**
 * Run a build for the current project.
 *
 * @see edi_builder_can_build().
 *
 * @ingroup Builder
 */
EAPI void
edi_builder_build(void);

/**
 * Run a test build for the current project.
 *
 * @see edi_builder_can_build().
 *
 * @ingroup Builder
 */
EAPI void
edi_builder_test(void);

/**
 * Run a resulting executable for the current project.
 *
 * @see edi_builder_can_run().
 *
 * @ingroup Builder
 */
EAPI void
edi_builder_run(const char *runpath, const char *args);

/**
 * Run a clean for the current project.
 *
 * @see edi_builder_can_build().
 *
 * @ingroup Builder
 */
EAPI void
edi_builder_clean(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_BUILDER_H_ */
