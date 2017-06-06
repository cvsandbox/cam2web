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

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN

#include <sdkddkver.h>
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <tchar.h>

#include "UiTools.hpp"

// Center the given window relative to the reference window
void CenterWindowTo( HWND hWnd, HWND hWndRef )
{
    RECT refRect, wndRect;

    GetWindowRect( hWndRef, &refRect );
    GetWindowRect( hWnd, &wndRect );

    SetWindowPos( hWnd, HWND_TOP,
        refRect.left + ( ( refRect.right - refRect.left ) - ( wndRect.right - wndRect.left ) ) / 2,
        refRect.top + ( ( refRect.bottom - refRect.top ) - ( wndRect.bottom - wndRect.top ) ) / 2,
        0, 0, SWP_NOSIZE );
}

// Initialize up/down control and its buddy control
void InitUpDownControl( HWND hwndUpDown, HWND hwndBuddy, uint16_t min, uint16_t max, uint16_t value )
{
    if ( ( hwndUpDown != nullptr ) && ( hwndBuddy != nullptr ) )
    {
        char strMax[32];

        sprintf( strMax, "%u", max );

        SendMessage( hwndBuddy, EM_SETLIMITTEXT, static_cast<WPARAM>( strlen( strMax ) ), 0 );
        SendMessage( hwndUpDown, UDM_SETRANGE32, min, max );
        SendMessage( hwndUpDown, UDM_SETPOS32, 0, value );
    }
}

// Ensure buddy control's volue is in the range of the specified up/down control
void EnsureUpDownBuddyInRange( HWND hwndUpDown, HWND hwndBuddy )
{
    uint32_t rangeMin = 0;
    uint32_t rangeMax = 0;
    TCHAR    buffer[32];

    SendMessage( hwndUpDown, UDM_GETRANGE32, (WPARAM) &rangeMin, (LPARAM) &rangeMax );
    GetWindowText( hwndBuddy, buffer, 32 );

    uint32_t value = _wtoi( buffer );

    if ( value < rangeMin ) value = rangeMin;
    if ( value > rangeMax ) value = rangeMax;

    SendMessage( hwndUpDown, UDM_SETPOS32, 0, value );

    int len = GetWindowTextLength( hwndBuddy );
    SendMessage( hwndBuddy, EM_SETSEL, len, len );
}
