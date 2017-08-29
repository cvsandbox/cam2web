# Customizing Web UI
All release builds of cam2web come with embedded Web resources, so the application can provide default Web UI without relying on any extra files. However, it is possible to override default user interface even without diving into build tools.

Using configuration UI on Windows or command line options on Linux, it is possible to specify folder name to use for serving custom Web content. When the option is not set, embedded UI is provided. And when it is set, whatever is found in the specified folder will be provided instead.

### Getting default Web files
The easiest way to start customizing Web UI is to get all default content first as a starting point. All the required files can be found in the two folders: [src/web](src/web) and [externals/jquery](externals/jquery). Those files must be put into a single folder, so the basic directory tree should look like this:
```
index.html
cameraproperties.html
styles.css
cam2web.png
cam2web_white.png
camera.js
cameraproperties.js
cameraproperty.js
jquery.js
jquery.mobile.css
jquery.mobile.js
```
Once all files are in place, the application can be configured to serve Web content from the folder containing them. To make sure it is all working, alter some styles in the **styles.css** or change some HTML in the **index.html**. If it does, you are ready to go further customizing the Web UI.
