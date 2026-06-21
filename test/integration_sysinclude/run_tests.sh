#!/bin/bash
#
# Optional-library integration test runner.
# Like run_tests_sysinclude.sh but for tests that require optional third-party
# libraries (SDL2, zlib, etc.).  Tests with a .require companion file are
# skipped if the prerequisite check command exits non-zero.
#
# Results line format (consumed by run_all_tests.sh):
#   Results: P passed, F failed, S skipped, T total
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
COMPILER="$PROJECT_DIR/build/ccompiler"
WORK_DIR="$PROJECT_DIR/build/test_tmp_integration_sysinclude_opt"
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
skip=0
total=0

for test_dir in "$SCRIPT_DIR"/*/; do
    [ -d "$test_dir" ] || continue
    name=$(basename "$test_dir")
    total=$((total + 1))

    require_file="$test_dir/${name}.require"
    if [ -f "$require_file" ]; then
        req_cmd="$(cat "$require_file")"
        if ! eval "$req_cmd" >/dev/null 2>&1; then
            echo "SKIP  $name  (requires: $req_cmd)"
            skip=$((skip + 1))
            continue
        fi
    fi

    if [ ! -f "$test_dir/${name}.c" ]; then
        if [ -f "$test_dir/run_test.sh" ]; then
            if bash "$test_dir/run_test.sh" >/dev/null 2>&1; then
                pass=$((pass + 1))
            else
                echo "FAIL  $name"
                fail=$((fail + 1))
            fi
        else
            skip=$((skip + 1))
        fi
        continue
    fi

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
            fail=$((fail + 1))
            continue
        fi
        pass=$((pass + 1))
        continue
    fi

    obj_files=()
    compile_failed=0

    for src in "$test_dir"*.c; do
        [ -f "$src" ] || continue
        src_name=$(basename "$src" .c)

        if ! (cd "$test_dir" && timeout "$TIMEOUT" "$COMPILER" "${compiler_flags[@]}" "${DEFAULT_IFLAGS[@]}" "$src") 2>/dev/null; then
            echo "FAIL  $name  (compiler error)"
            fail=$((fail + 1))
            compile_failed=1
            break
        fi

        if ! mv "$test_dir/${src_name}.asm" "$test_work/" 2>/dev/null; then
            echo "FAIL  $name  (asm output missing)"
            fail=$((fail + 1))
            compile_failed=1
            break
        fi

        if ! nasm -f elf64 "$test_work/${src_name}.asm" -o "$test_work/${src_name}.o" 2>/dev/null; then
            echo "FAIL  $name  (nasm error)"
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
            fail=$((fail + 1))
            continue
        fi
    fi

    pass=$((pass + 1))
done

echo ""
echo "Results: $pass passed, $fail failed, $skip skipped, $total total"
