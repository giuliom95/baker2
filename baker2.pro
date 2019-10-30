TEMPLATE = app
TARGET = bin/baker
QT += widgets

HEADERS +=	 src/core.hpp \
                 src/mainWindow.hpp \
                 src/math.hpp

SOURCES +=	src/core.cpp \
                src/mainWindow.cpp \
                src/main.cpp
			
INCLUDEPATH = "c:/Program Files/Intel/Embree3 x64/include" "c:/Users/Giulio/Downloads/tinyobjloader-master"
LIBS += -L"c:/Users/Giulio/Downloads/tinyobjloader-master/BUILD" -L"c:/Program Files/Intel/Embree3 x64/lib" -ltinyobjloader -lembree3
