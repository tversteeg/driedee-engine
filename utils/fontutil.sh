#!/bin/bash

convert -version > /dev/null || { echo "ImageMagick is not installed" ; exit 1 ; }

if [ $# -ne 1 ]
  then
    echo "Pass the font name to convert as the first argument" ;
		exit 1
fi

fc-list | grep -q "$1" || { echo "$1 is not a valid font" ; exit 1 ; }

FONT=$(fc-list | grep "$1" | head -1 | sed 's/:.*//')

QUERY=$(fc-query "$FONT")

#echo "$QUERY"

NAME=$(echo "$QUERY" | grep "family" | sed 's/.*"\(.*\)"[^"]*$/\1/')
HEIGHT=$(echo "$QUERY" | grep "pixelsize" | grep -Po '\d+')

convert -list font | grep -q $NAME || { echo "Can not find font in ImageMagick cache" ; exit 1 ; }

# Get the actual image width
convert -compress None -font "$FONT" -pointsize $HEIGHT label:"A" /tmp/sizes.pbm
sed -i '1d' /tmp/sizes.pbm

# Extract size of the image
TEXTSIZES=$(head -1 /tmp/sizes.pbm)
SIZES=(${TEXTSIZES// / })

FONTSTART=33
FONTEND=127
FONTRANGE=$(($FONTEND - $FONTSTART))

IMGWIDTH=$(($FONTRANGE * (${SIZES[0]} - 2)))

STRING=$(printf $(printf '\%o' {33..127}))
# Create a image with the whole array of letters
echo -n "$STRING" | convert -compress None \
	-font "$FONT" -pointsize $HEIGHT -size "${IMGWIDTH}x$HEIGHT" \
	caption:@- /tmp/output.pbm

# Remove header
sed -i '1,2d' /tmp/output.pbm

LOWERNAME=$(echo $NAME | tr '[:upper:]' '[:lower:]')
OUTPUTFILENAME="pf_$LOWERNAME.h"

# Generate C header file
echo -e \	"// Generated font file
static unsigned int ${LOWERNAME}fontwidth = $IMGWIDTH;
static unsigned int ${LOWERNAME}fontheight = $HEIGHT;
static char ${LOWERNAME}fontstart = $FONTSTART;
static char ${LOWERNAME}fontrange = $FONTRANGE;
static char ${LOWERNAME}fontdata[] = {
	" > $OUTPUTFILENAME

sed 's/ /,/g' /tmp/output.pbm >> $OUTPUTFILENAME

echo -e "\n};" >> $OUTPUTFILENAME

echo -e "Font header $NAME ($OUTPUTFILENAME) created\n\tglyph size: $((${SIZES[0]} - 2))x$HEIGHT\n\twidth: $IMGWIDTH"

#rm /tmp/sizes.pbm
#rm /tmp/output.pbm
