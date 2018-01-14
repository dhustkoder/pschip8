# Before using the Makefile you should download and install GnuMake and PSYQ SDK
# (you can download the PSYQ SDK from www.psxdev.net).
# Then set the PSYQ_DIR in setupenv.bat and in this Makefile and run the setupenv.bat
# now you can use make to build the project

# This Makefile should run on Windows XP (or less).
# This Makefile can also run on Linux with wine and dosbox installed.


PROJNAME=PSCHIP8
DEVELOPER=Dhust
PUBLISHER=DEVine

# Set this to the PSYQ directory
PSYQ_DIR=C:\PSYQ

# Set this to the project's directory
PROJ_DIR=C:\PSYQ\PROJECTS\$(PROJNAME)

# Set to PAL, NTSC_U, NTSC_J
DISPLAY_TYPE=NTSC_U

# Set to Release or Debug
BUILD_TYPE=Release


DATA_FILES=$(patsubst data/%, %, $(wildcard data/*))

CFLAGS=-Wall -Xo$$80010000 -DDISPLAY_TYPE_$(DISPLAY_TYPE)
CFLAGS_DEBUG=-O1 -G2 -DDEBUG
CFLAGS_RELEASE=-O2 -G0 -mgpopt -DNDEBUG
LIBS=


ifeq ($(DISPLAY_TYPE), PAL)
	LICENSEFILE=$(PSYQ_DIR)\CDGEN\LCNSFILE\LICENSEE.DAT
	PSXLICENSE_FLAGS=/eu
else ifeq ($(DISPLAY_TYPE), NTSC_U)
	LICENSEFILE=$(PSYQ_DIR)\CDGEN\LCNSFILE\LICENSEA.DAT
	PSXLICENSE_FLAGS=/us
else
	LICENSEFILE=$(PSYQ_DIR)\CDGEN\LCNSFILE\LICENSEJ.DAT
	PSXLICENSE_FLAGS=/jp
endif

ifeq ($(DISPLAY_TYPE),PAL)
	CPE2X_FLAGS=/ce
else
	CPE2X_FLAGS=/ca
endif

ifeq ($(BUILD_TYPE),Release)
        CFLAGS+=$(CFLAGS_RELEASE)
else
        CFLAGS+=$(CFLAGS_DEBUG)
endif


all: cdrom/$(DISPLAY_TYPE).ISO
main: $(DISPLAY_TYPE).EXE
asm: $(patsubst src/%.c, asm/%.asm, $(wildcard src/*.c))

%.asm: %.c
	ccpsx $(CFLAGS) -S $^ -o$@


%.ISO: $(DISPLAY_TYPE).IMG
	stripiso s 2352 $< $@

%.IMG: $(DISPLAY_TYPE).CTI
	buildcd -l -i$@ $<

%.CTI: $(DISPLAY_TYPE).CNF
	@echo Define ProjectPath $(PROJ_DIR)\ > $@
	@echo Define LicenseFile $(LICENSEFILE) >> $@
	@echo Disc CDROMXA_PSX ;the disk format >> $@
	@echo 	CatalogNumber 0000000000000 >> $@
	@echo 	LeadIn XA ;lead in track (track 0) >> $@
	@echo 		Empty 300 ;defines the lead in size (min 150) >> $@
	@echo 		PostGap 150 ;required gap at end of the lead in >> $@
	@echo 	EndTrack ;end of the lead in track >> $@
	@echo 	Track XA ;start of the XA (data) track >> $@
	@echo 		Pause 150 ;required pause in first track after the lead in >> $@
	@echo 		Volume ISO9660 ;define ISO 9660 volume >> $@
	@echo 			SystemArea [LicenseFile] >> $@
	@echo 			PrimaryVolume ;start point of primary volume >> $@
	@echo 				SystemIdentifier "PLAYSTATION" ;required indetifier (do not change) >> $@
	@echo 				ApplicationIdentifier "PLAYSTATION" >> $@
	@echo 				VolumeIdentifier "$(PROJNAME)" ;app specific identifiers (changeable) >> $@
	@echo 				VolumeSetIdentifier "$(PROJNAME)" >> $@
	@echo 				PublisherIdentifier "$(PUBLISHER)" >> $@
	@echo 				DataPreparerIdentifier "$(DEVELOPER)" >> $@
	@echo 				LPath ;path tables as specified for PlayStation >> $@
	@echo 				OptionalLpath >> $@
	@echo 				MPath >> $@
	@echo 				OptionalMpath >> $@
	@echo 				Hierarchy ;start point of root directory definition >> $@
	@echo 					XAFileAttributes  Form1 Audio >> $@
	@echo 					XAVideoAttributes ApplicationSpecific >> $@
	@echo 					XAAudioAttributes ADPCM_C Stereo ;you can also add 'Emphasis_On' before Stereo >> $@
	@echo 					File SYSTEM.CNF >> $@
	@echo 						XAFileAttributes Form1 Data ;set the attribute to form 1 >> $@
	@echo 						Source [ProjectPath]$(DISPLAY_TYPE).CNF ;where the file above can be found >> $@
	@echo 					EndFile >> $@
	@echo 					File MAIN.EXE >> $@
	@echo 						XAFileAttributes Form1 Data >> $@
	@echo 						Source [ProjectPath]$(DISPLAY_TYPE).EXE >> $@
	@echo 					EndFile >> $@

	$(foreach i, $(DATA_FILES), \
	@echo 					File $(i) >> $@ & \
	@echo 						XAFileAttributes Form1 Data >> $@ & \
	@echo 						Source [ProjectPath]data\$(i) >> $@ & \
	@echo 					EndFile >> $@ &)

	@echo 				EndHierarchy ;ends the root directory definition >> $@
	@echo 			EndPrimaryVolume ;ends the primary volume definition >> $@
	@echo 		EndVolume ;ends the ISO 9660 definition >> $@
	@echo 		Empty 300 >> $@
	@echo 		PostGap 150 ;required to change the track type >> $@
	@echo 	EndTrack ;ends the track definition (end of the XA track) >> $@
	@echo 	LeadOut XA ;note that the leadout track must be the same data type as the last track (IE: AUDIO, XA or MODE1) >> $@
	@echo 	Empty 150 >> $@
	@echo 	EndTrack >> $@
	@echo EndDisc >> $@

%.CNF: $(DISPLAY_TYPE).EXE
	@echo BOOT=cdrom:\MAIN.EXE;1 > $@
	@echo TCB=4 >> $@
	@echo EVENT=10 >> $@
	@echo STACK=801FFFF0 >> $@

%.EXE: %.CPE
	@copy $< MAIN.CPE
	cpe2x $(CPE2X_FLAGS) MAIN.CPE
	@del MAIN.CPE
	@copy MAIN.EXE $@
	@del MAIN.EXE

%.CPE: src/*.c src/*.h
	ccpsx $(CFLAGS) $(LIBS) src/*.c -o$@,$(patsubst %.CPE,%.MAP,$@),$(patsubst %.CPE,%.SYM,$@)

.PHONY clean:
	@del *.MAP *.SYM *.CPE *.IMG *.TOC *.EXE *.CNF *.CTI
