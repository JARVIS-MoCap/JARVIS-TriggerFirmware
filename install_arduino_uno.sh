export PLATFORMIO_CORE_DIR="PlatformIO/install"


PIO_DIR="PlatformIO/install/"
if ! [ -d "$PIO_DIR" ]; then
  python3 PlatformIO/platformio-core-installer/get-platformio.py
fi

./PlatformIO/install/penv/bin/platformio run -e uno
./PlatformIO/install/penv/bin/platformio run -e uno -t upload
