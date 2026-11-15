/* Stage 180: __unix__ is defined only for the x86_64-linux target; on the
 * x86_64-macos target (__APPLE__) this check passes through.  Explicit
 * per-target coverage lives in test/integration/test_target_predefined_macros. */
#if defined(__APPLE__)
int main() { return 42; }
#elif defined(__unix__)
int main() { return 42; }
#else
int main() { return 1; }
#endif
