if [ $# != 1 ] ; then
  echo usage: `basename ${0}` '<ALIAS>'
  exit 1
fi

alias="${1}"
msg="{\\\"cmd\\\": \\\"plug_set\\\", \\\"devid\\\": \\\"${alias}\\\", status\\\": 0}"

#echo "${msg}"
./publish.sh "${alias}" "${msg}"

