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

Now ```cd``` into the directory and finally run the build command. The build command is a little different depending on whether you have a raspberry pi pico h, pico w, pico 2h, or pico 2w.
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



The submodules are utilized almost exactly as they are in the [official PicoCalc repository](https://github.com/clockworkpi/PicoCalc) meaning the pin-layout for all peripheral functionality (SD card, LCD screen, keyboard input...) is identical - due to hardware limitations. This also means any and all hardware-specific procedures have not been changed from the source code of the original repository, as that may compromise functionality.

```lcdpsi``` is the only library I have modified slightly. I only added a few custom functions to deal with the UI. This is because the frame time on the PicoCalc is... frankly rather horrid. These new functions serve to get information and deal with the UI more efficiently, so for example instead of clearing the entire screen and redrawing everything from scratch only the top row of characters is cleared and redrawn (this example refers to the [Directive Terminal](#the-directive-terminal)). This has saved me a CONSIDERABLE amount of frame time.

# Credits
In terms of libraries and resources of others, I have in included the [FreeRTOS submodule from raspberrypi](https://github.com/raspberrypi/FreeRTOS-Kernel) in order to get FreeRTOS, and the ```lcdspi``` and ```i2ckbd``` folders/libraries from the [official PicoCalc repository](https://github.com/clockworkpi/PicoCalc).

There is also [this repository](https://github.com/Wilkua/pi-pico-freertos-starter) by the user Wilkua, which I have used as a basis for actually getting FreeRTOS to run. The config file for FreeRTOS was also copied from this repo. Please check it out if you'd like to make a project like this on your own.

# TODO
## Apps
Finally add the structure for apps to the mix. This will be a bit of work in-terms of CMake scripting, but should ultimately allow for apps to be implemented with ease.



