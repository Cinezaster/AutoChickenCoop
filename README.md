AutoChickenCoop
===============

A automated off the grid Chicken Coop. Closes and locks door at night against foxes and activates ventilation if it gets to hot in the coop.

Two light sensors measuring the light intensity every half a second. The average of the two sensors is compared with the imput of a potentiometer. When the value of the light intensity is lower then the threshold value of the potentiometer we check for 5min if that state is still valid and close the door of the coop. The same flow is used to open the door. This will prevents fast open en closing of door at dawn when the intensity of the light is floating around the threshold value.

The indoor temperature is checked every half minute and compared against the mapped value set by a potentiometer. If the temperature is higher a ventilator will be switched on to cool down the coop, to an acceptable temperature.

A button is added for testing purpose to open or close the coop. This a toggle action that decides if the door should open or close based on the state of the two end-stop switches.

For calibration purpose a flashing led is added to set the potentio meter.
