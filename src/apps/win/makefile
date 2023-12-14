# Compiler settings
CXX = g++
CXXFLAGS = -Wall -g -DUNICODE -D_UNICODE -D_DEBUG -mssse3 -std=c++0x -fpermissive
INCLUDES = -I../../core/ -I../../core/cameras/DirectShow/ -I../../../externals/mongoose/

# Application name
APP = cam2web

# Source files
CPP_SOURCES = $(wildcard *.cpp) $(wildcard ../../core/*.cpp) $(wildcard ../../core/cameras/DirectShow/*.cpp)
C_SOURCES = $(wildcard ../../../externals/mongoose/*.c)

# Object files
CPP_OBJECTS = $(CPP_SOURCES:.cpp=.o)
C_OBJECTS = $(C_SOURCES:.c=.o)

# Libraries to link against
LIBS = -lcomctl32 -lgdi32 -ljpeg -pthread -lIphlpapi -lole32 -lOleAut32 -lstrmiids  -luuid  -lrpcrt4 -lws2_32 -municode -Wl,-subsystem,windows

# Output folder
OUT_FOLDER = ../../../build/gcc/

# Resource file
RES_FILE = cam2web.rc
RES_OBJECT = cam2web_res.o # Changed to a unique name

# Default target
all: $(APP)

# Rule to compile the resource file
$(RES_OBJECT): $(RES_FILE)
	windres $(RES_FILE) -O coff -o $(RES_OBJECT)

# Linking the application
$(APP): $(CPP_OBJECTS) $(C_OBJECTS) $(RES_OBJECT)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(OUT_FOLDER)/$(APP) $(CPP_OBJECTS) $(C_OBJECTS) $(LIBS) $(RES_OBJECT)

# Compiling C++ source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compiling C source files into object files
%.o: %.c
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

copyweb:
	mkdir -p $(OUT_WEB)
	cp ../../web/* $(OUT_WEB)
	cp ../../../externals/jquery/*.js $(OUT_WEB)
	cp ../../../externals/jquery/*.css $(OUT_WEB)
	cp -r $(OUT_WEB) .

generateweb:
	mkdir -p $(OUT_INC)
	cp ../../web/* $(OUT_INC)
	cp ../../../externals/jquery/*.js $(OUT_INC)
	cp ../../../externals/jquery/*.css $(OUT_INC)
	$(WEB2H) -i $(OUT_INC)index.html -o $(OUT_INC)index.html.h
	$(WEB2H) -i $(OUT_INC)styles.css -o $(OUT_INC)styles.css.h
	$(WEB2H) -i $(OUT_INC)cam2web.png -o $(OUT_INC)cam2web.png.h
	$(WEB2H) -i $(OUT_INC)cam2web_white.png -o $(OUT_INC)cam2web_white.png.h
	$(WEB2H) -i $(OUT_INC)cameraproperties.html -o $(OUT_INC)cameraproperties.html.h    
	$(WEB2H) -i $(OUT_INC)camera.js -o $(OUT_INC)camera.js.h
	$(WEB2H) -i $(OUT_INC)cameraproperties.js -o $(OUT_INC)cameraproperties.js.h
	$(WEB2H) -i $(OUT_INC)jquery.js -o $(OUT_INC)jquery.js.h
	$(WEB2H) -i $(OUT_INC)jquery.mobile.js -o $(OUT_INC)jquery.mobile.js.h
	$(WEB2H) -i $(OUT_INC)jquery.mobile.css -o $(OUT_INC)jquery.mobile.css.h
	rm $(OUT_INC)*.html
	rm $(OUT_INC)*.css
	rm $(OUT_INC)*.js
# Phony targets
.PHONY: all clean
