#ARDUINO_DIR=~/arduino-1.8.9/
#ARDUINO_LIB_DIR=${ARDUINO_DIR}libraries/
#ARDUINO_EXEC=${ARDUINO_DIR}arduino

ARDUINO_LIB_DIR=~/Documents/Arduino/libraries/
#ARDUINO_LIB_DIR=~/Documents/tmp_test/

declare -a arr=(
    "git@github.com:PaulStoffregen/Time.git"
    "git@github.com:arduino-libraries/NTPClient.git"
    "git@github.com:pikot/WiFiManager.git" # development 
    "git@github.com:bblanchon/ArduinoJson.git"
    "git@github.com:lorol/LITTLEFS.git"
    "git@github.com:olikraus/U8g2_Arduino.git" 
    "git@github.com:davetcc/TaskManagerIO.git" #1.6.4
    "git@github.com:davetcc/IoAbstraction.git" 
    "git@github.com:davetcc/tcMenuLib.git" # 1.6.1
    "git@github.com:stevemarple/AsyncDelay.git"
    "git@github.com:stevemarple/SoftWire.git"
    "git@github.com:siara-cc/esp32_arduino_sqlite3_lib.git"
    )

function create_dir_ifnotexist {
    local _dir=$1

    if [ -d "$_dir" ]
    then
       return 0
    fi
    mkdir -p "$_dir"
    return 1
}

i=0
for url in "${arr[@]}"
do
   ((i=i+1))
   echo  $i ". try to get -- $url"

   if [[ $url !=  *\.git ]]; then
     echo $url " is not git repo, skip"
     continue
   fi
   postf=.git
   fname="${url##*/}"
   name="${fname%$postf}"

   new_repo=$ARDUINO_LIB_DIR$name
   create_dir_ifnotexist "$new_repo"
   found_dir=$?
   if [ "$found_dir" -eq 1 ] ; then
      git clone $url $new_repo
   else
      echo $i .  $name " found in library"
   fi
done
