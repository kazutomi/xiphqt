#!/bin/bash
# -----------------------------------------------------------------------------
# Written by Sebastian Pipping <sping@xiph.org>
# License is Public Domain.
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# Config
# -----------------------------------------------------------------------------
SCHEMA_PATH="../../../websites/xspf.org/validation"
RNC_FILE="${SCHEMA_PATH}/xspf-1_0.5.rnc"
RNG_FILE="${SCHEMA_PATH}/xspf-1_0.5.rng"
XSD_FILE="${SCHEMA_PATH}/xspf-1_0.2.xsd"
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
RNV=./rnv
XMLLINT=xmllint



# -----------------------------------------------------------------------------
# Reset log
# -----------------------------------------------------------------------------
rm ${LOG_FILE} &> /dev/null



# -----------------------------------------------------------------------------
# RNV version
# -----------------------------------------------------------------------------
echo "== RNV version ==" >> ${LOG_FILE}
${RNV} -v 2>> ${LOG_FILE}
echo "" >> ${LOG_FILE}



# -----------------------------------------------------------------------------
# XMLStarlet version
# -----------------------------------------------------------------------------
echo "== XMLStarlet version ==" >> ${LOG_FILE}
${XML_STARLET} --version >> ${LOG_FILE}
echo "" >> ${LOG_FILE}



# -----------------------------------------------------------------------------
# Test RNC
# -----------------------------------------------------------------------------
echo "== Relax NG Compact - RNV - Should have failed ==" >> ${LOG_FILE}
for i in ${FAIL_FILES}; do
	if ${RNV} ${RNC_FILE} $i &>/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}

echo "== Relax NG Compact - RNV - Should have passed ==" >> ${LOG_FILE}
for i in ${PASS_FILES}; do
	if ! ${RNV} ${RNC_FILE} $i &>/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}



# -----------------------------------------------------------------------------
# Test RNG
# -----------------------------------------------------------------------------
echo "== Relax NG XML - XMLStarlet - Should have failed ==" >> ${LOG_FILE}
${XML_STARLET} validate --err --list-good --relaxng ${RNG_FILE} ${FAIL_FILES} 1>> ${LOG_FILE} 2>> /dev/null
echo "" >> ${LOG_FILE}

echo "== Relax NG XML - XMLStarlet - Should have passed ==" >> ${LOG_FILE}
${XML_STARLET} validate --err --list-bad --relaxng ${RNG_FILE} ${PASS_FILES} 1>> ${LOG_FILE} 2>> /dev/null
echo "" >> ${LOG_FILE}

echo "== Relax NG XML - xmllint - Should have failed ==" >> ${LOG_FILE}
for i in ${FAIL_FILES}; do
	if ${XMLLINT} --relaxng ${RNG_FILE} --noout $i &>/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}

echo "== Relax NG XML - xmllint - Should have passed ==" >> ${LOG_FILE}
for i in ${PASS_FILES}; do
	if ! ${XMLLINT} --relaxng ${RNG_FILE} --noout $i &>/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}



# -----------------------------------------------------------------------------
# Test XSD/WXS
# -----------------------------------------------------------------------------
echo "== W3C XML Schema - XMLStarlet - Should have failed ==" >> ${LOG_FILE}
${XML_STARLET} validate --err --list-good --xsd ${XSD_FILE} ${FAIL_FILES} 1>> ${LOG_FILE} 2>> /dev/null
echo "" >> ${LOG_FILE}

echo "== W3C XML Schema - XMLStarlet - Should have passed ==" >> ${LOG_FILE}
${XML_STARLET} validate --err --list-bad --xsd ${XSD_FILE} ${PASS_FILES} 1>> ${LOG_FILE} 2>> /dev/null
echo "" >> ${LOG_FILE}

echo "== W3C XML Schema - xmllint - Should have failed ==" >> ${LOG_FILE}
for i in ${FAIL_FILES}; do
	if ${XMLLINT} --schema ${XSD_FILE} --noout $i &>/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}

echo "== W3C XML Schema - xmllint - Should have passed ==" >> ${LOG_FILE}
for i in ${PASS_FILES}; do
	if ! ${XMLLINT} --schema ${XSD_FILE} --noout $i &>/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}



# -----------------------------------------------------------------------------
# Show log
# -----------------------------------------------------------------------------
cat ${LOG_FILE}
