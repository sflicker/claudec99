#!/bin/bash
#
# Integration test runner. Compiles each test, assembles, links against
# libc via `cc -no-pie`, runs, and checks exit code + stdout.
#
# Companion files (per BASENAME):
#   .expected  expected stdout
#   .libs      extra -l flags
#   .args      runtime argv
#   .input     stdin
#   .status    expected exit code (default 0)
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
COMPILER="$PROJECT_DIR/build/ccompiler"
WORK_DIR="$PROJECT_DIR/build/test_tmp_integration"

mkdir -p "$WORK_DIR"

pass=0
fail=0
total=0

for src in "$SCRIPT_DIR"/*.c; do
    [ -f "$src" ] || continue
    name=$(basename "$src" .c)
    total=$((total + 1))

    libs_file="$SCRIPT_DIR/${name}.libs"
    args_file="$SCRIPT_DIR/${name}.args"
    input_file="$SCRIPT_DIR/${name}.input"
    status_file="$SCRIPT_DIR/${name}.status"
    expected_file="$SCRIPT_DIR/${name}.expected"

    if [ -f "$libs_file" ]; then
        extra_libs=($(cat "$libs_file"))
    else
        extra_libs=()
    fi
    if [ -f "$args_file" ]; then
        extra_args=($(cat "$args_file"))
    else
        extra_args=()
    fi
    if [ -f "$status_file" ]; then
        expected_status="$(cat "$status_file")"
    else
        expected_status=0
    fi

    # Compile .c -> .asm
    if ! "$COMPILER" "$src" 2>/dev/null; then
        echo "FAIL  $name  (compiler error)"
        fail=$((fail + 1))
        continue
    fi

    asm_file="${name}.asm"
    mv "$asm_file" "$WORK_DIR/" 2>/dev/null

    # Assemble
    if ! nasm -f elf64 "$WORK_DIR/${name}.asm" -o "$WORK_DIR/${name}.o" 2>/dev/null; then
        echo "FAIL  $name  (nasm error)"
        fail=$((fail + 1))
        continue
    fi

    # Link with libc via cc -no-pie.
    if ! cc -no-pie "$WORK_DIR/${name}.o" -o "$WORK_DIR/${name}" "${extra_libs[@]}" 2>/dev/null; then
        echo "FAIL  $name  (link error)"
        fail=$((fail + 1))
        continue
    fi

    # Run with optional stdin redirection and argv.
    stdout_file="$WORK_DIR/${name}.stdout"
    if [ -f "$input_file" ]; then
        "$WORK_DIR/${name}" "${extra_args[@]}" <"$input_file" >"$stdout_file"
    else
        "$WORK_DIR/${name}" "${extra_args[@]}" >"$stdout_file"
    fi
    actual=$?

    if [ "$actual" -ne "$expected_status" ]; then
        echo "FAIL  $name  (expected $expected_status, got $actual)"
        fail=$((fail + 1))
        continue
    fi

    if [ -f "$expected_file" ]; then
        if ! diff -q "$expected_file" "$stdout_file" >/dev/null; then
            echo "FAIL  $name  (output mismatch)"
            echo "  --- expected ---"
            cat "$expected_file"
            echo "  --- actual ---"
            cat "$stdout_file"
            echo "  --- diff ---"
            diff "$expected_file" "$stdout_file" || true
            fail=$((fail + 1))
            continue
        fi
        echo "PASS  $name  (exit code: $actual, output matched)"
    else
        echo "PASS  $name  (exit code: $actual)"
    fi
    pass=$((pass + 1))
done

echo ""
echo "Results: $pass passed, $fail failed, $total total"

rm -rf "$WORK_DIR"

if [ "$fail" -ne 0 ]; then
    exit 1
fi
exit 0
