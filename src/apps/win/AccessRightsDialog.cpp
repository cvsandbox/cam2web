/*
    cam2web - streaming camera to web

    Copyright (C) 2017-2018, cvsandbox, cvsandbox@gmail.com

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
#include <vector>
#include <algorithm>

#include <XStringTools.hpp>

#include "resource.h"
#include "EditUserDialog.hpp"
#include "EditAuthDomainDialog.hpp"
#include "Tools.hpp"
#include "UiTools.hpp"
#include "AppConfig.hpp"

#include "XWebServer.hpp"

using namespace std;

static const WCHAR* STR_ANYONE       = L"Anyone";
static const WCHAR* STR_USERS        = L"Users";
static const WCHAR* STR_ADMINS       = L"Admins";

static const WCHAR* STR_CONFIRMATION = L"Confirmation";

#define USER_GROUP_TO_INDEX(group)  ( ( group == UserGroup::Admin ) ? 2 : ( ( group == UserGroup::User ) ? 1 : 0 ) )
#define INDEX_TO_USER_GROUP(index)  ( ( index == 2 ) ? UserGroup::Admin : ( ( index == 1 ) ? UserGroup::User : UserGroup::Anyone ) )

// User related data
typedef struct
{

    string    Name;
    string    Domain;
    string    DigestHa1;
    UserGroup Group;
}
CameraUser;

typedef pair<string, string>     UserKey;
typedef map<UserKey, CameraUser> UsersMap;

// Populate combo boxes with items and make initial selections
static void InitComboBoxes( HWND hwndViewersBox, HWND hwndConfiguratorsBox, UserGroup viewersGroup, UserGroup configuratorsGroup )
{
    SendMessage( hwndViewersBox, CB_ADDSTRING, 0, (LPARAM) STR_ANYONE );
    SendMessage( hwndViewersBox, CB_ADDSTRING, 0, (LPARAM) STR_USERS );
    SendMessage( hwndViewersBox, CB_ADDSTRING, 0, (LPARAM) STR_ADMINS );
    SendMessage( hwndConfiguratorsBox, CB_ADDSTRING, 0, (LPARAM) STR_ANYONE );
    SendMessage( hwndConfiguratorsBox, CB_ADDSTRING, 0, (LPARAM) STR_USERS );
    SendMessage( hwndConfiguratorsBox, CB_ADDSTRING, 0, (LPARAM) STR_ADMINS );

    SendMessage( hwndViewersBox, CB_SETCURSEL, USER_GROUP_TO_INDEX( viewersGroup ), 0 );
    SendMessage( hwndConfiguratorsBox, CB_SETCURSEL, USER_GROUP_TO_INDEX( configuratorsGroup ), 0 );
}

// Initialize list view of existing users
static void InitUsersListView( HWND hwndListView )
{
    // add columns to the list view
    LVCOLUMN    lvc;

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt  = LVCFMT_LEFT;

    lvc.iSubItem = 0;
    lvc.pszText  = L"Name";
    lvc.cx       = 200;

    ListView_InsertColumn( hwndListView, 0, &lvc );

    lvc.iSubItem = 1;
    lvc.pszText  = L"Role";
    lvc.cx       = 100;

    ListView_InsertColumn( hwndListView, 1, &lvc );

    // add image list to the list view
    HMODULE     hModule    = GetModuleHandle( NULL );
    HIMAGELIST  hImageList = ImageList_Create( GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), ILC_COLOR32, 2, 2 );
    HICON       hiconItem;

    hiconItem = LoadIcon( hModule, MAKEINTRESOURCE( IDI_USER ) );
    ImageList_AddIcon( hImageList, hiconItem );
    DestroyIcon( hiconItem );

    hiconItem = LoadIcon( hModule, MAKEINTRESOURCE( IDI_ADMIN ) );
    ImageList_AddIcon( hImageList, hiconItem );
    DestroyIcon( hiconItem );

    ListView_SetImageList( hwndListView, hImageList, LVSIL_SMALL );

    // select full row when item is selected in the list view
    ListView_SetExtendedListViewStyle( hwndListView, LVS_EX_FULLROWSELECT );
}

// Add user to the users' list view
static void AddUserToListView( HWND hwndListView, const string& userName, UserGroup userGroup, bool selectIt = true )
{
    wstring userNameUnicode = Utf8to16( userName );
    LVITEM  lvI;

    lvI.pszText   = (WCHAR*) userNameUnicode.c_str( );
    lvI.mask      = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvI.stateMask = 0;
    lvI.iSubItem  = 0;
    lvI.state     = 0;
    lvI.iImage    = ( userGroup == UserGroup::Admin ) ? 1 : 0;
    lvI.iItem     = ListView_GetItemCount( hwndListView );

    ListView_InsertItem( hwndListView, &lvI );

    lvI.pszText   = ( userGroup == UserGroup::Admin ) ? L"Admin" : L"User";
    lvI.mask      = LVIF_TEXT;
    lvI.iSubItem  = 1;

    ListView_SetItem( hwndListView, &lvI );

    if ( selectIt )
    {
        ListView_SetItemState( hwndListView, lvI.iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
        ListView_EnsureVisible( hwndListView, lvI.iItem, FALSE );
    }
}

// Update user in the list view
static void UpdateUserInListView( HWND hwndListView, const string& userName, UserGroup userGroup )
{
    wstring userNameUnicode = Utf8to16( userName );
    int     itemsCount      = ListView_GetItemCount( hwndListView );
    WCHAR   buffer[64]      = L"";

    for ( int i = 0; i < itemsCount; i++ )
    {
        ListView_GetItemText( hwndListView, i, 0, buffer, sizeof( buffer ) );

        if ( userNameUnicode == buffer )
        {
            LVITEM  lvI;

            lvI.mask     = LVIF_IMAGE;
            lvI.iItem    = i;
            lvI.iSubItem = 0;
            lvI.iImage   = ( userGroup == UserGroup::Admin ) ? 1 : 0;

            ListView_SetItem( hwndListView, &lvI );

            lvI.mask     = LVIF_TEXT;
            lvI.iItem    = i;
            lvI.iSubItem = 1;
            lvI.pszText  = ( userGroup == UserGroup::Admin ) ? L"Admin" : L"User";

            ListView_SetItem( hwndListView, &lvI );

            break;
        }
    }
}

// Show users for the specified authentication domain
static void ShowUsersForDomain( HWND hwndListView, const UsersMap& users, const string& authDomain )
{
    ListView_DeleteAllItems( hwndListView );

    for ( UsersMap::const_iterator it = users.begin( ); it != users.end( ); ++it )
    {
        if ( it->second.Domain == authDomain )
        {
            AddUserToListView( hwndListView, it->second.Name, it->second.Group, false );
        }
    }

    if ( ListView_GetItemCount( hwndListView ) != 0 )
    {
        ListView_SetItemState( hwndListView, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
    }
}

// Get name of the user selected in the users' list view
static string GetSelectedUserName( HWND hwndListView )
{
    int    selectedItem = ListView_GetNextItem( hwndListView, -1, LVNI_SELECTED );
    string selectedUser;

    if ( selectedItem != -1 )
    {
        WCHAR buffer[64] = L"";

        ListView_GetItemText( hwndListView, selectedItem, 0, buffer, sizeof( buffer ) );

        selectedUser = Utf16to8( buffer );
    }

    return selectedUser;
}

// Save list of users to file
static void SaveUsersList( const string& fileName, const UsersMap& users )
{
    wstring unicodeFileName = Utf8to16( fileName );
    FILE*   file            = _wfopen( unicodeFileName.c_str( ), L"w" );

    if ( file != nullptr )
    {
        for ( auto kvp : users )
        {
            CameraUser user = kvp.second;

            fprintf( file, "%s:%s:%s:%d\n", user.Name.c_str( ), user.Domain.c_str( ), user.DigestHa1.c_str( ), user.Group );
        }
        fclose( file );
    }
}

// Load list of users from the specified file
static int LoadUsersList( const string& fileName, UsersMap& users )
{
    wstring unicodeFileName  = Utf8to16( fileName );
    FILE*   file             = _wfopen( unicodeFileName.c_str( ), L"r" );
    int     loadedUsersCount = 0;

    if ( file != nullptr )
    {
        char   buffer[256];

        while ( fgets( buffer, sizeof( buffer ) - 1, file ) )
        {
            string    userName;
            string    userDomain;
            string    digestHa1;
            UserGroup group     = UserGroup::User;
            char*     realmPtr  = strchr( buffer, ':' );
            char*     digestPtr = nullptr;

            if ( realmPtr != nullptr )
            {
                userName = string( buffer, realmPtr );
                digestPtr = strchr( ++realmPtr, ':' );

                if ( digestPtr != nullptr )
                {
                    userDomain = string( realmPtr, digestPtr );

                    // copy the rest as digest HA1
                    digestHa1 = string( ++digestPtr );
                    // remove trailing spaces
                    StringRTrim( digestHa1 );

                    size_t delIndex = digestHa1.find( ':' );

                    if ( delIndex != string::npos )
                    {
                        int groupId;

                        if ( sscanf( digestHa1.c_str( ) + delIndex + 1, "%d", &groupId ) == 1 )
                        {
                            if ( ( groupId > 0 ) && ( groupId <= 3 ) )
                            {
                                group = static_cast<UserGroup>( groupId );
                            }
                        }

                        digestHa1 = digestHa1.substr( 0, delIndex );
                    }
                }

                // make sure digest is 32 character long
                if ( digestHa1.length( ) == 32 )
                {
                    users.insert( UsersMap::value_type( UserKey( userName, userDomain ),
                                  CameraUser( { userName, userDomain, digestHa1, group } ) ) );
                    loadedUsersCount++;
                }
            }
        }

        fclose( file );
    }

    return loadedUsersCount;
}

// Some dialog related data
typedef struct
{
    AppConfig* AppConfig;
    UsersMap   Users;
    HICON      Icon;
    bool       ChangesDetected;
    string     AuthDomain;
}
DialogData;

// Message handler for Access Rights dialog box
INT_PTR CALLBACK AccessRightsDialogProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    static DialogData* data = nullptr;
    int                wmId;
    int                wmEvent;

    UsersMap Users;

    switch ( message )
    {
    case WM_INITDIALOG:

        data = new DialogData;

        data->AppConfig       = (AppConfig*) lParam;
        data->ChangesDetected = false;

        CenterWindowTo( hDlg, GetParent( hDlg ) );

        data->Icon = (HICON) LoadImage( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_ACCESS ), IMAGE_ICON,
                                        GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 );

        if ( data->Icon )
        {
            SendMessage( hDlg, WM_SETICON, ICON_SMALL, (LPARAM) data->Icon );
        }

        EnableWindow( GetDlgItem( hDlg, IDC_EDIT_USER_BUTTON ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_DELETE_USER_BUTTON ), FALSE );

        if ( data->AppConfig != nullptr )
        {
            HWND hwndUsersList = GetDlgItem( hDlg, IDC_USERS_LIST_VIEW );

            data->AuthDomain = data->AppConfig->AuthDomain( );

            SetWindowText( GetDlgItem( hDlg, IDC_AUTH_DOMAIN_EDIT ), Utf8to16( data->AuthDomain ).c_str( ) );

            InitComboBoxes( GetDlgItem( hDlg, IDC_VIEWERS_COMBO ), GetDlgItem( hDlg, IDC_CONFIGURATORS_COMBO ),
                            static_cast<UserGroup>( data->AppConfig->ViewersGroup( ) ),
                            static_cast<UserGroup>( data->AppConfig->ConfiguratorsGroup( ) ) );
            InitUsersListView( hwndUsersList );

            if ( LoadUsersList( data->AppConfig->UsersFileName( ), data->Users ) > 0 )
            {
                ShowUsersForDomain( hwndUsersList, data->Users, data->AuthDomain );
            }
        }

        return (INT_PTR) TRUE;

    case WM_DESTROY:

        if ( data->Icon )
        {
            DestroyIcon( data->Icon );
        }

        delete data;
        data = nullptr;

        return (INT_PTR) TRUE;

    case WM_COMMAND:
        wmId    = LOWORD( wParam );
        wmEvent = HIWORD( wParam );

        if ( ( wmId == IDOK ) || ( wmId == IDCANCEL ) )
        {
            bool allFine = true;

            if ( wmId == IDOK )
            {
                if ( data->AppConfig )
                {
                    data->AppConfig->SetAuthDomain( data->AuthDomain );

                    data->AppConfig->SetViewersGroup( static_cast<uint16_t>(
                        INDEX_TO_USER_GROUP( SendMessage( GetDlgItem( hDlg, IDC_VIEWERS_COMBO ), CB_GETCURSEL, 0, 0 ) ) ) );
                    data->AppConfig->SetConfiguratorsGroup( static_cast<uint16_t>(
                        INDEX_TO_USER_GROUP( SendMessage( GetDlgItem( hDlg, IDC_CONFIGURATORS_COMBO ), CB_GETCURSEL, 0, 0 ) ) ) );

                    SaveUsersList( data->AppConfig->UsersFileName( ), data->Users );
                }
            }
            else
            {
                if ( data->ChangesDetected )
                {
                    if ( CenteredMessageBox( hDlg, L"Are you sure you would like to leave the dialog discarding all changes?", STR_CONFIRMATION, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION ) == IDNO )
                    {
                        allFine = false;
                    }
                }
            }

            if ( allFine )
            {
                EndDialog( hDlg, wmId );
            }
            return (INT_PTR) TRUE;
        }
        else if ( wmId == IDC_CHANGE_DOMAIN_BUTTON )
        {
            HWND  hwndAuthDomainEdit = GetDlgItem( hDlg, IDC_AUTH_DOMAIN_EDIT );
            char  strAuthDomain[128];

            strcpy( strAuthDomain, data->AuthDomain.c_str( ) );

            if ( DialogBoxParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDD_EDIT_REALM_BOX ), hDlg, EditAuthDomainDialogProc, (LPARAM) strAuthDomain ) == IDOK )
            {
                if ( data->AuthDomain != strAuthDomain )
                {
                    data->AuthDomain = strAuthDomain;

                    SetWindowText( GetDlgItem( hDlg, IDC_AUTH_DOMAIN_EDIT ), Utf8to16( data->AuthDomain ).c_str( ) );
                    ShowUsersForDomain( GetDlgItem( hDlg, IDC_USERS_LIST_VIEW ), data->Users, data->AuthDomain );
                    data->ChangesDetected = true;
                }
            }

            SetFocus( hwndAuthDomainEdit );
            return (INT_PTR) TRUE;
        }
        else if ( ( wmId == IDC_ADD_USER_BUTTON ) || ( wmId == IDC_EDIT_USER_BUTTON ) || ( wmId == IDC_DELETE_USER_BUTTON ) )
        {
            HWND           hwndUsersList = GetDlgItem( hDlg, IDC_USERS_LIST_VIEW );
            vector<string> existingUserNames;

            for ( UsersMap::const_iterator it = data->Users.begin( ); it != data->Users.end( ); ++it )
            {
                if ( it->second.Domain == data->AuthDomain )
                {
                    existingUserNames.push_back( it->second.Name );
                }
            }

            if ( wmId == IDC_ADD_USER_BUTTON )
            {
                UserInfo userInfo;

                userInfo.ExistingUserNames = existingUserNames;

                if ( DialogBoxParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDD_EDIT_USER_BOX ), hDlg, EditUserDialogProc, (LPARAM) &userInfo ) == IDOK )
                {
                    UserGroup userGroup = ( userInfo.IsAdmin ) ? UserGroup::Admin : UserGroup::User;
                    string    digestHa1 = XWebServer::CalculateDigestAuthHa1( userInfo.Name, data->AuthDomain, userInfo.Password );

                    AddUserToListView( hwndUsersList, userInfo.Name, userGroup );
                    data->Users.insert( UsersMap::value_type( UserKey( userInfo.Name, data->AuthDomain ),
                                        CameraUser( { userInfo.Name, data->AuthDomain, digestHa1, userGroup } ) ) );

                    data->ChangesDetected = true;
                }
            }
            else if ( wmId == IDC_EDIT_USER_BUTTON )
            {
                string             selectedUserName = GetSelectedUserName( hwndUsersList );
                UsersMap::iterator itUsers          = data->Users.find( UserKey( selectedUserName, data->AuthDomain ) );

                if ( itUsers != data->Users.end( ) )
                {
                    bool     isAdmin = ( itUsers->second.Group == UserGroup::Admin );
                    UserInfo userInfo = { selectedUserName, "**********", isAdmin };

                    // remove user's name from the list of existing name - it is allowed to be used
                    existingUserNames.erase( std::remove( existingUserNames.begin( ), existingUserNames.end( ), selectedUserName ), existingUserNames.end( ) );

                    userInfo.ExistingUserNames = existingUserNames;

                    if ( DialogBoxParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDD_EDIT_USER_BOX ), hDlg, EditUserDialogProc, (LPARAM) &userInfo ) == IDOK )
                    {
                        if ( isAdmin != userInfo.IsAdmin )
                        {
                            itUsers->second.Group = ( userInfo.IsAdmin ) ? UserGroup::Admin : UserGroup::User;
                            UpdateUserInListView( hwndUsersList, userInfo.Name, itUsers->second.Group );
                            data->ChangesDetected = true;
                        }

                        if ( userInfo.PasswordChanged )
                        {
                            string authDomain = GetWindowString( GetDlgItem( hDlg, IDC_AUTH_DOMAIN_EDIT ), false );

                            itUsers->second.DigestHa1 = XWebServer::CalculateDigestAuthHa1( userInfo.Name, authDomain, userInfo.Password );

                            data->ChangesDetected = true;
                        }
                    }
                }
            }
            else if ( wmId == IDC_DELETE_USER_BUTTON )
            {
                if ( CenteredMessageBox( hDlg, L"Are you sure you would like to delete the selected user?", STR_CONFIRMATION, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION ) == IDYES )
                {
                    data->Users.erase( UserKey( GetSelectedUserName( hwndUsersList ), data->AuthDomain ) );
                    ListView_DeleteItem( hwndUsersList, ListView_GetNextItem( hwndUsersList, -1, LVNI_SELECTED ) );
                    data->ChangesDetected = true;
                }
            }

            SetFocus( hwndUsersList );
            return (INT_PTR) TRUE;
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pNotify = (LPNMHDR) lParam;

            // check if selection has changed in users' list view
            if ( ( pNotify != nullptr ) && ( (LPNMHDR) lParam )->idFrom == IDC_USERS_LIST_VIEW )
            {
                NMLISTVIEW* listNotify = (NMLISTVIEW*) lParam;

                if ( ( listNotify->hdr.code == LVN_ITEMCHANGED ) && 
                     ( ( listNotify->uNewState & LVIS_SELECTED ) || ( listNotify->uOldState & LVIS_SELECTED ) ) )
                {
                    int  selectedItem = ListView_GetNextItem( GetDlgItem( hDlg, IDC_USERS_LIST_VIEW ), -1, LVNI_SELECTED );
                    BOOL enableEdit   = ( selectedItem != -1 ) ? TRUE : FALSE;

                    EnableWindow( GetDlgItem( hDlg, IDC_EDIT_USER_BUTTON ), enableEdit );
                    EnableWindow( GetDlgItem( hDlg, IDC_DELETE_USER_BUTTON ), enableEdit );
                }

                return (INT_PTR) TRUE;
            }
        }
        break;
    }

    return (INT_PTR) FALSE;
}
