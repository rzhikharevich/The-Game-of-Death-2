CXXFLAGS = -std=c++14
LDFLAGS  = -lSDL2 -lSDL2_image
OBJS     = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
APP_NAME = deathgame

all: build

build: $(APP_NAME)

$(APP_NAME): $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $@

clean:
	-rm -f count $(OBJS) $(APP_NAME)
