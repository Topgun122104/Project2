#!/bin/bash

x-terminal-emulator -e "./gatewayP ../SampleConfigurationFiles/SampleGatewayPriConfig.txt ../src/GatewayP/gwp.log"

x-terminal-emulator -e "./databaseP ../SampleConfigurationFiles/SampleBackendPriConfig.txt ../src/DatabaseP/dbp.log"

x-terminal-emulator -e "./securitysystem ../SampleConfigurationFiles/SampleSecurityDeviceConfig.txt ../src/SecuritySystem/system.log"

x-terminal-emulator -e "./door ../SampleConfigurationFiles/SampleDoorConfig.txt ../SampleConfigurationFiles/SampleDoorInput.txt ../src/Door/door.log"

x-terminal-emulator -e "./keychain ../SampleConfigurationFiles/SampleKeychainConfig.txt ../SampleConfigurationFiles/SampleKeychainInput.txt ../src/KeyChain/keychain.log"

x-terminal-emulator -e "./motion ../SampleConfigurationFiles/SampleMotionConfig.txt ../SampleConfigurationFiles/SampleMotionInput.txt ../src/Motion/motion.log"
