# Accessing camera from other applications (WEB API)

The streamed camera can be accessed not only from web browser, but also from any other application supporting MJPEG streams (like VLC media player, for example, or different applications for IP cameras monitoring). The URL format to access MJPEG stream is:
```
http://ip:port/camera/mjpeg
```

In the case an individual image is required, the next URL provides the latest camera snapshot:
```
http://ip:port/camera/jpeg
```

### Camera information
To get some camera information, like device name, width, height, etc., an HTTP GET request should be sent the next URL:
```
http://ip:port/camera/info
```
It provides reply in JSON format, which may look like the one below:
```JSON
{
  "status":"OK",
  "config":
  {
    "device":"RaspberryPi Camera",
    "height":"480",
    "width":"640"
  }
}
````

### Changing camera’s settings
```
http://ip:port/camera/config
```
To get current camera’ settings, an HTTP GET request is sent to the above URL, which provides all values as JSON reply:
```JSON
{
  "status":"OK",
  "config":
  {
    "awb":"Auto",
    "brightness":"50",
    "contrast":"15",
    "effect":"None",
    "expmeteringmode":"Average",
    "expmode":"Auto",
    "hflip":"1",
    "saturation":"25",
    "sharpness":"100",
    "vflip":"1",
    "videostabilisation":"0"
  }
}
```
In the case if only some values are required, their names (separated with coma) can be passed as **vars** variable. For example:
```
http://ip:port/camera/config?vars=brightness,contrast
```

For setting camera’s properties, the same URL is used, but HTTP POST request must be used. The posted data must contain collection of variables to set encoded in JSON format.
```JSON
{
  "brightness":"50",
  "contrast":"15"
}
```
On success, the reply JSON will have **status** variable set to "OK". Or it will contain failure reason otherwise.

### Getting version information
To get information about version of the cam2web application streaming the camera, the next URL is used
```
http://ip:port/version
```
Which provides information in the format below:
```JSON
{
  "status":"OK",
  "config":
  {
    "platform":"RaspberryPi",
    "product":"cam2web",
    "version":"1.0.0"
  }
}
```

### Access rights
Accessing JPEG, MJPEG and camera information URLs is available to those who can view the camera. Access to camera configuration URL is available to those who can configure it. The version URL is accessible to anyone. See [Running cam2web](Running.md) for more information about access rights.
