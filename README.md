# ESP32-with-AD7705-both-channels
// THIS CODE WORKS on BOTH CHANNELS with 3v3 on a ESP32 DEV module with PlatformIO and the cheap chinese red module from ali
// The range in unipolar mode is strangely 31230 (0x8000) => 65535 (0xFFFF) on ch1 & ch2 connected to 2 potentiometers on 3v3
// DRDY does'nt work as expected and has been disabled, but some delay() have been added (to be shure...)
// With RESET connected to 3v3, you don't need to reset the chip during setup
// It seems that the same config can be obtained for both channels ONLY in the NORMAL mode, NOT with the self calibration mode !!!
// all routines are in the main file , the timing seems a little critical if you play with the init parameters
// My advise : play with it , then go buy an ADS1115 !!
