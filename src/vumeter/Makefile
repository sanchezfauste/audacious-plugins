PLUGIN = vumeter${PLUGIN_SUFFIX}

SRCS = vumeter.cc truepeakdsp.cc zita-resampler/resampler-table.cc zita-resampler/resampler.cc

include ../../buildsys.mk
include ../../extra.mk

plugindir := ${plugindir}/${VISUALIZATION_PLUGIN_DIR}

LD = ${CXX}
CFLAGS += ${PLUGIN_CFLAGS}
CPPFLAGS += ${PLUGIN_CPPFLAGS} -I../.. ${GTK_CFLAGS}
LIBS += -lm ${GTK_LIBS}
