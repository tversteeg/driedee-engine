#!/bin/bash

convert -version > /dev/null || { echo "ImageMagick is not installed" ; exit 1 ; }

if [ $# -ne 1 ]
then
	echo "Pass the font name to convert as the first argument" ; exit 1
fi

fc-list | grep -q "$1" || { echo "$1 is not a valid font" ; exit 1 ; }
fc-list | grep "$1" >> /tmp/matchingfonts.txt

FONTSTART=33
FONTEND=126
FONTRANGE=$(($FONTEND - $FONTSTART))

STRING=$(printf $(printf '\%o' {33..126}))
STRINGQUOTED=$(echo "$STRING" | sed 's/\"/\\\"/')

echo "
import sys
import fontforge

font = fontforge.open(sys.argv[1])

char = -1
for c in \"$STRINGQUOTED\":
	try:
		font[c]
	except TypeError:
		continue
	char = c

width = font[char].width

if char < 0:
	font.close()
	sys.exit(1)

for c in \"$STRINGQUOTED\":
	try:
		font[c]
	except TypeError:
		continue

	if font[c].width != width:
		font.close()
		sys.exit(1)

print font[char].width
font.close()
" > /tmp/monofacecheck.py

# Skip slanted & non-monospaced fonts
while : ; do
	if [[ ! -s /tmp/matchingfonts.txt ]] ; then
		rm /tmp/matchingfonts.txt
		echo "Could not find valid font"
		exit 1
	fi

	FONT=$(cat /tmp/matchingfonts.txt | head -1 | sed 's/:.*//')
	sed -i '1d' /tmp/matchingfonts.txt

	QUERY=$(fc-query "$FONT")
	SLANT=$(echo "$QUERY" | grep "slant:" | grep -Po '\d+')
	[[ $SLANT -eq 0 ]] || continue

	MONOSPACED=$(python /tmp/monofacecheck.py "$FONT") || continue
	break
done

rm /tmp/monofacecheck.py
rm /tmp/matchingfonts.txt

NAME=$(echo "$QUERY" | grep "family:" | sed 's/.*"\(.*\)"[^"]*$/\1/')

if [ $(echo "$QUERY" | grep -q "pixelsize:") ]; then
	HEIGHT=$(echo "$QUERY" | grep "pixelsize:" | grep -Po '\d+')
else
	TEXTHEIGHT=$(echo "$QUERY" | grep "size" | grep -Po '\d+')
	TEXTHEIGHTARR=(${TEXTHEIGHT// / })
	HEIGHT=${TEXTHEIGHTARR[1]}
fi

convert -list font | grep -q "$NAME" || { echo "Can not find font in ImageMagick cache" ; exit 1 ; }

# Get the actual image width
convert -compress None -font "$FONT" -pointsize $HEIGHT label:"A" /tmp/sizes.pbm
sed -i '1d' /tmp/sizes.pbm

# Extract size of the image
TEXTSIZES=$(head -1 /tmp/sizes.pbm)
rm /tmp/sizes.pbm
SIZES=(${TEXTSIZES// / })

REALWIDTH=$((${SIZES[0]} - 2))
IMGWIDTH=$(($FONTRANGE * $REALWIDTH))
# Create a image with the whole array of letters
echo -n "$STRING" | convert -compress None \
	-font "$FONT"	-pointsize $HEIGHT -size "${IMGWIDTH}x$HEIGHT" \
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
