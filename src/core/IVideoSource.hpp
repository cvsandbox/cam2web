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
