# esp32 dht22 to Azure

## Setup

```bash
mkdir -p $HOME/esp
cd  $HOME/esp
git clone --recurse -b release/v4.2 https://github.com/espressif/esp-idf.git
cd $HOME/esp/esp-idf
./install.sh

cd <your-dev-directory>
git clone https://github.com/damienpontifex/esp32-dht22.git
cd esp32-dht22.git
source $HOME/esp/esp-idf/export.sh
idf.py build
```

Uses [CMake FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html) to get dependent components as part of build
- https://github.com/espressif/esp-azure
- https://github.com/UncleRus/esp-idf-lib.git 
