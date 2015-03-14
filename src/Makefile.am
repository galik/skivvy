# /*-----------------------------------------------------------------.
# | Copyright (C) 2011 SooKee oasookee@googlemail.com                |
# '------------------------------------------------------------------'
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
# 
# http://www.gnu.org/licenses/gpl-2.0.html
# 
# '-----------------------------------------------------------------*/

skivvy_includedir = $(prefix)/include/skivvy
skivvy_include_bitsdir = $(prefix)/include/skivvy/bits
skivvy_librarydir = $(libdir)/skivvy
skivvy_manualsdir = $(datadir)/skivvy
plugin_librarydir = $(libdir)/skivvy

DEF_FLAGS = \
	-D STAMP=\"$(shell date +%Y%m%d-%H%M%S)\" \
	-D DEV=\"$(shell git diff --quiet || echo -dev)\" \
	-D COMMITS=\"$(shell printf %04d \"$(git log --after={yesterday} --pretty=oneline|wc -l)\")\" \
	-D REVISION=\"$(shell git log -n 1 --pretty=format:%h|tr [:lower:] [:upper:])\" \
	-D DEFAULT_PLUGIN_DIR=\"$(plugindir)\"

GEN_FLAGS = -Wl,-E -Wfatal-errors -Wall -Wextra -Winit-self -ansi -pedantic -pthread \
	-I$(top_srcdir)/src/include

AM_CPPFLAGS = $(SOOKEE_CFLAGS)
AM_CXXFLAGS = -std=gnu++11 $(GEN_FLAGS) $(DEF_FLAGS)

skivvy_include_HEADERS = \
	$(srcdir)/include/skivvy/ansi.h \
	$(srcdir)/include/skivvy/cal.h \
	$(srcdir)/include/skivvy/FloodController.h \
	$(srcdir)/include/skivvy/ios.h \
	$(srcdir)/include/skivvy/ircbot.h \
	$(srcdir)/include/skivvy/irc-constants.h \
	$(srcdir)/include/skivvy/irc.h \
	$(srcdir)/include/skivvy/IrcServer.h \
	$(srcdir)/include/skivvy/logrep.h \
	$(srcdir)/include/skivvy/mail.h \
	$(srcdir)/include/skivvy/message.h \
	$(srcdir)/include/skivvy/network.h \
	$(srcdir)/include/skivvy/openarena.h \
	$(srcdir)/include/skivvy/props.h \
	$(srcdir)/include/skivvy/rpc.h \
	$(srcdir)/include/skivvy/server.h \
	$(srcdir)/include/skivvy/socketstream.h \
	$(srcdir)/include/skivvy/stl.h \
	$(srcdir)/include/skivvy/store.h \
	$(srcdir)/include/skivvy/Timers.h \
	$(srcdir)/include/skivvy/types.h \
	$(srcdir)/include/skivvy/utils.h
	
skivvy_include_bits_HEADERS = \
	$(srcdir)/include/skivvy/bits/rpc.tcc

BOTLIBS = -ldl -L$(builddir)/.libs -lskivvy $(PCRECPP_LIBS) $(SOOKEE_LIBS)
# -Wl,-E is required for programs that load plugin's
BOTFLAGS = -Wl,-E

skivvy_library_LTLIBRARIES = \
	libskivvy.la

# IrcBot framework
libskivvy_la_SOURCES = \
	message.cpp \
	cal.cpp \
	FloodController.cpp \
	ios.cpp \
	ircbot.cpp \
	irc.cpp \
	IrcServer.cpp \
	logrep.cpp \
	network.cpp \
	props.cpp \
	rpc.cpp \
	server.cpp \
	store.cpp \
	Timers.cpp \
	types.cpp \
	utils.cpp
	
libskivvy_la_LIBADD = -L.libs $(SOOKEE_LIBS)
libskivvy_la_LDFLAGS = -version-info 0:0:0

# Development utils
#noinst_PROGRAMS = \
#	skivlet
#	convert-cvars \
#	test \
#	test-props \
#	test-server

#test_SOURCES = test.cpp
#test_LDADD = $(BOTLIBS)

#test_props_SOURCES = test-props.cpp
#test_props_LDADD = $(BOTLIBS)

#convert_cvars_SOURCES = convert-cvars.cpp
#convert_cvars_LDADD = $(BOTLIBS)

# Skivvy IrcBot
#bin_PROGRAMS = skivvy skivvy-client test-server
bin_PROGRAMS = skivvy skivvy-client

skivvy_SOURCES = skivvy.cpp
skivvy_LDADD = -ldl -lskivvy $(PCRECPP_LIBS) $(SOOKEE_LIBS)
skivvy_LDFLAGS = -Wl,-E

skivvy_client_SOURCES = skivvy-client.cpp
skivvy_client_LDADD = $(BOTLIBS) -lreadline

#skivlet_SOURCES = skivlet.cpp
#skivlet_LDADD = $(BOTLIBS)
#skivlet_LDFLAGS =

#test_server_SOURCES = test-server.cpp
#test_server_LDADD = $(BOTLIBS)
#test_server_LDFLAGS = -pthread

skivvy_manuals_DATA = $(top_srcdir)/docs/skivvy-manual.text

extra_DIST = $(top_srcdir)/docs/manual.text
