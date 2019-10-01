TEMPLATE = app
TARGET = bin/objviz
QT += widgets

HEADERS +=	src/mainWindow.hpp 

SOURCES +=	src/main.cpp \
			src/mainWindow.cpp

LIBS += -ltinyobjloader -lembree3