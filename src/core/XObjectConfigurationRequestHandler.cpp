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

#include <string.h>

#include "XObjectConfigurationRequestHandler.hpp"
#include "XSimpleJsonParser.hpp"

using namespace std;

const static char* StatusOK                   = "OK";
const static char* StatusInvalidJson          = "Invalid JSON object";
const static char* StatusUnknownProperty      = "Unknown property";
const static char* StatusInvalidPropertyValue = "Invalid property value";
const static char* StatusPropertyFailed       = "Failed setting property";

XObjectConfigurationRequestHandler::XObjectConfigurationRequestHandler( const string& uri,
                                                                        const shared_ptr<IObjectConfigurator>& objectToConfig ) :
    IWebRequestHandler( uri, false ),
    ObjectToConfig( objectToConfig )
{
}

// Handle object configuration request by providing its current configuration or setting specified varaibles/properties
void XObjectConfigurationRequestHandler::HandleHttpRequest( const IWebRequest& request, IWebResponse& response )
{
    string method = request.Method( );

    if ( method == "POST" )
    {
        HandlePost( request.Body( ), response );
    }
    else if ( method == "GET" )
    {
        HandleGet( request.GetVariable( "vars" ), response );
    }
    else
    {
        response.Printf( "HTTP/1.1 405 Method Not Allowed\r\n"
                         "Allow: GET, POST\r\n"
                         "Content-Type: text/plain\r\n"
                         "Connection: close\r\n"
                         "\r\n"
                         "Method Not Allowed" );
    }
}

// Get all or the list of specified variables
void XObjectConfigurationRequestHandler::HandleGet( const string& varsToGet, IWebResponse& response )
{
    map<string, string> values;
    string              reply = "{\"status\":\"OK\",\"config\":{";
    bool                first = true;

    if ( varsToGet.empty( ) )
    {
        // get all properties of the object
        values = ObjectToConfig->GetAllProperties( );
    }
    else
    {
        // get only specified properties (comma separated list)
        int start = 0;

        while ( varsToGet[start] != '\0' )
        {
            int end = start;

            while ( ( varsToGet[end] != '\0' ) && ( varsToGet[end] != ',' ) )
            {
                end++;
            }

            int count = end - start;

            if ( count != 0 )
            {
                string varName = varsToGet.substr( start, count );
                string varValue;

                if ( ObjectToConfig->GetProperty( varName, varValue ) == XError::Success )
                {
                    values.insert( pair<string, string>( varName, varValue ) );
                }
            }

            start = end;
            if ( varsToGet[start] == ',' )
            {
                start++;
            }
        }
    }

    // form a JSON response with values of the properties
    for ( auto kvp : values )
    {
        if ( !first )
        {
            reply += ",";
        }

        reply += "\"";
        reply += kvp.first;
        reply += "\":\"";
        reply += kvp.second;
        reply += "\"";

        first = false;
    }

    reply += "}}";

    response.Printf( "HTTP/1.1 200 OK\r\n"
                     "Content-Type: application/json\r\n"
                     "Content-Length: %d\r\n"
                     "Cache-Control: no-store, must-revalidate\r\nPragma: no-cache\r\nExpires: 0\r\n"
                     "\r\n"
                     "%s", (int) reply.length( ), reply.c_str( ) );

}

// Set all variables specified in the posted JSON
void XObjectConfigurationRequestHandler::HandlePost( const string& body, IWebResponse& response )
{
    const char*         status = StatusOK;
    map<string, string> values;
    string              reply = "{\"status\":\"";
    string              failedProperty;

    if ( !XSimpleJsonParser( body, values ) )
    {
        status = StatusInvalidJson;
    }
    else
    {
        for ( auto kvp : values )
        {
            XError ecode = ObjectToConfig->SetProperty( kvp.first, kvp.second );

            if ( ecode != XError::Success )
            {
                failedProperty = kvp.first;
                switch ( ecode.Code( ) )
                {
                case XError::UnknownProperty:
                    status = StatusUnknownProperty;
                    break;
                case XError::InvalidPropertyValue:
                    status = StatusInvalidPropertyValue;
                    break;
                default:
                    status = StatusPropertyFailed;
                    break;
                }
            }
        }
    }

    reply += status;
    if ( !failedProperty.empty( ) )
    {
        reply += "\",\"property\":\"";
        reply += failedProperty;
    }
    reply += "\"}";

    response.Printf( "HTTP/1.1 200 OK\r\n"
                     "Content-Type: application/json\r\n"
                     "Content-Length: %d\r\n"
                     "Cache-Control: no-store, must-revalidate\r\nPragma: no-cache\r\nExpires: 0\r\n"
                     "\r\n"
                     "%s", (int) reply.length( ), reply.c_str( ) );
}
