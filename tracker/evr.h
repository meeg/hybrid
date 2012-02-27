#ifndef PDSEVR_HH
#define PDSEVR_HH

/*
  erapi.h -- Definitions for Micro-Research Event Receiver
             Application Programming Interface

  Author: Jukka Pietarinen (MRF)
  Date:   08.12.2006

*/

/*
  Note: Byte ordering is big-endian.
 */

/*#if __BYTE_ORDER == __LITTLE_ENDIAN
#define be16_to_cpu(x) bswap_16(x)
#define be32_to_cpu(x) bswap_32(x)
#else
#define be16_to_cpu(x) ((unsigned short)(x))
#define be32_to_cpu(x) ((unsigned long)(x))
#endif
*/

#define be16_to_cpu(x) ((unsigned short)(x))
#define be32_to_cpu(x) ((unsigned long)(x))

#define EVR_MAX_FPOUT_MAP   32
#define EVR_MAX_UNIVOUT_MAP 32
#define EVR_MAX_TBOUT_MAP   64
#define EVR_MAX_FPIN_MAP    16
#define EVR_MAX_UNIVIN_MAP  16
#define EVR_MAX_TBIN_MAP    64
#define EVR_MAX_BUFFER      2048
#define EVR_MAPRAMS         2
#define EVR_MAX_PRESCALERS  64
#define EVR_MAX_PULSES      32
#define EVR_MAX_CML_OUTPUTS 8
#define EVR_MAX_EVENT_CODE  255
#define EVR_DIAG_MAX_COUNTERS 32

#include <stdint.h>

namespace Pds {

class Pulse {
public:
  uint32_t  Control;
  uint32_t  Prescaler;
  uint32_t  Delay;
  uint32_t  Width;
};

class CML {
public:
  /* Bit patterns contain pattern bits in the 20 lowest bit locations */
  uint32_t  Pattern00; /* bit pattern for low state */
  uint32_t  Pattern01; /* bit pattern for rising edge */
  uint32_t  Pattern10; /* bit pattern for falling edge */
  uint32_t  Pattern11; /* bit pattern for high state */
  uint32_t  Control;   /* CML Control Register */
  uint32_t  Resv[3];
};

class MapRamItem {
public:
  uint32_t  IntEvent;
  uint32_t  PulseTrigger;
  uint32_t  PulseSet;
  uint32_t  PulseClear;
};

class FIFOEvent {
public:
  uint32_t TimestampHigh;
  uint32_t TimestampLow;
  uint32_t EventCode;
};

class Evr {
public:
  enum {Local119MHz=0x018741AD};
  Evr();
  void Reset();
  int Enable(int state);
  int GetEnable();
  void DumpStatus();
  int GetViolation(int clear);
  int DumpMapRam(int ram);
  int MapRamEnable(int ram, int enable);
  int SetForwardEvent(int ram, int code, int enable);
  int EnableEventForwarding(int enable);
  int GetEventForwarding();
  int SetLedEvent(int ram, int code, int enable);
  int SetFIFOEvent(int ram, int code, int enable);
  int SetLatchEvent(int ram, int code, int enable);
  int SetFIFOStopEvent(int ram, int code, int enable);
  int ClearFIFO();
  int GetFIFOEvent(FIFOEvent *fe);
  int EnableFIFOStopEvent(int enable);
  int GetFIFOStopEvent();
  int EnableFIFO(int enable);
  int GetFIFOState();
  int DumpFIFO();
  int SetPulseMap(int ram, int code, int trig,
		  int set, int clear);
  int ClearPulseMap(int ram, int code, int trig,
		    int set, int clear);
  int SetPulseParams(int pulse, int presc,
		     int delay, int width);
  void DumpPulses(int pulses);
  int SetPulseProperties(int pulse, int polarity,
			 int map_reset_ena, int map_set_ena, int map_trigger_ena,
			 int enable);
  int SetUnivOutMap(int output, int map);
  void DumpUnivOutMap(int outputs);
  int SetFPOutMap(int output, int map);
  void DumpFPOutMap(int outputs);
  int SetTBOutMap(int output, int map);
  void DumpTBOutMap(int outputs);
  void IrqAssignHandler(int fd, void (*handler)(int));
  int IrqEnable(int mask);
  int GetIrqFlags();
  int ClearIrqFlags(int mask);
  void IrqHandled(int fd);
  int SetPulseIrqMap(int map);
  void ClearDiagCounters();
  void EnableDiagCounters(int enable);
  uint32_t GetDiagCounter(int idx);
  int UnivDlyEnable(int dlymod, int enable);
  int UnivDlySetDelay(int dlymod, int dly0, int dly1);
  void DumpHex();
  int SetFracDiv(int fracdiv);
  int GetFracDiv();
  int SetDBufMode(int enable);
  int GetDBufStatus();
  int ReceiveDBuf(int enable);
  int GetDBuf(char *dbuf, int size);
  int SetTimestampDivider(int div);
  int GetTimestampCounter();
  int GetSecondsCounter();
  int GetTimestampLatch();
  int GetSecondsLatch();
  int SetTimestampDBus(int enable);
  int SetPrescaler(int presc, int div);
  int SetExtEvent(int ttlin, int code, int enable);
  int SetBackEvent(int ttlin, int code, int enable);
  int SetBackDBus(int ttlin, int dbus);
  int SetTxDBufMode(int enable);
  int GetTxDBufStatus();
  int SendTxDBuf(char *dbuf, int size);
  int GetFormFactor();

  //private:
  uint32_t  Status;                              /* 0000: Status Register */
  uint32_t  Control;                             /* 0004: Main Control Register */
  uint32_t  IrqFlag;                             /* 0008: Interrupt Flags */
  uint32_t  IrqEn;                               /* 000C: Interrupt Enable */
  uint32_t  PulseIrqMap;                         /* 0010:  */
  uint32_t  Resv0x0014;                          /* 0014: Reserved */
  uint32_t  Resv0x0018;                          /* 0018: Reserved */
  uint32_t  Resv0x001C;                          /* 001C: Reserved */
  uint32_t  DataBufControl;                      /* 0020: Data Buffer Control */
  uint32_t  TxDataBufControl;                    /* 0024: TX Data Buffer Control */
  uint32_t  Resv0x0028;                          /* 0028: Reserved */
  uint32_t  FPGAVersion;                         /* 002C: FPGA version */
  uint32_t  Resv0x0030[(0x040-0x030)/4];         /* 0030-003F: Reserved */
  uint32_t  EvCntPresc;                          /* 0040: Event Counter Prescaler */
  uint32_t  EvCntControl;                        /* 0044: Event Counter Control */
  uint32_t  Resv0x0048;                          /* 0048: Reserved */
  uint32_t  UsecDiv;                             /* 004C: round(Event clock/1 MHz) */
  uint32_t  ClockControl;                        /* 0050: Clock Control */
  uint32_t  Resv0x0054[(0x05C-0x054)/4];         /* 0054-005B: Reserved */
  uint32_t  SecondsShift;                        /* 005C: Seconds Counter Shift Register */
  uint32_t  SecondsCounter;                      /* 0060: Seconds Counter */
  uint32_t  TimestampEventCounter;               /* 0064: Timestamp Event counter */
  uint32_t  SecondsLatch;                        /* 0068: Seconds Latch Register */
  uint32_t  TimestampLatch;                      /* 006C: Timestamp Latch Register */
  uint32_t  FIFOSeconds;                         /* 0070: Event FIFO Seconds Register */
  uint32_t  FIFOTimestamp;                       /* 0074: Event FIFO Timestamp Register */
  uint32_t  FIFOEvt;                             /* 0078: Event FIFO Event Code Register */
  uint32_t  Resv0x007C;                          /* 007C: Reserved */
  uint32_t  FracDiv;                             /* 0080: Fractional Synthesizer SY87739L Control Word */
  uint32_t  Resv0x0084;                          /* 0084: Reserved */
  uint32_t  RxInitPS;                            /* 0088: Initial value for RF Recovery DCM phase */
  uint32_t  Resv0x008C;
  uint32_t  GPIODir;                             /* 0090: GPIO signal direction */
  uint32_t  GPIOIn;                              /* 0094: GPIO input register */
  uint32_t  GPIOOut;                             /* 0098: GPIO output register */
  uint32_t  Resv0x009Cto0x00FC[(0x100-0x09C)/4]; /* 009C-00FF: Reserved */
  uint32_t  Prescaler[EVR_MAX_PRESCALERS];       /* 0100-01FF: Prescaler Registers */
  Pulse PulseOut[EVR_MAX_PULSES]; /* 0200-03FF: Pulse Output Registers */
  uint16_t  FPOutMap[EVR_MAX_FPOUT_MAP];         /* 0400-043F: Front panel output mapping */
  uint16_t  UnivOutMap[EVR_MAX_UNIVOUT_MAP];     /* 0440-047F: Universal I/O output mapping */
  uint16_t  TBOutMap[EVR_MAX_TBOUT_MAP];         /* 0480-04FF: TB output mapping */
  uint32_t  FPInMap[EVR_MAX_FPIN_MAP];           /* 0500-053F: Front panel input mapping */
  uint32_t  UnivInMap[EVR_MAX_UNIVIN_MAP];       /* 0540-057F: Universal I/O input mapping */
  uint32_t  Resv0x0580[(0x600-0x580)/4];         /* 0580-05FF: Reserved */
  CML  CMLOut[EVR_MAX_CML_OUTPUTS];/* 0600-06FF: CML Output Structures */
  uint32_t  Resv0x0700[(0x800-0x700)/4];         /* 0700-07FF: Reserved */
  uint32_t  Databuf[EVR_MAX_BUFFER/4];           /* 0800-0FFF: Data Buffer */
  uint32_t  DiagIn;                              /* 1000:      Diagnostics input bits */
  uint32_t  DiagCE;                              /* 1004:      Diagnostics count enable */
  uint32_t  DiagReset;                           /* 1008:      Diagnostics count reset */
  uint32_t  Resv0x100C[(0x1080-0x100C)/4];       /* 100C-1080: Reserved */
  uint32_t  DiagCounter[EVR_DIAG_MAX_COUNTERS];  /* 1080-10FF: Diagnostics counters */
  uint32_t  Resv0x1100[(0x1800-0x1100)/4];       /* 1100-17FF: Reserved */
  uint32_t  TxDatabuf[EVR_MAX_BUFFER/4];         /* 1800-1FFF: TX Data Buffer */
  uint32_t  Resv0x2000[(0x4000-0x2000)/4];       /* 2000-3FFF: Reserved */
  MapRamItem MapRam[EVR_MAPRAMS][EVR_MAX_EVENT_CODE+1];
                                            /* 4000-4FFF: Map RAM 1 */
                                            /* 5000-5FFF: Map RAM 2 */
};

}

/* -- Control Register bit mappings */
#define C_EVR_CTRL_MASTER_ENABLE    31
#define C_EVR_CTRL_EVENT_FWD_ENA    30
#define C_EVR_CTRL_TXLOOPBACK       29
#define C_EVR_CTRL_RXLOOPBACK       28
#define C_EVR_CTRL_TS_CLOCK_DBUS    14
#define C_EVR_CTRL_RESET_TIMESTAMP  13
#define C_EVR_CTRL_LATCH_TIMESTAMP  10
#define C_EVR_CTRL_MAP_RAM_ENABLE   9
#define C_EVR_CTRL_MAP_RAM_SELECT   8
#define C_EVR_CTRL_FIFO_ENABLE      6
#define C_EVR_CTRL_FIFO_DISABLE     5
#define C_EVR_CTRL_FIFO_STOP_EV_EN  4
#define C_EVR_CTRL_RESET_EVENTFIFO  3
/* -- Status Register bit mappings */
#define C_EVR_STATUS_DBUS_HIGH      31
#define C_EVR_STATUS_LEGACY_VIO     16
#define C_EVR_STATUS_FIFO_STOPPED   5
/* -- Interrupt Flag/Enable Register bit mappings */
#define C_EVR_IRQ_MASTER_ENABLE   31
#define C_EVR_NUM_IRQ             6
#define C_EVR_IRQFLAG_DATABUF     5
#define C_EVR_IRQFLAG_PULSE       4
#define C_EVR_IRQFLAG_EVENT       3
#define C_EVR_IRQFLAG_HEARTBEAT   2
#define C_EVR_IRQFLAG_FIFOFULL    1
#define C_EVR_IRQFLAG_VIOLATION   0
#define EVR_IRQ_MASTER_ENABLE     (1 << C_EVR_IRQ_MASTER_ENABLE)
#define EVR_IRQFLAG_DATABUF       (1 << C_EVR_IRQFLAG_DATABUF)
#define EVR_IRQFLAG_PULSE         (1 << C_EVR_IRQFLAG_PULSE)
#define EVR_IRQFLAG_EVENT         (1 << C_EVR_IRQFLAG_EVENT)
#define EVR_IRQFLAG_HEARTBEAT     (1 << C_EVR_IRQFLAG_HEARTBEAT)
#define EVR_IRQFLAG_FIFOFULL      (1 << C_EVR_IRQFLAG_FIFOFULL)
#define EVR_IRQFLAG_VIOLATION     (1 << C_EVR_IRQFLAG_VIOLATION)
/* -- Databuffer Control Register bit mappings */
#define C_EVR_DATABUF_LOAD        15
#define C_EVR_DATABUF_RECEIVING   15
#define C_EVR_DATABUF_STOP        14
#define C_EVR_DATABUF_RXREADY     14
#define C_EVR_DATABUF_CHECKSUM    13
#define C_EVR_DATABUF_MODE        12
#define C_EVR_DATABUF_SIZEHIGH    11
#define C_EVR_DATABUF_SIZELOW     2
/* -- Databuffer Control Register bit mappings */
#define C_EVR_TXDATABUF_COMPLETE   20
#define C_EVR_TXDATABUF_RUNNING    19
#define C_EVR_TXDATABUF_TRIGGER    18
#define C_EVR_TXDATABUF_ENA        17
#define C_EVR_TXDATABUF_MODE       16
#define C_EVR_TXDATABUF_SIZEHIGH   11
#define C_EVR_TXDATABUF_SIZELOW    2
/* -- Clock Control Register bit mapppings */
#define C_EVR_CLKCTRL_RECDCM_RUN     15
#define C_EVR_CLKCTRL_RECDCM_INITD   14
#define C_EVR_CLKCTRL_RECDCM_PSDONE  13
#define C_EVR_CLKCTRL_EVDCM_STOPPED  12
#define C_EVR_CLKCTRL_EVDCM_LOCKED  11
#define C_EVR_CLKCTRL_EVDCM_PSDONE  10
#define C_EVR_CLKCTRL_CGLOCK        9
#define C_EVR_CLKCTRL_RECDCM_PSDEC  8
#define C_EVR_CLKCTRL_RECDCM_PSINC  7
#define C_EVR_CLKCTRL_RECDCM_RESET  6
#define C_EVR_CLKCTRL_EVDCM_PSDEC   5
#define C_EVR_CLKCTRL_EVDCM_PSINC   4
#define C_EVR_CLKCTRL_EVDCM_SRUN    3
#define C_EVR_CLKCTRL_EVDCM_SRES    2
#define C_EVR_CLKCTRL_EVDCM_RES     1
#define C_EVR_CLKCTRL_USE_RXRECCLK  0
/* -- CML Control Register bit mappings */
#define C_EVR_CMLCTRL_REFCLKSEL     3
#define C_EVR_CMLCTRL_RESET         2
#define C_EVR_CMLCTRL_POWERDOWN     1
#define C_EVR_CMLCTRL_ENABLE        0
/* -- Pulse Control Register bit mappings */
#define C_EVR_PULSE_OUT             7
#define C_EVR_PULSE_SW_SET          6
#define C_EVR_PULSE_SW_RESET        5
#define C_EVR_PULSE_POLARITY        4
#define C_EVR_PULSE_MAP_RESET_ENA   3
#define C_EVR_PULSE_MAP_SET_ENA     2
#define C_EVR_PULSE_MAP_TRIG_ENA    1
#define C_EVR_PULSE_ENA             0
/* -- Map RAM Internal event mappings */
#define C_EVR_MAP_SAVE_EVENT        31
#define C_EVR_MAP_LATCH_TIMESTAMP   30
#define C_EVR_MAP_LED_EVENT         29
#define C_EVR_MAP_FORWARD_EVENT     28
#define C_EVR_MAP_STOP_FIFO         27
#define C_EVR_MAP_HEARTBEAT_EVENT   5
#define C_EVR_MAP_RESETPRESC_EVENT  4
#define C_EVR_MAP_TIMESTAMP_RESET   3
#define C_EVR_MAP_TIMESTAMP_CLK     2
#define C_EVR_MAP_SECONDS_1         1
#define C_EVR_MAP_SECONDS_0         0
/* -- Output Mappings */
#define C_EVR_SIGNAL_MAP_BITS       6
#define C_EVR_SIGNAL_MAP_PULSE      0
#define C_EVR_SIGNAL_MAP_DBUS       32
#define C_EVR_SIGNAL_MAP_PRESC      40
#define C_EVR_SIGNAL_MAP_HIGH       62
#define C_EVR_SIGNAL_MAP_LOW        63
/* GPIO mapping for delay module */
#define EVR_UNIV_DLY_DIN    0x01
#define EVR_UNIV_DLY_SCLK   0x02
#define EVR_UNIV_DLY_LCLK   0x04
#define EVR_UNIV_DLY_DIS    0x08
/* -- FP Input Mapping bits */
#define C_EVR_FPIN_EXTEVENT_BASE   0
#define C_EVR_FPIN_BACKEVENT_BASE  8
#define C_EVR_FPIN_BACKDBUS_BASE   16
#define C_EVR_FPIN_EXT_ENABLE      24
#define C_EVR_FPIN_BACKEV_ENABLE   25

/* ioctl commands */
#define EV_IOC_MAGIC 220
#define EV_IOCRESET  _IO(EV_IOC_MAGIC, 0)
#define EV_IOCIRQEN  _IO(EV_IOC_MAGIC, 1)
#define EV_IOCIRQDIS _IO(EV_IOC_MAGIC, 2)

#endif
