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

#include "XV4LCamera.hpp"
#include "XWebServer.hpp"
#include "XVideoSourceToWeb.hpp"
#include "XObjectConfiguratorRequestHandler.hpp"
#include "XObjectInformationRequestHandler.hpp"
#include "XManualResetEvent.hpp"

using namespace std;

XManualResetEvent ExitEvent;

// Different application settings
struct
{
    uint32_t FrameWidth;
    uint32_t FrameHeight;
    uint32_t FrameRate;
}
Settings;

// Raise exit event when signal is received
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
    return true;
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
    shared_ptr<XV4LCamera>  xcamera = XV4LCamera::Create( );

    // prepare some read-only informational properties of the camera
    PropertyMap cameraInfo;
    char        strVideoSize[32];

    sprintf( strVideoSize,      "%u", Settings.FrameWidth );
    sprintf( strVideoSize + 16, "%u", Settings.FrameHeight );

    cameraInfo.insert( PropertyMap::value_type( "device", "V4L Camera" ) );
    cameraInfo.insert( PropertyMap::value_type( "width",  strVideoSize ) );
    cameraInfo.insert( PropertyMap::value_type( "height", strVideoSize + 16 ) );

    // create and configure web server
    XWebServer                       server( "", 8000 );
    XVideoSourceToWeb                video2web;

    //xcamera->SetVideoSize( Settings.FrameWidth, Settings.FrameHeight );
    //xcamera->SetFrameRate( Settings.FrameRate );

    server.SetDocumentRoot( "./web/" ).
//           AddHandler( make_shared<XObjectConfiguratorRequestHandler>( "/camera/config", xcameraConfig ) ).
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

