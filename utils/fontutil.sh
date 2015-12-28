#!/bin/bash

if [ $# -ne 2 ]
  then
    echo "Pass the font name to convert as the first argument, and the output name as the second one" ;
		exit 1
fi

fc-list | grep -q "$1" || { echo "$1 is not a valid font" ; exit 1 ; }

FONT=$(fc-list | grep "$1" | head -1 | sed 's/:.*//')

QUERY=$(fc-query "$FONT")

# echo "$QUERY"

NAME=$(echo "$QUERY" | grep "family" | sed 's/.*"\(.*\)"[^"]*$/\1/')
WIDTH=$(( $(echo "$QUERY" | grep "weight" | grep -Po '\d+') / 10 ))
IMGWIDTH=$(((127 - 33) * $WIDTH))
HEIGHT=$(echo "$QUERY" | grep "pixelsize" | grep -Po '\d+')

STRING=$(printf $(printf '\%o' {33..127}))

convert -version > /dev/null || { echo "ImageMagick is not installed" ; exit 1 ; }
convert -list font | grep -q $NAME || { echo "Can not find font in ImageMagick cache" ; exit 1 ; }

echo -n "$STRING" | convert -compress None \
	-font "$FONT" -pointsize $HEIGHT -size $IMGWIDTH \
	caption:@- /tmp/output.pbm

sed -i '1d' /tmp/output.pbm
