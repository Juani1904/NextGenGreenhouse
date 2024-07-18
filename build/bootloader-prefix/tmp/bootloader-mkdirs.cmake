# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/juani/ESP32/v5.2.2/esp-idf/components/bootloader/subproject"
  "C:/Users/juani/OneDrive/Documentos/Programacion/ESP32 Proyects/nextgenGreenhouse/build/bootloader"
  "C:/Users/juani/OneDrive/Documentos/Programacion/ESP32 Proyects/nextgenGreenhouse/build/bootloader-prefix"
  "C:/Users/juani/OneDrive/Documentos/Programacion/ESP32 Proyects/nextgenGreenhouse/build/bootloader-prefix/tmp"
  "C:/Users/juani/OneDrive/Documentos/Programacion/ESP32 Proyects/nextgenGreenhouse/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/juani/OneDrive/Documentos/Programacion/ESP32 Proyects/nextgenGreenhouse/build/bootloader-prefix/src"
  "C:/Users/juani/OneDrive/Documentos/Programacion/ESP32 Proyects/nextgenGreenhouse/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/juani/OneDrive/Documentos/Programacion/ESP32 Proyects/nextgenGreenhouse/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/juani/OneDrive/Documentos/Programacion/ESP32 Proyects/nextgenGreenhouse/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
