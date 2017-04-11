include $(top_srcdir)/common.mk

INCLUDES += -I/usr/local/mioji/include -I/usr/include/openssl/

bin_PROGRAMS=
lib_LIBRARIES=
lib_LTLIBRARIES=

lib_LTLIBRARIES+=libpreroute.la
libpreroute_la_SOURCES= Route.cpp LYConstData.cpp City.cpp RouteConfig.cpp
libpreroute_la_LIBADD=-L/usr/local/mioji/lib -lboost_regex -lglog -lmjmysql -lpthread -ljson -lmjcommon -lhiredis -L/usr/lib64/mysql -lmysqlclient ../common/libcommon.la 
libpreroute_la_LDFLAGS=-fPIC

bin_PROGRAMS+=PreRoute
PreRoute_SOURCES=testView.cpp
PreRoute_LDADD=-lpreroute libpreroute.la

AM_CPPFLAGS+=-DSS_DOCID_BITS=256
AM_CPPFLAGS+=-std=c++0x
