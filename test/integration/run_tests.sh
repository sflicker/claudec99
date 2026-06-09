#!/bin/bash
#
# Integration test runner. For each test subdirectory, compiles every .c file,
# assembles to objects, links them together with cc -no-pie, runs, and checks
# exit code + stdout.
#
# Each test lives in its own subdirectory: test/integration/<name>/
# Main file: <name>.c
# Companion files (per directory basename):
#   .expected  expected stdout
#   .libs      extra -l flags
#   .args      runtime argv
#   .cflags    compiler flags (e.g. -DNAME=VAL)
#   .input     stdin
#   .status    expected exit code (default 0)
#   .error     if present, test expects a compile error; file content is an
#              expected substring of the compiler's stderr output
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
COMPILER="$PROJECT_DIR/build/ccompiler"
WORK_DIR="$PROJECT_DIR/build/test_tmp_integration"
TIMEOUT=${CLAUDEC99_TEST_TIMEOUT:-30}
DEFAULT_IFLAGS=("-I${PROJECT_DIR}/test/include")

mkdir -p "$WORK_DIR"

pass=0
fail=0
total=0

for test_dir in "$SCRIPT_DIR"/*/; do
    [ -d "$test_dir" ] || continue
    name=$(basename "$test_dir")

    # Stage 96: directories with run_test.sh but no <name>.c use a custom script.
    if [ ! -f "$test_dir/${name}.c" ]; then
        if [ -f "$test_dir/run_test.sh" ]; then
            total=$((total + 1))
            if bash "$test_dir/run_test.sh" >/dev/null 2>&1; then
                echo "PASS  $name"
                pass=$((pass + 1))
            else
                echo "FAIL  $name"
                fail=$((fail + 1))
            fi
        fi
        continue
    fi
    total=$((total + 1))

    test_work="$WORK_DIR/$name"
    mkdir -p "$test_work"
    cp -a "$test_dir/." "$test_work/"

    libs_file="$test_dir/${name}.libs"
    args_file="$test_dir/${name}.args"
    cflags_file="$test_dir/${name}.cflags"
    input_file="$test_dir/${name}.input"
    status_file="$test_dir/${name}.status"
    expected_file="$test_dir/${name}.expected"
    error_file="$test_dir/${name}.error"

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
    if [ -f "$cflags_file" ]; then
        compiler_flags=($(cat "$cflags_file"))
    else
        compiler_flags=()
    fi
    if [ -f "$status_file" ]; then
        expected_status="$(cat "$status_file")"
    else
        expected_status=0
    fi

    # Error test: expect the compiler to fail on the main source file.
    if [ -f "$error_file" ]; then
        expected_error="$(cat "$error_file")"
        compile_exit=0
        (cd "$test_dir" && timeout "$TIMEOUT" "$COMPILER" "${compiler_flags[@]}" "${DEFAULT_IFLAGS[@]}" "$test_dir/${name}.c") >/dev/null 2>"$test_work/${name}.stderr" || compile_exit=$?
        if [ "$compile_exit" -eq 0 ]; then
            echo "FAIL  $name  (expected compile error, but succeeded)"
            fail=$((fail + 1))
            continue
        fi
        actual_error="$(cat "$test_work/${name}.stderr")"
        if [ -n "$expected_error" ] && ! echo "$actual_error" | grep -qF "$expected_error"; then
            echo "FAIL  $name  (error message mismatch)"
            echo "  expected substring: $expected_error"
            echo "  actual: $actual_error"
            fail=$((fail + 1))
            continue
        fi
        echo "PASS  $name  (expected compile error)"
        pass=$((pass + 1))
        continue
    fi

    obj_files=()
    compile_failed=0

    for src in "$test_dir"*.c; do
        [ -f "$src" ] || continue
        src_name=$(basename "$src" .c)

        if ! (cd "$test_dir" && timeout "$TIMEOUT" "$COMPILER" "${compiler_flags[@]}" "${DEFAULT_IFLAGS[@]}" "$src") 2>/dev/null; then
            echo "FAIL  $name  (compiler error: $src_name.c)"
            fail=$((fail + 1))
            compile_failed=1
            break
        fi

        if ! mv "$test_dir/${src_name}.asm" "$test_work/" 2>/dev/null; then
            echo "FAIL  $name  (asm output missing: ${src_name}.asm)"
            fail=$((fail + 1))
            compile_failed=1
            break
        fi

        if ! nasm -f elf64 "$test_work/${src_name}.asm" -o "$test_work/${src_name}.o" 2>/dev/null; then
            echo "FAIL  $name  (nasm error: $src_name)"
            fail=$((fail + 1))
            compile_failed=1
            break
        fi

        obj_files+=("$test_work/${src_name}.o")
    done

    [ "$compile_failed" -eq 1 ] && continue

    if ! cc -no-pie "${obj_files[@]}" -o "$test_work/$name" "${extra_libs[@]}" 2>/dev/null; then
        echo "FAIL  $name  (link error)"
        fail=$((fail + 1))
        continue
    fi

    stdout_file="$test_work/${name}.stdout"
    if [ -f "$input_file" ]; then
        (cd "$test_work" && timeout "$TIMEOUT" "$test_work/$name" "${extra_args[@]}" <"$input_file" >"$stdout_file")
    else
        (cd "$test_work" && timeout "$TIMEOUT" "$test_work/$name" "${extra_args[@]}" >"$stdout_file")
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
