/* Stage 180: under the host's default target exactly one OS family is
 * predefined, and each family's macros come as a pair:
 *   x86_64-linux: __linux__ and __unix__
 *   x86_64-macos: __APPLE__ and __MACH__ */
int main() {
#if defined(__linux__) && defined(__APPLE__)
    return 1;
#elif !defined(__linux__) && !defined(__APPLE__)
    return 2;
#elif defined(__APPLE__) != defined(__MACH__)
    return 3;
#elif defined(__linux__) != defined(__unix__)
    return 4;
#elif !defined(__x86_64__)
    return 5;
#else
    return 42;
#endif
}
