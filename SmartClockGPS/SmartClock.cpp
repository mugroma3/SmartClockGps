#include <Arduino.h>
#include <LiquidCrystal.h>
#include <EEPROMex.h>
#include "Timer.h"                     //https://github.com/JChristensen/Timer/tree/v2.1

String HC05_ERRORMESSAGE[29] = {
  "Command Error/Invalid Command",
  "Results in default value",
  "PSKEY write error",
  "Device name is too long (>32 characters)",
  "No device name specified (0 length)",
  "Bluetooth address NAP is too long",
  "Bluetooth address UAP is too long",
  "Bluetooth address LAP is too long",
  "PIO map not specified (0 length)",
  "Invalid PIO port Number entered",
  "Device Class not specified (0 length)",
  "Device Class too long",
  "Inquire Access Code not Specified (0 length)",
  "Inquire Access Code too long",
  "Invalid Inquire Access Code entered",
  "Pairing Password not specified (0 length)",
  "Pairing Password too long (> 16 characters)",
  "Invalid Role entered",
  "Invalid Baud Rate entered",
  "Invalid Stop Bit entered",
  "Invalid Parity Bit entered",
  "No device in the Pairing List",
  "SPP not initialized",
  "SPP already initialized",
  "Invalid Inquiry Mode",
  "Inquiry Timeout occured",
  "Invalid/zero length address entered",
  "Invalid Security Mode entered",
  "Invalid Encryption Mode entered"
};

