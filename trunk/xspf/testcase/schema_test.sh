#!/bin/bash
# -----------------------------------------------------------------------------
# Written by Sebastian Pipping <dllmain@xiph.org>
# License is Public Domain.
#
# 2007-03-03
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Config
# -----------------------------------------------------------------------------
RNG_FILE=xspf-1_0.5.rng
XSD_FILE=xspf-1_0.2.xsd
PASS_FILES="for_version_1/pass/*.xspf"
FAIL_FILES="for_version_1/fail/*.xspf"
LOG_FILE="schema_test_log.txt"

# -----------------------------------------------------------------------------
# Cygwin or Unix?
# -----------------------------------------------------------------------------
if [ ${TERM} == cygwin ]; then
	XML_STARLET=xml
else
	XML_STARLET=xmlstarlet
fi

# -----------------------------------------------------------------------------
# Reset log
# -----------------------------------------------------------------------------
rm ${LOG_FILE} &> /dev/null

# -----------------------------------------------------------------------------
# Test RNG
# -----------------------------------------------------------------------------
echo "== RNG - Should have failed ==" >> ${LOG_FILE}
./${XML_STARLET} validate --list-good --relaxng ${RNG_FILE} ${FAIL_FILES} >> ${LOG_FILE}
echo "" >> ${LOG_FILE}
echo "== RNG - Should have passed ==" >> ${LOG_FILE}
./${XML_STARLET} validate --list-bad --relaxng ${RNG_FILE} ${PASS_FILES} >> ${LOG_FILE}
echo "" >> ${LOG_FILE}

# -----------------------------------------------------------------------------
# Test XSD
# -----------------------------------------------------------------------------
echo "== XSD - Should have failed ==" >> ${LOG_FILE}
./${XML_STARLET} validate --list-good --xsd ${XSD_FILE} ${FAIL_FILES} >> ${LOG_FILE}
echo "" >> ${LOG_FILE}
echo "== XSD - Should have passed ==" >> ${LOG_FILE}
./${XML_STARLET} validate --list-bad --xsd ${XSD_FILE} ${PASS_FILES} >> ${LOG_FILE}

# -----------------------------------------------------------------------------
# Show log
# -----------------------------------------------------------------------------
cat ${LOG_FILE}
