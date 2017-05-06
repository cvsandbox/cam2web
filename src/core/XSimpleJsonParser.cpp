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

#include "XSimpleJsonParser.hpp"

using namespace std;

static bool ExtractString( const char** ptr, string& str );
static bool ExtractValue( const char** ptr, string& str, bool addQuotesToStrings );
static bool ExtractArrayAsString( const char** ptr, string& str );
static bool ExtractObjectAsString( const char** ptr, string& str );

// Skip spaces in a string pointed by the pointer
#define SKIP_SPACES(strptr) while ( ( *strptr != '\0' ) && ( *strptr == ' ' ) ) { strptr++; }

// Parse JSON string and provide it as name-value map. Nested objects/arrays
// are not supported - provided as they are in string format.
bool XSimpleJsonParser( const string& jsonStr, map<string, string>& values )
{
    const char* ptr = jsonStr.c_str( );
    string      strName, strValue;

    values.clear( );

    SKIP_SPACES( ptr );

    if ( *ptr != '{' )
    {
        return false;
    }

    for ( ; ; )
    {
        ptr++;

        if ( !ExtractString( &ptr, strName ) )
        {
            break;
        }

        if ( *ptr != ':' )
        {
            return false;
        }

        ptr++;

        if ( !ExtractValue( &ptr, strValue, false ) )
        {
            return false;
        }

        values.insert( pair<string, string>( strName, strValue ) );

        if ( *ptr == '}' )
        {
            // all done
            break;
        }

        if ( *ptr != ',' )
        {
            // broken JSON
            return false;
        }
    }

    if ( *ptr != '}' )
    {
        return false;
    }

    return true;
}

// Extract a string from JSON
bool ExtractString( const char** ptr, string& str )
{
    const char* p = *ptr;

    SKIP_SPACES( p );

    if ( *p != '"' )
    {
        return false;
    }
    else
    {
        string s;

        p++;

        while ( ( *p != '\0' ) && ( *p != '"' ) )
        {
            if ( *p != '\\' )
            {
                s.push_back( *p );
                p++;
            }
            else
            {
                p++;
                if ( *p == '\0' )
                {
                    return false;
                }
                else
                {
                    char esc = *p;

                    p++;

                    switch ( esc )
                    {
                    case '"':
                        s.push_back( '"' );
                        break;
                    case '\\':
                        s.push_back( '\\' );
                        break;
                    case '/':
                        s.push_back( '/' );
                        break;
                    case 'b':
                        s.push_back( '\b' );
                        break;
                    case 'f':
                        s.push_back( '\f' );
                        break;
                    case 'n':
                        s.push_back( '\n' );
                        break;
                    case 'r':
                        s.push_back( '\r' );
                        break;
                    case 't':
                        s.push_back( '\t' );
                        break;
                    case 'u':
                        // unicode is not supported - just skip 4 hex digits
                        for ( int i = 0; i < 4; i++ )
                        {
                            if ( *p == '\0' )
                            {
                                return false;
                            }
                            p++;
                        }
                        break;
                    default:
                        return false;
                    }
                }
            }
        }

        if ( *p != '"' )
        {
            return false;
        }

        // move to the next character after the string and spaces
        p++;
        SKIP_SPACES( p );

        *ptr = p;
        str  = s;
    }

    return true;
}

// Extract a value from JSON
bool ExtractValue( const char** ptr, string& str, bool addQuotesToStrings )
{
    const char* p = *ptr;
    string      s;

    SKIP_SPACES( p );

    switch ( *p )
    {
    case '"':   // string value
        if ( !ExtractString( &p, s ) )
        {
            return false;
        }
        if ( addQuotesToStrings )
        {
            s.insert( 0, 1, '"' );
            s.push_back( '"' );
        }
        break;

    case '{':   // object value
        if ( !ExtractObjectAsString( &p, s ) )
        {
            return false;
        }
        break;

    case '[':   // array value
        if ( !ExtractArrayAsString( &p, s ) )
        {
            return false;
        }
        break;

    default:
        if ( ( *p == 't' ) || ( *p == 'f' ) || ( *p == 'n' ) || ( *p == '-' ) || ( ( *p >= '0' ) && ( *p <= '9' ) ) )
        {
            // number or true/false/null  - extract everything up to the next space or ,}] - no checking for valid numbers
            while ( ( *p != '\0' ) )
            {
                if ( ( *p == ' ' ) || ( *p == ',' ) || ( *p == '}' ) || ( *p == ']' ) )
                {
                    break;
                }
                s.push_back( *p );
                p++;
            }

            if ( ( ( s[0] == 't' ) && ( s != "true"  ) ) ||
                 ( ( s[0] == 'f' ) && ( s != "false" ) ) ||
                 ( ( s[0] == 'n' ) && ( s != "null"  ) ) )
            {
                return false;
            }

            SKIP_SPACES( p );
        }
        else
        {
            // broken value
            return false;
        }
    }

    *ptr = p;
    str  = s;

    return true;
}

// Extract JSPN array as string
static bool ExtractArrayAsString( const char** ptr, string& str )
{
    const char* p = *ptr;

    SKIP_SPACES( p );

    if ( *p != '[' )
    {
        return false;
    }
    else
    {
        string s, strValue;

        for ( ; ; )
        {
            p++;

            if ( !ExtractValue( &p, strValue, true ) )
            {
                break;
            }

            if ( !s.empty( ) )
            {
                s.append( "," );
            }
            s.append( strValue );

            if ( *p == ']' )
            {
                break;
            }

            if ( *p != ',' )
            {
                return false;
            }
        }

        if ( *p != ']' )
        {
            return false;
        }

        s.insert( 0, 1, '[' );
        s.push_back( ']' );

        p++; // move to the next character after the array and spaces
        SKIP_SPACES( p );

        *ptr = p;
        str  = s;
    }

    return true;
}

// Extract JSON object as string
static bool ExtractObjectAsString( const char** ptr, string& str )
{
    const char* p = *ptr;

    SKIP_SPACES( p );

    if ( *p != '{' )
    {
        return false;
    }
    else
    {
        string s, strName, strValue;

        for (  ;; )
        {
            p++;

            if ( !ExtractString( &p, strName ) )
            {
                break;
            }

            if ( *p != ':' )
            {
                return false;
            }

            p++;

            if ( !ExtractValue( &p, strValue, true ) )
            {
                return false;
            }

            if ( !s.empty( ) )
            {
                s.append( "," );
            }
            s.append( strName );
            s.push_back( ':' );
            s.append( strValue );

            if ( *p == '}' )
            {
                break;
            }

            if ( *p != ',' )
            {
                return false;
            }
        }

        if ( *p != '}' )
        {
            return false;
        }

        s.insert( 0, 1, '{' );
        s.push_back( '}' );

        // move to the next character after the object and spaces
        p++;
        SKIP_SPACES( p );

        *ptr = p;
        str  = s;
    }

    return true;
}
