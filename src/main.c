/*
 * Compiler/bootstrap entry point.
 *
 * This is intentionally not wired to an ST17H66B startup vector yet. The first
 * repository milestone is to validate the GCC/CMake build path before importing
 * or recreating the target-specific startup, linker, ROM and BLE contracts.
 */

void melb_application_entry(void)
{
    for (;;) {
        /* Hardware bring-up code will replace this bootstrap loop. */
    }
}
