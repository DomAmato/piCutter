Laser_Engraver
==============

A Raspberry Pi controlled laser cutter using python

This software allows for the control of a laser engraver with a Raspberry Pi.  

This code is based off of the code by Xiang Zhai and Ian D. Miller.

It has been altered to use the EasyDriver Stepper Motor Driver:
https://www.sparkfun.com/products/10267

Currently adding variable speed control and variable laser power


Raster Image To Gcode Converter
===============================

Takes raster images, converts them to grayscale, applys a threshold and then converts the processed image to gcode

regular mode creates smaller file sizes but interpreters may have problems since it will not write a change in the x/y/z 
unless there is a change

verbose mode will output all numbers and commands 
