# Building cam2web

Before cam2web can be built in release configuration, it is required to build web2h tool provided with it, which translates some of the common web files (HTML, CSS, JS, JPEG and PNG) into header files. Those are then compiled and linked into the cam2web executable, so it could provide default web interface without relying on external files.

If building in debug configuration however, the web2h is not required – all web content is served from files located in ./web folder.

## Building on Windows
Microsoft Visual Studio solution files are provided for both web2h and cam2web applications (Express 2013 can be used, for example). First build **src/tools/web2h/make/msvc/web2h.sln** and then **src/apps/win/cam2web.sln**. On success, it will produce **build/msvc/[configuration]/bin** folder, which contains applications’ executables.

## Building on Linux and Raspberry Pi
Makefiles for GNU make are provided for both web2h and cam2web. Running bellow commands from the project’s root folder, will produce the required executables in **build/gcc/release/bin**.
```Bash
pushd .
cd src/tools/web2h/make/gcc/
make
popd

pushd .
cd src/apps/linux/
# or cd src/apps/pi/
make
popd
```
Note: libjpeg development library must be installed for cam2web build to succeed (which may not be installed by default) :
```
sudo apt-get install libjpeg-dev
```
