#!/bin/sh

GREEN='\033[32m'
RED='\033[31m'
RESET='\033[0m'

tests=(
    basic.ifj26
    custom.ifj26
    core/factorial.ifj26
    core/strings.ifj26
    extensions/boolthen.ifj26
    extensions/control.ifj26
    extensions/extkw.ifj26
    extensions/funexp.ifj26
    extensions/interpolateds.ifj26
    extensions/paramlists.ifj26
    extensions/valmem.ifj26
)

for test in "${tests[@]}"; do
    "./ifj26compiler" < "tests/data/$test" > /dev/null 2>&1
    status=$?

    if (( status == 0 )); then
       printf "${GREEN}%-3s${RESET} %s\n" "OK" "$test"
    else
       printf "${RED}%-3s${RESET} %s\n" "$status" "$test"
    fi
done