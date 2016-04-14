DEPS     = jsoncpp sdl2 SDL2_image
CXXFLAGS = -std=c++14 $(shell pkg-config --cflags $(DEPS))
LDFLAGS  = $(shell pkg-config --libs $(DEPS))
OBJS     = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
APP_NAME = deathgame

all: build

build: $(APP_NAME)

$(APP_NAME): $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $@

clean:
	-rm -f count $(OBJS) $(APP_NAME)
