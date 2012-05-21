#!/bin/bash

#variables
output="../data/locale/messages.pot"
version="0.4"

#automatically get all string from source code
xgettext -o ${output} -c" /" -k_ -k__ --package-name=meandmyshadow --package-version=${version} ../src/*.cpp

#little hack to get tranlator comments work
sed -i 's/#. \/ /#  /g' ${output}
echo >> ${output}

#make SDL key names translatable
keys=("backspace" "tab" "clear" "return" "pause" "escape" "space" "delete" "enter" "equals"
"up" "down" "right" "left" "insert" "home" "end" "page up" "page down" "numlock" "caps lock"
"scroll lock" "right shift" "left shift" "right ctrl" "left ctrl" "right alt" "left alt"
"right meta" "left meta" "left super" "right super" "alt gr" "compose" "help" "print screen"
"sys req" "break" "menu" "power" "euro" "undo")
for i in ${!keys[*]}; do
	echo "#  TRANSLATORS: name of a key" >> ${output}
	echo "msgid \"${keys[$i]}\"" >> ${output}
	echo "msgstr \"\"" >> ${output}
	echo >> ${output}
done
