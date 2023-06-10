#!/bin/bash
set -e

DATETIME=$(date -R)
VERSION=$(grep "project(" CMakeLists.txt | grep -oP '\(\K[^\)]+' | cut -f 6 -d ' ')
TARFILE="ueberzugpp.tar.xz"
DSCFILE="ueberzugpp.dsc"
declare -a FILES=("$TARFILE" "$DSCFILE")

sed -e "s;@@VERSION@@;${VERSION};g" -e "s;@@DATETIME@@;${DATETIME};g" < debian/changelog.in > debian/changelog

tar cfJ "$TARFILE" ./*

cat > "$DSCFILE" << EOF
Format: 3.0 (native)
Source: ueberzugpp
Binary: ueberzugpp
Architecture: any
Version: $VERSION
Maintainer: JustKidding <jk@vin.ovh>
Homepage: https://github.com/jstkdng/ueberzugpp
Standards-Version: 4.5.1
Build-Depends: $(sed -n '/^Build-Depends:/,/^$/p' < debian/control | tr -d '\n' | cut -f 2 -d : | sed -r 's;^ *| *$;;g')
Package-List:
 ueberzugpp deb misc optional arch=any
Checksums-Sha1:
 $(sha1sum < "$TARFILE" | cut -f 1 -d ' ') $(stat -c '%s' "$TARFILE") $TARFILE
Checksums-Sha256:
 $(sha256sum < "$TARFILE" | cut -f 1 -d ' ') $(stat -c '%s' "$TARFILE") $TARFILE
Files:
 $(md5sum < "$TARFILE" | cut -f 1 -d ' ') $(stat -c '%s' "$TARFILE") $TARFILE
EOF

cat "$DSCFILE"

BASE_URL="https://api.opensuse.org/source/$OBS_PROJECT/ueberzugpp"
for FILE in "${FILES[@]}"
do
    URL="$BASE_URL/$FILE?rev=upload"
    curl -XPUT -H 'Content-Type: application/octet-stream' -u "$OBS_AUTH" --data-binary "@$FILE" "$URL"
done

curl -XPOST -u "$OBS_AUTH" "$BASE_URL" -F "cmd=commit"

