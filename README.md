# cfg-cam #
command-line application that saves and loads DirectShow webcam settings to/from the file

## Usage ##
Make sure that cameras are running (this application doesn't build new graph for the device).
All settings stored in cam_sett.cfg file at the same folder as .exe file.

Run without options to load saved settings.

Run with option "--savedev" to save current settings of all enabled DirectShow video input devices.

Run with option "--help" to get brief help info.

## Requirements ##
- Run: Windows 7 or late; any available DirectShow video input device with adjustable Video Proc Amp or Camera Control.
- Compile: Qt v5.8.0; Visual Studio 2013 UPD5 default compilers.

## Legal ##
Joined under MPLv2 if other not mentioned but whole project still under GPLv2.

## Conclusion ##
Some stuff just exist.
