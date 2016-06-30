# Directories
SRC_DIR=src
INCLUDE_DIR=include
BUILD_DIR=build

# Commands
CC=avr-g++
OBJCOPY=avr-objcopy

# Settings
CPP_STD=c++11
O_LEVEL=2# -O3 may break something
PORT=/dev/ttyACM0# Port to use if no other port was found
OUT_NAME=dancetuxtux# Output names

all: main.o capacitive.o build_dir
	@ # Links all object files
	${CC} -mmcu=atmega32u4 -fPIC -flto ${BUILD_DIR}'/capacitive.o' ${BUILD_DIR}'/main.o' -Wl,-emain -O${O_LEVEL} -o ${BUILD_DIR}'/'${OUT_NAME}'.elf'
	${OBJCOPY} -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 \
							   ${BUILD_DIR}'/'${OUT_NAME}'.elf' ${BUILD_DIR}'/'${OUT_NAME}'.eep' 
	${OBJCOPY} -O ihex -R .eeprom ${BUILD_DIR}'/'${OUT_NAME}'.elf' ${BUILD_DIR}'/'${OUT_NAME}'.hex'  # converts binary to readable text, containing HEX
	chmod 644 ${BUILD_DIR}'/'${OUT_NAME}'.elf' # compiler generates executable, we don't want output is executable on our computer
	chmod 755 ${BUILD_DIR}'/'${OUT_NAME}'.hex' # This is not necessary, but highlights what to upload on the board

main.o: ${SRC_DIR}/main.cpp build_dir
	@ # Main program
	${CC} -c -std=${CPP_STD} -O${O_LEVEL} -mmcu=atmega32u4 -flto -Wall -Werror -Wfatal-errors -I${INCLUDE_DIR}'/' \
	    ${SRC_DIR}'/main.cpp' -o ${BUILD_DIR}'/main.o'

capacitive.o: ${SRC_DIR}/capacitive.cpp build_dir	
	${CC} -c -std=${CPP_STD} -O${O_LEVEL} -mmcu=atmega32u4 -flto -Wall -Werror -Wfatal-errors -I${INCLUDE_DIR}'/' \
	    ${SRC_DIR}'/capacitive.cpp' -o ${BUILD_DIR}'/capacitive.o'

install: upload
upload: all
	@printf "==================================================\n"
	@printf "====== Press the RESET BUTTON on your board ======\n"
	@printf "====== When you've done press any key to continue ... "
	@bash -c "read -n 1 -s" # Waits the user press any key (Warning: read is a bash built in)
	@printf "\n"	
	if [ -e /dev/ttyACM1 ]; then   \
	    UPLOAD_PORT=/dev/ttyACM1;  \
	elif [ -e /dev/ttyACM0 ]; then \
	    UPLOAD_PORT=/dev/ttyACM0;  \
	else                           \
	    UPLOAD_PORT=${PORT};       \
	fi;                            \
	stty -F $${UPLOAD_PORT} ispeed 1200 ospeed 1200 && \
		avrdude -C/etc/avrdude.conf -patmega32u4 -cavr109 -v -v -v -v -P $${UPLOAD_PORT} -b57600 -D -Uflash:w:${BUILD_DIR}'/'${OUT_NAME}'.hex':i

build_dir:
	if ! [ -e ${BUILD_DIR} ]; then \
	    mkdir ${BUILD_DIR};        \
	fi # If build directory does not exists makes one
        
clean:
	rm -r ${BUILD_DIR}
