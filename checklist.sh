#!/usr/bin bash

find_files() {
    find src tests -type f \
    \(\
        -name '*.h' -o \
        -name '*.c' -o \
        -name '*.cpp' \
    \)
}

# Project tag
find_files |
while IFS= read -r file; do
    if ! grep -Eq '@project[[:space:]]+[^[:space:]]' "$file"; then
        echo "[!] Missing project tag in $file"
    fi
done

# Filename tag
find_files |
while IFS= read -r file; do
    name=$(basename "$file")

    if ! grep -Eq "@file[[:space:]]+$name([[:space:]]|$)" "$file"; then
        echo "[!] Incorrect filename header in $file"
    fi
done

# Author tag
find_files |
while IFS= read -r file; do
    if ! grep -Eq '@author[[:space:]]+[^[:space:]]' "$file"; then
        echo "[!] Invalid author tag in $file"
    fi
done

# Brief tag
find_files |
while IFS= read -r file; do
    if ! grep -Eq '@brief[[:space:]]+[^[:space:]]' "$file"; then
        echo "[!] Missing brief description in $file"
    fi
done
