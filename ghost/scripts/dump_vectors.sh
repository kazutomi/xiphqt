#!/bin/sh

echo '<Vector<ObjectRef> ' > nbands.vec; cat $* | ../libghost/testghost /dev/stdin /dev/null | sed -e 's/^/<Vector<float> /' -e 's/$/>/' >> nbands.vec; echo ' > ' >> nbands.vec
