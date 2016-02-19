while [ "1" = "1" ]
do
	./arduino_plug_off.sh "plc_0"
	sleep 10
	./arduino_plug_on.sh "plc_0"
	sleep 10
done
