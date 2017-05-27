/*
    web2h - converts web files into header files to be used with web2cam

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void ShowUsage( );
int GenerateHeaderFile( const char* inputFileName, const char* outputFileName, const char* mimeType );
const char* ResolveMimeType( const char* fileName );

// suported MIME types
const char* STR_TEXT_HTML  = "text/html";
const char* STR_TEXT_CSS   = "text/css";
const char* STR_APP_JS     = "application/x-javascript";
const char* STR_IMAGE_JPEG = "image/jpeg";
const char* STR_IMAGE_PNG  = "image/png";

// supported extension
const char* STR_EXTENSIONS = "html, css, js, jpeg, png";

int main( int argc, char* argv[] )
{
    char* inputFileName      = nullptr;
    char* outputFileName     = nullptr;
    bool  freeOutputFileName = false;

    // since all options are provided in the form of "-option value", we must
    // have odd number of arguments including the executable name
    if ( ( argc == 1 ) || ( ( argc % 2 ) == 0 ) )
    {
        ShowUsage( );
        return 1;
    }

    // check command line argument
    for ( int i = 1; i < argc; i += 2 )
    {
        if ( strcmp( argv[i], "-i" ) == 0 )
        {
            inputFileName = argv[i + 1];
        }
        else if ( strcmp( argv[i], "-o" ) == 0 )
        {
            outputFileName = argv[i + 1];
        }
        else
        {
            ShowUsage( );
            return 1;
        }
    }

    // input file name is a must
    if ( inputFileName == nullptr )
    {
        ShowUsage( );
        return 1;
    }

    // generate output file name if it is not specified
    if ( outputFileName == nullptr )
    {
        outputFileName = (char*) malloc( strlen( inputFileName ) + 3 );
        strcpy( outputFileName, inputFileName );
        strcat( outputFileName, ".h" );
        freeOutputFileName = true;
    }
    
    const char* mimeType = ResolveMimeType( inputFileName );
    int         ret      = 0;

    if ( mimeType == nullptr )
    {
        ret = 2;
        printf( "Error: failed work out MIME type of the input file. Supported files: %s. \n\n", STR_EXTENSIONS );
    }
    else
    {
        ret = GenerateHeaderFile( inputFileName, outputFileName, mimeType );
    }

    if ( freeOutputFileName )
    {
        free( outputFileName );
    }

	return ret;
}

void ShowUsage( )
{
    printf( "web2h :: Converts web files into header files \n\n" );
    printf( "Usage: web2h -i input [-o output] \n\n" );
    printf( "       -i input web file to convert (%s); \n", STR_EXTENSIONS );
    printf( "       -o output header file name to create. \n\n" );
}

int GenerateHeaderFile( const char* inputFileName, const char* outputFileName, const char* mimeType )
{
    FILE* inputFile = fopen( inputFileName, "rb" );
    int   ret       = 0;

    if ( inputFile == nullptr )
    {
        printf( "Error: failed openning the specified input file:\n%s\n\n", inputFileName );
        ret = 3;
    }
    else
    {
        FILE* outputFile = fopen( outputFileName, "w" );

        if ( outputFile == nullptr )
        {
            printf( "Error: failed creating/openning the specified output file:\n%s\n\n", outputFileName );
            ret = 4;
        }
        else
        {
            const char* inputBaseName = inputFileName;
            const char* separator     = strrchr( inputBaseName, '\\' );
            
            if ( separator != nullptr )
            {
                inputBaseName = separator + 1;
            }

            separator = strrchr( inputBaseName, '/' );
            if ( separator != nullptr )
            {
                inputBaseName = separator + 1;
            }

#ifdef _MSC_VER
            char* outVarName = _strdup( inputBaseName );
#else
            char* outVarName = strdup( inputBaseName );
#endif

            // replace '.' with '_' in input file name to be used to form a variable name
            for ( int i = 0; outVarName[i] != 0; i++ )
            {
                if ( outVarName[i] == '.' )
                {
                    outVarName[i] = '_';
                }
            }

            // get input file's size
            fseek( inputFile, 0L, SEEK_END );
            int inputFileSize = ftell( inputFile );
            rewind( inputFile );

            // write output file
            fprintf( outputFile, "/*\n" );
            fprintf( outputFile, " * Auto generated header file to be used with web2cam\n" );
            fprintf( outputFile, " *\n" );
            fprintf( outputFile, " * Source file: %s\n", inputBaseName );
            fprintf( outputFile, " */\n\n" );

            fprintf( outputFile, "#ifndef %s_h\n", outVarName );
            fprintf( outputFile, "#define %s_h\n\n", outVarName );

            fprintf( outputFile, "#include \"XWebServer.hpp\"\n\n" );

            fprintf( outputFile, "uint8_t %s_data[]\n", outVarName );
            fprintf( outputFile, "{\n" );

            uint8_t buffer[20];
            size_t  read;

            while ( ( read = fread( buffer, 1, 20, inputFile ) ) != 0 )
            {
                fprintf( outputFile, "    " );

                for ( size_t i = 0; i < read; i++ )
                {
                    fprintf( outputFile, "0x%02X, ", buffer[i] );
                }

                fprintf( outputFile, "\n" );
            }

            fprintf( outputFile, "};\n\n" );


            fprintf( outputFile, "XEmbeddedContent web_%s =\n", outVarName );
            fprintf( outputFile, "{\n" );
            fprintf( outputFile, "    %d,\n", inputFileSize );
            fprintf( outputFile, "    \"%s\",\n", mimeType );
            fprintf( outputFile, "    %s_data,\n", outVarName );
            fprintf( outputFile, "};\n\n" );

            fprintf( outputFile, "#endif\n" );

            free( outVarName );
            fclose( outputFile );

            printf( "Generated header file: %s \n\n", outputFileName );
        }

        fclose( inputFile );
    }

    return ret;
}

const char* ResolveMimeType( const char* fileName )
{
    const char* ret = nullptr;
    const char* ext = strrchr( fileName, '.' );

    if ( ext != nullptr )
    {
        ext++;

        if ( strcmp( ext, "html" ) == 0 )
        {
            ret = STR_TEXT_HTML;
        }
        else if ( strcmp( ext, "css" ) == 0 )
        {
            ret = STR_TEXT_CSS;
        }
        else if ( strcmp( ext, "js" ) == 0 )
        {
            ret = STR_APP_JS;
        }
        else if ( strcmp( ext, "jpeg" ) == 0 )
        {
            ret = STR_IMAGE_JPEG;
        }
        else if ( strcmp( ext, "png" ) == 0 )
        {
            ret = STR_IMAGE_PNG;
        }
    }

    return ret;
}
