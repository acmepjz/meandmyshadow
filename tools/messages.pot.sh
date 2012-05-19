#!/bin/sh

#automatically get all string from source code
xgettext -o ../data/locale/messages.pot -c" /" -k_ -k__ --package-name=meandmyshadow --package-version=0.4 ../src/*.cpp

#little hack to get tranlator comments work
sed -i 's/#. \/ /#  /g' ../data/locale/messages.pot
