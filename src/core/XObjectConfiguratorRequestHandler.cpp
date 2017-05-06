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

#include <string.h>

#include "XObjectConfiguratorRequestHandler.hpp"
#include "XSimpleJsonParser.hpp"

using namespace std;

const static char* StatusOK                   = "OK";
const static char* StatusInvalidJson          = "Invalid JSON object";
const static char* StatusUnknownProperty      = "Unknown property";
const static char* StatusInvalidPropertyValue = "Invalid property value";
const static char* StatusPropertyFailed       = "Failed setting property";

XObjectConfiguratorRequestHandler::XObjectConfiguratorRequestHandler( const string& uri,
                                                                      const shared_ptr<IObjectConfigurator>& objectToConfig ) :
    IWebRequestHandler( uri, false ),
    ObjectToConfig( objectToConfig )
{
}

// Handle object configuration request by providing its current configuration or setting specified varaibles/properties
void XObjectConfiguratorRequestHandler::HandleHttpRequest( const IWebRequest& request, IWebResponse& response )
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
void XObjectConfiguratorRequestHandler::HandleGet( const string& varsToGet, IWebResponse& response )
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
void XObjectConfiguratorRequestHandler::HandlePost( const string& body, IWebResponse& response )
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
