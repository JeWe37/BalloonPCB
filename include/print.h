#ifndef PRINT_H
#define PRINT_H
namespace ITM {
  typedef struct {
    volatile  union {
      volatile   uint8_t    u8;                 /*!< Offset: 0x000 ( /W)  ITM Stimulus Port 8-bit */
      volatile   uint16_t   u16;                /*!< Offset: 0x000 ( /W)  ITM Stimulus Port 16-bit */
      volatile   uint32_t   u32;                /*!< Offset: 0x000 ( /W)  ITM Stimulus Port 32-bit */
    } PORT [32U];                               /*!< Offset: 0x000 ( /W)  ITM Stimulus Port Registers */
    uint32_t RESERVED0[864U];
    volatile  uint32_t TER;                     /*!< Offset: 0xE00 (R/W)  ITM Trace Enable Register */
    uint32_t RESERVED1[15U];
    volatile  uint32_t TPR;                     /*!< Offset: 0xE40 (R/W)  ITM Trace Privilege Register */
    uint32_t RESERVED2[15U];
    volatile  uint32_t TCR;                     /*!< Offset: 0xE80 (R/W)  ITM Trace Control Register */
    uint32_t RESERVED3[29U];
    volatile   uint32_t IWR;                    /*!< Offset: 0xEF8 ( /W)  ITM Integration Write Register */
    volatile const  uint32_t IRR;               /*!< Offset: 0xEFC (R/ )  ITM Integration Read Register */
    volatile  uint32_t IMCR;                    /*!< Offset: 0xF00 (R/W)  ITM Integration Mode Control Register */
    uint32_t RESERVED4[43U];
    volatile   uint32_t LAR;                    /*!< Offset: 0xFB0 ( /W)  ITM Lock Access Register */
    volatile const  uint32_t LSR;               /*!< Offset: 0xFB4 (R/ )  ITM Lock Status Register */
    uint32_t RESERVED5[6U];
    volatile const  uint32_t PID4;              /*!< Offset: 0xFD0 (R/ )  ITM Peripheral Identification Register #4 */
    volatile const  uint32_t PID5;              /*!< Offset: 0xFD4 (R/ )  ITM Peripheral Identification Register #5 */
    volatile const  uint32_t PID6;              /*!< Offset: 0xFD8 (R/ )  ITM Peripheral Identification Register #6 */
    volatile const  uint32_t PID7;              /*!< Offset: 0xFDC (R/ )  ITM Peripheral Identification Register #7 */
    volatile const  uint32_t PID0;              /*!< Offset: 0xFE0 (R/ )  ITM Peripheral Identification Register #0 */
    volatile const  uint32_t PID1;              /*!< Offset: 0xFE4 (R/ )  ITM Peripheral Identification Register #1 */
    volatile const  uint32_t PID2;              /*!< Offset: 0xFE8 (R/ )  ITM Peripheral Identification Register #2 */
    volatile const  uint32_t PID3;              /*!< Offset: 0xFEC (R/ )  ITM Peripheral Identification Register #3 */
    volatile const  uint32_t CID0;              /*!< Offset: 0xFF0 (R/ )  ITM Component  Identification Register #0 */
    volatile const  uint32_t CID1;              /*!< Offset: 0xFF4 (R/ )  ITM Component  Identification Register #1 */
    volatile const  uint32_t CID2;              /*!< Offset: 0xFF8 (R/ )  ITM Component  Identification Register #2 */
    volatile const  uint32_t CID3;              /*!< Offset: 0xFFC (R/ )  ITM Component  Identification Register #3 */
  } ITM_Type;

  ITM_Type* ITM = (ITM_Type*)0xE0000000UL;      /*!< ITM configuration struct */

  static inline uint32_t ITM_SendChar(uint32_t ch) {
    if (((ITM->TCR & 1UL) != 0UL) /* ITM enabled */         &&
        ((ITM->TER & 1UL) != 0UL) /* ITM Port #0 enabled */ ) {
      while (ITM->PORT[0U].u32 == 0UL);
      ITM->PORT[0U].u8 = (uint8_t)ch;
    }
    return (ch);
  }
}

extern "C" int _write(int file, char *ptr, int len) {
  for (int i = 0 ; i < len ; i++)
    ITM::ITM_SendChar((*ptr++));
  return len;
}

namespace gen {
  template<bool B, class T = void> struct enable_if {};

  template<class T> struct enable_if<true, T> {
    typedef T type;
  };
}
#endif
