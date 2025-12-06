# LµOS
Loser µ-controller Operating System

A neat little contraption of my own design.

Contents:
- [The Directive Terminal](#the-directive-terminal)
- [Build guide](#build-guide)
	- [Getting and building the project](#getting-and-building-the-project)
- [Changes and Libraries](#changes-and-libraries)
- [Credits](#credits)


# The Directive Terminal
In the top 2 rows of the screen, there will always be the directive terminal in view. Any and all system features have to be accessed using this feature.
While there are currently only 2 commands (.clear and .test), more will be added with the extension of the system.

There are 3 types of inputs the directive terminal accepts:
- App launch operations
  - Which are prepended with a comma (,)
- System directives
  - Which are prepended with a full-stop (.)
- App referals
  - Which can start with any character except the aforementioned ones.

### App launch operations
These are prepended with a comma (,) and serve no purpose other than launching a respective app. Here is an example for the launching of iowiz (which is an app that will be released in an upcoming commit):
`>,iowiz`
> [!NOTE]
> The '>' is just an indicative character to show where the current command line is. It exists only for aesthetic reasons.

### System directives
Prepended with a comma (,) these are used to control the pico on the admin level (hah! as if I had the patience to invent a user space xD)

There are only 2 of these at the present moment, however a general `.help` directive will be released soon, which will include a detailed list of commands and apps as well as general functionality of the system.
> [!NOTE]
> When this directive will be available, it is reasonable to assume that it is not always up to date as I may forget to do so on occasion.

### App referals
These, while having a confusing name, serve quite a fundamental function for apps. Simply put, whenever an app is open but is not in focus - that is to say all keyboard input goes to the directive terminal - then the directive will go directly to a function in the app whenever enter is pressed. This only works if the input is not prepended with a comma (,) or full-stop (.).

# Build guide
This build guide assumes you are operating on the linux operating system where it is much easier to install the required tools and other project dependencies. 

Please consult the internet for instructions on how to get the following requirements when compiling this project:
- The [pico sdk](https://github.com/raspberrypi/pico-sdk) for hardware support
- [CMake](https://cmake.org/download/) to guide compiling
- [git](https://github.com/git-guides) for ease of installing

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

Now `cd` into the directory and finally run the build command. The build command is a little different depending on whether you have a raspberry pi pico h, pico w, pico 2h, or pico 2w.
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

However, `lcdpsi` has been heavily modified to work with so called regions. Every app now needs to define a region of the screen via the `lcd_region_create` function. This has also made the library considerably harder to read. In the future, comments will be added and the code will be revised in order to combat this issue.

Further explanations for regions may or may not be added to the README and a wiki (if I ever get around to making a wiki that is)

# Credits
In terms of libraries and resources of others, I have in included the [FreeRTOS submodule from raspberrypi](https://github.com/raspberrypi/FreeRTOS-Kernel) in order to get FreeRTOS, and the `lcdspi` and `i2ckbd` folders/libraries from the [official PicoCalc repository](https://github.com/clockworkpi/PicoCalc).

There is also [this repository](https://github.com/Wilkua/pi-pico-freertos-starter) by the user Wilkua, which I have used as a basis for actually getting FreeRTOS to run. The config file for FreeRTOS was also copied from this repo. Please check it out if you'd like to make a project like this on your own.

# TODO
## Apps (in progress)
While the basic structure still needs work, the recent region update to the lcdspi library will make the programming of apps easier, knowing draw calls will not interfere with each other... okay that is more of a lie, the draw calls may still interfere with each other when different FreeRTOS tasks are calling on the lcdspi library - which is a problem that currently does not need solving but I should definitely not ignore it.

Further additions have to be made for the structure of app development, that being said progress has been going well.
