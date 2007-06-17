#!/bin/bash
# -----------------------------------------------------------------------------
# Written by Sebastian Pipping <sping@xiph.org>
# License is Public Domain.
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# Config
# -----------------------------------------------------------------------------
SCHEMA_PATH="../../../websites/xspf.org/validation"
RNC_0_FILE="${SCHEMA_PATH}/xspf-0_0.5.rnc"
RNC_1_FILE="${SCHEMA_PATH}/xspf-1_0.5.rnc"
RNG_1_FILE="${SCHEMA_PATH}/xspf-0_0.5.rng"
RNG_1_FILE="${SCHEMA_PATH}/xspf-1_0.5.rng"
XSD_FILE="${SCHEMA_PATH}/xspf-1_0.2.xsd"
FAIL_0_FILES="for_version_0/fail/*.xspf"
PASS_0_FILES="for_version_0/pass/*.xspf"
FAIL_1_FILES="for_version_1/fail/*.xspf"
PASS_1_FILES="for_version_1/pass/*.xspf"
LOG_FILE="schema_test_log.txt"



# -----------------------------------------------------------------------------
# Cygwin or Unix?
# -----------------------------------------------------------------------------
if [ ${TERM} == cygwin ]; then
	XML_STARLET=xml
else
	XML_STARLET=xmlstarlet
fi
RNV=rnv
XMLLINT=xmllint
SPIFF_CHECK=spiff_check



# -----------------------------------------------------------------------------
# Reset log
# -----------------------------------------------------------------------------
rm ${LOG_FILE} &> /dev/null



# -----------------------------------------------------------------------------
# Versions
# -----------------------------------------------------------------------------
echo "===== Versions =====" >> ${LOG_FILE}
echo "RNV "`${RNV} -v 2>&1 | grep version | sed -r "s/rnv version (.+)/\1/"` >> ${LOG_FILE}
echo "xmllint #"`${XMLLINT} --version 2>&1 | grep version | sed -r "s/[^0-9]+//"` >> ${LOG_FILE}
echo "XMLStarlet "`${XML_STARLET} --version` >> ${LOG_FILE}
echo "spiff_check "`${SPIFF_CHECK} --version | sed -r "s/[^0-9]+//"` >> ${LOG_FILE}
echo "" >> ${LOG_FILE}



# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------
echo "===== XSPF-0, Relax NG Compact, RNV, should have failed =====" >> ${LOG_FILE}
for i in ${FAIL_0_FILES}; do
	if ${RNV} ${RNC_0_FILE} $i &>/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}

echo "===== XSPF-0, Relax NG Compact, RNV, should have passed =====" >> ${LOG_FILE}
#for i in ${PASS_0_FILES}; do
#	if ! ${RNV} ${RNC_0_FILE} $i &>/dev/null ; then
#		echo $i >> ${LOG_FILE}
#	fi
#done
echo "" >> ${LOG_FILE}



echo "===== XSPF-0, Relax NG XML, XMLStarlet, should have failed =====" >> ${LOG_FILE}
${XML_STARLET} validate --err --list-good --relaxng ${RNG_0_FILE} ${FAIL_0_FILES} 1>> ${LOG_FILE} 2>> /dev/null
echo "" >> ${LOG_FILE}

echo "===== XSPF-0, Relax NG XML, XMLStarlet, should have passed =====" >> ${LOG_FILE}
#${XML_STARLET} validate --err --list-bad --relaxng ${RNG_0_FILE} ${PASS_0_FILES} 1>> ${LOG_FILE} 2>> /dev/null
echo "" >> ${LOG_FILE}



echo "===== XSPF-0, Relax NG XML, xmllint, should have failed =====" >> ${LOG_FILE}
for i in ${FAIL_0_FILES}; do
	if ${XMLLINT} --relaxng ${RNG_0_FILE} --noout $i &>/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}

echo "===== XSPF-0, Relax NG XML, xmllint, should have passed =====" >> ${LOG_FILE}
#for i in ${PASS_0_FILES}; do
#	if ! ${XMLLINT} --relaxng ${RNG_0_FILE} --noout $i &>/dev/null ; then
#		echo $i >> ${LOG_FILE}
#	fi
#done
echo "" >> ${LOG_FILE}



echo "===== XSPF-0, without schema, spiff_check, should have failed =====" >> ${LOG_FILE}
for i in ${FAIL_0_FILES}; do
	OUTPUT=`cat $i | ${SPIFF_CHECK} -`
	if [ "${OUTPUT}" == "Valid XSPF-0." ]; then
		echo $i
	fi
done
echo "" >> ${LOG_FILE}

echo "===== XSPF-0, without schema, spiff_check, should have passed =====" >> ${LOG_FILE}
#for i in ${PASS_0_FILES}; do
#	OUTPUT=`cat $i | ${SPIFF_CHECK} -`
#	if [ "${OUTPUT}" != "Valid XSPF-0." ]; then
#		echo $i
#	fi
#done
echo "" >> ${LOG_FILE}



echo "===== XSPF-1, Relax NG Compact, RNV, should have failed =====" >> ${LOG_FILE}
for i in ${FAIL_1_FILES}; do
	if ${RNV} ${RNC_1_FILE} $i &>/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}

echo "===== XSPF-1, Relax NG Compact, RNV, should have passed =====" >> ${LOG_FILE}
for i in ${PASS_1_FILES}; do
	if ! ${RNV} ${RNC_1_FILE} $i &>/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}



echo "===== XSPF-1, Relax NG XML, XMLStarlet, should have failed =====" >> ${LOG_FILE}
${XML_STARLET} validate --err --list-good --relaxng ${RNG_1_FILE} ${FAIL_1_FILES} 1>> ${LOG_FILE} 2>> /dev/null
echo "" >> ${LOG_FILE}

echo "===== XSPF-1, Relax NG XML, XMLStarlet, should have passed =====" >> ${LOG_FILE}
${XML_STARLET} validate --err --list-bad --relaxng ${RNG_1_FILE} ${PASS_1_FILES} 1>> ${LOG_FILE} 2>> /dev/null
echo "" >> ${LOG_FILE}



echo "===== XSPF-1, Relax NG XML, xmllint, should have failed =====" >> ${LOG_FILE}
for i in ${FAIL_1_FILES}; do
	if ${XMLLINT} --relaxng ${RNG_1_FILE} --noout $i &>/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}

echo "===== XSPF-1, Relax NG XML, xmllint, should have passed =====" >> ${LOG_FILE}
for i in ${PASS_1_FILES}; do
	if ! ${XMLLINT} --relaxng ${RNG_1_FILE} --noout $i &>/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}



echo "===== XSPF-1, W3C XML Schema, XMLStarlet, should have failed =====" >> ${LOG_FILE}
${XML_STARLET} validate --err --list-good --xsd ${XSD_FILE} ${FAIL_1_FILES} 1>> ${LOG_FILE} 2>> /dev/null
echo "" >> ${LOG_FILE}

echo "===== XSPF-1, W3C XML Schema, XMLStarlet, should have passed =====" >> ${LOG_FILE}
${XML_STARLET} validate --err --list-bad --xsd ${XSD_FILE} ${PASS_1_FILES} 1>> ${LOG_FILE} 2>> /dev/null
echo "" >> ${LOG_FILE}



echo "===== XSPF-1, W3C XML Schema, xmllint, should have failed =====" >> ${LOG_FILE}
for i in ${FAIL_1_FILES}; do
	if ${XMLLINT} --schema ${XSD_FILE} --noout $i &>/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}

echo "===== XSPF-1, W3C XML Schema, xmllint, should have passed =====" >> ${LOG_FILE}
for i in ${PASS_1_FILES}; do
	if ! ${XMLLINT} --schema ${XSD_FILE} --noout $i &>/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}



echo "===== XSPF-1, without schema, spiff_check, should have failed =====" >> ${LOG_FILE}
for i in ${FAIL_1_FILES}; do
	OUTPUT=`cat $i | ${SPIFF_CHECK} -`
	if [ "${OUTPUT}" == "Valid XSPF-1." ]; then
		echo $i
	fi
done
echo "" >> ${LOG_FILE}

echo "===== XSPF-1, without schema, spiff_check, should have passed =====" >> ${LOG_FILE}
for i in ${PASS_1_FILES}; do
	OUTPUT=`cat $i | ${SPIFF_CHECK} -`
	if [ "${OUTPUT}" != "Valid XSPF-1." ]; then
		echo $i
	fi
done
echo "" >> ${LOG_FILE}



# -----------------------------------------------------------------------------
# Show log
# -----------------------------------------------------------------------------
cat ${LOG_FILE}
