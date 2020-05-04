#ARDUINO_DIR=~/arduino-1.8.9/
#ARDUINO_LIB_DIR=${ARDUINO_DIR}libraries/
#ARDUINO_EXEC=${ARDUINO_DIR}arduino

ARDUINO_LIB_DIR=~/Documents/Arduino/libraries/
#ARDUINO_LIB_DIR=~/Documents/tmp_test/

declare -a arr=(
    "git@github.com:pikot/MLX90615.git"
    "git@github.com:pikot/EmotiBit_BMI160.git"
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
