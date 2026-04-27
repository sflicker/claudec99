# ClaudeC99 Stage 13 Overview

## This is an outline of the stage 13

## Task
  - Extend the compiler to support one-dimensional arrays and basic pointer arithmetic.

## Goals
  - Add local fixed-sized arrays
  - Add array indexing expressions
  - Add pointer arithmetic for supported object pointers
  - Treat array indexing as pointer arithmetic plus dereference

## Stages

### Stage 13.1
  - Array type + local array declarations + stack allocation

### Stage 13.2
  - Array indexing for direct array variables
  Example
  ```C
      int a[3];
      a[0] = 1;
      return a[0]; 
  ```

### Stage 13.3
  - Array-to-pointer decay
  Example
  ```C
      int f(int *p) { 
          return p[0]; 
      }
      
      int main() {
          int a[1];
          a[0] = 7;
          return f(a);
      }  
  ```

### Stage 13.4 
  - Pointer arithmetic scaling

  ```C
      int *p = a;
      *(p + 1) = 8;
  ```

### Stage 13.5
  - Pointer indexing and address-of indexed elements
  Example
  ```C 
      p[1] = 9;
      int *q = &a[1]; 
  ```

### Stage 13.6
  - Arrays of pointers and invalid-case diagnostics

### Stages 13.7 - Optional
  - Pointer difference
