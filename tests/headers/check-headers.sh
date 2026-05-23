#!/bin/sh

set -u

script_dir=`dirname "$0"`
repo_root=`cd "$script_dir/../.." && pwd`
build_dir="$repo_root/tests/headers/build"
report="$repo_root/tests/headers/header-errors"

cxx=${CXX:-g++}
cxxflags=${CXXFLAGS:-}

rm -rf "$build_dir"
mkdir -p "$build_dir"
: > "$report"

if test ! -f "$repo_root/config.h"; then
    {
        echo "warning: $repo_root/config.h was not found."
        echo "Run this from a configured build tree for the most accurate results."
        echo
    } >> "$report"
fi

glut_probe="$build_dir/glut-probe.cc"
cat > "$glut_probe" <<EOF
#include <GL/glut.h>
int main() { return 0; }
EOF
if "$cxx" -DHAVE_CONFIG_H -I"$repo_root" -I"$repo_root/src" $cxxflags -c "$glut_probe" -o "$build_dir/glut-probe.o" > "$build_dir/glut-probe.log" 2>&1; then
    have_glut=1
else
    have_glut=0
fi

x11_probe="$build_dir/x11-probe.cc"
cat > "$x11_probe" <<EOF
#include <X11/Intrinsic.h>
#include <X11/extensions/XShm.h>
int main() { return 0; }
EOF
if "$cxx" -DHAVE_CONFIG_H -I"$repo_root" -I"$repo_root/src" $cxxflags -c "$x11_probe" -o "$build_dir/x11-probe.o" > "$build_dir/x11-probe.log" 2>&1; then
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

    if test "$rel" = "src/glcthugha.h" && test "$have_glut" -eq 0; then
        skipped=`expr "$skipped" + 1`
        {
            echo "==== $rel ===="
            echo "skipped: GL/glut.h is not available in this configured build environment."
            echo
        } >> "$report"
        continue
    fi

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

    if "$cxx" -DHAVE_CONFIG_H -I"$repo_root" -I"$repo_root/src" $cxxflags -c "$stub" -o "$obj" > "$log" 2>&1; then
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
