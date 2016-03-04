if [ $# != 1 ] ; then
  echo usage: `basename ${0}` '<ALIAS>'
  exit 1
fi

alias="${1}"
msg="{\\\"cmd\\\":\\\"plug_get\\\",\\\"devid\\\":\\\"${alias}\\\"}"

#echo "${msg}"
./publish.sh "${alias}" "${msg}"

