# LµOS
Loser µ-controller Operating System

A neat little contraption of my own design.

Contents:
- [The Basics](#the-basics)
- [The Directive Terminal](#the-directive-terminal)
- [Build guide](#build-guide)
	- [Getting and building the project](#getting-and-building-the-project)
- [Changes and Libraries](#changes-and-libraries)
- [Credits](#credits)


# The basics

The main way to operate this "operating system" is to use the [directive terimal](#the-directive-terminal) 


# The Directive Terminal



# Build guide

This build guide assumes you are operating on the linux operating system where it is much easier to install the required tools and other project dependencies. 

Please consult the internet for instructions on how to get the following requirements when compiling this project:
- The [pico sdk](https://github.com/raspberrypi/pico-sdk)
- [CMake](https://cmake.org/download/)
- [git](https://github.com/git-guides)

> [!WARNING]
> The links provided above do not cover everything that needs to be done in order to get this project working on windows. Manual intervention may be required

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
Now ```cd``` into the directory and finally run the build command. The build command is a little different depending on whether you have a raspberry pi pico h, pico w, pico 2 h, or pico 2w.
Here are the commands for each:

pico h:
```
cmake -B build -DPICO_BOARD=pico
cd build
make
```

pico w:
```
cmake -B build -DPICO_BOARD=pico_w
cd build
make
```

pico 2/2w:
```
cmake -B build -DPICO_BOARD=pico2
cd build
make
```

And there you go! A finished uf2 file for your pico, ready to be flashed!

# Changes and libraries
The structure of the code is quite simple. Any and all libraries are included in either the ```main.c``` file, or the ```CMakeLists.txt``` file. Anything CMake related can be found in its respective subdirectory. For example, the ```i2ckbd``` library (see [credits](#credits)) is inside the respective ```i2ckbd/``` folder. This trend continues for all libraries that are not a C standard.

The submodules are utilized almost exactly as they are in the [official PicoCalc repository](https://github.com/clockworkpi/PicoCalc) meaning the pin-layout for all peripheral functionality (SD card, LCD screen, keyboard input...) is identical - due to hardware limitations. This also means any and all hardware-specific procedures have not been changed from the source code of the original repository, as that may compromise functionality.

```lcdpsi``` is the only library I have modified slightly. No functionality was changed, I only added a few custom functions to deal with the UI. This is because the frame time on the PicoCalc is... frankly rather horrid. These new functions serve to get information and deal with the UI more efficiently, so for example instead of clearing the entire screen and redrawing everything from scratch only the top row of characters is cleared and redrawn (this example refers to the [Directive Terminal](#the-directive-terminal)). This has saved me a CONSIDERABLE amount of frame time.

# Credits
In terms of libraries and resources of others, I have in included the [FreeRTOS submodule from raspberrypi](https://github.com/raspberrypi/FreeRTOS-Kernel) in order to get FreeRTOS, and the ```lcdspi``` and ```i2ckbd``` folders/libraries from the [official PicoCalc repository](https://github.com/clockworkpi/PicoCalc).

There is also [this repository](https://github.com/Wilkua/pi-pico-freertos-starter) by the user Wilkua, which I have used as a basis for actually getting FreeRTOS to run. The config file for FreeRTOS was also copied from this repo. Please check it out if you'd like to make a project like this on your own.

# TODO
### Change directive_print to use lcd_print_string
For now, the function is using lcd_putc to print the directive, however this messes with the lcd_char_pos variable in lcdspi.c. I have no idea how critical that variable is, aside from its definition it only seems to be used twice, but it may be that that messes with the entire drawing protocol. As a solution, although it is painful, I will change the directive_print() function to use lcd_print_string() for singular characters, in the hopes that the visual glitches on the picocalc will diminish.

### Add directive history 
Currently, the directive history, while recorded, doesn't seem to work properly. My best guess is that the pointers don't point where they should, but there is no evidence to suggest that. It's far more likely I'm accessing memory that I shouldn't, however I don't know where. Another possibility is that I simply forgot to erase a few debug lines here and there that just erase the history, the most likely culprit in that regard is copy_directive_history() since it copie's the directive history into a memory area that is allowed to be changed, and there are about 4 entry points into this system that, if any of them don't work, won't display the correct directive - or any for that matter.
An entry point being a point in the code that interferes with the variables directive_history and directive_changed_history. Especially the latter.





