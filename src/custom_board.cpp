#include <Arduino.h>
#include "storage.h"
#include "logger.h"
#include "radio.h"
#include "print.h"
#include "sensors.h"

void setup() {
  setvbuf(stdout, NULL, _IONBF, 0);
  Wire.begin();
  setupUSB();
  pinMode(ENUSB, INPUT);
  setupGPS();
  i2c::setupBMP();
  i2c::setupSPS30();
  initFlash();
  transm.setupLora();
  Wire.setClock(50000); // required due to poor signal integrity
}

void processData(const gps_fix& fix) {
  if (fix.valid.date && fix.valid.time) {
    NeoGPS::clock_t c = fix.dateTime;
    store::values.set(store::t{c});
  }
  if (fix.valid.location) {
    store::values.set(store::lat{fix.latitudeL()});
    store::values.set(store::lng{fix.longitudeL()});
  }
  if (fix.valid.heading)
    store::values.set(store::heading{fix.heading()});
  if (fix.valid.speed)
    store::values.set(store::spd{fix.speed_kph()});
  if (fix.valid.altitude)
    store::values.set(store::altGPS{fix.altitude()});
  store::values.set(store::ms{millis()});
  store::values.set(store::ch4{gas::readCH4()});
  store::values.set(store::co{gas::readCO()});
  store::values.set(store::o3{gas::readO3()});
  i2c::BMPReturn bmp = i2c::getBMPValues();
  store::values.set(store::tIntBMP{bmp.T});
  store::values.set(store::p{bmp.P});
  store::values.set(store::altBMP{bmp.Alt});
  i2c::SHT21Return sht21 = i2c::getSHT21Values();
  store::values.set(store::tExt{sht21.T});
  store::values.set(store::hum{sht21.hum});
  sps30_measurement sps30 = i2c::getSPS30Values();
  if (!isnan(sps30.typical_particle_size)) {
    store::values.set(store::pm1{sps30.mc_1p0}); 
    store::values.set(store::pm2_5{sps30.mc_2p5}); 
    store::values.set(store::pm4{sps30.mc_4p0}); 
    store::values.set(store::pm10{sps30.mc_10p0}); 
    store::values.set(store::nc0_5{sps30.nc_0p5}); 
    store::values.set(store::nc1{sps30.nc_1p0}); 
    store::values.set(store::nc2_5{sps30.nc_2p5}); 
    store::values.set(store::nc4{sps30.nc_4p0}); 
    store::values.set(store::nc10{sps30.nc_10p0}); 
    store::values.set(store::avgP{sps30.typical_particle_size}); 
  }
  store::values.values(f);
  if (transm.checkTransmit())
    transm.transmitMsg(store::arr);
}

void loop() {
  if (digitalRead(ENUSB) == HIGH) { //USB active
    if (!enabled) {
      printf(PSTR("Starting USB\n"));
      f.close();
      USBComposite.begin();
      enabled = true;
    }
    MassStorage.loop();
  } else { //USB inactive
    if (enabled) {
      printf(PSTR("Stopping USB\n"));
      enabled = false;
      USBComposite.end();
      
      printf("Mounting flash\n");
      if (!fatfs.begin(&flash)) {
        printf("Error failed to mount flash chip, check connection\n");
      }
      String fname = filename();
      printf(PSTR("Filename: %s\n"), fname.c_str());
      f = fatfs.open(fname, FILE_WRITE);
      
      store::values.names(f);
      delay(1500);
    }
    getGPSValues(processData);
  }
}
