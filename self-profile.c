#define _GNU_SOURCE
#include "self-profile.h"

/*
 * Reference:
 *  Hook main() using LD_PRELOAD:
 *  - https://gist.github.com/apsun/1e144bf7639b22ff0097171fa0f8c6b1
 *  Using makefile, LD_PRELOAD to executable file:
 *  - https://stackoverflow.com/questions/30594838/using-makefile-ld-preload-to-executable-file
 */

/* Trampoline for the real main() */
static int (*main_orig)(int, char **, char **);

/* main() that gets called by __libc_start_main() */
int main_hook(int argc, char **argv, char **envp)
{
    PROFILE_BEGIN();
    printf("Sorting...\n");
    PROFILE_START();
    int ret = main_orig(argc, argv, envp);
    PROFILE_STOP();
    /*
     * I'm a bit concerned about this part of the code.
     * It would be great if we could use the output section universally like main function.
     * I'll think about it a bit more.
     */
    PROFILE_REPORT();
    PROFILE_END();

    return ret;
}

/*
 * Wrapper for __libc_start_main() that replaces the real main
 * function with our hooked version.
 */
int __libc_start_main(
    int (*main)(int, char **, char **),
    int argc,
    char **argv,
    int (*init)(int, char **, char **),
    void (*fini)(void),
    void (*rtld_fini)(void),
    void *stack_end)
{
    /* Save the real main function address */
    main_orig = main;

    /* Find the real __libc_start_main()... */
    typeof(&__libc_start_main) orig = dlsym(RTLD_NEXT, "__libc_start_main");

    /* ... and call it with our custom main function */
    return orig(main_hook, argc, argv, init, fini, rtld_fini, stack_end);
}

