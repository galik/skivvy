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
	-D DEV=\"$(shell git diff --quiet || echo -dev)\" \
	-D COMMITS=\"$(shell printf %04d \"$(git log --after={yesterday} --pretty=oneline|wc -l)\")\" \
	-D REVISION=\"$(shell git log -n 1 --pretty=format:%h|tr [:lower:] [:upper:])\" \
	-D DEFAULT_PLUGIN_DIR=\"$(plugindir)\"

GEN_FLAGS = -Wl,-E -Wfatal-errors -Wall -Wextra -Winit-self -ansi -pedantic -pipe -pthread \
	-I$(top_srcdir)/src/include
	
AM_CXXFLAGS = $(GEN_FLAGS) $(DEF_FLAGS) -std=gnu++11

#BOOST_LIBS = -lboost_regex-mt

skivvy_HEADERS = \
	$(srcdir)/include/skivvy/bits/rpc.tcc \
	$(srcdir)/include/skivvy/FloodController.h \
	$(srcdir)/include/skivvy/ios.h \
	$(srcdir)/include/skivvy/ircbot.h \
	$(srcdir)/include/skivvy/logrep.h \
	$(srcdir)/include/skivvy/props.h \
	$(srcdir)/include/skivvy/rpc.h \
	$(srcdir)/include/skivvy/socketstream.h \
	$(srcdir)/include/skivvy/str.h \
	$(srcdir)/include/skivvy/store.h \
	$(srcdir)/include/skivvy/types.h

noinst_HEADERS = \
	$(srcdir)/mh/megahal.h \
	$(srcdir)/include/skivvy/cal.h \
	$(srcdir)/include/skivvy/irc.h \
	$(srcdir)/include/skivvy/irc-constants.h \
	$(srcdir)/include/skivvy/stl.h \
	$(srcdir)/include/skivvy/network.h \
	$(srcdir)/include/skivvy/server.h \
	$(srcdir)/include/skivvy/plugin-artibot.h \
	$(srcdir)/include/skivvy/plugin-chanops.h \
	$(srcdir)/include/skivvy/plugin-compile.h \
	$(srcdir)/include/skivvy/plugin-diction.h \
	$(srcdir)/include/skivvy/plugin-example.h \
	$(srcdir)/include/skivvy/plugin-factoid.h \
	$(srcdir)/include/skivvy/plugin-oastats.h \
	$(srcdir)/include/skivvy/plugin-pfinder.h \
	$(srcdir)/include/skivvy/plugin-rquotes.h \
	$(srcdir)/include/skivvy/plugin-rserver.h \
	$(srcdir)/include/skivvy/plugin-trivial.h

BOTLIBS = -lskivvy -ldl -lpcrecpp
# -Wl,-E is required for programs that load plugin's
BOTFLAGS = -Wl,-E

lib_LTLIBRARIES = \
	libskivvy.la

plugin_LTLIBRARIES =  \
	skivvy-plugin-artibot.la \
	skivvy-plugin-chanops.la \
	skivvy-plugin-compile.la \
	skivvy-plugin-diction.la \
	skivvy-plugin-example.la \
	skivvy-plugin-factoid.la \
	skivvy-plugin-oastats.la \
	skivvy-plugin-pfinder.la \
	skivvy-plugin-rquotes.la \
	skivvy-plugin-rserver.la \
	skivvy-plugin-trivial.la

# IrcBot framework
libskivvy_la_SOURCES = \
	FloodController.cpp \
	ios.cpp \
	irc.cpp \
	ircbot.cpp \
	logrep.cpp \
	network.cpp \
	props.cpp \
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
skivvy_plugin_factoid_la_SOURCES = plugin-factoid.cpp
skivvy_plugin_oastats_la_SOURCES = plugin-oastats.cpp
skivvy_plugin_pfinder_la_SOURCES = plugin-pfinder.cpp
skivvy_plugin_rquotes_la_SOURCES = plugin-rquotes.cpp
skivvy_plugin_rserver_la_SOURCES = plugin-rserver.cpp
skivvy_plugin_trivial_la_SOURCES = plugin-trivial.cpp

skivvy_plugin_artibot_la_LDFLAGS = -module
skivvy_plugin_chanops_la_LDFLAGS = -module
skivvy_plugin_compile_la_LDFLAGS = -module
skivvy_plugin_diction_la_LDFLAGS = -module
skivvy_plugin_example_la_LDFLAGS = -module
skivvy_plugin_factoid_la_LDFLAGS = -module
skivvy_plugin_oastats_la_LDFLAGS = -module
skivvy_plugin_pfinder_la_LDFLAGS = -module
skivvy_plugin_rquotes_la_LDFLAGS = -module
skivvy_plugin_rserver_la_LDFLAGS = -module
skivvy_plugin_trivial_la_LDFLAGS = -module

# Development utils
noinst_PROGRAMS = \
	convert-cvars \
	skivlet \
	test \
	test-props \
	test-server

test_SOURCES = test.cpp
test_LDADD = $(BOTLIBS)

test_props_SOURCES = test-props.cpp
test_props_LDADD = $(BOTLIBS)

test_server_SOURCES = test-server.cpp
test_server_LDADD = $(BOTLIBS)

convert_cvars_SOURCES = convert-cvars.cpp
convert_cvars_LDADD = $(BOTLIBS)

skivlet_SOURCES = skivlet.cpp
skivlet_LDADD = $(BOTLIBS)
skivlet_LDFLAGS =

# Skivvy IrcBot
bin_PROGRAMS = skivvy skivvy-client

skivvy_client_SOURCES = skivvy-client.cpp
skivvy_client_LDADD = $(BOTLIBS)

skivvy_SOURCES = skivvy.cpp
skivvy_LDADD = $(BOTLIBS)
skivvy_LDFLAGS = $(BOTFLAGS)

docs_DATA = $(top_srcdir)/docs/manual.text

extra_DIST = $(top_srcdir)/docs/manual.text
