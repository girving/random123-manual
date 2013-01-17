#!/bin/sh
# Run the C preprocessor on an OpenCL kernel to generate a C string array
# suitable for clCreateProgramWithSource.  This allows us to create
# standalone OpenCL programs that do not depend on paths to the source
# tree (the programs will still run the OpenCL run-time compiler to
# compile the kernel, but the kernel is a string within the program, with
# no external include dependencies)
# Mark Moraes, D. E. Shaw Research

# indenting the cpp output makes errors from the OpenCL runtime compiler
# much more understandable.
: ${GENCL_INDENT=indent}
usage="Usage: $0 inputoclfilename outputfilename"
case $# in
2)	;;
*)	echo "$usage" >&2; exit 1;;
esac
case "$1" in
''|-*)	echo "Invalid or empty inputoclfilename: $1
$usage" >&2; exit 1;;
esac
set -e
out="$2"
echo 'const static char *opencl_src = "\n\' > "$out"
${CC-cc} -xc -E -P -nostdinc -D__OPENCL_VERSION__=1 $CPPFLAGS "$1" | 
	if type "${GENCL_INDENT}" > /dev/null 2>&1; then
		"${GENCL_INDENT}" -kr
	else
		cat
	fi | 
	sed -n -e'1,$s,\\,\\\\,g;s,",\\",g;s,$,\\n\\,;p' >> "$out"
echo '";' >> "$out"
