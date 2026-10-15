#!/bin/sh

find_files() {
    find src tests -type f \
    \(\
        -name '*.h' -o \
        -name '*.c' -o \
        -name '*.cpp' \
    \)
}

username=$(git config user.name)

case "$username" in
    "Filip Hajdušek"|"Vojtěch Wyslych"|"adrahovska"|"tobiasmikan")
        ;;
    *)
        echo "[!] Possible git username config mismatch: '$username'"; echo
        ;;
esac

# Project tag
find_files |
while IFS= read -r file; do
    if ! grep -Eq '@project[[:space:]]+[^[:space:]]' "$file"; then
        echo "[p] Missing project tag in $file"
    fi
done

# Filename tag
find_files |
while IFS= read -r file; do
    name=$(basename "$file")

    if ! grep -Eq "@file[[:space:]]+$name([[:space:]]|$)" "$file"; then
        echo "[f] Incorrect filename header in $file"
    fi
done

# Author tag
find_files |
while IFS= read -r file; do
    if ! grep -Eq '@author[[:space:]]+[^[:space:]]' "$file"; then
        echo "[a] Invalid author tag in $file"
    fi
done

# Brief tag
find_files |
while IFS= read -r file; do
    if ! grep -Eq '@brief[[:space:]]+[^[:space:]]' "$file"; then
        echo "[b] Missing brief description in $file"
    fi
done
