#!/bin/bash

cat "$@" | grep "seq\|par" | sed 's/[a-zA-Z]//g' | sed 's/[[:blank:]]//g' | sed 'N;s/\n/	/'
