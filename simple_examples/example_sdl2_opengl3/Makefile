# ImAnim example: SDL2 + OpenGL3
# Cross-platform Makefile for Linux and macOS

EXE = im_anim_example
IMGUI_DIR = ../../extern/imgui
IMANIM_DIR = ../../src
DEMO_DIR = ../../demo
SOURCES = main.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
SOURCES += $(IMANIM_DIR)/im_anim.cpp
SOURCES += $(DEMO_DIR)/demo_im_anim.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

CXXFLAGS = -std=c++11 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(IMANIM_DIR)
CXXFLAGS += -g -Wall -Wformat
CXXFLAGS += `sdl2-config --cflags`

UNAME_S := $(shell uname -s)
LINUX_GL_LIBS = -lGL

ifeq ($(UNAME_S), Linux)
	LIBS = $(LINUX_GL_LIBS) `sdl2-config --libs`
endif

ifeq ($(UNAME_S), Darwin)
	LIBS = -framework OpenGL `sdl2-config --libs`
endif

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMANIM_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(DEMO_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete: $(EXE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
