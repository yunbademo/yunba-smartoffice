#APPKEY="56a0a88c4407a3cd028ac2fe"
#SECKEY="sec-6PHAeft01lIQbpcHvSte64Nnc3WnMOMfzrNUqp3qNcCXlHwp"

APPKEY="5697113d4407a3cd028abead"
SECKEY="sec-AMmcqDDkGDBWYsg55j0Gd5mc7Q1gWrQIz0bpC6b3Uaw9VyG1"

data="{\"method\":\"publish_to_alias\", \"appkey\":\"${APPKEY}\", \"seckey\":\"${SECKEY}\", \"alias\":\"${1}\", \"msg\":\"${2}\"}"

echo ${data}
curl -l -H "Content-type: application/json" -X POST -d "${data}" http://rest.yunba.io:8080

echo

