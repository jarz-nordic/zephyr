west build -b nrf52832_strzelnica --pristine -- -DOVERLAY_CONFIG=overlay-bt-tiny.conf
west sign -t imgtool -- --key ~/zephyrproject/bootloader/mcuboot/root-rsa-2048.pem
