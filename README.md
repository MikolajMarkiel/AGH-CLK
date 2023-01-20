# AGH-CLK
Tiny project of embedded wristwatch based on STM32 realised for University - course Design-Laboratory

## Project assumptions
The main goal was to make self-contained embedded wristwatch module that could be used with dedicated wrist as a complete wristwatch

## Technical specification
-	LCD screen 240x240 1,3 inches
-	Clock precision: -6 sec/day (would be better with capacitor corrections)
-	Current consumption: 54,5 mA (without sleep feature)
-	Estimated lifetime per cycle – around 9h (would be better with implemented sleep feature)
-	Charging: 5 VDC; 400 mA; around 75 minutes to full charge battery 500mAh 

## Overview
This project has been designed for purpose of learning STM32 programming and designing PCB with KiCad software. It won't be supported or improved in future. 
There are many mistakes such as huge current consumption, clock precision or aliased display. It could be of course improved for example by adding sleep feature, but it is not necessary to do right now, because others projects also wait to be designed. 

For more information about the project lets read AGH-CLK-Design_Process_Specification inside "Documentation" folder.