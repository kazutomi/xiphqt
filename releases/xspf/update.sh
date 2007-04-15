#! /bin/bash
if [ $# -lt 1 ]
then
	echo "USAGE: $0 <version>"
	exit
fi

 

echo \
&& echo "========== libspiff-$1.tar.bz2 ==========" \
&& wget http://downloads.sourceforge.net/libspiff/libspiff-$1.tar.bz2 \
&& md5sum libspiff-$1.tar.bz2 >> MD5SUMS \
&& sha1sum libspiff-$1.tar.bz2 >> SHA1SUMS \
&& echo \
&& echo "========== libspiff-$1.tar.gz ==========" \
&& wget http://downloads.sourceforge.net/libspiff/libspiff-$1.tar.gz \
&& md5sum libspiff-$1.tar.gz >> MD5SUMS \
&& sha1sum libspiff-$1.tar.gz >> SHA1SUMS \
&& echo \
&& echo "========== libspiff-$1.zip ==========" \
&& wget http://downloads.sourceforge.net/libspiff/libspiff-$1.zip \
&& md5sum libspiff-$1.zip >> MD5SUMS \
&& sha1sum libspiff-$1.zip >> SHA1SUMS \
\
\
\
&& echo "<tr>" \
&& echo "<td>libSpiff</td>" \
&& echo "<td>$1</td>" \
&& echo "<td>" \
&& echo "	<a href=\"http://downloads.xiph.org/releases/xspf/libspiff-$1.tar.bz2\">libspiff-$1.tar.bz2</a><br />" \
&& echo "	<a href=\"http://downloads.xiph.org/releases/xspf/libspiff-$1.tar.gz\">libspiff-$1.tar.gz</a><br />" \
&& echo "	<a href=\http://downloads.xiph.org/releases/xspf/libspiff-$1.zip\">libspiff-$1.zip</a>" \
&& echo "</td>" \
&& echo "<td>" \
&& echo "	"`du libspiff-$1.tar.bz2 | sed -r "s/\s+.+//"`"K<br />" \
&& echo "	"`du libspiff-$1.tar.gz | sed -r "s/\s++.+//"`"K<br />" \
&& echo "	"`du libspiff-$1.zip | sed -r "s/\s+.+//"` \
&& echo "</td>" \
&& echo "<td>" \
&& echo "	"`md5sum libspiff-$1.tar.bz2 | sed -r "s/  .+//"`"<br />" \
&& echo "	"`md5sum libspiff-$1.tar.gz | sed -r "s/  .+//"`"<br />" \
&& echo "	"`md5sum libspiff-$1.zip | sed -r "s/  .+//"` \
&& echo "</td>" \
&& echo "</tr>"
