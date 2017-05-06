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

#include "XManualResetEvent.hpp"
#include <condition_variable>

using namespace std;
using namespace chrono;

namespace Private
{
    class XManualResetEventData
    {
    public:
        uint32_t           Counter;
        bool               Triggered;
        mutex              Mutex;
        condition_variable CondVariable;

    public:
        XManualResetEventData( ) :
            Counter( 0 ), Triggered( false ),
            Mutex( ), CondVariable( )
        {

        }
    };
}

XManualResetEvent::XManualResetEvent( ) :
    mData( new Private::XManualResetEventData( ) )
{
}

XManualResetEvent::~XManualResetEvent( )
{
    delete mData;
}

// Set event to not signalled state
void XManualResetEvent::Reset( )
{
    unique_lock<mutex> lock( mData->Mutex );
    mData->Triggered = false;
}

// Set event to signalled state
void XManualResetEvent::Signal( )
{
    unique_lock<mutex> lock( mData->Mutex );

    mData->Triggered = true;
    mData->Counter++;
    mData->CondVariable.notify_all( );
}

// Wait till the event gets into signalled state
void XManualResetEvent::Wait( )
{
    unique_lock<mutex> lock( mData->Mutex );
    uint32_t           lastCounterValue = mData->Counter;

    while ( ( !mData->Triggered ) && ( mData->Counter == lastCounterValue ) )
    {
        mData->CondVariable.wait( lock );
    }
}

// Wait the specified amount of time (milliseconds) till the event gets signalled
bool XManualResetEvent::Wait( uint32_t msec )
{
    steady_clock::time_point waitTill = steady_clock::now( ) + milliseconds( msec );
    unique_lock<mutex>       lock( mData->Mutex );
    uint32_t                 lastCounterValue = mData->Counter;

    if ( !mData->Triggered )
    {
        mData->CondVariable.wait_until( lock, waitTill );
    }

    return ( ( mData->Triggered ) || ( mData->Counter != lastCounterValue ) );
}

// Check current state of the event
bool XManualResetEvent::IsSignaled( )
{
    unique_lock<mutex> lock( mData->Mutex );
    return mData->Triggered;
}
