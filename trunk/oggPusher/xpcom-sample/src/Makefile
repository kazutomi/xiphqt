CXX   = c++
CPPFLAGS +=     -fno-rtti              \
		-fno-exceptions        \
		-shared  	       \
 
# Change this to point at your Gecko SDK directory. 
GECKO_SDK_PATH = $(shell pkg-config --variable=sdkdir libxul)
 
# GCC only define which allows us to not have to #include mozilla-config 
# in every .cpp file.  If your not using GCC remove this line and add 
# #include "mozilla-config.h" to each of your .cpp files. 
#GECKO_CONFIG_INCLUDE = -include mozilla-config.h 
 
#GECKO_DEFINES  =  -DXPCOM_GLUE
 
GECKO_INCLUDES = $(shell pkg-config --cflags libxul)
 
GECKO_LDFLAGS =  -L$(GECKO_SDK_PATH)/lib -lxpcom -lnspr4 -Wl,-z,-defs

FILES = MyComponent.cpp MyComponentModule.cpp 

TARGET = MyComponent.so

build: 
	$(CXX) -Wall -Os -o $(TARGET)  $(GECKO_INCLUDES) $(CPPFLAGS) $(CXXFLAGS) $(FILES) /usr/lib/xulrunner-devel-1.9/lib/libxpcomglue_s.a $(GECKO_LDFLAGS)
	chmod +x $(TARGET)
	strip $(TARGET)
 
clean: 
	rm $(TARGET)
