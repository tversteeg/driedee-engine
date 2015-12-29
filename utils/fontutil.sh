#!/bin/bash

convert -version > /dev/null || { echo "ImageMagick is not installed" ; exit 1 ; }

if [ $# -ne 1 ]
then
	echo "Pass the font name to convert as the first argument" ; exit 1
fi

fc-list | grep -q "$1" || { echo "$1 is not a valid font" ; exit 1 ; }
fc-list | grep "$1" >> /tmp/matchingfonts.txt

# Skip slanted fonts
while : ; do
	FONT=$(cat /tmp/matchingfonts.txt | head -1 | sed 's/:.*//')
	QUERY=$(fc-query "$FONT")
	SLANT=$(echo "$QUERY" | grep "slant:" | grep -Po '\d+')
	sed -i '1d' /tmp/matchingfonts.txt
	[[ $SLANT > 0 ]] || break
done

echo "$QUERY"

NAME=$(echo "$QUERY" | grep "family:" | sed 's/.*"\(.*\)"[^"]*$/\1/')
HEIGHT=$(echo "$QUERY" | grep "pixelsize:" | grep -Po '\d+')
if [[ $HEIGHT < 4 ]]
then
	HEIGHT=32
fi

rm /tmp/matchingfonts.txt

# echo "$QUERY"

convert -list font | grep -q "$NAME" || { echo "Can not find font in ImageMagick cache" ; exit 1 ; }

# Get the actual image width
convert -compress None -font "$FONT" -pointsize $HEIGHT label:"A" /tmp/sizes.pbm
sed -i '1d' /tmp/sizes.pbm

FONTSTART=33
FONTEND=127
FONTRANGE=$(($FONTEND - $FONTSTART))

# Extract size of the image
TEXTSIZES=$(head -1 /tmp/sizes.pbm)
rm /tmp/sizes.pbm
SIZES=(${TEXTSIZES// / })

REALWIDTH=$((${SIZES[0]} - 2))
IMGWIDTH=$(($FONTRANGE * $REALWIDTH))

STRING=$(printf $(printf '\%o' {33..127}))
# Create a image with the whole array of letters
echo -n "$STRING" | convert -compress None \
	-font "$FONT" -pointsize $HEIGHT -size "${IMGWIDTH}x$HEIGHT" \
	caption:@- /tmp/output.pbm

# cp /tmp/output.pbm preview.pbm

# Remove header
sed -i '1,2d' /tmp/output.pbm

LOWERNAME=$(echo $NAME | tr -d '[[:space:]]' | tr '[:upper:]' '[:lower:]')
OUTPUTFILENAME="pf_$LOWERNAME.h"

# Generate C header file
echo -e \	"// Generated font file
static unsigned int ${LOWERNAME}fonttotalwidth = $IMGWIDTH;
static unsigned int ${LOWERNAME}fontwidth = $REALWIDTH;
static unsigned int ${LOWERNAME}fontheight = $HEIGHT;
static char ${LOWERNAME}fontstart = $FONTSTART;
static char ${LOWERNAME}fontrange = $FONTRANGE;
static char ${LOWERNAME}fontdata[] = {
	" > $OUTPUTFILENAME

sed 's/ /,/g' /tmp/output.pbm >> $OUTPUTFILENAME
rm /tmp/output.pbm

echo -e "\n};" >> $OUTPUTFILENAME

echo -e "Font header $NAME ($OUTPUTFILENAME) created\n\tglyph size: ${REALWIDTH}x$HEIGHT\n\twidth: $IMGWIDTH"
