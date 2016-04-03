if [ $# -lt 2 ] ; then
  echo usage: `basename ${0}` '<ALIAS> <on_off|mod|fan|inc|dec>'
  echo e.g.: `basename ${0}` '"temp_ctrl_0"' '"mod"'
  exit 1
fi

alias="${1}"
cmd="${2}"

msg="{\\\"cmd\\\":\\\"${cmd}\\\",\\\"devid\\\":\\\"${alias}\\\"}"

#echo "${msg}"
./publish.sh "${alias}" "${msg}"

