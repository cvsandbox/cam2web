/*
    cam2web - streaming camera to web

    Copyright (C) 2017-2019, cvsandbox, cvsandbox@gmail.com

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
#include <wincrypt.h>
#include <iphlpapi.h>

#include "Tools.hpp"

using namespace std;

static const char* STR_ARGB_FORMAT32 = "%08X";
static const char* STR_ARGB_FORMAT24 = "%06X";

// Convert specfied UTF8 string to wide character string
wstring Utf8to16( const string& utf8string )
{
    wstring ret;

    int required = MultiByteToWideChar( CP_UTF8, 0, utf8string.c_str( ), -1, nullptr, 0 );

    if ( required > 0 )
    {
        wchar_t* utf16string = new wchar_t[required];

        if ( MultiByteToWideChar( CP_UTF8, 0, utf8string.c_str( ), -1, utf16string, required ) > 0 )
        {
            ret = wstring( utf16string );
        }

        delete[] utf16string;
    }

    return ret;
}

// Convert specfied wide character string to UTF8 string
string Utf16to8( const wstring& utf16string )
{
    string ret;

    int required = WideCharToMultiByte( CP_UTF8, 0, utf16string.c_str( ), -1, nullptr, 0, nullptr, nullptr );

    if ( required > 0 )
    {
        char* utf8string = new char[required];

        if ( WideCharToMultiByte( CP_UTF8, 0, utf16string.c_str( ), -1, utf8string, required, nullptr, nullptr ) > 0 )
        {
            ret = string( utf8string );
        }

        delete[] utf8string;
    }

    return ret;
}

// Calculate MD5 hash string for the given buffer
string GetMd5Hash( const uint8_t* buffer, int bufferLength )
{
    HCRYPTPROV hCryptProv = NULL;
    HCRYPTHASH hHash = NULL;
    string     md5str;

    if ( CryptAcquireContext( &hCryptProv, NULL, NULL, PROV_RSA_FULL, 0 ) )
    {
        if ( CryptCreateHash( hCryptProv, CALG_MD5, 0, 0, &hHash ) )
        {
            if ( CryptHashData( hHash, buffer, bufferLength, 0 ) )
            {
                DWORD dataLength;
                DWORD hashSize;

                dataLength = sizeof( hashSize );
                if ( CryptGetHashParam( hHash, HP_HASHSIZE, (BYTE*) &hashSize, &dataLength, 0 ) )
                {
                    BYTE* hashValue = new BYTE[hashSize];

                    dataLength = hashSize;
                    if ( CryptGetHashParam( hHash, HP_HASHVAL, hashValue, &dataLength, 0 ) )
                    {
                        char hex[3];

                        for ( DWORD i = 0; i < hashSize; i++ )
                        {
                            _itoa( hashValue[i], hex, 16 );
                            md5str += hex;
                        }
                    }

                    delete [] hashValue;
                }
            }
        }
    }

    if ( hHash != NULL )
    {
        CryptDestroyHash( hHash );
    }
    if ( hCryptProv != NULL )
    {
        CryptReleaseContext( hCryptProv, 0 );
    }

    return md5str;
}

// Get local IP address as string (if a single valid IP found). Returns empty string if fails to resolve.
string GetLocalIpAddress( )
{
    PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO *) malloc( sizeof( IP_ADAPTER_INFO ) );
    PIP_ADAPTER_INFO pAdapter     = nullptr;
    ULONG            ulOutBufLen  = sizeof( IP_ADAPTER_INFO );
    string           localIp;

    if ( pAdapterInfo != nullptr )
    {
        if ( GetAdaptersInfo( pAdapterInfo, &ulOutBufLen ) == ERROR_BUFFER_OVERFLOW )
        {
            free( pAdapterInfo );
            pAdapterInfo = (IP_ADAPTER_INFO *) malloc( ulOutBufLen );
        }
    }

    if ( pAdapterInfo != nullptr )
    {
        if ( GetAdaptersInfo( pAdapterInfo, &ulOutBufLen ) == NO_ERROR )
        {
            pAdapter = pAdapterInfo;

            while ( pAdapter )
            {
                if ( ( strcmp( pAdapter->IpAddressList.IpAddress.String, "0.0.0.0" ) != 0 ) &&
                     ( strcmp( pAdapter->GatewayList.IpAddress.String, "0.0.0.0" ) != 0 ) &&
                     ( pAdapter->IpAddressList.Next == nullptr ) &&
                     ( pAdapter->GatewayList.Next == nullptr ) )
                {
                    if ( localIp.empty( ) )
                    {
                        localIp = string( pAdapter->IpAddressList.IpAddress.String );
                    }
                    else
                    {
                        // if there is more than one valid IP, assume we've failed
                        localIp.clear( );
                        break;
                    }
                }

                pAdapter = pAdapter->Next;
            }
        }

        free( pAdapterInfo );
    }

    return localIp;
}

// Convert xargb to string
std::string XargbToString( xargb color )
{
    char buffer[16];

    if ( color.components.a != 0xFF )
    {
        sprintf( buffer, STR_ARGB_FORMAT32, color.argb );
    }
    else
    {
        color.components.a = 0;
        sprintf( buffer, STR_ARGB_FORMAT24, color.argb );
    }

    return string( buffer );
}

// Parse xargb from string
bool StringToXargb( const string& str, xargb* color )
{
    size_t len      = str.length( );
    xargb  newColor = { 0xFF000000 };
    bool   parsed   = false;

    if ( len == 8 )
    {
        parsed = ( sscanf( str.c_str( ), STR_ARGB_FORMAT32, &newColor.argb ) == 1 );
    }
    else if ( len == 6 )
    {
        // in the case if only 6 hex numbers present, suppose it is RGB value with alpha set to FF
        if ( sscanf( str.c_str( ), STR_ARGB_FORMAT24, &newColor.argb ) == 1 )
        {
            newColor.components.a = 0xFF;
            parsed = true;
        }
    }

    if ( ( color != nullptr ) && ( parsed ) )
    {
        *color = newColor;
    }

    return parsed;
}
