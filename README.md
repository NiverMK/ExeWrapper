# ExeWrapper
Utility allows to pack .exe file into program's resources and run it (Unpack it from resources -> Create new process -> Write unpacked data into created process memory).
Using this mechanism it's possible to do with .exe file various things: from its encryption to dll injections into created process.

# How to use
### Base flow:
- Open ExeWrapper folder and run ExeWrapper.exe;
- Drag and drop desired *.exe file into console and press enter;
- Inside opened in console folder will be created a new *.exe file with a name of the source *.exe. Move it into desired folder.
- Run it.

To repack already wrapped .exe run it with param --wrap.

# Technical Information
Used C++ Version: C++20

Used libraries: STL, WinAPI, NtDll

Tested on Windows 10 (x64) and 11 (x64). 

x64 ExeWrapper can wrap and run x64 .exe files.
x32 ExeWrapper can wrap and run x32 .exe files on x64 system.

# TODO
- Move source *.exe attributes into created one;
- Add ability to wrap x32 .exe by x64 ExeWrapper and run it on x64 system.