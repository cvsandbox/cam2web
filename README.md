![cam2web](images/cam2web.png)
# cam2web

cam2web is an application, which allows streaming a camera to web as MJPEG stream (an URL to provide individual JPEGs is also available). It allows turning a conventional USB camera (or laptop's internal camera) into an IP camera accessible over HTTP. Versions of this application are provided for:
* Windows - streaming cameras supporting DirectShow API;
* Linux - streaming cameras supporting V4L2 API;
* Raspberry Pi - streaming from Raspberry Pi Camera Module.

The streamed camera can be viewed as from different applications supporting MJPEG streams, as from a web browser. All versions of the application provide default web UI, which can be customized to get custom look and feel. If a camera supports settings like brightness, contrast, saturation, etc. - those are available to configure using the web UI (or REST API).

The cam2web application supports HTTP digest authentication and allows to configure who can view the camera or change its settings (for example, it can be viewed by everyone and configured only by users, or viewed by users and configured by admins only, etc).

* [Building the source code](Building.md)
* [Running the application](Running.md)
* [Accessing camera from other applications (WEB API)](WebAPI.md)
* [Customizing Web UI](CustomWebUi.md)
