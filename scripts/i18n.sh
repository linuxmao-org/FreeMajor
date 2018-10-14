#!/bin/bash
set -e
set -x

po_update() {
    if test -f "$1"; then
        msgmerge -U "$1" "$1"t
    else
        msginit -i "$1"t -l "$locale" -o "$1"
    fi
}

for locale in "fr"; do
    mkdir -p po/"$locale"
    xgettext -k_ -L C++ --from-code=UTF-8 -c -s -o po/"$locale"/gmajctl.pot \
             sources/*.{h,cc} \
             sources/device/*.{h,cc} \
             sources/model/*.{h,cc} \
             sources/ui/*.{h,cc,cxx} \
             sources/utility/*.{h,cc}
    po_update po/"$locale"/gmajctl.po
    rm -f po/"$locale"/*.pot
done
