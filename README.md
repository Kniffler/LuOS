# LµOS
Loser µ-controller Operating System

A neat little contraption of my own design.

# Basics
The structure of the code is quite simple. Any and all libraries are included in either the ```main.c``` file, or the ```CMakeLists.txt``` file. Anything CMake related can be found in its respective subdirectory. For example, the ```i2ckbd``` library (see [credits](#credits)) is inside the respective ```i2ckbd/``` folder. This trend continues for all libraries that are not a C standard.

# Credits
In terms of libraries and resources of others, I have in included the [FreeRTOS submodule from raspberrypi](https://github.com/raspberrypi/FreeRTOS-Kernel) in order to get FreeRTOS, and the ```lcdspi``` and ```i2ckbd``` folders/libraries from the [official PicoCalc repository](https://github.com/clockworkpi/PicoCalc).
There is also [this repository](https://github.com/Wilkua/pi-pico-freertos-starter) by the user Wilkua, which I have used as a basis for actually getting FreeRTOS to run. The config file for FreeRTOS was also copied from this repo. Please check it out if you'd like to make a project like this on your own.
