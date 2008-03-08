#! /bin/bash
if [ $# -lt 1 ]
then
	echo "USAGE: $0 <version>"
	exit
fi


 
function downloadLibSpiff() { # $1 version, $2 extension
	echo "========== libspiff-$1.$2 =========="
	if [ ! -f "libspiff-$1.$2" ] ; then
		wget "http://downloads.sourceforge.net/libspiff/libspiff-$1.$2"
		if [ ! -f "libspiff-$1.$2" ] ; then
			echo "FAILED on libspiff-$1.$2"
			exit 1
		fi	
	fi
}

function appendChecksum() { # $1 version, $2 extension
	md5sum libspiff-$1.$2 >> MD5SUMS
	sha1sum libspiff-$1.$2 >> SHA1SUMS
}



echo "========== downloading =========="
downloadLibSpiff $1 "tar.bz2"
downloadLibSpiff $1 "tar.gz"
downloadLibSpiff $1 "tar.lzma"
downloadLibSpiff $1 "zip"

echo
echo "========== appending checksums =========="
appendChecksum $1 "tar.bz2"
appendChecksum $1 "tar.gz"
appendChecksum $1 "tar.lzma"
appendChecksum $1 "zip"

echo
echo "========== adding files =========="
svn add libspiff-$1.tar.bz2
svn add libspiff-$1.tar.gz
svn add libspiff-$1.tar.lzma
svn add libspiff-$1.zip



echo
echo "========== building html =========="
echo "<tr>"
echo "<td>libSpiff</td>"
echo "<td>$1</td>"
echo "<td>"
echo "	<a href=\"http://downloads.xiph.org/releases/xspf/libspiff-$1.tar.bz2\">libspiff-$1.tar.bz2</a><br />"
echo "	<a href=\"http://downloads.xiph.org/releases/xspf/libspiff-$1.tar.gz\">libspiff-$1.tar.gz</a><br />"
echo "	<a href=\"http://downloads.xiph.org/releases/xspf/libspiff-$1.tar.lzma\">libspiff-$1.tar.lzma</a><br />"
echo "	<a href=\"http://downloads.xiph.org/releases/xspf/libspiff-$1.zip\">libspiff-$1.zip</a>"
echo "</td>"
echo "<td>"
echo "	"`du libspiff-$1.tar.bz2 | sed -r "s/\s+.+//"`"K<br />"
echo "	"`du libspiff-$1.tar.gz | sed -r "s/\s++.+//"`"K<br />"
echo "	"`du libspiff-$1.tar.lzma | sed -r "s/\s++.+//"`"K<br />"
echo "	"`du libspiff-$1.zip | sed -r "s/\s+.+//"`"K"
echo "</td>"
echo "<td>"
echo "	"`md5sum libspiff-$1.tar.bz2 | sed -r "s/  .+//"`"<br />"
echo "	"`md5sum libspiff-$1.tar.gz | sed -r "s/  .+//"`"<br />"
echo "	"`md5sum libspiff-$1.tar.lzma | sed -r "s/  .+//"`"<br />"
echo "	"`md5sum libspiff-$1.zip | sed -r "s/  .+//"`
echo "</td>"
echo "</tr>"

