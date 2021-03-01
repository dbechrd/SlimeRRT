#!/bin/sh
###############################################################################
# Copyright 2021 - Dan Bechard
# Deletes a random line of code from a random source file to cause chaos. >:)
###############################################################################

# Directory where source files are kept
sourcedir="src"

# Directory depth to search
depth=1

# Pick a random file in the src directory
filename="$(find "$sourcedir" -maxdepth "$depth" -type f -regex '.*\.\(c\|h\|cpp\|hpp\)' | shuf -n 1)"

# Pick a random line number in the file
linenum="$(shuf -i 1-$(wc -l < "$filename") -n 1)"

echo "$filename:$linenum"

# [Print] Delete that line number from the file and print result
sed -e $(printf '%dd;' "$linenum") "$filename"

# [MODIFY] Delete that line number from the file, actually
#sed -i -e $(printf '%dd;' "$linenum") "$filename"
