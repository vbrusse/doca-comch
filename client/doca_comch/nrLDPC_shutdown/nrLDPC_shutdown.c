/*
 * Component: nrLDPC_shutdown
 *
 * Date: 2025/07/28
 *
 */

#include <stdint.h>

// ALIAS DECLARATION
// LDPCshutdown declared as an alias for nrLDPC_encod
extern int32_t LDPCshutdown(void)
        __attribute__((alias("nrLDPC_shutdown")));
// END ALIAS

/*
 * nrLDPC_shutdown - This is a function from OAI interface when using the interface 'LDPC segment coding'.
 *
 * This function terminates the new 5G NR LDPC function required by OAI interface in the libldpc_armral.so
 * library as required to be dynamically loaded at run-time by the oai shared library loader.
 * Currentely there is nothing to do (dummy function).
 *
 * @return: EXIT_SUCCESS on success and EXIT_FAILURE otherwise
 */
int32_t nrLDPC_shutdown(void)
{

        /* Library-specific initialization logic */

        return 0;                            /* Return 0 on success, other values on failure */
}
