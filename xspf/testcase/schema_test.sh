#!/bin/bash
# -----------------------------------------------------------------------------
# Written by Sebastian Pipping <sping@xiph.org>
# License is Public Domain.
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# Config
# -----------------------------------------------------------------------------
SCHEMA_PATH="../../../websites/xspf.org/validation"
RNC_0_FILE="${SCHEMA_PATH}/xspf-0_0.7.rnc"
RNC_1_FILE="${SCHEMA_PATH}/xspf-1_0.7.rnc"
RNG_0_FILE="${SCHEMA_PATH}/xspf-0_0.7.rng"
RNG_1_FILE="${SCHEMA_PATH}/xspf-1_0.7.rng"
XSD_FILE="${SCHEMA_PATH}/xspf-1_0.2.xsd"
FAIL_0_FILES="for_version_0/fail/*.xspf"
PASS_0_FILES="for_version_0/pass/*.xspf"
FAIL_1_FILES="for_version_1/fail/*.xspf"
PASS_1_FILES="for_version_1/pass/*.xspf"
LOG_FILE="schema_test_log.txt"

if [[ ! -d "${SCHEMA_PATH}" ]]; then
	echo "ERROR: Schema dir missing"
	echo "Run $ svn co http://svn.xiph.org/websites/xspf.org/validation ${SCHEMA_PATH}/"
	exit 1
fi


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
CHECK_PY=../../../websites/validator.xspf.org/check.py


# -----------------------------------------------------------------------------
# Reset log
# -----------------------------------------------------------------------------
rm ${LOG_FILE} &> /dev/null



# -----------------------------------------------------------------------------
# Versions
# -----------------------------------------------------------------------------
echo "===== Versions =====" >> ${LOG_FILE}
if [[ ! -f ${RNV} && `which ${RNV}` == "" ]]; then
	echo "ERROR: RNV missing" ; exit 1
fi
echo "RNV "`${RNV} -v 2>&1 | grep version | sed -r "s/rnv version (.+)/\1/"` >> ${LOG_FILE}

if [ ! ${XMLLINT} --version &>/dev/null ]; then
	echo "ERROR: xmllint missing" ; exit 1
fi
echo "xmllint #"`${XMLLINT} --version 2>&1 | grep version | sed -r "s/[^0-9]+//"` >> ${LOG_FILE}

if [ ! ${XML_STARLET} --version &>/dev/null ]; then
	echo "ERROR: XMLStarlet missing" ; exit 1
fi
echo "XMLStarlet "`${XML_STARLET} --version` >> ${LOG_FILE}

if [ ! ${SPIFF_CHECK} --version &>/dev/null ]; then
	echo "ERROR: spiff_check missing" ; exit 1
fi
echo "spiff_check "`${SPIFF_CHECK} --version | sed -r "s/[^0-9]+//"` >> ${LOG_FILE}

if [ ! -f "${CHECK_PY}" ]; then
	echo "ERROR: check.py missing" ; exit 1
fi
"${CHECK_PY}" --shell&>/dev/null
if [[ $? == 2 ]]; then
	echo 'check.py sanity check failed:'
	echo '----------------------------------------------------'
	"${CHECK_PY}" --shell
	echo '----------------------------------------------------'
	exit 1
fi
echo "check.py r"`svn info ${CHECK_PY} | grep "Revision:" | sed -r "s/Revision: (.+)/\1/"` >> ${LOG_FILE}
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
# NO TEST CASES
echo "" >> ${LOG_FILE}



echo "===== XSPF-0, without schema, spiff_check, should have failed =====" >> ${LOG_FILE}
for i in ${FAIL_0_FILES}; do
	if ${SPIFF_CHECK} - < "${i}" >/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}

echo "===== XSPF-0, without schema, spiff_check, should have passed =====" >> ${LOG_FILE}
# NO TEST CASES
echo "" >> ${LOG_FILE}



echo "===== XSPF-0, without schema, check.py, should have failed =====" >> ${LOG_FILE}
for i in ${FAIL_0_FILES}; do
	if ${CHECK_PY} --shell $i &>/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}

echo "===== XSPF-0, without schema, check.py, should have passed =====" >> ${LOG_FILE}
# NO TEST CASES
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
	if ${SPIFF_CHECK} - < "${i}" >/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}

echo "===== XSPF-1, without schema, spiff_check, should have passed =====" >> ${LOG_FILE}
for i in ${PASS_1_FILES}; do
	if ! ${SPIFF_CHECK} - < "${i}" >/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}



echo "===== XSPF-1, without schema, check.py, should have failed =====" >> ${LOG_FILE}
for i in ${FAIL_1_FILES}; do
	if ${CHECK_PY} --shell $i &>/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}

echo "===== XSPF-1, without schema, check.py, should have passed =====" >> ${LOG_FILE}
for i in ${PASS_1_FILES}; do
	if ! ${CHECK_PY} --shell $i &>/dev/null ; then
		echo $i >> ${LOG_FILE}
	fi
done
echo "" >> ${LOG_FILE}



# -----------------------------------------------------------------------------
# Show log
# -----------------------------------------------------------------------------
cat ${LOG_FILE}
exit 0

