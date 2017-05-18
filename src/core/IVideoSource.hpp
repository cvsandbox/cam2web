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

#ifndef IVIDEO_SOURCE_HPP
#define IVIDEO_SOURCE_HPP

#include <stdint.h>
#include "IVideoSourceListener.hpp"

// Interface for video sources continuously providing frames to display/process
class IVideoSource
{
public:
    virtual ~IVideoSource( ) { }

    // Start video source so it initializes and begins providing video frames
    virtual bool Start( ) = 0;
    // Signal video source to stop, so it could finalize and clean-up
    virtual void SignalToStop( ) = 0;
    // Wait till video source (its thread) stops
    virtual void WaitForStop( ) = 0;
    // Check if video source is still running
    virtual bool IsRunning( ) = 0;

    // Get number of frames received since the start of the video source
    virtual uint32_t FramesReceived( ) = 0;

    // Set video source listener returning the old one
    virtual IVideoSourceListener* SetListener( IVideoSourceListener* listener ) = 0;
};

#endif // IVIDEO_SOURCE_HPP
