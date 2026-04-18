You are helping build a small C99 compiler in incremental stages.

We have already completed Stage 1, which supports a minimal program:

```
int main() { return 42; }
```

Now we are implementing **Stage 2**, which adds support for basic integer expressions.

---

## Context

The current compiler already has:

* tokenizer
* parser
* AST
* code generation
* ability to compile and run a program returning a constant integer

Do NOT rewrite everything from scratch. Extend the existing design.

---

## Stage 2 Specification

(Read and follow this exactly)

See file docs/stages/claude99_spec_stage_02_integer_expressions.md

---

## Goals for this step

Extend the compiler to support:

* integer literals
* binary expressions using +, -, *, /
* parentheses for grouping
* correct operator precedence and left associativity

---

## Implementation Plan (IMPORTANT)

Before writing code:

1. Briefly describe how you will extend:

   * tokenizer
   * AST
   * parser
   * code generation
2. Show the updated AST node structure
3. Show function signatures for:

   * parse_expression
   * parse_term
   * parse_factor

Keep this short and concrete.

---

## Implementation

Then provide code changes in logical sections:

### 1. Tokenizer updates

* new token types
* integer parsing
* setting int_value

### 2. AST updates

* binary operation node definition

### 3. Parser updates

* recursive descent functions:

  * parse_expression
  * parse_term
  * parse_factor

### 4. Code generation

* generating assembly for:

  * addition
  * subtraction
  * multiplication
  * division

Assume x86-64 and follow the same conventions used in Stage 1.

---

## Testing

Create a simple test script that:

* compiles all test `.c` files
* builds executables
* runs them
* checks that exit code == 42
* prints pass/fail results

---

## Constraints

* Do NOT implement:

  * unary operators
  * variables
  * multiple statements
  * anything beyond this spec
* Keep the design simple and consistent with Stage 1
* Prefer clarity over cleverness

---

## Output Format

* Keep explanations short
* Clearly separate code sections
* Provide complete, working snippets (not fragments)

---

If something is ambiguous, choose the simplest reasonable implementation and proceed.
