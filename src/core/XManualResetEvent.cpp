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
