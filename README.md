# esp32_automatic_gardening
this code is derrived from https://github.com/TheBrunez/ESP32_FlowerCare so credits also go to him :)

when moisture falls below a certain threshold, a corresponding pin is driven high in intervalls(so that soil can soak up the water) to activate a water pump.
when moisture exeeds another certain threshold, moisture is getting checked in slower intervalls until it falls below the first threshold again.

this code is only tested with 2 devices, but should work with more or less when changed in the config.h
also not long term tested. please handle the whole thing as work in progress.

for better understanding of how to use the code, also reffer to theBrunez readme.
for even better understanding try to understand the code itself, it was never ment to be published but then people asked ;)
