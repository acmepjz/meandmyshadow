import sys, os, os.path
from collections import defaultdict

#The path to the levelpack.
levelpackPath = ""

#The dictionary which will hold all the translatable strings.
dictionary = defaultdict(list)

#The file stream to write to.
potFile = None

#Some vars to keep track of where we are/get stuff from.
curfile = ""
curline = 1


#Method that will create the .pot file and open the file stream.
def openPotFile():
    for root, dirs, files in os.walk(levelpackPath):
        #Check if the directory 'locale' is present.
        fullpath = os.path.join(root, 'locale')
        if not os.path.exists(fullpath):
            os.makedirs(fullpath)
        fullpath = os.path.join(fullpath, 'messages.pot')
        global potFile
        potFile = open(fullpath,"w")
        break


#Method that loops through the files of the levelpack.
def loopfiles():
    for root, dirs, files in os.walk(levelpackPath):
        if root.count(os.sep) < 1:
            for f in files:
                fullpath = os.path.join(root, f)
                global curfile
                curfile = fullpath
                looplines(fullpath)

#Method that loops through the lines of a given file.
def looplines(f):
    fileInput = open(f,"r")
    global curline
    curline = 1
    for line in fileInput:
        lookup(line)
        curline += 1
    fileInput.close();

#Method that looks up the line to check if it contains a translatable string.
def lookup(line):
    signs = 0
    #Check for a '=' sign.
    for c in line:
        if c == '=':
            signs += 1
    if signs > 1:
        print "WARNING: Multiple '=' signs, using first one."
    if signs > 0:
        key = line.split( "=" )[0].rstrip('\r\n"').lstrip('"').strip()
        value = line.split( "=" )[1]

        #Check if the key is a translatable one.
        if key in ['congratulations', 'description', 'name', 'message']:
            #writeEntry('#: ' + curfile + ':' + str(curline), value)
            dictionary[value].append('#: ' + curfile + ':' + str(curline))

#Method that will write the header.
def writeHeader():
    potFile.write('# SOME DESCRIPTIVE TITLE.\n')
    potFile.write('# Copyright (C) YEAR THE PACKAGE\'S COPYRIGHT HOLDER\n')
    potFile.write('# This file is distributed under the same license as the PACKAGE package.\n')
    potFile.write('# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.\n')
    potFile.write('#\n')
    potFile.write('#, fuzzy\n')
    potFile.write('msgid ""\n')
    potFile.write('msgstr ""\n')
    potFile.write('"Project-Id-Version: PACKAGE VERSION\\n"\n')
    potFile.write('"Report-Msgid-Bugs-To: \\n"\n')
    potFile.write('"POT-Creation-Date: 2012-04-01 17:56+0300\\n"\n')
    potFile.write('"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n"\n')
    potFile.write('"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n"\n')
    potFile.write('"Language-Team: LANGUAGE <LL@li.org>\\n"\n')
    potFile.write('"Language: \\n"\n')
    potFile.write('"MIME-Version: 1.0\\n"\n')
    potFile.write('"Content-Type: text/plain; charset=CHARSET\\n"\n')
    potFile.write('"Content-Transfer-Encoding: 8bit\\n"\n')
    potFile.write('\n')

#Method that performs the actual writing.
def writeEntries():
    for msgid, comments in dictionary.iteritems():
        #Remove any trailing or leading '"' or '/n'
        msgid = msgid.rstrip('\r\n"').lstrip('"')

        #Write it to the file.
        for comment in comments:
            potFile.write(comment)
            potFile.write('\n')
        potFile.write('msgid "' + msgid + '"\n')
        potFile.write('msgstr ""\n')
        potFile.write('\n')


#First check command line arguments.
if len(sys.argv) != 2:
    print "Usage: python ./extract_pot.py <path/to/levelpack/>"
    sys.exit(0)

#Set the levelpack path
levelpackPath = sys.argv[1]

#Gather the translatable strings.
loopfiles()

#Now create the pot file and fill it.
openPotFile()
writeHeader()
writeEntries()
