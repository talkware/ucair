# Installing Visual C++ #

For the free Express version, you can download it from http://www.microsoft.com/express/Windows/

# Installing boost #

Download the boost installer from http://www.boostpro.com/download

Compilers: the correct VC version

Variants: at least Multithread and Multithread Debug

Components: at least DateTime, Filesystem, ProgramOptions, Regex, Signals, Thread

Destination folder: I'm using C:\Program Files\boost\boost\_1\_42 for example.

# Installing sqlite3 #

Download the src from
http://www.sqlite.org/download.html

I've created a VC solution [here](http://ucair.googlecode.com/files/sqlite3_vc_sln.zip).

Copy sqlite3.h to C:\Program Files\sqlite3\include

Build under Debug, and copy the lib file to C:\Program Files\sqlite3\lib\sqlite3d.lib

Build under Release, and copy the lib file to C:\Program Files\sqlite3\lib\sqlite3.lib

# Installing MXML #

Download the src from
http://www.minixml.org/software.php

I've created a VC solution [here](http://ucair.googlecode.com/files/mxml_vc_sln.zip).

Copy config.h and mxml.h to C:\Program Files\MXML\include

Build under Debug, and copy the lib file to C:\Program Files\MXML\lib\mxmld.lib

Build under Release, and copy the lib file to C:\Program Files\MXML\lib\mxml.lib

# Compiling UCAIR #

Download the src from the download page.

Go to Project Properties, adjust C/C++ | General | Additional Include Directories, Linker | General | Additional Library Directories.

Build the project and run. You can then type http://localhost:8080/search in your browser (FF3 or IE8)

If program takes a long time to start in debug mode, you can change config.ini to use col\_stats\_small, which loads faster.