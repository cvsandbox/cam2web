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
#include "EditUserDialog.hpp"
#include "Tools.hpp"
#include "UiTools.hpp"

using namespace std;

#define STR_ERROR   TEXT( "Error" )

// Message handler for Add/Edit User dialog box
INT_PTR CALLBACK EditUserDialogProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    static HICON      hIcon     = NULL;
    static UserInfo*  pUserInfo = nullptr;
    int               wmId;
    int               wmEvent;

    switch ( message )
    {
    case WM_INITDIALOG:
        {
            HWND hwndUserNameEdit   = GetDlgItem( hDlg, IDC_USER_NAME_EDIT );
            HWND hwndUserRoleCombo  = GetDlgItem( hDlg, IDC_USER_ROLE_COMBO );
            HWND hwndPasswordEdit   = GetDlgItem( hDlg, IDC_PASSWORD_EDIT );
            HWND hwndRePasswordEdit = GetDlgItem( hDlg, IDC_RE_PASSWORD_EDIT );

            CenterWindowTo( hDlg, GetParent( hDlg ) );

            hIcon = (HICON) LoadImage( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_FAMILY ), IMAGE_ICON,
                                       GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 );

            if ( hIcon )
            {
                SendMessage( hDlg, WM_SETICON, ICON_SMALL, (LPARAM) hIcon );
            }

            // limit length of user name/password to something sensible
            SendMessage( hwndUserNameEdit, EM_SETLIMITTEXT, 32, 0 );
            SendMessage( hwndPasswordEdit, EM_SETLIMITTEXT, 32, 0 );
            SendMessage( hwndRePasswordEdit, EM_SETLIMITTEXT, 32, 0 );

            SendMessage( hwndUserRoleCombo, CB_ADDSTRING, 0, (LPARAM) L"User" );
            SendMessage( hwndUserRoleCombo, CB_ADDSTRING, 0, (LPARAM) L"Admin" );
            SendMessage( hwndUserRoleCombo, CB_SETCURSEL, 0, 0 );

            pUserInfo = (UserInfo*) lParam;

            if ( pUserInfo != nullptr )
            {
                if ( !pUserInfo->Name.empty( ) )
                {
                    wstring password = Utf8to16( pUserInfo->Password );

                    SetWindowText( hwndUserNameEdit, Utf8to16( pUserInfo->Name ).c_str( ) );
                    SetWindowText( hwndPasswordEdit, password.c_str( ) );
                    SetWindowText( hwndRePasswordEdit, password.c_str( ) );
                    SendMessage( hwndUserRoleCombo, CB_SETCURSEL, ( pUserInfo->IsAdmin ) ? 1 : 0, 0 );

                    // allow editting everything except user name
                    SendMessage( hwndUserNameEdit, EM_SETREADONLY, 1, 0 );

                    SetWindowText( hDlg, L"Edit user" );
                }
                else
                {
                    // disable OK button, since user name/password are all empty
                    EnableWindow( GetDlgItem( hDlg, IDOK ), FALSE );
                }

                pUserInfo->PasswordChanged = false;
            }
        }
        return (INT_PTR) TRUE;

    case WM_DESTROY:

        if ( hIcon )
        {
            DestroyIcon( hIcon );
        }

        pUserInfo = nullptr;

        return (INT_PTR) TRUE;

    case WM_COMMAND:
        wmId    = LOWORD( wParam );
        wmEvent = HIWORD( wParam );

        if ( ( wmId == IDOK ) || ( wmId == IDCANCEL ) )
        {
            bool allFine = true;

            if ( ( wmId == IDOK ) && ( pUserInfo ) )
            {
                string userName  = GetWindowString( GetDlgItem( hDlg, IDC_USER_NAME_EDIT ), true );
                string password1 = GetWindowString( GetDlgItem( hDlg, IDC_PASSWORD_EDIT ), false );
                string password2 = GetWindowString( GetDlgItem( hDlg, IDC_RE_PASSWORD_EDIT ), false );

                allFine = false;

                if ( std::find( pUserInfo->ExistingUserNames.begin( ), pUserInfo->ExistingUserNames.end( ), userName ) != pUserInfo->ExistingUserNames.end( ) )
                {
                    CenteredMessageBox( hDlg, L"A user with the specified name already exists.", STR_ERROR, MB_OK | MB_ICONERROR );
                }
                else if ( password1 != password2 )
                {
                    CenteredMessageBox( hDlg, L"The two passwords don't match.", STR_ERROR, MB_OK | MB_ICONERROR );
                }
                else if ( userName.find( ':' ) != string::npos )
                {
                    CenteredMessageBox( hDlg, L"User names are not allowed to contain colon character (':').", STR_ERROR, MB_OK | MB_ICONERROR );
                }
                else
                {
                    allFine = true;

                    pUserInfo->Name     = userName;
                    pUserInfo->Password = password1;
                    pUserInfo->IsAdmin  = ( SendMessage( GetDlgItem( hDlg, IDC_USER_ROLE_COMBO ), CB_GETCURSEL, 0, 0 ) == 1 );
                }
            }

            if ( allFine )
            {
                EndDialog( hDlg, wmId );
            }
            return (INT_PTR) TRUE;
        }
        else if ( wmEvent == EN_CHANGE )
        {
            if ( ( wmId == IDC_USER_NAME_EDIT ) || ( wmId == IDC_PASSWORD_EDIT ) || ( wmId == IDC_RE_PASSWORD_EDIT ) )
            {
                string userName  = GetWindowString( GetDlgItem( hDlg, IDC_USER_NAME_EDIT ), true );
                string password1 = GetWindowString( GetDlgItem( hDlg, IDC_PASSWORD_EDIT ), false );
                string password2 = GetWindowString( GetDlgItem( hDlg, IDC_RE_PASSWORD_EDIT ), false );
                bool   isEmpty   = ( userName.empty( ) || password1.empty( ) || password2.empty( ) );

                EnableWindow( GetDlgItem( hDlg, IDOK ), ( isEmpty ) ? FALSE : TRUE );

                if ( ( pUserInfo != nullptr ) && ( ( wmId == IDC_PASSWORD_EDIT ) || ( wmId == IDC_RE_PASSWORD_EDIT ) ) )
                {
                    pUserInfo->PasswordChanged = true;
                }

                return (INT_PTR) TRUE;
            }
        }

        break;
    }

    return (INT_PTR) FALSE;
}
