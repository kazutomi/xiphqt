#!/bin/sh

E1=`sed 's/>/\n/g' band1.vq | grep -c Vector`
E2=`sed 's/>/\n/g' band2.vq | grep -c Vector`
E3a=`sed 's/>/\n/g' band3a.vq | grep -c Vector`
E3b=`sed 's/>/\n/g' band3b.vq | grep -c Vector`
E3c=`sed 's/>/\n/g' band3c.vq | grep -c Vector`
L3a=`grep length band3a.vq | awk '{print $2}' | sed 's/>//'`
L3b=`grep length band3b.vq | awk '{print $2}' | sed 's/>//'`
L3c=`grep length band3c.vq | awk '{print $2}' | sed 's/>//'`

echo "#define ENTRIES1 $E1" > bands_quant.c
echo "#define ENTRIES2 $E2" >> bands_quant.c
echo "#define ENTRIES3A $E3a" >> bands_quant.c
echo "#define ENTRIES3B $E3b" >> bands_quant.c
echo "#define ENTRIES3C $E3c" >> bands_quant.c

echo "#define LEN3A $L3a" >> bands_quant.c
echo "#define LEN3B $L3b" >> bands_quant.c
echo "#define LEN3C $L3c" >> bands_quant.c

cat band1.vq| perl -ne 's/> *<Vector/,\n/g;s/>//g;s/<Vector//g;s/<length .*/};/g;s/<KMeans//;s/<means/float cdbk_band1\[\]=\{/;s/([0-9\-]) +([\-0-9\.])/$1, $2/g; print' >> bands_quant.c
cat band2.vq| perl -ne 's/> *<Vector/,\n/g;s/>//g;s/<Vector//g;s/<length .*/};/g;s/<KMeans//;s/<means/float cdbk_band2\[\]=\{/;s/([0-9\-]) +([\-0-9\.])/$1, $2/g; print' >> bands_quant.c
cat band3a.vq| perl -ne 's/> *<Vector/,\n/g;s/>//g;s/<Vector//g;s/<length .*/};/g;s/<KMeans//;s/<means/float cdbk_band3a\[\]=\{/;s/([0-9\-]) +([\-0-9\.])/$1, $2/g; print' >> bands_quant.c
cat band3b.vq| perl -ne 's/> *<Vector/,\n/g;s/>//g;s/<Vector//g;s/<length .*/};/g;s/<KMeans//;s/<means/float cdbk_band3b\[\]=\{/;s/([0-9\-]) +([\-0-9\.])/$1, $2/g; print' >> bands_quant.c
cat band3c.vq| perl -ne 's/> *<Vector/,\n/g;s/>//g;s/<Vector//g;s/<length .*/};/g;s/<KMeans//;s/<means/float cdbk_band3c\[\]=\{/;s/([0-9\-]) +([\-0-9\.])/$1, $2/g; print' >> bands_quant.c
