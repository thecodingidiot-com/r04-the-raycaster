ifeq ($(strip $(PVSNESLIB_HOME)),)
$(error "Please create an environment variable PVSNESLIB_HOME by following this guide: https://github.com/alekmaul/pvsneslib/wiki/Installation")
endif

# ROMNAME and variables are used in snes_rules file
export ROMNAME := raycaster
export ROMTITLE := R04 RAYCASTER




include ${PVSNESLIB_HOME}/devkitsnes/snes_rules

#---------------------------------------------------------------------------------
.PHONY: all cleanLogs

all: buildWithSummary
buildActual: $(OBJS) $(ROMNAME).sfc

clean: cleanBuildRes cleanRom cleanGfx cleanLogs
