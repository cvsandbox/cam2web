#
#   cam2web - streaming camera to web
#
#   Copyright (C) 2017, cvsandbox, cvsandbox@gmail.com
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, write to the Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#

# Additional folders to look for source files
VPATH = ../../../externals/mongoose/ \
        ../../core \
        ../../core/cameras/RaspiCam

# C code
SRC_C = mongoose.c 
# C++ code
SRC_CPP = cam2web.cpp XImage.cpp XJpegEncoder.cpp XManualResetEvent.cpp \
    XRaspiCamera.cpp XRaspiCameraConfig.cpp XVideoSourceToWeb.cpp XWebServer.cpp \
    XSimpleJsonParser.cpp XObjectConfiguratorRequestHandler.cpp XError.cpp

# Output name    
OUT = cam2web

# Compiler to use
COMPILER = g++
# Base compiler flags
CFLAGS = -O2 -s -DNDEBUG -std=c++0x

# Object files list
OBJ = $(SRC_CPP:.cpp=.o) $(SRC_C:.c=.o)

# Additional include folders
INCLUDE = -I../../../externals/mongoose/ \
    -I../../core \
    -I../../core/cameras/RaspiCam \
    -I/opt/vc/include \
    -I/opt/vc/include/interface/vcos \
    -I/opt/vc/include/interface/vcos/pthreads \
    -I/opt/vc/include/interface/vmcs_host/linux

# Libraries to use
LIBS = -lmmal_core -lmmal_util -lmmal_vc_client -lvcos -lbcm_host -ljpeg

# Folders to look for additional libraries
LIBDIR = -L/opt/vc/lib

# Enable threads in Mongoose
CFLAGS += -DMG_ENABLE_THREADS

CFLAGS += $(INCLUDE) 
LDFLAGS = $(LIBDIR) $(LIBS) -pthread

# ===================================

all: build
 
%.o: %.c
	$(COMPILER) $(CFLAGS) -c $^ -o $@
%.o: %.cpp
	$(COMPILER) $(CFLAGS) -c $^ -o $@

$(OUT): $(OBJ)
	$(COMPILER) -o $@ $(OBJ) $(LDFLAGS)

build: $(OUT)

clean:
	rm $(OBJ) $(OUT)

