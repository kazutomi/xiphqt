# env.sh - setup environment to use positron without installing
#
# source this file in a bourne-compatible shell to setup the environment
# in a way that allows one to use positron without installing (very useful
# for debugging, testing from cvs, etc).

if [ "$1" ] ; then
  OLDDIR=`pwd`
  cd "$1"
else
  OLDDIR="."
fi

POSITRONTOP="`pwd`"
cd $OLDDIR

if [ ! -x "$POSITRONTOP/scripts/positron" ] ; then
  echo "Usage: source <this script> [<positron top-level directory>]"
  return 1
fi

PYTHONPATH="$POSITRONTOP"
PATH="$POSITRONTOP/scripts:$PATH"

export POSITRONTOP
export PYTHONPATH
export PATH
