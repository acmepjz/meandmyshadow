#!/bin/bash

# Copyright (C) 2012 Me and My Shadow
#
# This file is part of Me and My Shadow.
#
# Me and My Shadow is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Me and My Shadow is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Me and My Shadow.  If not, see <http://www.gnu.org/licenses/>.

#variables
output="../data/locale/messages.pot"
version="0.5svn"

#automatically get all string from source code
xgettext -o ${output} -c" /" -k_ -k__ --package-name=meandmyshadow --package-version=${version} ../src/*.cpp

#little hack to get tranlator comments work
#also fix the charset problem
sed -i 's/#. \/ /#  /g; s/charset=CHARSET/charset=UTF-8/g' ${output}
echo >> ${output}

#make SDL key names translatable
#NOTE: this may generate invalid pot file since some names (e.g. Delete) are already defined
keys=("RETURN" "ESCAPE" "BACKSPACE" "TAB" "SPACE" "CAPSLOCK" "PRINTSCREEN" "SCROLLLOCK"
"PAUSE" "INSERT" "HOME" "PAGEUP" "DELETE" "END" "PAGEDOWN" "RIGHT" "LEFT" "DOWN" "UP" "NUMLOCK"
"SYSREQ" "LEFT CTRL" "LEFT SHIFT" "LEFT ALT" "LEFT GUI" "RIGHT CTRL" "RIGHT SHIFT" "RIGHT ALT"
"RIGHT GUI")
for i in ${!keys[*]}; do
	echo "#  TRANSLATORS: name of a key" >> ${output}
	echo "msgctxt \"keys\"" >> ${output}
	echo "msgid \"${keys[$i]}\"" >> ${output}
	echo "msgstr \"\"" >> ${output}
	echo >> ${output}
done
