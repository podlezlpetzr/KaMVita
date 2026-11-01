#!/usr/bin/env bash
set -e

cd "$(dirname "$0")"

echo "=== BUILD KAMVITA ==="

cmake --build build -j"$(nproc)"

echo
echo "=== FIND NEXT VPK NUMBER ==="

LAST=$(
    find build \
        -maxdepth 1 \
        -type f \
        -name 'KaMVita[0-9]*.vpk' \
        | sed -n \
            's#.*KaMVita\([0-9][0-9]*\)\.vpk#\1#p' \
        | sort -n \
        | tail -1
)

if [ -z "$LAST" ]; then
    NEXT=194
else
    NEXT=$((LAST + 1))
fi

VPK="build/KaMVita${NEXT}.vpk"

cp -f \
    build/KaMVita.vpk \
    "$VPK"

echo
echo "=== VERSIONED VPK ==="
echo "$VPK"

ls -lh "$VPK"

echo
echo "=== SHA256 ==="
sha256sum \
    build/KaMVita.vpk \
    "$VPK"

echo
echo "VPK_NUMBER=$NEXT"
echo "VPK_FILE=$VPK"
echo "VERSIONED_VPK=OK"
