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

#include "resource.h"
#include "EditAuthDomainDialog.hpp"
#include "Tools.hpp"
#include "UiTools.hpp"

using namespace std;

#define STR_ERROR   TEXT( "Error" )

// Message handler for Edit HTTP auth domain dialog box
INT_PTR CALLBACK EditAuthDomainDialogProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    static HICON hIcon = NULL;
    static char* strAuthDomain = nullptr;
    int          wmId;
    int          wmEvent;

    switch ( message )
    {
    case WM_INITDIALOG:
        {
            HWND hwndAuthDomainEdit = GetDlgItem( hDlg, IDC_AUTH_DOMAIN_EDIT );

            CenterWindowTo( hDlg, GetParent( hDlg ) );

            hIcon = (HICON) LoadImage( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_PASSWORD ), IMAGE_ICON,
                                       GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 );

            if ( hIcon )
            {
                SendMessage( hDlg, WM_SETICON, ICON_SMALL, (LPARAM) hIcon );
            }

            SendMessage( hwndAuthDomainEdit, EM_SETLIMITTEXT, 32, 0 );

            strAuthDomain = (char*) lParam;

            if ( strAuthDomain != nullptr )
            {
                SetWindowText( hwndAuthDomainEdit, Utf8to16( strAuthDomain ).c_str( ) );
            }

            EnableWindow( GetDlgItem( hDlg, IDOK ), ( GetWindowTextLength( hwndAuthDomainEdit ) > 0 ) ? TRUE : FALSE );
        }
        return (INT_PTR) TRUE;

    case WM_DESTROY:

        if ( hIcon )
        {
            DestroyIcon( hIcon );
        }

        return (INT_PTR) TRUE;

    case WM_COMMAND:
        wmId    = LOWORD( wParam );
        wmEvent = HIWORD( wParam );

        if ( ( wmId == IDOK ) || ( wmId == IDCANCEL ) )
        {
            if ( ( wmId == IDOK ) && ( strAuthDomain ) )
            {
                string authDomain = GetWindowString( GetDlgItem( hDlg, IDC_AUTH_DOMAIN_EDIT ), true );

                strcpy( strAuthDomain, authDomain.c_str( ) );
            }

            EndDialog( hDlg, wmId );
            return (INT_PTR) TRUE;
        }
        else if ( ( wmEvent == EN_CHANGE ) && ( wmId == IDC_AUTH_DOMAIN_EDIT ) )
        {
            string authDomain = GetWindowString( GetDlgItem( hDlg, IDC_AUTH_DOMAIN_EDIT ), true );

            EnableWindow( GetDlgItem( hDlg, IDOK ), ( authDomain.empty( ) ) ? FALSE : TRUE );

            return (INT_PTR) TRUE;
        }

        break;
    }

    return (INT_PTR) FALSE;
}
