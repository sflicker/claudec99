# ClaudeC99 Stage 56-02 Command line macro definitions: -D

## Task
  - add support for defining preprocessor macros from the compiler command line

## In Scope
Support:
```bash
ccocmpiler -DDEBUG file.c
```

## Rules
-DNAME    behaves like #define NAME 1
-DNAME=VAL behaves like #define NAME VAL
-DLEVEL=3   behaves like #define LEVEL 3

These macros should be available before preprocessing the main source file, so the work with

```C
#ifdef DEBUG
#if LEVEL >= 2
```

## Out of scope
  - -U
  - shell-quote edge cases beyond normal argv handling
  - function-like command-line macros
  - multiple include search paths

## Tests
USE INTEGRATION TEST
```C
----- .args ---------
-DLEVEL=3
---------------------

----- base .c file --
#if LEVEL == 3
int main() { return 42; } // expect 42
#else
it main() { return 1; }
#endif
---------------------
```

```C
-------- .args -------
-DDEBUG
----------------------

----- base .c file ---
#ifdef DEBUG
int main() { return 42; } // expect 42
#else
int main() { return 1; }
#endif
```