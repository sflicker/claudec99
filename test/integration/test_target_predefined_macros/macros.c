/* Stage 180: each identifier survives preprocessing only when its
 * platform macro is predefined for the selected --target. */
#if defined(__linux__)
int has_linux;
#endif
#if defined(__unix__)
int has_unix;
#endif
#if defined(__APPLE__)
int has_apple;
#endif
#if defined(__MACH__)
int has_mach;
#endif
#if defined(__x86_64__)
int has_x86_64;
#endif
#if defined(__aarch64__)
int has_aarch64;
#endif
#if defined(__arm64__)
int has_arm64;
#endif
