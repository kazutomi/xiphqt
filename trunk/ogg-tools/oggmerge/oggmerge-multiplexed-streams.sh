#!/bin/sh

dir=`mktemp -d`
if [ ! -d "$dir" ]; then
  echo "failed to create temporary directory"
  exit 1
fi

merged_file="merged.ogx"

files=$(oggsplit -o "$dir" "$@" | grep "writing stream to" | sed -e "s^.*\`\(.*\)'.*^\1^")
echo $files

for ((c=1;;++c)); do
  chain=`printf ".c%02d." $c`
  files_in_this_chain=
  for f in $files
  do
    echo "$f" | grep -q "$chain"
    if [ $? -eq 0 ]; then
      files_in_this_chain="$files_in_this_chain $f"
    fi
  done
  if [ -z "$files_in_this_chain" ]; then break; fi
  tmpfile=`mktemp`
  oggmerge -o "$tmpfile" $files_in_this_chain
  cat "$tmpfile" >> "$merged_file"
  rm -f "$tmpfile"
done

exit 0

# don't rm -fr $dir, just in case... :)
rm -f $files
rmdir "$dir"