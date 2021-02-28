#ARDUINO_DIR=/home/orangepi/prg/arduino-1.8.9/
ARDUINO_LIB_DIR=${ARDUINO_DIR}libraries/
ARDUINO_EXEC=${ARDUINO_DIR}arduino

#  avrispmkII
#  --useprogrammer avrispmkII
#stty -F /dev/ttyUSB0 9600

# --verbose-build

#----useprogrammer--pref programmer=arduino:avrispmkII
${ARDUINO_EXEC} --board arduino:avr:nano:cpu=atmega328old --port /dev/ttyUSB0   --verbose-upload --upload  sketch_simple_bot.ino


#arduino --upload --board arduino:avr:mega:cpu=atmega2560 --port /dev/ttyUSB1 --useprogrammer --pref programmer=arduino:buspirate <sketch>

