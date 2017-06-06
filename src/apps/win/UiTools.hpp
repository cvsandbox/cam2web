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

#ifndef UI_TOOLS_HPP
#define UI_TOOLS_HPP

#include <stdint.h>

// Center the given window relative to the reference window
void CenterWindowTo( HWND hWnd, HWND hWndRef );

// Initialize up/down control and its buddy control
void InitUpDownControl( HWND hwndUpDown, HWND hwndBuddy, uint16_t min, uint16_t max, uint16_t value );
// Ensure buddy control's volue is in the range of the specified up/down control
void EnsureUpDownBuddyInRange( HWND hwndUpDown, HWND hwndBuddy );



#endif // UI_TOOLS_HPP
