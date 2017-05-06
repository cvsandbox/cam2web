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

#ifndef XDEVICE_NAME_HPP
#define XDEVICE_NAME_HPP

#include <string>

// Name of a DirectShow device including its moniker string
class XDeviceName
{
public:
	XDeviceName( std::string moniker, std::string name );

	const std::string Name( ) const { return mName; }
	const std::string Moniker( ) const { return mMoniker; }

	// Check if two device names are equal (moniker is checked only)
	bool operator==( const XDeviceName& rhs ) const;
	bool operator==( const std::string& rhsMoniker ) const;

	// Check if two device names are NOT equal
    bool operator!=( const XDeviceName& rhs  ) const
	{
        return ( !( (*this) == rhs ) ) ;
    }
    bool operator!=( const std::string& rhsMoniker  ) const
	{
        return ( !( (*this) == rhsMoniker ) ) ;
    }

private:
	std::string mMoniker;
	std::string mName;
};

#endif // XDEVICE_NAME_HPP
