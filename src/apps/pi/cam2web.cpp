/*
    cam2web - streaming camera to web

    Copyright (C) 2017, cvsandbox, cvsandbox@gmail.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "XRaspiCamera.hpp"
#include "XRaspiCameraConfig.hpp"
#include "XWebServer.hpp"
#include "XVideoSourceToWeb.hpp"
#include "XObjectConfiguratorRequestHandler.hpp"
#include "XObjectInformationRequestHandler.hpp"
#include "XManualResetEvent.hpp"

using namespace std;

XManualResetEvent ExitEvent;

struct
{
    uint32_t FrameWidth;
    uint32_t FrameHeight;
    uint32_t FrameRate;
}
Settings;

void sigIntHandler( int s )
{
    ExitEvent.Signal( );
}

// Set default values for settings
void SetDefaultSettings( )
{
    Settings.FrameWidth  = 640;
    Settings.FrameHeight = 480;
    Settings.FrameRate   = 30;
}

// Parse command line and override default settings
bool ParsetCommandLine( int argc, char* argv[] )
{
    static const uint32_t SupportedWidth[]  = { 320, 480, 640, 800, 1120 };
    static const uint32_t SupportedHeight[] = { 240, 360, 480, 600, 840 };

    bool ret = true;
    int  i;
    
    for ( i = 1; i < argc; i++ )
    {
        char* ptrDelimiter = strchr( argv[i], ':' );
        
        if ( ( ptrDelimiter == nullptr ) || ( argv[i][0] != '-' ) )
        {
            break;
        }
        
        string key   = string( argv[i] + 1, ptrDelimiter - argv[i] - 1 );
        string value = string( ptrDelimiter + 1 );
        
        if ( ( key.empty( ) ) || ( value.empty( ) ) )
            break;
        
        if ( key == "size" )
        {
            int v = value[0] - '0';

            if ( ( v < 0 ) || ( v > 4 ) )
                break;
            
            Settings.FrameWidth  = SupportedWidth[v];
            Settings.FrameHeight = SupportedHeight[v];
        }
        else if ( key == "fps" )
        {
            int scanned = sscanf( value.c_str( ), "%u", &(Settings.FrameRate) );
            
            if ( scanned != 1 )
                break;
                
            if ( ( Settings.FrameRate < 1 ) || ( Settings.FrameRate > 30 ) )
                Settings.FrameRate = 30;
        }
        else
        {
            break;
        }
    }
    
    if ( i != argc )
    {
        printf( "cam2web - streaming camera to web \n\n" );
        printf( "Available command line options: \n" );
        printf( "  -size:<0-4> Sets video size to one from the list below: \n" );
        printf( "              0: 320x240 \n" );
        printf( "              1: 480x360 \n" );
        printf( "              2: 640x480 (default) \n" );
        printf( "              3: 800x600 \n" );
        printf( "              4: 1120x840 \n" );
        printf( "  -fps:<1-30> Sets camera frame rate. Same is used for MJPEG stream. \n" );
        printf( "\n" );
        
        ret = false;
    }
    
    return ret;
}

int main( int argc, char* argv[] )
{
    struct sigaction sigIntAction;
    
    SetDefaultSettings( );
    
    if ( !ParsetCommandLine( argc, argv ) )
    {
        return -1;
    }

    // set-up handler for certain signals
    sigIntAction.sa_handler = sigIntHandler;
    sigemptyset( &sigIntAction.sa_mask );
    sigIntAction.sa_flags = 0;

    sigaction( SIGINT,  &sigIntAction, NULL );
    sigaction( SIGTERM, &sigIntAction, NULL );
    sigaction( SIGHUP,  &sigIntAction, NULL );

    // create camera object
    shared_ptr<XRaspiCamera>         xcamera       = XRaspiCamera::Create( );
    shared_ptr<IObjectConfigurator>  xcameraConfig = make_shared<XRaspiCameraConfig>( xcamera );
    
    // prepare some read-only informational properties of the camera
    PropertyMap cameraInfo;
    char        strVideoSize[32];

    sprintf( strVideoSize,      "%u", Settings.FrameWidth );
    sprintf( strVideoSize + 16, "%u", Settings.FrameHeight );

    cameraInfo.insert( PropertyMap::value_type( "device", "RaspberryPi Camera" ) );
    cameraInfo.insert( PropertyMap::value_type( "width",  strVideoSize ) );
    cameraInfo.insert( PropertyMap::value_type( "height", strVideoSize + 16 ) );

    // create and configure web server
    XWebServer                       server( "", 8000 );
    XVideoSourceToWeb                video2web;
        
    xcamera->SetVideoSize( Settings.FrameWidth, Settings.FrameHeight );
    xcamera->SetFrameRate( Settings.FrameRate );
    
    server.SetDocumentRoot( "./web/" ).
           AddHandler( make_shared<XObjectConfiguratorRequestHandler>( "/camera/config", xcameraConfig ) ).
           AddHandler( make_shared<XObjectInformationRequestHandler>( "/camera/info", make_shared<XObjectInformationMap>( cameraInfo ) ) ).
           AddHandler( video2web.CreateJpegHandler( "/camera/jpeg" ) ).
           AddHandler( video2web.CreateMjpegHandler( "/camera/mjpeg", Settings.FrameRate ) );

    if ( server.Start( ) )
    {
        printf( "Web server started on port %d ...\n", server.Port( ) );
        printf( "Ctrl+C to stop.\n" );

        xcamera->SetListener( video2web.VideoSourceListener( ) );
        xcamera->Start( );

        ExitEvent.Wait( );

        xcamera->SignalToStop( );
        xcamera->WaitForStop( );
        server.Stop( );

        printf( "Done \n" );
    }
    else
    {
        printf( "Failed starting web server on port %d", server.Port( ) );
    }
        
    return 0;
}

