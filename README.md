These programs are clients for the P25 networking now built into the MMDVM Host.

The Parrot is very simple minded and can only handle one client at a time and is
therefore not suitable for use as a shared resource via the Internet.

The Gateway allows for use of P25 Talk Groups to control the access to the various
P25 reflectors.

The Gateway has an ini file that contain the parameters for running the software.
The filename of the ini file is passed as a parameter on the command line. The
Parrot takes the UDP port number to listen on as an argument.

The MMDVM .ini file should have the IP address and port number of the client in
the [P25 Network] settings.

The file that contains the information about the reachable reflectors is held in
the P25Hosts.txt file that should be donwloaded from the DVRef.com web site. A
script to do this under Linux is included. This is handled automatically in WPSD
and Pi-Star.

They build on 32-bit and 64-bit Linux as well as on Windows using Visual Studio
2019 on x86 and x64.

This software is licenced under the GPL v2 and is primarily intended for amateur
and educational use.
