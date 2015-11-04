all: gateway door database motion keychain securitysystem

gateway: gateway.c
	gcc gateway.c -pthread -o gateway

door: door.c
	gcc door.c -o door

database: database.c
	gcc database.c -pthread -o database

motion: motion.c
	gcc motion.c -pthread -o motion

keychain: keychain.c
	gcc keychain.c -pthread -o keychain

securitysystem: securitysystem.c -pthread -o securitysystem
