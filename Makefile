PRGNAME     = vbanext.elf
CC          = gcc
CXX 		= g++

#### Configuration

# Possible values : retrostone, rs97, rs90
PORT = gcw0
# Possible values : alsa, oss, portaudio
SOUND_ENGINE = alsa

#### End of Configuration

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"

INCLUDES	= -Ilibretro-common/include -Isrc
INCLUDES	+= -Ishell/headers -Ishell/video/$(PORT) -Ishell/audio -Ishell/scalers -Ishell/input/sdl -Ishell/fonts -Ishell/menu

DEFINES		= -DLSB_FIRST -DINLINE="inline" -DWANT_16BPP -DFRONTEND_SUPPORTS_RGB565 -DINLINE="inline" -DNDEBUG -DWANT_STEREO_SOUND
DEFINES		+= -DGIT_VERSION=\"$(GIT_VERSION)\"
# Extra defines
DEFINES		+= -DUSE_FRAME_SKIP -DUSE_TWEAKS -DRETROSTONE -DENABLE_JOYSTICKCODE -DIPU_SCALING_NONATIVE

CFLAGS		= -Ofast -g3 -march=native -fno-common $(INCLUDES) $(DEFINES)
CXXFLAGS	= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11
LDFLAGS     = -lc -lgcc -lm -lSDL -lz -pthread -lportaudio

ifeq ($(SOUND_ENGINE), alsa)
LDFLAGS 		+= -lasound
endif
ifeq ($(SOUND_ENGINE), portaudio)
LDFLAGS 		+= -lasound -lportaudio
endif

# Files to be compiled
SRCDIR 		=  ./src ./libretro-common ./libretro-common/rthreads ./shell ./shell/scalers ./shell/emu ./shell/menu
SRCDIR		+= ./shell/input/sdl/ ./shell/video/$(PORT) ./shell/audio/$(SOUND_ENGINE)
VPATH		= $(SRCDIR)
SRC_C		= $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))
SRC_CPP		= $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.cpp))
OBJ_C		= $(notdir $(patsubst %.c, %.o, $(SRC_C)))
OBJ_CPP		= $(notdir $(patsubst %.cpp, %.o, $(SRC_CPP)))
OBJS		= $(OBJ_C) $(OBJ_CPP)

# Rules to make executable
$(PRGNAME): $(OBJS)  
	$(CC) $(CFLAGS) -std=gnu99 -o $(PRGNAME) $^ $(LDFLAGS)
	
$(OBJ_CPP) : %.o : %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(PRGNAME)$(EXESUFFIX) *.o
