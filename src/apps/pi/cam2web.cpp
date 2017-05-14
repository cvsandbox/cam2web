/*
    cam2web - streaming camera to web

    BSD 2-Clause License

    Copyright (c) 2017, cvsandbox, cvsandbox@gmail.com
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include "XManualResetEvent.hpp"

using namespace std;

XManualResetEvent ExitEvent;

void sigIntHandler( int s )
{
    ExitEvent.Signal( );
}

int main( void )
{
    struct sigaction sigIntAction;

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

    // create and configure web server
    XWebServer                       server( "", 8000 );
    XVideoSourceToWeb                video2web;
    
    server.SetDocumentRoot( "./web/" ).
           AddHandler( make_shared<XObjectConfiguratorRequestHandler>( "/config", xcameraConfig ) ).      
           AddHandler( video2web.CreateJpegHandler( "/jpeg" ) ).
           AddHandler( video2web.CreateMjpegHandler( "/mjpeg", 30 ) );

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

