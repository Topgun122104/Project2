#!/bin/bash

x-terminal-emulator -e "./gateway ../SampleConfigurationFiles/SampleGatewayConfig.txt ../src/Gateway/gw.log"

x-terminal-emulator -e "./database ../SampleConfigurationFiles/SampleBackendConfig.txt ../src/Database/db.log"

x-terminal-emulator -e "./securitysystem ../SampleConfigurationFiles/SampleSecurityDeviceConfig.txt ../src/SecuritySystem/system.log"

x-terminal-emulator -e "./door ../SampleConfigurationFiles/SampleDoorConfig.txt ../SampleConfigurationFiles/SampleDoorInput.txt ../src/Door/door.log"

x-terminal-emulator -e "./keychain ../SampleConfigurationFiles/SampleKeychainConfig.txt ../SampleConfigurationFiles/SampleKeychainInput.txt ../src/KeyChain/keychain.log"

x-terminal-emulator -e "./motion ../SampleConfigurationFiles/SampleMotionConfig.txt ../SampleConfigurationFiles/SampleMotionInput.txt ../src/Motion/motion.log"
