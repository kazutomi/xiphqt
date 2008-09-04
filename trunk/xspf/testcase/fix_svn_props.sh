#! /usr/bin/env bash
# -----------------------------------------------------------------------------
# Written by Sebastian Pipping <sping@xiph.org>
# License is Public Domain.
# -----------------------------------------------------------------------------
PWD_BACKUP=${PWD}
SCRIPT_DIR=`dirname "${PWD}/$0"`
cd "${SCRIPT_DIR}" || exit 1
function fail() { cd "${PWD_BACKUP}" ; exit 1; }
####################################################################


## Apply native EOL style
find . -name '.svn' -prune -o \( -name '*.xspf' \) \
        -exec svn propset svn:eol-style native {} \;

## Make scripts executable
find . -name '.svn' -prune -o \( -name '*.sh' \) -exec svn propset svn:executable '*' {} \;
find . -name '.svn' -prune -o \( -not -name '*.sh' \) -exec svn propdel svn:executable {} \;


####################################################################
cd "${PWD_BACKUP}" || fail
exit 0

