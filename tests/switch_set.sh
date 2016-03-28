if [ $# -lt 2 ] ; then
  echo usage: `basename ${0}` 'ALIAS STATUS'
  echo e.g.: `basename ${0}` '"switch_0"' '"[1, 1, 0]"'
  exit 1
fi

alias="${1}"
status="${2}"

msg="{\\\"cmd\\\":\\\"switch_set\\\",\\\"devid\\\":\\\"${alias}\\\",\\\"status\\\":${status}}"

#echo "${msg}"
./publish.sh "${alias}" "${msg}"

