# this is mac version
# links for other os here -> https://github.com/duff2013/ulptool

ARDU_ULPTOOL_DIR=~/Library/Arduino15/packages/esp32/
ESP32_CORE_VER=1.0.6 
BINUTILS_DIR=esp32ulp-elf-binutils
ULPTOOL_DIR=ulptool-2.4.1

ulp_dir=ulp_tmp
rm -rf $ulp_dir

#if [ ! -d "$ulp_dir" ]
#then
	mkdir $ulp_dir
#fi    
cd $ulp_dir


#   found_dir=$?
 #  if [ "$found_dir" -eq 1 ] ; then
  #    git clone $url $new_repo
   #else
    #  echo $i .  $name " found in library"
   #fi

#esp32ulp-elf-binutils
#ulptool-2.4.1

wget https://github.com/duff2013/ulptool/archive/refs/tags/2.4.1.tar.gz
wget https://github.com/espressif/binutils-esp32ulp/releases/download/v2.28.51-esp-20191205/binutils-esp32ulp-macos-2.28.51-esp-20191205.tar.gz

tar -xvf 2.4.1.tar.gz
tar -xvf binutils-esp32ulp-macos-2.28.51-esp-20191205.tar.gz


cp ${ULPTOOL_DIR}/platform.local.txt ${ARDU_ULPTOOL_DIR}hardware/esp32/${ESP32_CORE_VER}/ 
cp -R $ULPTOOL_DIR ${ARDU_ULPTOOL_DIR}tools/ulptool
cp -R $BINUTILS_DIR ${ARDU_ULPTOOL_DIR}tools/ulptool/src/esp32ulp-elf-binutils

