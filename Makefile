#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := SerialMoose

EXTRA_COMPONENT_DIRS = $(IDF_PATH)/examples/system/console/advanced/components

include $(IDF_PATH)/make/project.mk
