#!/usr/bin/env -S sudo bash
# Usage: make-disk-img.sh disk.img file...
# disk.img must exist
# loads the files into the disk

MNT=/mnt
LOOP=/dev/loop0

function main() {
    local disk="${1}"
    true "${disk:?no disk}"
    losetup --offset 1048576 --sizelimit 46934528 "${LOOP}" "${disk}"
    mkdosfs -F 32 "${LOOP}"
    mount "${LOOP}" "${MNT}"
    shift
    true "${@:?no files}"
    cp "$@" "${MNT}"
    umount "${MNT}"
    losetup -d "${LOOP}"
}

main "$@"
