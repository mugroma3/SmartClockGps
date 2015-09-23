# SmartClockGPS
A clock that synchronizes the time with a gps signal.
GPS modules receive atomic timestamp data from the satellites that they connect to.
This makes it possible to create a precise clock using GPS data.
This project intends to create a self-synchronizing clock, that will never lose time as long as it can get a GPS signal.
This can be very useful for a car clock for example.
Since we have gps data with latitude and longitude, it is also possible to detect which timezone the clock is in,
and automatically adjust the time accordingly (the timestamp received in the GPS data is a UTC timestamp).

This code is an Arduino sketch that is a basis for creating such a "Smart" clock.
In the first testing phase, we are using a bluetooth GPS module with a Venus Skytraq chip (WondeX BT-100Y Bluetooth GPS Receiver).
We are also using an HC-05 bluetooth module to connect to the GPS module and stream the GPS data over serial to the Arduino.
