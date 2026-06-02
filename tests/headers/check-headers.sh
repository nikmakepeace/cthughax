#!/bin/sh

set -u

script_dir=`dirname "$0"`
repo_root=`cd "$script_dir/../.." && pwd`
build_dir="$repo_root/tests/headers/build"
cmake_build_dir=${CTH_CMAKE_BUILD_DIR:-"$repo_root/build"}
report="$repo_root/tests/headers/header-errors"

cxx=${CXX:-g++}
cxxflags=${CXXFLAGS:-}

rm -rf "$build_dir"
mkdir -p "$build_dir"
: > "$report"

include_flags="-I$cmake_build_dir/src -I$cmake_build_dir -I$repo_root/src -I$repo_root"

if test ! -f "$cmake_build_dir/config.h"; then
    {
        echo "warning: $cmake_build_dir/config.h was not found."
        echo "Run cmake -S $repo_root -B $cmake_build_dir before checking headers."
        echo
    } >> "$report"
fi

x11_probe="$build_dir/x11-probe.cc"
cat > "$x11_probe" <<EOF
#include <X11/Intrinsic.h>
#include <X11/extensions/XShm.h>
int main() { return 0; }
EOF
if "$cxx" -DHAVE_CONFIG_H $include_flags $cxxflags -c "$x11_probe" -o "$build_dir/x11-probe.o" > "$build_dir/x11-probe.log" 2>&1; then
    have_x11=1
else
    have_x11=0
fi

failures=0
skipped=0
total=0

for header in "$repo_root"/src/*.h; do
    total=`expr "$total" + 1`

    base=`basename "$header" .h`
    stub="$build_dir/$base.cc"
    log="$build_dir/$base.log"
    obj="$build_dir/$base.o"
    rel="src/`basename "$header"`"

    if test "$rel" = "src/xcthugha.h" && test "$have_x11" -eq 0; then
        skipped=`expr "$skipped" + 1`
        {
            echo "==== $rel ===="
            echo "skipped: X11/Intrinsic.h or X11/extensions/XShm.h is not available in this configured build environment."
            echo
        } >> "$report"
        continue
    fi

    {
        echo "#include \"$rel\""
        echo
        echo "int main()"
        echo "{"
        echo "    return 0;"
        echo "}"
    } > "$stub"

    if "$cxx" -DHAVE_CONFIG_H $include_flags $cxxflags -c "$stub" -o "$obj" > "$log" 2>&1; then
        :
    else
        failures=`expr "$failures" + 1`
        {
            echo "==== $rel ===="
            cat "$log"
            echo
        } >> "$report"
    fi
done

{
    echo "Checked $total headers."
    echo "Skipped: $skipped."
    echo "Failures: $failures."
    echo "Generated stubs and logs: $build_dir"
    echo "Error report: $report"
} >&2

if test "$failures" -ne 0; then
    exit 127
fi

exit 0
