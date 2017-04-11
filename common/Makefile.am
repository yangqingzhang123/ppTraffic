include $(top_srcdir)/common.mk

INCLUDES += -I/usr/include/openssl -I/usr/local/mioji/include

bin_PROGRAMS=
lib_LIBRARIES=
lib_LTLIBRARIES=

lib_LTLIBRARIES+=libcommon.la
libcommon_la_SOURCES=MyConfig.cpp MyThreadPool.cpp threadpool.h MyCommon.cpp
libcommon_la_LIBADD=-L/usr/local/mioji/lib -lssl -lcrypto -lhiredis -lpthread -L/usr/lib64/mysql -lmysqlclient -ljson
libcommon_la_LDFLAGS=-fPIC

AM_CPPFLAGS+=-DSS_DOCID_BITS=256 -D__STDC_FORMAT_MACROS
AM_CPPFLAGS+=-std=c++0x

