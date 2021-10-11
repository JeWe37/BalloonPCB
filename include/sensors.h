#ifndef SENSORS_H
#define SENSORS_H
#include "print.h"

#include "Adafruit_BMP280.h"
#include "Sodaq_SHT2x.h"
#include "sps30.h"

#define CH4 PB0
#define CO PB1
#define O3 PA4

#define NaN (float)(0./0.)

#define SPS_RETRY_CNT 10

namespace gas {
  float readCH4() {
    return analogRead(CH4) / 4096.0f * 3.3f;
  }

  float readCO() {
    return analogRead(CO) / 4096.0f * 3.3f;
  }
  
  float readO3() {
    return analogRead(O3) / 4096.0f * 3.3f;
  }
}

namespace i2c {
  Adafruit_BMP280 bmp;

  void setupBMP() {
    printf(PSTR("Connecting BMP280\n"));
    if (!bmp.begin()) {
      printf(PSTR("Could not find BMP280\n"));
      while (true);
    }
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                    Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                    Adafruit_BMP280::STANDBY_MS_1000); /* Standby time. */
  }

  struct BMPReturn {
    float T, P, Alt; //degrees celsius, pascals, meters
  };

  BMPReturn getBMPValues() {
    return BMPReturn{bmp.readTemperature(), bmp.readPressure(), bmp.readAltitude()};
  }

  struct SHT21Return {
    float T, hum;
  };

  SHT21Return getSHT21Values() {
    return SHT21Return{SHT2x.GetTemperature(), SHT2x.GetHumidity()};
  }

  bool gave_up = false;

  void setupSPS30() {
    s16 ret;
    printf(PSTR("Connecting SPS30\n"));
    int tries = SPS_RETRY_CNT;
    while (sps30_probe() != 0 && --tries) {
      printf(PSTR("Trying again\n"));
      delay(500);
    }
    if (!tries) {
      gave_up = true;
      printf("Giving up!\n");
    }
    if ((ret = sps30_set_fan_auto_cleaning_interval_days(4))) {
      printf(PSTR("Error setting the auto-clean interval: %d\n"), ret);
      while (true);
    }
    if ((ret = sps30_start_measurement() < 0)) {
      printf(PSTR("Error starting measurement: %d\n"), ret);
    }
    printf(PSTR("Connected SPS30\n"));
  }

  sps30_measurement getSPS30Values() {
    sps30_measurement m;
    s16 ret;
    if (gave_up)
      ret = -10;
    else {
      u16 data_ready;
      do {
        ret = sps30_read_data_ready(&data_ready);
        if (ret < 0)
          printf(PSTR("Error reading data-ready flag: %d\n"), ret);
        else if (!data_ready)
          printf(PSTR("Data not ready, no new measurement available\n"));
        else
          break;
        delay(100); /* retry in 100ms */
      } while (true);
      ret = sps30_read_measurement(&m);
    }
    if (ret < 0) {
      printf(PSTR("Error reading measurement: %d\n"), ret);
      return sps30_measurement{NaN, NaN, NaN, NaN, NaN, NaN, NaN, NaN, NaN, NaN};
    } else {
      return m;
    }
  }
}
#endif
