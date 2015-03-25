CONFIG := debug
debug_CFLAG := -Wall -g -DGSL_DLL
release_CFLAG := -Wall -O2 -DGSL_DLL
CONFIG_CFLAG := $($(CONFIG)_CFLAG)
CONFIG_SRC := pie_config.h
