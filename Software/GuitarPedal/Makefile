# Project Name
TARGET =  guitarpedal

CPP_STANDARD = -std=gnu++20

# Library Locations
DAISYSP_DIR ?= dependencies/DaisySP
LIBDAISY_DIR ?= dependencies/libDaisy
USE_DAISYSP_LGPL=1

# Sources
CMSIS_DSP_DIR = ${LIBDAISY_DIR}/Drivers/CMSIS-DSP

CMSIS_DSP_SRC = \
$(CMSIS_DSP_DIR)/Source/FilteringFunctions/arm_fir_f32.c \
$(CMSIS_DSP_DIR)/Source/FilteringFunctions/arm_fir_init_f32.c \
$(CMSIS_DSP_DIR)/Source/SupportFunctions/arm_fill_f32.c \
$(CMSIS_DSP_DIR)/Source/BasicMathFunctions/arm_scale_f32.c
C_DEFS += -DARM_MATH_LOOPUNROLL

CPP_SOURCES = guitar_pedal.cpp guitar_pedal_storage.cpp $(wildcard UI/*.cpp) $(wildcard Util/*.cpp) \
$(wildcard Hardware-Modules/*.cpp)
CPP_SOURCES += Effect-Modules/base_effect_module.cpp
C_SOURCES += $(CMSIS_DSP_SRC)

# Naming the module sources explicitly to make it easier to add/remove from build. Otherwise all global attributes
#  will be included in the build and take up memory resources, even when the .h file is commented out. 

CPP_SOURCES += Effect-Modules/tape_deck_module.cpp
#CPP_SOURCES += Effect-Modules/reverb_delay_module.cpp
#CPP_SOURCES += Effect-Modules/polyoctave_module.cpp


# Keyboard oriented modules
#CPP_SOURCES += Effect-Modules/fm_keys_module.cpp
#CPP_SOURCES += Effect-Modules/midi_keys_module.cpp
#CPP_SOURCES += Effect-Modules/modal_keys_module.cpp
#CPP_SOURCES += Effect-Modules/pluckecho_module.cpp
#CPP_SOURCES += Effect-Modules/string_keys_module.cpp

# Compiler options
OPT=-Ofast
# OPT=-Og -g

# Boot options
APP_TYPE = BOOT_SRAM

# Place the large NAM A2 history buffer in AXI-SRAM instead of DTCMRAM.
# nam_a2_sections.lds is an INSERT fragment that augments libDaisy's main linker
# script (selected below via APP_TYPE). The ld manual requires the INSERT script
# to appear FIRST on the command line, and libDaisy's core Makefile does
# `LDFLAGS += ... -T$(LDSCRIPT)`, so adding ours here (before the include) places
# it ahead of the main script. See nam_a2_sections.lds and nam_a2_runtime.h.
LDFLAGS += -T$(abspath nam_a2_sections.lds)


# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# Included as system instead of regular includes to avoid warnings from the
# 3rd party dependencies
C_INCLUDES += -isystem ./dependencies/q/q/q_lib/include
C_INCLUDES += -isystem ./dependencies/q/infra/include
C_INCLUDES += -isystem ./dependencies/gcem/include

C_INCLUDES += -isystem ./dependencies/CloudSeed
LIBS += -lcloudseed
LIBDIR += -Ldependencies/CloudSeed/build

# Add CMSIS-DSP include path
C_INCLUDES += -I$(CMSIS_DSP_DIR)/Include

# Parse variants from argument to make, if none is supplied, will build for 125B
# options:
# make -j8 VARIANT=125B
# make -j8 VARIANT=1590B
# make -j8 VARIANT=1590B_SMD
# make -j8 VARIANT=TERRARIUM
# make -j8 VARIANT=FUNBOX
ifdef VARIANT
   CFLAGS += -DVARIANT_$(VARIANT)
   $(info VARIANT=$(VARIANT))
endif


# --- ARM SDK Version check --- [start]
# Issues have been experienced with the latest version of arm-none-eabi-gcc, so
# we add a check here to help guide users to use a supported version

# Get compiler version string safely
GCC_VERSION := $(shell arm-none-eabi-gcc -dumpversion)
GCC_VERSION_MAJOR := $(word 1, $(subst ., ,$(GCC_VERSION)))
GCC_VERSION_MAJOR_REQUIRED := 15

ifneq ($(GCC_VERSION_MAJOR),$(GCC_VERSION_MAJOR_REQUIRED))
$(error Compiler version of arm-none-eabi-gcc: $(GCC_VERSION) is not supported. Use version $(GCC_VERSION_MAJOR_REQUIRED).x.x)
endif
# --- ARM SDK Version check --- [end]