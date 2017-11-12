# Running cam2web
cam2web application comes as a single executable, which does not require any installation to run correctly. Once built or extracted from downloaded package, it is ready to run and stream a camera over HTTP.  Release builds have all web resources embedded into the executable, so no extra files are needed to access camera from web browsers.

## Windows version
Windows version of cam2web provides graphical user interface, which allows to start/stop camera streaming and set different configuration settings. The application’s main window lists all detected cameras and their supported resolutions. Once selection is done, streaming can be started. All other settings are optional and are used to change default behaviour.

**Note**: The resolution box shows default average frame rate for all resolutions supported by a camera. However, some cameras support a range of frames rates - minimum/maximum rate. For such cameras it is possible to override default frame rate and set the one needed. But, don't expect all frame rate values to work from the provided range. Due to limitations of DirectShow API and badly written drivers of some cameras, many frame rate values may no work.

![winMain](https://github.com/cvsandbox/cam2web/blob/master/images/win_main.png)

When a camera is streamed, it can be accessed from a web browser – http://ip:port/ (or by clicking the link in the main window for a quick start). If camera provides any configuration settings, like brightness, contrast, saturation, etc., those can be changed from the web UI.

The "Settings" window allows changing different application’s option like HTTP port to listen on, maximum frame rate of MJPEG stream, quality level of JPEG images, etc.:

![winSettings](https://github.com/cvsandbox/cam2web/blob/master/images/win_settings.png)

The "Access rights" window allows to configure who can view the camera and change its settings. By default, anyone can do everything. However, if there is no desire to keep the camera public, it is possible to limit access only to registered users. Furthermore, changing camera’s settings can be restricted more and allowed to administrators only.

![winAccessRights](https://github.com/cvsandbox/cam2web/blob/master/images/win_access_rights.png)

**Note**: changing any application’s settings requires restarting camera streaming.

The Windows version of cam2web also provides few command line options:
* /start – Automatically start camera streaming on application start.
* /minimize – Minimize application’s window on its start.
* /fcfg:file_name – Name of configuration file to store application’s settings. By default, the application stores all settings (including last run camera/resolution) in app.cfg stored in cam2web folder within user’s home directory. However, the name of configuration file can be set different, so several instances of the application could run, having different settings and streaming different cameras.

## Linux and Raspberry Pi versions
Both Linux and Raspberry Pi versions are implemented as command line applications, which do not provide any graphical user interface. Running them will start streaming of the available camera automatically using default settings. However, if **-?** option is specified, the list of supported command line options is provided, which includes their description.

Same as with Windows version, once the camera is streamed, it is accessible on http://ip:port/ URL.

Unlike Windows version, the Linux/Pi version does not provide means for editing users’ list who can access camera. Instead, the [Apache htdigest](https://httpd.apache.org/docs/2.4/programs/htdigest.html) tool is used to manage users’ file, which name can then be specified as one of the cam2web’s command line options. This creates a limitation though – only one user with administrator role can be created, which is **admin**. All other names get user role.

