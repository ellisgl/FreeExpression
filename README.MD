# Open Source firmware replacement for Cricut(tm) Expression #

----------

**IMPORTANT: REPROGRAMMING THE CPU WILL PERMANENTLY ERASE YOUR ORIGINAL FIRMWARE THERE IS NO WAY TO GET THE ORIGINAL FIRMWARE BACK OTHER THAN BUYING ANOTHER MACHINE.** 

**About the Cricut** 
Cricut cutting machines can be had for about $50 or less on Craigslist. They contain two stepper motors, a moving solenoid powered knife holder that generates 12 volts to the solenoid using PWM.  They typically contain wheels that control speed, pressure, and other settings (one of them is usually an optical encoder, the others are potentiometers), a display of sorts, and a keyboard that has a ton of fairly useless buttons with green LED backlights.  Additionally, an onboard USB port and the Atmel ATMega1281 chip running at 16mhz provides all the horsepower you would need.  The stepper motors are controlled by eight mosfets.  So in reality, you get a fairly decent XY controller board with a PWM output for solenoid control.

*******************************************************************************
Original firmware cannot be copied from another machine (it has been locked and cannot be read from the CPU)

Original firmware cannot be restored through any PC application used for normal firmware upgrades.

This firmware has only been tested on the machine that I have. It may not work on your machine.  It may cause permanent damage, and it
will certainly void your warranty.

This firmware cannot read your original cartridges ever.

Cricut is a trademark of PROVO CRAFT and this firmware is **not** a PROVO CRAFT product. This firmware  was developed completely independently, without any documentation of PROVO CRAFT products, and without any and all original firmware. If you have problems with this firmware, do not consult PROVO CRAFT. 

Forked from https://github.com/thetazzbot/FreeExpression, which was was forked from https://github.com/Arlet/Freecut.

This will be aimed at the Expression V1 machine, and not the Cake or Personal machines. This code base is intended to be "clean" and be open source chain friendly.


## If you still want to proceed:

You will need a ATMEL programmer device, search for "ATMEL 51 AVR USB ISP ASP Microcontroller Programmer". And if you don't want to open the machine, you might want to get one of these adapters built: https://gitlab.com/EllisGL/cricut-isp-cartridge (I have a couple of these, and would be willing to part with my extras)


### Building: ##

Install developements tools (gcc, make, etc), avr-gcc and avrdude.
in the `src` folder, run `make clean && make`


### Burning: ##

avrdude -c usbasp -p m1281 -U flash:w:FreeExpression.hex


### Using
Use Inkscape to draw and send a file to the cutter.  Inkscape 0.91 has an export to Plot feature (under Extensions) which sends a HPGL langauge file to the COM port where the cutter is connected to. Use 9600 baud and  make sure XON/XOFF is selected for flow control. Inkscape works good on Windows and Linux machines.

----------


## Keypad key functions: ##

- On/Off: Powers the machine and executes carriage homing sequence.
- Move buttons: Moves the carriage and the media freely.
- Home (ResetAll): Re-homes the carriage, media is assumed to be unloaded.
- Load Paper: Pulls in media or cutting mat about 1/2".
- Unload Paper: Moves the mat/media back to the loading point. 
- X,Y Offset (BackSpace): Sets the current X and Y position as location 0,0 for following cuts
- Cutter Down (Numeral 1): Drops the cutter down. This only works when the media is loaded. Useful  for measuring actual cutting force with a scale.
- Cutter Up (Numeral 2): Moves the cutter away from the media.
- Cutting Speed	(Xtra2): In conjunction with the +/keys the cutting speed can be chosen in 9 steps.
- Cutting Pressure (Xtra1) : In conjunction with the +/keys the cutting pressure can be adjusted in 9 steps.
- Stop: Aborts the currently cached cutting operations, but new data arriving from the PC will still trigger subsequent motion.

## Dials ##

- Dial Speed: 	Also adjusts the cutting speed and is used to read the cutting speed after power up. Only the mid range of the speed choices can be selected, use the +/keys to get all the way to the end.

- Dial Pressure: Also adjusts the cutting pressure and is used to read the initial pressure on power-up. Only the mid range of the pressure choices can be selected, use the +/keys for the full range.

- Dial Size:  Not used. The machine can cut up to 80"x12"wide.

## Multi-cut and media lock ##

After the machine moves the media/carriage there is a time of about 1 minute (depending on speed setting) where the motors stay engaged and you can not move the media/carriage by hand. After this timeout the motors go into standby and you can move carriage and media by hand. This timeout is useful so that one can cut the same shape multiple times and not loose registration - for thick material multi-cut.

# Command language #
Currently supports HPGL -- HP Graphic Language. Cutting speed and pressure is NOT taken from the language input, only from keyboard and associated dials.

After the machine moves the media/carriage there is a time of about 1 minute at speed 5 where the motors stay engaged and you can not move the media/carriage by hand. After this timeout the motors go into standby and you can move carriage and media by hand. This timeout is useful so that one can cut the same shape multiple times and not loose registration - for thick material multi-cut.


# CAD and Cutting #
From the CAD system export via DXF format, then open that file in Inkscape. On the input dialog choose manual scale factor of 25.4 if the original CAD file was done in inch scale, otherwise set to 1.0. 
To cut, select all objects then use PATH/Object to Path, the Extensions/Export/Plot.  Plotter resolution set to 400 dpi.  Pen number, force and speed are ignored. Rotation and Mirror as desired. 
Connection Settings are 9600/XON/XOFF and HPGL for language. Com port number as per device manger. To execute press Apply. 
