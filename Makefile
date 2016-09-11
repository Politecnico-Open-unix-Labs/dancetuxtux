# Directories
SRC_DIR=src
INCLUDE_DIR=include
BUILD_DIR=build

# Commands
CC=avr-gcc
OBJCOPY=avr-objcopy

# Settings
LANG_STD=gnu11
O_LEVEL=2 # -O3 may break something
CC_FLAGS=-mmcu=atmega32u4 -flto -ffast-math -Wall -Werror -Wfatal-errors
LD_FLAGS=-mmcu=atmega32u4 -fuse-linker-plugin -flto -fwhole-program -Wl,-emain
PORT=/dev/ttyACM0 # Port to use if no other port was found
OUT_NAME=dancetuxtux# Output names

all: ${BUILD_DIR}/main.o ${BUILD_DIR}/capacitive.o ${BUILD_DIR}/capacitive_lowlevel.o ${BUILD_DIR}/pin_utils.o \
        ${BUILD_DIR}/timer_utils.o ${BUILD_DIR}/circular_buffer.o ${BUILD_DIR}/USB.o ${BUILD_DIR}
	@ # Links all object files
	${CC} ${LD_FLAGS} ${BUILD_DIR}'/timer_utils.o' ${BUILD_DIR}'/pin_utils.o' ${BUILD_DIR}'/capacitive.o' ${BUILD_DIR}'/circular_buffer.o' \
	    -s ${BUILD_DIR}'/capacitive_lowlevel.o' ${BUILD_DIR}'/USBCore.o' ${BUILD_DIR}'/main.o' \
	    -O${O_LEVEL} -o ${BUILD_DIR}'/'${OUT_NAME}'.elf'
	@ # NB: the executable is stripped to reduce size. You may remove -s option if ypu want to disable stripping
	${OBJCOPY} -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 \
							   ${BUILD_DIR}'/'${OUT_NAME}'.elf' ${BUILD_DIR}'/'${OUT_NAME}'.eep'
	${OBJCOPY} -O ihex -R .eeprom ${BUILD_DIR}'/'${OUT_NAME}'.elf' ${BUILD_DIR}'/'${OUT_NAME}'.hex'  # converts binary to readable text, containing HEX
	chmod 644 ${BUILD_DIR}'/'${OUT_NAME}'.elf' # compiler generates executable, we don't want output is executable on our computer
	chmod 755 ${BUILD_DIR}'/'${OUT_NAME}'.hex' # This is not necessary, but highlights what to upload on the board
	@ printf "         \033[32;1m.--------------------.\033[0m\n"
	@ printf "         \033[32;1m|   BUILD SUCCESS!   |\033[0m\n"
	@ printf "         \033[32;1m'--------------------'\033[0m\n\n"

${BUILD_DIR}/main.o: ${SRC_DIR}/main.c ${BUILD_DIR}
	@ # Main program
	${CC} -c -std=${LANG_STD} -O${O_LEVEL} ${CC_FLAGS} -I${INCLUDE_DIR}'/' ${SRC_DIR}'/main.c' -o ${BUILD_DIR}'/main.o'

${BUILD_DIR}/capacitive_lowlevel.o: ${SRC_DIR}/capacitive_lowlevel.c ${BUILD_DIR}
	${CC} -c -std=${LANG_STD} -O${O_LEVEL} ${CC_FLAGS} -I${INCLUDE_DIR}'/' ${SRC_DIR}'/capacitive_lowlevel.c' -o ${BUILD_DIR}'/capacitive_lowlevel.o'

${BUILD_DIR}/capacitive.o: ${SRC_DIR}/capacitive.c ${BUILD_DIR}
	${CC} -c -std=${LANG_STD} -O${O_LEVEL} ${CC_FLAGS} -I${INCLUDE_DIR}'/' ${SRC_DIR}'/capacitive.c' -o ${BUILD_DIR}'/capacitive.o'

${BUILD_DIR}/pin_utils.o: ${SRC_DIR}/pin_utils.c ${BUILD_DIR}
	${CC} -c -std=${LANG_STD} -O${O_LEVEL} ${CC_FLAGS} -I${INCLUDE_DIR}'/' ${SRC_DIR}'/pin_utils.c' -o ${BUILD_DIR}'/pin_utils.o'

${BUILD_DIR}/timer_utils.o: ${SRC_DIR}/timer_utils.c ${BUILD_DIR}
	${CC} -c -std=${LANG_STD} -O${O_LEVEL} ${CC_FLAGS} -I${INCLUDE_DIR}'/' ${SRC_DIR}'/timer_utils.c' -o ${BUILD_DIR}'/timer_utils.o'

${BUILD_DIR}/circular_buffer.o: ${SRC_DIR}/circular_buffer.c ${BUILD_DIR}
	${CC} -c -std=${LANG_STD} -O${O_LEVEL} ${CC_FLAGS} -I${INCLUDE_DIR}'/' ${SRC_DIR}'/circular_buffer.c' -o ${BUILD_DIR}'/circular_buffer.o'

${BUILD_DIR}/USB.o: ${SRC_DIR}/USBCore.c ${BUILD_DIR}
	${CC} -c -std=${LANG_STD} -O${O_LEVEL} ${CC_FLAGS} -I${INCLUDE_DIR}'/' ${SRC_DIR}'/USBCore.c' -o ${BUILD_DIR}'/USBCore.o'

.PHONY: upload install
install: upload
upload: all
	@printf "     \033[33;1m.------------------------------------------------.\033[0m\\n"
	@printf "     \033[33;1m|      Press the \033[31;1mRESET BUTTON\033[33;1m on your board      |\033[0m\\n"
	@printf "     \033[33;1m|   When you've done press any key to continue   |\033[0m\\n"
	@printf "     \033[33;1m'------------------------------------------------'\033[0m\\n"
	@bash -c "read -n 1 -s" # Waits the user press any key (Warning: read is a bash built in)
	@printf "\n"
	if [ -e /dev/ttyACM1 ]; then                                            \
	    UPLOAD_PORT=/dev/ttyACM1;                                           \
	elif [ -e /dev/ttyACM0 ]; then                                          \
	    UPLOAD_PORT=/dev/ttyACM0;                                           \
	else                                                                    \
	    UPLOAD_PORT=${PORT};                                                \
	    printf "     \033[31;1mWARNING: no device detected\033[0m\\n";      \
	fi;                                                                     \
	stty -F $${UPLOAD_PORT} ispeed 1200 ospeed 1200 &&                      \
		avrdude -C/etc/avrdude.conf -patmega32u4 -cavr109 -v -v -v -v -P $${UPLOAD_PORT} -b57600 -D -Uflash:w:${BUILD_DIR}'/'${OUT_NAME}'.hex':i
	@ printf "         \033[32;1m.---------------------.\033[0m\n"
	@ printf "         \033[32;1m|   UPLOAD SUCCESS!   |\033[0m\n"
	@ printf "         \033[32;1m'---------------------'\033[0m\n\n"

${BUILD_DIR}:
	@ # If exists build but it is not a directory removes it
	if [ -e ${BUILD_DIR} ] && ! [ -d ${BUILD_DIR} ]; then \
	    rm ${BUILD_DIR};                                  \
	fi

	@ # Now if build does not exists creates one
	if ! [ -e ${BUILD_DIR} ]; then \
	    mkdir ${BUILD_DIR};        \
	fi

.PHONY: clean
clean:
	rm -r ${BUILD_DIR}
