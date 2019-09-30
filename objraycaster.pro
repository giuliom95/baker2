TEMPLATE = app
TARGET = bin/objviz
QT += widgets

HEADERS +=	src/mainWindow.hpp \
			src/canvasWidget.hpp

SOURCES +=	src/main.cpp \
			src/mainWindow.cpp \
			src/canvasWidget.cpp 