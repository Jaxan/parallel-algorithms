#!/bin/bash
input="$1"
output=`basename "$1"`
output="images/${output%.*}.png"

convert "$input" -sigmoidal-contrast 3,50% -resize 1024x1024^ -gravity center -extent 1024x1024 -colorspace Gray "$output"
