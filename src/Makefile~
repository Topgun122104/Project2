all: gateway smartDevice sensor

gateway: gateway.c
	gcc gateway.c -pthread -o gateway

device: device.c
	gcc smartDevice.c -o device

sensor: sensor.c
	gcc sensor.c -pthread -o sensor
