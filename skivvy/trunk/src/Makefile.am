# /*-----------------------------------------------------------------.
# | Copyright (C) 2011 SooKee oasookee@googlemail.com               |
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

skivvydir = $(prefix)/include/skivvy
plugindir = $(datadir)/skivvy/plugins
docsdir = $(datadir)/skivvy

# Required for GCC on some systems it seems
#	-D _GLIBCXX_USE_NANOSLEEP
#	-D __GXX_EXPERIMENTAL_CXX0X__
#	-D _GLIBCXX_USE_SCHED_YIELD

DEF_FLAGS = \
	-D _GLIBCXX_USE_NANOSLEEP \
	-D _GLIBCXX_USE_SCHED_YIELD \
	-D STAMP=\"$(shell date +%Y%m%d-%H%M%S)\" \
	-D REVISION=\"$(shell svn info $(top_srcdir)|grep Revision:|cut -f 2 -d ' ')\" \
	-D DEFAULT_PLUGIN_DIR=\"$(plugindir)\"

GEN_FLAGS = -Wfatal-errors -Wall -Wextra -Winit-self -ansi -pedantic -pipe -pthread \
	-I$(top_srcdir)/src/include
	
AM_CXXFLAGS = $(GEN_FLAGS) $(DEF_FLAGS) -std=gnu++11

skivvy_HEADERS = \
	$(srcdir)/include/skivvy/ios.h \
	$(srcdir)/include/skivvy/rpc.h \
	$(srcdir)/include/skivvy/str.h \
	$(srcdir)/include/skivvy/bits/rpc.tcc \
	$(srcdir)/include/skivvy/types.h \
	$(srcdir)/include/skivvy/ircbot.h \
	$(srcdir)/include/skivvy/logrep.h \
	$(srcdir)/include/skivvy/socketstream.h

noinst_HEADERS = \
	$(srcdir)/mh/megahal.h \
	$(srcdir)/include/skivvy/cal.h \
	$(srcdir)/include/skivvy/irc.h \
	$(srcdir)/include/skivvy/irc-constants.h \
	$(srcdir)/include/skivvy/rcon.h \
	$(srcdir)/include/skivvy/stl.h \
	$(srcdir)/include/skivvy/network.h \
	$(srcdir)/include/skivvy/server.h \
	$(srcdir)/include/skivvy/plugin-artibot.h \
	$(srcdir)/include/skivvy/plugin-chanops.h \
	$(srcdir)/include/skivvy/plugin-compile.h \
	$(srcdir)/include/skivvy/plugin-diction.h \
	$(srcdir)/include/skivvy/plugin-example.h \
	$(srcdir)/include/skivvy/plugin-grabber.h \
	$(srcdir)/include/skivvy/plugin-oastats.h \
	$(srcdir)/include/skivvy/plugin-pfinder.h \
	$(srcdir)/include/skivvy/plugin-rconics.h \
	$(srcdir)/include/skivvy/plugin-rquotes.h \
	$(srcdir)/include/skivvy/plugin-rserver.h \
	$(srcdir)/include/skivvy/plugin-trivial.h

BOTLIBS = -lskivvy -ldl

lib_LTLIBRARIES = \
	libskivvy.la

plugin_LTLIBRARIES =  \
	skivvy-plugin-artibot.la \
	skivvy-plugin-chanops.la \
	skivvy-plugin-compile.la \
	skivvy-plugin-diction.la \
	skivvy-plugin-example.la \
	skivvy-plugin-grabber.la \
	skivvy-plugin-oastats.la \
	skivvy-plugin-pfinder.la \
	skivvy-plugin-rconics.la \
	skivvy-plugin-rquotes.la \
	skivvy-plugin-rserver.la \
	skivvy-plugin-trivial.la

# IrcBot framework
libskivvy_la_SOURCES = \
	ios.cpp \
	irc.cpp \
	ircbot.cpp \
	logrep.cpp \
	network.cpp \
	rpc.cpp \
	server.cpp \
	str.cpp \
	types.cpp
	
# IrcBot plugins
skivvy_plugin_artibot_la_SOURCES = plugin-artibot.cpp mh/megahal.cpp
skivvy_plugin_chanops_la_SOURCES = plugin-chanops.cpp
skivvy_plugin_compile_la_SOURCES = plugin-compile.cpp
skivvy_plugin_diction_la_SOURCES = plugin-diction.cpp
skivvy_plugin_example_la_SOURCES = plugin-example.cpp
skivvy_plugin_grabber_la_SOURCES = plugin-grabber.cpp
skivvy_plugin_oastats_la_SOURCES = plugin-oastats.cpp
skivvy_plugin_pfinder_la_SOURCES = plugin-pfinder.cpp
skivvy_plugin_rconics_la_SOURCES = plugin-rconics.cpp rcon.cpp
skivvy_plugin_rquotes_la_SOURCES = plugin-rquotes.cpp
skivvy_plugin_rserver_la_SOURCES = plugin-rserver.cpp
skivvy_plugin_trivial_la_SOURCES = plugin-trivial.cpp

skivvy_plugin_artibot_la_LDFLAGS = -module
skivvy_plugin_chanops_la_LDFLAGS = -module
skivvy_plugin_compile_la_LDFLAGS = -module
skivvy_plugin_diction_la_LDFLAGS = -module
skivvy_plugin_example_la_LDFLAGS = -module
skivvy_plugin_grabber_la_LDFLAGS = -module
skivvy_plugin_oastats_la_LDFLAGS = -module
skivvy_plugin_pfinder_la_LDFLAGS = -module
skivvy_plugin_rconics_la_LDFLAGS = -module
skivvy_plugin_rquotes_la_LDFLAGS = -module
skivvy_plugin_rserver_la_LDFLAGS = -module
skivvy_plugin_trivial_la_LDFLAGS = -module

# Development utils
noinst_PROGRAMS = test test-server convert-cvars

test_SOURCES = test.cpp
test_LDADD = $(BOTLIBS)

test_server_SOURCES = test-server.cpp
test_server_LDADD = $(BOTLIBS)

convert_cvars_SOURCES = convert-cvars.cpp
convert_cvars_LDADD = $(BOTLIBS)

# Skivvy IrcBot
bin_PROGRAMS = skivvy skivvy-client

skivvy_client_SOURCES = skivvy-client.cpp
skivvy_client_LDADD = $(BOTLIBS)

skivvy_SOURCES = skivvy.cpp
skivvy_LDADD = $(BOTLIBS)

docs_DATA = $(top_srcdir)/docs/manual.text

extra_DIST = $(top_srcdir)/docs/manual.text
