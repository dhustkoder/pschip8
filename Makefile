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


.PHONY all: clean main iso
.PHONY clean:
	@del *.MAP *.SYM *.CPE *.IMG *.TOC *.EXE *.CNF *.CTI


main: MAIN.EXE
asm: $(patsubst src/%.c, asm/%.asm, $(wildcard src/*.c))
iso: $(PROJNAME).ISO

MAIN.EXE: MAIN.CPE
	cpe2x $(CPE2X_FLAGS) MAIN.CPE

MAIN.CPE:
	ccpsx $(CFLAGS) $(LIBS) src/*.c -oMAIN.CPE,MAIN.SYM,MAIN.MAP

asm/%.asm: src/%.c
	ccpsx $(CFLAGS) -S $< -o$@

$(PROJNAME).ISO: $(PROJNAME).IMG
	stripiso s 2352 $(PROJNAME).IMG $(PROJNAME).ISO
	psxlicense $(PSXLICENSE_FLAGS) /i $(PROJNAME).ISO

$(PROJNAME).IMG: $(PROJNAME).CTI
	buildcd -l -i$(PROJNAME).IMG $(PROJNAME).CTI

$(PROJNAME).CTI: $(PROJNAME).CNF
	@del *.CTI
	echo Define ProjectPath $(PROJ_DIR)\ >> $(PROJNAME).CTI
	echo Define LicenseFile $(LICENSEFILE) >> $(PROJNAME).CTI
	echo Disc CDROMXA_PSX ;the disk format >> $(PROJNAME).CTI
	echo 	CatalogNumber 0000000000000 >> $(PROJNAME).CTI
	echo 	LeadIn XA ;lead in track (track 0) >> $(PROJNAME).CTI
	echo 		Empty 300 ;defines the lead in size (min 150) >> $(PROJNAME).CTI
	echo 		PostGap 150 ;required gap at end of the lead in >> $(PROJNAME).CTI
	echo 	EndTrack ;end of the lead in track >> $(PROJNAME).CTI
	echo 	Track XA ;start of the XA (data) track >> $(PROJNAME).CTI
	echo 		Pause 150 ;required pause in first track after the lead in >> $(PROJNAME).CTI
	echo 		Volume ISO9660 ;define ISO 9660 volume >> $(PROJNAME).CTI
	echo 			SystemArea [LicenseFile] >> $(PROJNAME).CTI
	echo 			PrimaryVolume ;start point of primary volume >> $(PROJNAME).CTI
	echo 				SystemIdentifier "PLAYSTATION" ;required indetifier (do not change) >> $(PROJNAME).CTI
	echo 				ApplicationIdentifier "PLAYSTATION" >> $(PROJNAME).CTI
	echo 				VolumeIdentifier "$(PROJNAME)" ;app specific identifiers (changeable) >> $(PROJNAME).CTI
	echo 				VolumeSetIdentifier "$(PROJNAME)" >> $(PROJNAME).CTI
	echo 				PublisherIdentifier "$(PUBLISHER)" >> $(PROJNAME).CTI
	echo 				DataPreparerIdentifier "$(DEVELOPER)" >> $(PROJNAME).CTI
	echo 				LPath ;path tables as specified for PlayStation >> $(PROJNAME).CTI
	echo 				OptionalLpath >> $(PROJNAME).CTI
	echo 				MPath >> $(PROJNAME).CTI
	echo 				OptionalMpath >> $(PROJNAME).CTI
	echo 				Hierarchy ;start point of root directory definition >> $(PROJNAME).CTI
	echo 					XAFileAttributes  Form1 Audio >> $(PROJNAME).CTI
	echo 					XAVideoAttributes ApplicationSpecific >> $(PROJNAME).CTI
	echo 					XAAudioAttributes ADPCM_C Stereo ;you can also add 'Emphasis_On' before Stereo >> $(PROJNAME).CTI
	echo 					File SYSTEM.CNF >> $(PROJNAME).CTI
	echo 						XAFileAttributes Form1 Data ;set the attribute to form 1 >> $(PROJNAME).CTI
	echo 						Source [ProjectPath]SYSTEM.CNF ;where the file above can be found >> $(PROJNAME).CTI
	echo 					EndFile >> $(PROJNAME).CTI
	echo 					File MAIN.EXE >> $(PROJNAME).CTI
	echo 						XAFileAttributes Form1 Data >> $(PROJNAME).CTI
	echo 						Source [ProjectPath]MAIN.EXE >> $(PROJNAME).CTI
	echo 					EndFile >> $(PROJNAME).CTI
	echo					File BRIX.CH8 >> $(PROJNAME).CTI
	echo						XAFileAttributes Form1 Data >> $(PROJNAME).CTI
	echo						Source [ProjectPath]data\BRIX.CH8 >> $(PROJNAME).CTI
	echo					EndFile >> $(PROJNAME).CTI
	echo					File BLINKY.CH8 >> $(PROJNAME).CTI
	echo						XAFileAttributes Form1 Data >> $(PROJNAME).CTI
	echo						Source [ProjectPath]data\BLINKY.CH8 >> $(PROJNAME).CTI
	echo					EndFile >> $(PROJNAME).CTI
	echo					File BKG.TIM >> $(PROJNAME).CTI
	echo						XAFileAttributes Form1 Data >> $(PROJNAME).CTI
	echo						Source [ProjectPath]data\BKG.TIM >> $(PROJNAME).CTI
	echo					EndFile >> $(PROJNAME).CTI
	echo				EndHierarchy ;ends the root directory definition >> $(PROJNAME).CTI
	echo 			EndPrimaryVolume ;ends the primary volume definition >> $(PROJNAME).CTI
	echo 		EndVolume ;ends the ISO 9660 definition >> $(PROJNAME).CTI
	echo 		Empty 300 >> $(PROJNAME).CTI
	echo 		PostGap 150 ;required to change the track type >> $(PROJNAME).CTI
	echo 	EndTrack ;ends the track definition (end of the XA track) >> $(PROJNAME).CTI
	echo 	LeadOut XA ;note that the leadout track must be the same data type as the last track (IE: AUDIO, XA or MODE1) >> $(PROJNAME).CTI
	echo 	Empty 150 >> $(PROJNAME).CTI
	echo 	EndTrack >> $(PROJNAME).CTI
	echo EndDisc >> $(PROJNAME).CTI

$(PROJNAME).CNF:
	@del *.CNF
	echo BOOT=cdrom:\MAIN.EXE;1 >> SYSTEM.CNF
	echo TCB=4 >> SYSTEM.CNF
	echo EVENT=10 >> SYSTEM.CNF
	echo STACK=801FFFF0 >> SYSTEM.CNF


