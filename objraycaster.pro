TEMPLATE = app
TARGET = bin/objviz
QT += widgets

HEADERS +=	src/core.hpp \
			src/mainWindow.hpp \
			src/math.hpp

SOURCES +=	src/core.cpp \
			src/mainWindow.cpp \
			src/main.cpp
			

LIBS += -ltinyobjloader -lembree3