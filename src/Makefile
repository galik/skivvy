include Makefile.local

lib_dir = $(prefix)/lib/skivvy
include_dir = $(prefix)/include
bin_dir = $(prefix)/bin

# requirements
REQ_CXX11_TEST := $(shell echo "int main(){}" | $(CXX) -o /dev/null -x c++ -std=c++11 - 2> /dev/null; echo $$?)
REQ_CXX11_FAIL := "ERROR: Requires g++ v4.7 or above to comiile."
REQ_CXX11 = if(($(REQ_CXX11_TEST))); then echo $(REQ_CXX11_FAIL); exit 1; fi

REQ_PCRE_TEST := $(shell printf %-3s $(shell pcre-config --version|tr -d '.')|tr ' ' '0')
REQ_PCRE_FAIL := "ERROR: Requires PCRE v7.8 or above."
REQ_PCRE = if(($(REQ_PCRE_TEST) < 780)); then echo $(REQ_PCRE_FAIL); exit 1; fi

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
	
	INCLUDES = \
		-Iinclude \
		-I$(prefix)/include

ifeq ($(build_debug),1)
	CXXFLAGS = \
		$(GENERALFLAGS) \
		$(DEBUGFLAGS) \
		-Iinclude \
		-I../../libsookee/src/include \
		-fPIC
else
	CXXFLAGS = \
		$(GENERALFLAGS) \
		$(RELEASEFLAGS) \
		-Iinclude \
		-I$(prefix)/include \
		-fPIC
endif
	
libskivvy_so_LDFLAGS = \
	-shared \
	-ldl \
	-lpcrecpp \
	-L/home/galik/dev/lib/sookee -lsookee \
	-pthread \
	-Wl,-E \
	-Wl,-soname,libskivvy.so.0 \
	-rdynamic \
	-Wl,-rpath,$(lib_dir)

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
	-ldl \
	-lpcrecpp \
	-pthread \
	$(LDFLAGS) \
	-L. -lskivvy

skivvy_SOURCES := skivvy.cpp
skivvy_OBJECTS := $(patsubst %.cpp,%.o, $(skivvy_SOURCES))
skivvy_HEADERS := $(patsubst %.cpp,%.h, $(skivvy_SOURCES))

all: reqs libskivvy.so.0.0.0 skivvy

reqs:
	@$(REQ_CXX11)
	@$(REQ_PCRE)

libskivvy.so.0.0.0: $(libskivvy_so_OBJECTS)
	$(CXX) $(INCLUDES) $(libskivvy_so_LDFLAGS) -o $@ $(libskivvy_so_OBJECTS)
	ln -f -s $@ libskivvy.so
	
skivvy: $(skivvy_OBJECTS)
	$(CXX) $(INCLUDES) $(skivvy_LDFLAGS) -o $@ $(skivvy_OBJECTS)

%.o: %.cpp %.h
	$(CXX) $(INCLUDES) $(CPPFLAGS) -c -o $@ $<

install: all
	mkdir -p $(lib_dir)
	cp libskivvy.so.0.0.0 $(lib_dir)
	/sbin/ldconfig -n $(lib_dir)
	ln -f -s $(lib_dir)/libskivvy.so.0 $(lib_dir)/libskivvy.so
	mkdir -p $(include_dir)/skivvy
	cp -r include/skivvy $(include_dir)
	mkdir -p $(bin_dir)
	cp skivvy $(bin_dir)
	
uninstall:
	rm -f $(lib_dir)/libskivvy*
	rm -fr $(include_dir)/skivvy
	rm $(bin_dir)/skivvy

clean: reqs
	rm -fr *.o *.so.?.?.? 
	
.PHONY: reqs install uninstall
