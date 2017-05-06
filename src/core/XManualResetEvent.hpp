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

#ifndef XMANUAL_RESET_EVENT_HPP
#define XMANUAL_RESET_EVENT_HPP

#include <stdint.h>

namespace Private
{
    class XManualResetEventData;
}

// Manual reset synchronization event
class XManualResetEvent
{
private:
    XManualResetEvent( const XManualResetEvent& );
    XManualResetEvent& operator= ( const XManualResetEvent& );

public:
    XManualResetEvent( );
    ~XManualResetEvent( );

    // Set event to not signalled state
    void Reset( );
    // Set event to signalled state
    void Signal( );
    // Wait till the event gets into signalled state
    void Wait( );
    // Wait the specified amount of time (milliseconds) till the event gets signalled
    bool Wait( uint32_t msec );
    // Check current state of the event
    bool IsSignaled( );

private:
    Private::XManualResetEventData* mData;
};

#endif // XMANUAL_RESET_EVENT_HPP
