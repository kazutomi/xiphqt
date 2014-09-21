#! /bin/bash
if [ $# -lt 1 ]
then
	echo "USAGE: $0 <version>"
	exit
fi


 
function downloadLibXspf() { # $1 version, $2 extension
	echo "========== libxspf-$1.$2 =========="
	if [ ! -f "libxspf-$1.$2" ] ; then
		wget "http://downloads.sourceforge.net/libspiff/libxspf-$1.$2"
		if [ ! -f "libxspf-$1.$2" ] ; then
			echo "FAILED on libxspf-$1.$2"
			exit 1
		fi	
	fi
}

function appendChecksum() { # $1 version, $2 extension
	md5sum libxspf-$1.$2 >> MD5SUMS
	sha1sum libxspf-$1.$2 >> SHA1SUMS
	sha256sum libxspf-$1.$2 >> SHA256SUMS.txt
}



echo "========== downloading =========="
downloadLibXspf $1 "tar.bz2"
downloadLibXspf $1 "tar.gz"
downloadLibXspf $1 "tar.lzma"
downloadLibXspf $1 "zip"

echo
echo "========== appending checksums =========="
appendChecksum $1 "tar.bz2"
appendChecksum $1 "tar.gz"
appendChecksum $1 "tar.lzma"
appendChecksum $1 "zip"

echo
echo "========== adding files =========="
svn add libxspf-$1.tar.bz2
svn add libxspf-$1.tar.gz
svn add libxspf-$1.tar.lzma
svn add libxspf-$1.zip



echo
echo "========== building html =========="
echo "<tr>"
echo "<td>libxspf</td>"
echo "<td>$1</td>"
echo "<td>"
echo "	<a href=\"http://downloads.xiph.org/releases/xspf/libxspf-$1.tar.bz2\">libxspf-$1.tar.bz2</a><br />"
echo "	<a href=\"http://downloads.xiph.org/releases/xspf/libxspf-$1.tar.gz\">libxspf-$1.tar.gz</a><br />"
echo "	<a href=\"http://downloads.xiph.org/releases/xspf/libxspf-$1.tar.lzma\">libxspf-$1.tar.lzma</a><br />"
echo "	<a href=\"http://downloads.xiph.org/releases/xspf/libxspf-$1.zip\">libxspf-$1.zip</a>"
echo "</td>"
echo "<td>"
echo "	"`du libxspf-$1.tar.bz2 | sed -r "s/\s+.+//"`"K<br />"
echo "	"`du libxspf-$1.tar.gz | sed -r "s/\s++.+//"`"K<br />"
echo "	"`du libxspf-$1.tar.lzma | sed -r "s/\s++.+//"`"K<br />"
echo "	"`du libxspf-$1.zip | sed -r "s/\s+.+//"`"K"
echo "</td>"
echo "<td>"
echo "	"`md5sum libxspf-$1.tar.bz2 | sed -r "s/  .+//"`"<br />"
echo "	"`md5sum libxspf-$1.tar.gz | sed -r "s/  .+//"`"<br />"
echo "	"`md5sum libxspf-$1.tar.lzma | sed -r "s/  .+//"`"<br />"
echo "	"`md5sum libxspf-$1.zip | sed -r "s/  .+//"`
echo "</td>"
echo "</tr>"

