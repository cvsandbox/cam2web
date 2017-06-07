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

#include "resource.h"
#include "SettingsDialog.hpp"
#include "Tools.hpp"
#include "UiTools.hpp"
#include "AppConfig.hpp"

using namespace std;

// Message handler for Settings dialog box
INT_PTR CALLBACK SettingsDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER( lParam );

    static AppConfig* appConfig = nullptr;
    int               wmId;
    int               wmEvent;

    switch ( message )
    {
    case WM_INITDIALOG:
        appConfig = (AppConfig*) lParam;

        CenterWindowTo( hDlg, GetParent( hDlg ) );

        if ( appConfig != nullptr )
        {
            InitUpDownControl( GetDlgItem( hDlg, IDC_JPEG_Q_SPIN ), GetDlgItem( hDlg, IDC_JPEG_Q_EDIT ), 1, 100, appConfig->JpegQuality( ) );
            InitUpDownControl( GetDlgItem( hDlg, IDC_MJPEG_RATE_SPIN ), GetDlgItem( hDlg, IDC_MJPEG_RATE_EDIT ), 1, 30, appConfig->MjpegFrameRate( ) );
            InitUpDownControl( GetDlgItem( hDlg, IDC_HTTP_PORT_SPIN ), GetDlgItem( hDlg, IDC_HTTP_PORT_EDIT ), 1, 65535, appConfig->HttpPort( ) );

            wstring strWebContent = Utf8to16( appConfig->CustomWebContent( ) );
            SetWindowText( GetDlgItem( hDlg, IDC_CUSTOM_WEB_EDIT ), strWebContent.c_str( ) );
        }

        return (INT_PTR) TRUE;

    case WM_DESTROY:
        appConfig = nullptr;
        return (INT_PTR) TRUE;

    case WM_COMMAND:
        wmId    = LOWORD( wParam );
        wmEvent = HIWORD( wParam );

        if ( ( wmId == IDOK ) || ( wmId == IDCANCEL ) )
        {
            if ( wmId == IDOK )
            {
                if ( appConfig )
                {
                    appConfig->SetJpegQuality( (uint16_t) SendMessage( GetDlgItem( hDlg, IDC_JPEG_Q_SPIN ), UDM_GETPOS32, 0, 0 ) );
                    appConfig->SetMjpegFrameRate( (uint16_t) SendMessage( GetDlgItem( hDlg, IDC_MJPEG_RATE_SPIN ), UDM_GETPOS32, 0, 0 ) );
                    appConfig->SetHttpPort( (uint16_t) SendMessage( GetDlgItem( hDlg, IDC_HTTP_PORT_SPIN ), UDM_GETPOS32, 0, 0 ) );

                    HWND   hwndWebContentEdit = GetDlgItem( hDlg, IDC_CUSTOM_WEB_EDIT );
                    int    len                = GetWindowTextLength( hwndWebContentEdit ) + 1;
                    WCHAR* strWebContent      = new WCHAR[len];

                    GetWindowText( hwndWebContentEdit, strWebContent, len );

                    string utf8rWebContent = Utf16to8( strWebContent );
                    appConfig->SetCustomWebContent( StringTrim( utf8rWebContent ) );

                    delete [] strWebContent;
                }
            }

            EndDialog( hDlg, wmId );
            return (INT_PTR) TRUE;
        }
        else if ( wmEvent == EN_CHANGE )
        {
            if ( wmId == IDC_JPEG_Q_EDIT )
            {
                EnsureUpDownBuddyInRange( GetDlgItem( hDlg, IDC_JPEG_Q_SPIN ), GetDlgItem( hDlg, IDC_JPEG_Q_EDIT ) );
            }
            else if ( wmId == IDC_MJPEG_RATE_EDIT )
            {
                EnsureUpDownBuddyInRange( GetDlgItem( hDlg, IDC_MJPEG_RATE_SPIN ), GetDlgItem( hDlg, IDC_MJPEG_RATE_EDIT ) );
            }
            else if ( wmId == IDC_HTTP_PORT_EDIT )
            {
                EnsureUpDownBuddyInRange( GetDlgItem( hDlg, IDC_HTTP_PORT_SPIN ), GetDlgItem( hDlg, IDC_HTTP_PORT_EDIT ) );
            }
            else
            {
                break;
            }
            
            return (INT_PTR) TRUE;
        }
        break;
    }

    return (INT_PTR) FALSE;
}
