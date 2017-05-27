#
#   web2h - converts web files into header files to be used with web2cam
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
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, write to the Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#

# Additional folders to look for source files
VPATH = ../../

# C++ code
SRC_CPP = web2h.cpp

# Output name    
OUT = web2h

# Compiler to use
COMPILER = g++
# Base compiler flags
CFLAGS = -O2 -s -DNDEBUG -std=c++0x

# Object files list
OBJ = $(SRC_CPP:.cpp=.o)

# Output folder for the build result
OUT_FOLDER = ../../../../../build/gcc/release/bin

# ===================================

all: build
 
%.o: %.cpp
	$(COMPILER) $(CFLAGS) -c $^ -o $@

$(OUT): $(OBJ)
	$(COMPILER) -o $@ $(OBJ)

build: $(OUT)
	mkdir -p $(OUT_FOLDER)
	cp $(OUT) $(OUT_FOLDER)

clean:
	rm $(OBJ) $(OUT)

