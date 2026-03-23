# LµOS
Loser µ-controller Operating System

A neat little contraption of my own design.

Contents:
- [The Bootloader](#bootloader)
- [The settings file structure](#the-settings-file-structure)
- [Build guide](#build-guide)
	- [Getting and building the project](#getting-and-building-the-project)
- [Changes and Libraries](#changes-and-libraries)
- [TODO](#todo)

# The Bootloader
Upon launching the picocalc the bootloader will be the first thing you interface with, every time. Much like the picocalc comes out of the box. However this bootloader also comes equipped with a custom font and interface (more on that later) as well as a settings menu. This settings menu is/will be able to access a configuration file structure on the SD card such that other apps may access it.

# The settings file structure
The menu of the bootloader is split up into 2, using the `splitter.h` interface and as such cannot go deeper than 1 folder into the file-system of the SD card. This behaviour is intentional, as it allows for a bit of organization while also keeping the ability to hide files from the user - such as the setting files.

# Build guide
This build guide assumes you are using the linux operating system where it is much easier to install the required tools and other project dependencies. 

Please consult the internet for instructions on how to get the following depedencies when compiling this project:
- The [pico sdk](https://github.com/raspberrypi/pico-sdk) for hardware support
- [CMake](https://cmake.org/download/) to guide compiling
- [git](https://github.com/git-guides) for ease of installing

> [!WARNING]
> The links provided above do not cover everything that needs to be done in order to get this project working on windows. Manual intervention may be required.

A text editor of your choice also doesn't hurt.

## Getting and building the project
Assuming you met the dependencies earlier, you can now clone this repository locally using
```
git clone https://github.com/Kniffler/LuOS
```
or 
```
gh repo clone Kniffler/LuOS
```
if you have [github cli](https://cli.github.com/) installed.

Now `cd` into the directory and finally run the build command. The build command is a little different depending on whether you have a raspberry pi pico h, pico w, pico 2h, or pico 2w.
Here are the commands for each:

pico h:
```
cmake -B build
make -C build
```

pico w:
```
cmake -B build -DPICO_BOARD=pico_w
make -C build
```

pico 2/2w:
```
cmake -B build -DPICO_BOARD=pico2
make -C build
```
The resultant uf2 file can now be found under /path/of/repository/LuOS/build/LUOS.uf2

> [!IMPORTANT]
> The `build.sh` script is not to be used if you want to run this project as is. If you plan on modifying the source code then this script is at your disposal for quick debugging however, you will need to change the DEVICE_MOUNTS_BY_NAME variable in the script if your pico mounts under a different label.

# Changes and libraries

In terms of libraries and resources of others, I have:
- Included the `lcdspi` and `i2ckbd` folders/libraries from the [official PicoCalc repository](https://github.com/clockworkpi/PicoCalc) keeping [uthash](https://github.com/troydhanson/uthash) as a submodule.
- Added the [pico-vfs](https://github.com/oyama/pico-vfs) repository as a submodule.
- Used [Picocalc_SD_Boot](https://github.com/adwuard/Picocalc_SD_Boot) for most (well, all) of the booloader code.

There is also [this article](https://github.com/adwuard/Picocalc_SD_Boot) which is linked in the Picocalc_SD_Boot repository source code. It goes into detail about the boot routine of the pico and paints a great picture of how the bootloader code works. I highly recommend checking it out.


`lcdpsi` has been heavily modified to work with so called regions. Every app now needs to define one or more regions of the screen via the `lcd_region_create` function. This has also made the library considerably harder to read. In the future, comments will be added and the code will be revised in order to combat this issue.

Further explanations for regions may or may not be added to the README and a wiki (if I ever get around to making a wiki that is)


# TODO
## Finish the splitter.h library

