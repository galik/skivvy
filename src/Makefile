
prefix = $(HOME)/dev/cpp/skivvy/install

# requirements
REQ_CXX11_TEST := $(shell echo "int main(){}" | $(CXX) -o /dev/null -x c++ -std=c++11 - 2> /dev/null; echo $$?)
REQ_CXX11_FAIL := "ERROR: Requires g++ v4.7 or above to comiile."

DEBUGFLAGS = \
	-D _GLIBCXX_DEBUG \
	-D _GLIBCXX_DEBUG_PEDANTIC \
	-D DEBUG \
	-O0 \
	-g3

RELEASEFLAGS = \
	-O3 \
	-g0

GENERALFLAGS = \
	-D _GLIBCXX_USE_NANOSLEEP \
	-D _GLIBCXX_USE_SCHED_YIELD \
	-pthread \
	-std=c++11 \
	-Wall \
	-Werror

CXXFLAGS = \
	$(GENERALFLAGS) \
	$(DEBUGFLAGS) \
	-Iinclude \
	-fPIC
	
libskivvy_so_LDFLAGS = \
	-shared \
	-ldl \
	-lpcrecpp \
	-pthread \
	-Wl,-E \
	-Wl,-soname,libskivvy.so.0 \
	-rdynamic \
	-Wl,-rpath,$(prefix)/lib/skivvy

libskivvy_so_SOURCES := \
	cal.cpp \
	FloodController.cpp \
	ios.cpp \
	irc.cpp \
	ircbot.cpp \
	IrcServer.cpp \
	logrep.cpp \
	message.cpp \
	network.cpp \
	props.cpp \
	rpc.cpp \
	server.cpp \
	store.cpp \
	Timers.cpp \
	types.cpp \
	utils.cpp
	
libskivvy_so_OBJECTS := $(patsubst %.cpp,%.o, $(libskivvy_so_SOURCES))
libskivvy_so_HEADERS := $(patsubst %.cpp,%.h, $(libskivvy_so_SOURCES))

skivvy_LDFLAGS = \
	-Wl,-E \
	-I../../libsookee/install/include \
	-ldl \
	-lpcrecpp \
	-pthread \
	-L$(HOME)/dev/cpp/libsookee/install/lib/sookee -lsookee \
	-L$(HOME)/dev/cpp/skivvy/src -lskivvy

skivvy_SOURCES := skivvy.cpp
skivvy_OBJECTS := $(patsubst %.cpp,%.o, $(skivvy_SOURCES))
skivvy_HEADERS := $(patsubst %.cpp,%.h, $(skivvy_SOURCES))

all: reqs libskivvy.so.0.0.0 skivvy

reqs:
	@if(($(REQ_CXX11_TEST))); then echo $(REQ_CXX11_FAIL); exit 1; fi

libskivvy.so.0.0.0: $(libskivvy_so_OBJECTS)
	$(CXX) $(libskivvy_so_LDFLAGS) -o $@ $(libskivvy_so_OBJECTS)
	ln -f -s $@ libskivvy.so
	
skivvy: $(skivvy_OBJECTS)
	$(CXX) $(skivvy_LDFLAGS) -o $@ $(skivvy_OBJECTS)

%.o: %.cpp %.h
	$(CXX) -c -o $@ $<

install: all
	mkdir -p $(prefix)/lib/skivvy
	cp libskivvy.so.0.0.0 $(prefix)/lib/skivvy
	/sbin/ldconfig -n $(prefix)/lib/skivvy
	ln -f -s $(prefix)/lib/skivvy/libskivvy.so.0 $(prefix)/lib/skivvy/libskivvy.so
	mkdir -p $(prefix)/include/skivvy
	cp -r include/skivvy $(prefix)/include
	mkdir -p $(prefix)/bin
	cp skivvy $(prefix)/bin
	
uninstall:
	rm -fr $(prefix)/lib/skivvy
	rm -fr $(prefix)/include/skivvy
	rm $(prefix)/bin/skivvy

clean:
	rm -fr *.o *.so.?.?.? 
	
.PHONY: reqs install uninstall

