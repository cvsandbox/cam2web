var Camera = (function ()
{
    var jpegUrl       = '/camera/jpeg';
    var mjpegUrl      = '/camera/mjpeg';
    var mjpegMode;
    var frameInterval;
    var imageElement;
    var timeStart;

    function refreshImage( )
    {
        if ( mjpegMode )
        {
            imageElement.src = mjpegUrl;
        }
        else
        {
            timeStart = new Date( ).getTime( );
            imageElement.src = jpegUrl + '?t=' + timeStart;
        }
    }
    
    function onImageError( )
    {
        // try rotating between MJPEG/JPEG modes - browsers like IE don't get MJPEG at all
        mjpegMode = !mjpegMode;
        // also give it a small pause on error
        setTimeout( refreshImage, 1000 );
    }

    function onImageLoaded( )
    {
        if ( !mjpegMode )
        {
            var timeTaken = new Date( ).getTime( ) - timeStart;
            setTimeout( refreshImage, ( timeTaken > frameInterval ) ? 0 : frameInterval - timeTaken );
        }
    }
    
    var start = function( fps )
    {
        imageElement = document.getElementById( 'camera' );
        
        imageElement.onload  = onImageLoaded;
        imageElement.onerror = onImageError;
        
        if ( ( typeof fps == 'number' ) &&  ( fps != 0 ) )
        {
            frameInterval = 1000 / fps;
        }
        else
        {
            frameInterval = 100;
        }
        
        // always try capturing in MJPEG mode
        mjpegMode = true;
        
        refreshImage( );
    };

    return {
        Start: start
    }
} )( );
