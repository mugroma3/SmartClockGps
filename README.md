# SmartClockGPS
A clock that synchronizes the time with a gps signal.
GPS modules receive atomic timestamp data from the satellites that they connect to.
This makes it possible to create a precise clock using GPS data.
This project intends to create a self-synchronizing clock, that will never lose time as long as it can get a GPS signal.
This can be very useful for a car clock for example.

Since we have gps data with latitude and longitude, we can also roughly approximate the current timezone with a simple little formula (current timezone = floor(longitude degrees + 7.5) / 15), and automatically adjust the time accordingly (the timestamp received in the GPS data is in fact a UTC timestamp). This clearly does not account for or take into consideration Daylight Savings Time. In order to have any greater precision or to account also for DST automatically, it would be necessary to use a web service that can calculate precise timezone data based on GPS coordinates using for example the timezone database (tzdata, https://www.iana.org/time-zones) and using political data about the geographical mapping of the timezones according to country.

This code is an Arduino sketch that is a basis for creating such a "Smart" clock.
In the first testing phase, we are using a bluetooth GPS module with a Venus Skytraq chip (WondeX BT-100Y Bluetooth GPS Receiver).
We are also using an HC-05 bluetooth module to connect to the GPS module and stream the GPS data over serial to the Arduino.
