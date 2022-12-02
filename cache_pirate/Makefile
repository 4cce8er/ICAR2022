CXX_FLAGS += -fopenmp -lpthread
ifneq ($(DEBUG),)
    CXX_FLAGS += -std=c++11 -O0 -g -Wall
else
    CXX_FLAGS += -std=c++14 -O2 -g -Wall
endif

ifneq ($(PCM_PATH),)
    $(info    [INFO] PCM is installed in $(PCM_PATH))
else
    $(error    PCM_PATH is not found)
    exit(1)
endif

TARGETS = cachePirate forkTest

.PHONY: all
all: $(TARGETS)

cachePirate : cachePirate.cc
	$(CXX) $(CXX_FLAGS) $< -o $@ -I$(PCM_PATH)/src/ -lpcm -L$(PCM_PATH)/build/lib/

forkTest : forkTest.cc
	$(CXX) $(CXX_FLAGS)  $< -o $@ -I$(PCM_PATH)/src/ -lpcm -L$(PCM_PATH)/build/lib/

.PHONY: clean
clean:
	rm -f $(TARGETS)
