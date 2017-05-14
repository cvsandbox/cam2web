#
#   cam2web - streaming camera to web
#
#   BSD 2-Clause License
#
#   Copyright (c) 2017, cvsandbox, cvsandbox@gmail.com
#   All rights reserved.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are met:
#
#   * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
#   * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

