
echo "Download new application"
batchisp -device AT32UC3A3256S -hardware usb -operation erase f memory flash addrange 0x0000 0x3ffff blankcheck loadbuffer \\PC13\workstore\janz\STICK_20\USB_MASS.elf program verify

echo "Program DFU to start application"
rem batchisp -device at32uc3a3256s -hardware usb -operation memory user addrange 0x1F8 0x1F8 fillbuffer 0x92 program
rem batchisp -device at32uc3a3256s -hardware usb -operation memory user addrange 0x1F9 0x1F9 fillbuffer 0x9E program
rem batchisp -device at32uc3a3256s -hardware usb -operation memory user addrange 0x1FA 0x1FA fillbuffer 0x0E program
rem batchisp -device at32uc3a3256s -hardware usb -operation memory user addrange 0x1FB 0x1FB fillbuffer 0x62 program
batchisp -device at32uc3a3256s -hardware usb -operation memory user addrange 0x1FC 0x1FC fillbuffer 0xE1 program
batchisp -device at32uc3a3256s -hardware usb -operation memory user addrange 0x1FD 0x1FD fillbuffer 0x1E program
batchisp -device at32uc3a3256s -hardware usb -operation memory user addrange 0x1FE 0x1FE fillbuffer 0xFD program
batchisp -device at32uc3a3256s -hardware usb -operation memory user addrange 0x1FF 0x1FF fillbuffer 0xD9 program

echo "Reset stick to start application"
batchisp -device at32uc3a3256s -hardware usb -operation start reset 0




