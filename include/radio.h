#ifndef RADIO_H
#define RADIO_H
#include "print.h"
//tweaked radiolib not to use software serial, adjusted neogps config; ignore warnings, they shouldn't matter really
#include "NeoGPS_cfg.h"
#include "ublox/ubxNMEA.h"
#include "RadioLib.h"

#include "rs.hpp"

#define gpsPort Serial2
#define GPS_PORT_NAME "Serial"

#define dutyCycle 0.1f
#define frequ 868.525
#define bw 250.0
#define spread 12
#define codeRate 8
#define syncWord 0x37
#define power 22

#define SXNRST PA8
#define BUSY PA0
#define DIO1 PA1
#define NSS PA9

ubloxNMEA gps;

void setupGPS() {
  gpsPort.begin(9600);
  printf(PSTR("Looking for GPS device on " GPS_PORT_NAME "\n"));
}

void getGPSValues(void (*f)(const gps_fix&)) {
  while (!gps.available(gpsPort));
  gps_fix ret = gps.read();
  gps.send_P(&gpsPort, F("PUBX,00"));
  gps.send_P(&gpsPort, F("PUBX,04"));
  f(ret);
}

#define MAX_PACKET_SIZE 64

#define MIN(a,b) (((a)<(b))?(a):(b))

#define INTER_PACKET 500000 // in us

void transmitFinished();

void timeout();

namespace impl {
  using seqType = uint16_t;

  template<int32_t N, typename Enable = void> struct calcPacketNum {
    static constexpr size_t value = calcPacketNum<N+sizeof(seqType)-MAX_PACKET_SIZE>::value+1;
  };

  template<int32_t N> struct calcPacketNum<N, typename gen::enable_if<N <= 0>::type> {
    static constexpr size_t value = 0;
  };
}

template<size_t len, size_t ecclen = 8> class transmitter {  
  impl::seqType seq;
  
  static constexpr size_t packetNum = impl::calcPacketNum<len+ecclen>::value;

  static constexpr size_t encLen = packetNum*sizeof(impl::seqType)+ecclen+len;

  static constexpr size_t baseLen = encLen/packetNum;

  static constexpr size_t addOne = encLen%packetNum;

  byte transmission[encLen] = {0};
  
  struct slice {
    byte* beg;
    size_t l;
    size_t without_seq;
  };

  static constexpr slice getNthDat(byte* arr, unsigned n) {
    return {arr+n*baseLen+MIN(addOne, n), baseLen+(addOne > n)-((n == packetNum-1)*ecclen)-sizeof(impl::seqType), n*(baseLen-sizeof(impl::seqType))+MIN(addOne, n)};
  }

  void addSeqNums(byte message[], byte msg[]) {
    for (unsigned i = 0; i < packetNum; ++i) {
      slice s = getNthDat(message, i);
      *reinterpret_cast<impl::seqType*>(s.beg) = seq++;
      memcpy(s.beg+sizeof(impl::seqType), msg+s.without_seq, s.l);
    }
  }

  unsigned packet;

  RS::ReedSolomon<encLen-ecclen, ecclen> rs;

  SX1262 lora = new Module(NSS, DIO1, SXNRST, BUSY);
  
  uint32 startTime;

  void nextPacket() {
    size_t l = baseLen+(addOne > packet);
    int ret = lora.startTransmit(transmission+baseLen*packet+MIN(packet, addOne), l);
    if (ret != ERR_NONE) {
      packet = packetNum;
      printf(PSTR("Error transmitting: %d\n"), ret);
    } else
      packet++;
  }

  friend void transmitFinished();

  friend void timeout();
public:
  transmitter()
  : seq(0), packet(0) {}

  void setupLora() {
    printf("Setting up SX1262\n");
    int ret = lora.begin(frequ, bw, spread, codeRate, syncWord, power, 8, 3.0);  //..., preambleLength, TCXOVoltage
    if (ret != ERR_NONE) {
      printf(PSTR("Error starting SX1262: %d\n"), ret);
      while (true);
    }
    lora.setDio1Action(transmitFinished);
    if (lora.setCRC(false) != ERR_NONE) {
      printf(PSTR("Error disabling CRC\n"));
      while (true);
    }
    if (lora.setDio2AsRfSwitch(false) != ERR_NONE) {
      printf(PSTR("Error disabling RF switch\n"));
      while (true);
    }
    if (lora.setCurrentLimit(140.0) != ERR_NONE) {
      printf(PSTR("Error setting current limit"));
      while (true);
    }
    Timer2.setMode(TIMER_CH1, TIMER_OUTPUTCOMPARE);
    Timer2.setPeriod(INTER_PACKET); // in microseconds
    Timer2.setCompare(TIMER_CH1, 1);      // overflow might be small
    Timer2.attachInterrupt(TIMER_CH1, timeout);
    Timer2.pause();
  }

  void transmitMsg(byte msg[]) {
    byte message[encLen-ecclen];
    addSeqNums(message, msg);
    rs.Encode(message, transmission);
    startTime = millis()+INTER_PACKET/1000*(packetNum-1);
    nextPacket();
  }

  bool checkTransmit() {
    return !packet && millis() > startTime;
  }
};

transmitter<store::values.size()> transm;

void timeout() {
  Timer2.pause();
  transm.nextPacket();
}

void transmitFinished() {
  if (transm.packet == transm.packetNum) {
    transm.packet = 0;
    printf("starttime: %ld\n", transm.startTime);
    transm.startTime += (millis() - transm.startTime) / dutyCycle;
    printf("starttime: %ld\n", transm.startTime);
  } else
    Timer2.resume(); //start transmitting next packet in 500ms
}
#endif
