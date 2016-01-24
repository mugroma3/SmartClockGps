# SmartClockGPS
A clock that synchronizes the time with a gps signal.
GPS modules receive atomic timestamp data from the satellites that they connect to.
This makes it possible to create a precise clock using GPS data.
This project intends to create a self-synchronizing clock, that will never lose time as long as it can get a GPS signal.
This can be very useful for a car clock for example.

*Since we have gps data with latitude and longitude, it could also be possible to detect which timezone the clock is in,
and automatically adjust the time accordingly (the timestamp received in the GPS data is in fact a UTC timestamp). This would however require quite some parsing to be in any way accurate, using for example the timezone database (tzdata, https://www.iana.org/time-zones).*

*--update on above comment: this is hardly feasible without using a web service, which you normally would not have in a project such as this one... unless you are not concerned about 100% accuracy; in fact a rough timezone approximation based on longitude can be done by multiplying the longitude (positive if you're east of Greenwich, negative if west) by 24 hours/360° . Perhaps can be left as a user option, with the other possibility being to manually set a timezone. Perhaps Olson TZDATA could be used by reducing it to the rules for current timezones, removing all the unneeded historical rules, in order to account for DST... --*

This code is an Arduino sketch that is a basis for creating such a "Smart" clock.
In the first testing phase, we are using a bluetooth GPS module with a Venus Skytraq chip (WondeX BT-100Y Bluetooth GPS Receiver).
We are also using an HC-05 bluetooth module to connect to the GPS module and stream the GPS data over serial to the Arduino.
