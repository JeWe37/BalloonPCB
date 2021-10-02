#ifndef STORAGE_H
#define STORAGE_H
#include "print.h"

#include "USBComposite.h"
#include "Adafruit_SPIFlash.h"
#include "SdFat.h"

#define ENUSB PA10
#define NCS PB5

#define PRODUCT_ID 0x29

USBMassStorage MassStorage;

Adafruit_FlashTransport_SPI flashTransport(NCS, &SPI);
Adafruit_SPIFlash flash(&flashTransport);

const uint32_t speed = SPI_CLOCK_DIV2 ;
bool enabled = false;
uint32 cardSize;

bool write(const uint8_t *writebuff, uint32_t startSector, uint16_t numSectors) {
  return flash.writeBlocks(startSector, writebuff, numSectors);
}

bool read(uint8_t *readbuff, uint32_t startSector, uint16_t numSectors) {
  return flash.readBlocks(startSector, readbuff, numSectors);
}

bool flush() {
  return flash.syncBlocks();
}

File f;
FatFileSystem fatfs;

String filename() {
  FatFile root;
  if (!root.open("/")) {
    printf(PSTR("Open root failed!"));
    while (true);
  }
  for (uint16_t i = 0; i < 1000; ++i) {
    String res = "data" + String(i) + ".csv";
    if (!root.exists(res.c_str()))
      return res;
    else
      printf(PSTR("%s already exists!\n"), res.c_str());
  }
  printf(PSTR("Could not find free file name"));
}

SPIFlash_Device_t dev[] = {W25Q128JV_PM};

#define SECTOR_SIZE 512

void initFlash() {
  printf("Connecting flash\n");
  while (!flash.begin(dev)) {
    delay(50);
    printf("Trying again\n");
  }
  cardSize = flash.size()/SECTOR_SIZE;
  MassStorage.setDriveData(0, cardSize, read, write, flush);
}

void setupUSB() {
  printf("Setting up USB\n");
  USBComposite.setProductId(PRODUCT_ID);
  MassStorage.registerComponent();
}
#endif
