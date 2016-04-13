if [ $# -lt 2 ] ; then
  echo usage: `basename ${0}` '<ALIAS> <on_off|mode|fan|inc|dec>'
  echo e.g.: `basename ${0}` '"temp_ctrl_0"' '"mode"'
  exit 1
fi

alias="${1}"
cmd="${2}"

msg="{\\\"cmd\\\":\\\"${cmd}\\\",\\\"devid\\\":\\\"${alias}\\\"}"

#echo "${msg}"
./publish.sh "${alias}" "${msg}"

