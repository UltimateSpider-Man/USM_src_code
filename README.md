Ultimate Spider-Man Src Code
======

is a almost completed src code game engine that supports playing Ultimate Spider-Man by Treyarch. You need to own the game for src code to play Ultimate Spider-Man.

Current Status
--------------

The engine is not complete and is at the prototyping stage.

Requiments:
* Linux or Ubuntu on Windows 10
* MinGW-w64 compiler


How to build and install:
------------------------
* Copy `shaders` directory from project to the game's folder.
* Rename `binkw32.dll` in the game's folder to `binkw32_.dll`
* Download the repository to a directory convenient for you using the command:
`git clone https://github.com/UltimateSpider-Man/USM_src_code.git` 
* `cd openusm`
* `cmake -B build` (a `build` directory and makefile will be created)
* `cd build`
* `make -jN`, where N - how many threads you want to allot for compiling.
* Copy the compiled `USM.exe` from `build` directory to the game's folder.


License
------------

The code should only be used for educational, documentation and modding purposes.
I do not encourage piracy or commercial use.
Please keep derivate work open source and give proper credit.

