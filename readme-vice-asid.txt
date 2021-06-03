VICE 2.4 asid hack

v0.1
	first working, slighty cleaned up version.

aTc@k-n-p.org


usage:

To enable the asid output, use the "-sounddev asid" command line option.
Select the midi port to use with "-soundargs".
The list of ports is output when the asid driver is started (either on the command line, or in vice.log)
It defaults to port 0, which is usually some internal port.

x64 -sounddev asid -soundargs 1

It can also be set in vice.ini :

SoundDeviceName="asid"
SoundDeviceArg="1"



