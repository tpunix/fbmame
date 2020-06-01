###########################################################################
#
#   tiny.mak
#
#   Small driver-specific example makefile
#   Use make SUBTARGET=tiny to build
#
#   Copyright Nicola Salmoria and the MAME Team.
#   Visit  http://mamedev.org for licensing and usage restrictions.
#
###########################################################################

MAMESRC = $(SRC)/mame
MAMEOBJ = $(OBJ)/mame

AUDIO = $(MAMEOBJ)/audio
DRIVERS = $(MAMEOBJ)/drivers
LAYOUT = $(MAMEOBJ)/layout
MACHINE = $(MAMEOBJ)/machine
VIDEO = $(MAMEOBJ)/video

OBJDIRS += $(AUDIO) $(DRIVERS) $(LAYOUT) $(MACHINE) $(VIDEO)



#-------------------------------------------------
# Specify all the CPU cores necessary for the
# drivers referenced in tiny.c.
#-------------------------------------------------

CPUS += Z80
CPUS += MIPS
CPUS += RSP
CPUS += V60
CPUS += M680X0
CPUS += SH2
CPUS += V810
CPUS += UPD7725
CPUS +=



#-------------------------------------------------
# Specify all the sound cores necessary for the
# drivers referenced in tiny.c.
#-------------------------------------------------

SOUNDS += DMADAC
SOUNDS += YMF271
SOUNDS += YMZ280B
SOUNDS += OKIM6295
SOUNDS += AY8910
SOUNDS += MSM5205
SOUNDS += YM2413
SOUNDS += YM2610
SOUNDS += YM2203
SOUNDS += YM2608
SOUNDS += YMF278B
SOUNDS += ST0016
SOUNDS += NILE
SOUNDS += ES5506


#-------------------------------------------------
# specify available video cores
#-------------------------------------------------

#-------------------------------------------------
# specify available machine cores
#-------------------------------------------------

MACHINES += EEPROMDEV
MACHINES += JVS
MACHINES +=

#-------------------------------------------------
# specify available bus cores
#-------------------------------------------------
BUSES += 


#-------------------------------------------------
# This is the list of files that are necessary
# for building all of the drivers referenced
# in tiny.c
#-------------------------------------------------


DRVLIBS += $(DRIVERS)/aleck64.o $(MACHINE)/n64.o $(VIDEO)/n64.o $(VIDEO)/rdpblend.o $(VIDEO)/rdpspn16.o $(VIDEO)/rdptpipe.o

DRVLIBS += $(DRIVERS)/bnstars.o $(DRIVERS)/ms32.o $(VIDEO)/ms32.o $(MACHINE)/jalcrpt.o
DRVLIBS += $(DRIVERS)/jalmah.o
DRVLIBS += $(DRIVERS)/fromance.o $(VIDEO)/fromance.o $(VIDEO)/vsystem_spr2.o
DRVLIBS += $(DRIVERS)/fromanc2.o $(VIDEO)/fromanc2.o
DRVLIBS += $(DRIVERS)/psikyo4.o $(VIDEO)/psikyo4.o

DRVLIBS += $(DRIVERS)/srmp2.o $(VIDEO)/srmp2.o $(VIDEO)/seta001.o
DRVLIBS += $(DRIVERS)/srmp5.o $(DRIVERS)/srmp6.o $(MACHINE)/st0016.o
DRVLIBS += $(DRIVERS)/ssv.o $(VIDEO)/ssv.o $(VIDEO)/st0020.o

# naomi
#CPUS += SH4
#CPUS += ARM7
#SOUNDS += AICA
#MACHINES += AICARTC
#MACHINES += X76F100
#MACHINES += INTELFLASH
#DRVLIBS += $(DRIVERS)/naomi.o $(MACHINE)/dc.o $(VIDEO)/powervr2.o
#DRVLIBS += $(MACHINE)/naomi.o $(MACHINE)/naomim1.o $(MACHINE)/naomim2.o $(MACHINE)/naomim4.o
#DRVLIBS += $(MACHINE)/naomig1.o $(MACHINE)/naomibd.o $(MACHINE)/naomirom.o $(MACHINE)/naomigd.o
#DRVLIBS += $(MACHINE)/mie.o $(MACHINE)/maple-dc.o $(MACHINE)/mapledev.o $(MACHINE)/dc-ctrl.o $(MACHINE)/jvs13551.o
#DRVLIBS += $(MACHINE)/315-5881_crypt.o $(MACHINE)/awboard.o


#-------------------------------------------------
# layout dependencies
#-------------------------------------------------






