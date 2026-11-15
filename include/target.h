#ifndef CCOMPILER_TARGET_H
#define CCOMPILER_TARGET_H

/* Stage 179: code generation target selected by --target=<name>. */
typedef enum {
    TARGET_X86_64_LINUX,   /* default on Linux: ELF64/NASM */
    TARGET_X86_64_MACOS,   /* Mach-O64/NASM, runs under Rosetta 2 on M1 */
    TARGET_ARM64_MACOS     /* reserved for Phase 1+ */
} Target;

#endif
