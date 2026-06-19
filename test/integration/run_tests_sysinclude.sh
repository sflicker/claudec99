#!/bin/bash
#
# System-header integration test runner.  Identical to run_tests.sh but uses
# GCC system include paths instead of the project's test/include stubs.
# Not part of the standard build.sh CI run — use manually to validate that
# tests whose only obstacle was a preprocessor expression gap now pass with
# real system headers.
#
# Expected results after stage 139:
#   ~97 tests pass (all except those with unrelated failures documented in
#   the stage-139 notes: angle-include, recursive-include, intentional errors,
#   const-discard, and one signature-mismatch test).
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
COMPILER="$PROJECT_DIR/build/ccompiler"
WORK_DIR="$PROJECT_DIR/build/test_tmp_integration_sysinclude"
TIMEOUT=${CLAUDEC99_TEST_TIMEOUT:-30}
DEFAULT_IFLAGS=(
    "-I/usr/lib/gcc/x86_64-linux-gnu/13/include"
    "-I/usr/local/include"
    "-I/usr/include/x86_64-linux-gnu"
    "-I/usr/include"
)

mkdir -p "$WORK_DIR"

pass=0
fail=0
total=0

for test_dir in "$SCRIPT_DIR"/*/; do
    [ -d "$test_dir" ] || continue
    name=$(basename "$test_dir")

    if [ ! -f "$test_dir/${name}.c" ]; then
        if [ -f "$test_dir/run_test.sh" ]; then
            total=$((total + 1))
            if bash "$test_dir/run_test.sh" > /dev/null 2>&1; then
                pass=$((pass + 1))
            else
                fail=$((fail + 1))
                echo "FAIL  $name"
            fi
        fi
        continue
    fi

    total=$((total + 1))

    SOURCE_DIR="$test_dir"
    BASENAME="$name"

    EXPECTED_FILE="$SOURCE_DIR/${BASENAME}.expected"
    LIBS_FILE="$SOURCE_DIR/${BASENAME}.libs"
    CFLAGS_FILE="$SOURCE_DIR/${BASENAME}.cflags"
    ARGS_FILE="$SOURCE_DIR/${BASENAME}.args"
    INPUT_FILE="$SOURCE_DIR/${BASENAME}.input"
    STATUS_FILE="$SOURCE_DIR/${BASENAME}.status"
    ERROR_FILE="$SOURCE_DIR/${BASENAME}.error"

    EXTRA_CFLAGS=()
    if [ -f "$CFLAGS_FILE" ]; then
        while IFS= read -r flag; do
            EXTRA_CFLAGS+=("$flag")
        done < "$CFLAGS_FILE"
    fi

    EXTRA_LIBS=()
    if [ -f "$LIBS_FILE" ]; then
        while IFS= read -r lib; do
            EXTRA_LIBS+=("$lib")
        done < "$LIBS_FILE"
    fi

    EXPECTED_STATUS=0
    if [ -f "$STATUS_FILE" ]; then
        EXPECTED_STATUS=$(cat "$STATUS_FILE")
    fi

    C_FILES=("$test_dir"*.c)

    COMPILE_OUT=""
    COMPILE_RC=0
    ASM_FILES=()
    OBJ_FILES=()

    for src in "${C_FILES[@]}"; do
        base=$(basename "$src" .c)
        asm="$WORK_DIR/${name}_${base}.asm"
        obj="$WORK_DIR/${name}_${base}.o"
        COMPILE_OUT=$("$COMPILER" "${EXTRA_CFLAGS[@]}" "${DEFAULT_IFLAGS[@]}" "$src" -o "$asm" 2>&1)
        COMPILE_RC=$?
        if [ $COMPILE_RC -ne 0 ]; then break; fi
        ASM_FILES+=("$asm")
        OBJ_FILES+=("$obj")
    done

    if [ -f "$ERROR_FILE" ]; then
        if [ $COMPILE_RC -eq 0 ]; then
            fail=$((fail + 1))
            echo "FAIL  $name  (expected compile error, but compilation succeeded)"
            continue
        fi
        expected_err=$(cat "$ERROR_FILE")
        if echo "$COMPILE_OUT" | grep -qF "$expected_err"; then
            pass=$((pass + 1))
        else
            fail=$((fail + 1))
            echo "FAIL  $name  (wrong compile error)"
        fi
        continue
    fi

    if [ $COMPILE_RC -ne 0 ]; then
        fail=$((fail + 1))
        echo "FAIL  $name  (compiler error)"
        continue
    fi

    nasm_ok=1
    for asm in "${ASM_FILES[@]}"; do
        base=$(basename "$asm" .asm)
        obj="$WORK_DIR/${base}.o"
        if ! nasm -f elf64 "$asm" -o "$obj" 2>/dev/null; then
            nasm_ok=0; break
        fi
        OBJ_FILES+=("$obj")
    done

    if [ $nasm_ok -eq 0 ]; then
        fail=$((fail + 1))
        echo "FAIL  $name  (nasm error)"
        continue
    fi

    BIN="$WORK_DIR/${name}.out"
    if ! cc -no-pie "${OBJ_FILES[@]}" -o "$BIN" "${EXTRA_LIBS[@]}" 2>/dev/null; then
        fail=$((fail + 1))
        echo "FAIL  $name  (link error)"
        continue
    fi

    RUN_ARGS=()
    if [ -f "$ARGS_FILE" ]; then
        while IFS= read -r arg; do
            RUN_ARGS+=("$arg")
        done < "$ARGS_FILE"
    fi

    if [ -f "$INPUT_FILE" ]; then
        ACTUAL_OUTPUT=$(timeout "$TIMEOUT" "$BIN" "${RUN_ARGS[@]}" < "$INPUT_FILE" 2>/dev/null)
    else
        ACTUAL_OUTPUT=$(timeout "$TIMEOUT" "$BIN" "${RUN_ARGS[@]}" 2>/dev/null)
    fi
    ACTUAL_STATUS=$?

    if [ $ACTUAL_STATUS -ne $EXPECTED_STATUS ]; then
        fail=$((fail + 1))
        echo "FAIL  $name  (expected $EXPECTED_STATUS, got $ACTUAL_STATUS)"
        continue
    fi

    if [ -f "$EXPECTED_FILE" ]; then
        EXPECTED_OUTPUT=$(cat "$EXPECTED_FILE")
        if [ "$ACTUAL_OUTPUT" != "$EXPECTED_OUTPUT" ]; then
            fail=$((fail + 1))
            echo "FAIL  $name  (stdout mismatch)"
            continue
        fi
    fi

    pass=$((pass + 1))
done

echo ""
echo "Results: $pass passed, $fail failed, $total total"
