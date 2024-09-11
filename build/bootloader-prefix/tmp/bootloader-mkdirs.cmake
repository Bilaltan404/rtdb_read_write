# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/26097970/esp/v5.0/esp-idf/components/bootloader/subproject"
  "D:/Users/26097970/Desktop/ESP_OTA/enesvs/esp32_ota_studies/esp32c3_readDataFromFirebaseRTDB/build/bootloader"
  "D:/Users/26097970/Desktop/ESP_OTA/enesvs/esp32_ota_studies/esp32c3_readDataFromFirebaseRTDB/build/bootloader-prefix"
  "D:/Users/26097970/Desktop/ESP_OTA/enesvs/esp32_ota_studies/esp32c3_readDataFromFirebaseRTDB/build/bootloader-prefix/tmp"
  "D:/Users/26097970/Desktop/ESP_OTA/enesvs/esp32_ota_studies/esp32c3_readDataFromFirebaseRTDB/build/bootloader-prefix/src/bootloader-stamp"
  "D:/Users/26097970/Desktop/ESP_OTA/enesvs/esp32_ota_studies/esp32c3_readDataFromFirebaseRTDB/build/bootloader-prefix/src"
  "D:/Users/26097970/Desktop/ESP_OTA/enesvs/esp32_ota_studies/esp32c3_readDataFromFirebaseRTDB/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/Users/26097970/Desktop/ESP_OTA/enesvs/esp32_ota_studies/esp32c3_readDataFromFirebaseRTDB/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/Users/26097970/Desktop/ESP_OTA/enesvs/esp32_ota_studies/esp32c3_readDataFromFirebaseRTDB/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
