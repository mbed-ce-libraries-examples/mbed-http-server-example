# HTTP Server Example

This project is a conversion of the [http webserver example](https://os.mbed.com/teams/sandbox/code/http-webserver-example/) by Jan Jongboom to Mbed CE.  

Note: As currently written, this project should work with any Mbed board that has an Ethernet connection. Support for wifi boards is also possible, but you will need to instantiate the wi-fi interface with the right credentials.

## How to set up this project:

1. Clone it to your machine.  Don't forget to use `--recursive` to clone the submodules: `git clone --recursive https://github.com/mbed-ce-libraries-examples/mbed-http-server-example.git`
2. Set up the GNU ARM toolchain (and other programs) on your machine using [the toolchain setup guide](https://github.com/mbed-ce/mbed-os/wiki/Toolchain-Setup-Guide).
3. Set up the CMake project for editing.  We have three ways to do this:
    - On the [command line](https://github.com/mbed-ce/mbed-os/wiki/Project-Setup:-Command-Line)
    - Using the [CLion IDE](https://github.com/mbed-ce/mbed-os/wiki/Project-Setup:-CLion)
    - Using the [VS Code IDE](https://github.com/mbed-ce/mbed-os/wiki/Project-Setup:-VS-Code)
4. Build the `flash-mbed-http-server-example` target to upload the code to a connected device.