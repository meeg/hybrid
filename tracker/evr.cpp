/*
  erapi.c -- Functions for Micro-Research Event Receiver
             Application Programming Interface

  Author: Jukka Pietarinen (MRF)
  Date:   08.12.2006

*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <endian.h>
#include <byteswap.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include "evr.h"

using namespace Pds;

/*
#define DEBUG 1
*/
#define DEBUG_PRINTF printf

Evr::Evr() {
  Reset();
}

void Evr::Reset() {
  SetFracDiv(Local119MHz); // SLAC specific 119MHz
  Enable(0);
  IrqEnable(0);
  ClearFIFO();
  ClearIrqFlags(0xffffffff);
  SetTimestampDivider(1); // clock at 119MHz
  // disable all opcodes by default, clear all pulse mappings.
  int set=-1; int clear=-1;
  for (unsigned ram=0;ram<2;ram++) {
    for (unsigned iopcode=0;iopcode<=EVR_MAX_EVENT_CODE;iopcode++) {
      SetFIFOEvent(ram, iopcode, 0);
      for (unsigned jpulse=0;jpulse<EVR_MAX_PULSES;jpulse++)
        ClearPulseMap(ram, iopcode, jpulse, set, clear);
    }
  }
}

int Evr::Enable(int state)
{
  if (state)
    Control |= be32_to_cpu(1 << C_EVR_CTRL_MASTER_ENABLE);
  else
    Control &= be32_to_cpu(~(1 << C_EVR_CTRL_MASTER_ENABLE));
  
  return GetEnable();
}

int Evr::GetEnable()
{
  return be32_to_cpu(Control & be32_to_cpu(1 << C_EVR_CTRL_MASTER_ENABLE));
}

int Evr::GetViolation(int clear)
{
  int result;

  result = be32_to_cpu(IrqFlag & be32_to_cpu(1 << C_EVR_IRQFLAG_VIOLATION));
  if (clear && result)
    IrqFlag = be32_to_cpu(result);

  return result;
}

void Evr::DumpStatus()
{
  int result;

  result = be32_to_cpu(Status);
  DEBUG_PRINTF("Status %08x ", result);
  if (result & (1 << C_EVR_STATUS_LEGACY_VIO))
    DEBUG_PRINTF("LEGVIO ");
  if (result & (1 << C_EVR_STATUS_FIFO_STOPPED))
    DEBUG_PRINTF("FIFOSTOP ");
  DEBUG_PRINTF("\n");
  result = be32_to_cpu(Control);
  DEBUG_PRINTF("Control %08x: ", result);
  if (result & (1 << C_EVR_CTRL_MASTER_ENABLE))
    DEBUG_PRINTF("MSEN ");
  if (result & (1 << C_EVR_CTRL_EVENT_FWD_ENA))
    DEBUG_PRINTF("FWD ");
  if (result & (1 << C_EVR_CTRL_TXLOOPBACK))
    DEBUG_PRINTF("TXLP ");
  if (result & (1 << C_EVR_CTRL_RXLOOPBACK))
    DEBUG_PRINTF("RXLP ");
  if (result & (1 << C_EVR_CTRL_TS_CLOCK_DBUS))
    DEBUG_PRINTF("DSDBUS ");
  if (result & (1 << C_EVR_CTRL_MAP_RAM_ENABLE))
    DEBUG_PRINTF("MAPENA ");
  if (result & (1 << C_EVR_CTRL_MAP_RAM_SELECT))
    DEBUG_PRINTF("MAPSEL ");
  DEBUG_PRINTF("\n");
  result = be32_to_cpu(IrqFlag);
  DEBUG_PRINTF("IRQ Flag %08x: ", result);
  if (result & (1 << C_EVR_IRQ_MASTER_ENABLE))
    DEBUG_PRINTF("IRQEN ");
  if (result & (1 << C_EVR_IRQFLAG_DATABUF))
    DEBUG_PRINTF("DBUF ");
  if (result & (1 << C_EVR_IRQFLAG_PULSE))
    DEBUG_PRINTF("PULSE ");
  if (result & (1 << C_EVR_IRQFLAG_EVENT))
    DEBUG_PRINTF("EVENT ");
  if (result & (1 << C_EVR_IRQFLAG_HEARTBEAT))
    DEBUG_PRINTF("HB ");
  if (result & (1 << C_EVR_IRQFLAG_FIFOFULL))
    DEBUG_PRINTF("FF ");
  if (result & (1 << C_EVR_IRQFLAG_VIOLATION))
    DEBUG_PRINTF("VIO ");
  DEBUG_PRINTF("\n");
  result = be32_to_cpu(DataBufControl);
  DEBUG_PRINTF("DataBufControl %08x\n", result);
}

int Evr::DumpMapRam(int ram)
{
  int code;
  int intev;
  int ptrig, pset, pclr;

  for (code = 0; code <= EVR_MAX_EVENT_CODE; code++)
    {
      intev = be32_to_cpu(MapRam[ram][code].IntEvent);
      ptrig = be32_to_cpu(MapRam[ram][code].PulseTrigger);
      pset = be32_to_cpu(MapRam[ram][code].PulseSet);
      pclr = be32_to_cpu(MapRam[ram][code].PulseClear);

	  DEBUG_PRINTF("Code 0x%02x (%3d): ", code, code);
	  DEBUG_PRINTF(" 0x%08x ",ptrig);
	  DEBUG_PRINTF("\n");
    }
  return 0;
}

/*int Evr::DumpMapRam(int ram)
{
  int code;
  int intev;
  int ptrig, pset, pclr;

  for (code = 0; code <= EVR_MAX_EVENT_CODE; code++)
    {
      intev = be32_to_cpu(MapRam[ram][code].IntEvent);
      ptrig = be32_to_cpu(MapRam[ram][code].PulseTrigger);
      pset = be32_to_cpu(MapRam[ram][code].PulseSet);
      pclr = be32_to_cpu(MapRam[ram][code].PulseClear);

      if (intev ||
	  ptrig ||
	  pset ||
	  pclr)
	{
	  DEBUG_PRINTF("Code 0x%02x (%3d): ", code, code);
	  if (intev & (1 << C_EVR_MAP_SAVE_EVENT))
	    DEBUG_PRINTF("SAVE ");
	  if (intev & (1 << C_EVR_MAP_LATCH_TIMESTAMP))
	    DEBUG_PRINTF("LTS ");
	  if (intev & (1 << C_EVR_MAP_LED_EVENT))
	    DEBUG_PRINTF("LED ");
	  if (intev & (1 << C_EVR_MAP_FORWARD_EVENT))
	    DEBUG_PRINTF("FWD ");
	  if (intev & (1 << C_EVR_MAP_STOP_FIFO))
	    DEBUG_PRINTF("STOPFIFO ");
	  if (intev & (1 << C_EVR_MAP_HEARTBEAT_EVENT))
	    DEBUG_PRINTF("HB ");
	  if (intev & (1 << C_EVR_MAP_RESETPRESC_EVENT))
	    DEBUG_PRINTF("RESPRSC ");
	  if (intev & (1 << C_EVR_MAP_TIMESTAMP_RESET))
	    DEBUG_PRINTF("RESTS ");
	  if (intev & (1 << C_EVR_MAP_TIMESTAMP_CLK))
	    DEBUG_PRINTF("TSCLK ");
	  if (intev & (1 << C_EVR_MAP_SECONDS_1))
	    DEBUG_PRINTF("SEC1 ");
	  if (intev & (1 << C_EVR_MAP_SECONDS_0))
	    DEBUG_PRINTF("SEC0 ");
	  if (ptrig)
	    DEBUG_PRINTF("Trig %08x", ptrig);
	  if (pset)
	    DEBUG_PRINTF("Set %08x", pset);
	  if (pclr)
	    DEBUG_PRINTF("Clear %08x", pclr);
	  DEBUG_PRINTF("\n");
	}
    }
  return 0;
}
*/
int Evr::MapRamEnable(int ram, int enable)
{
  int result;

  if (ram < 0 || ram > 1)
    return -1;

  result = be32_to_cpu(Control);
  result &= ~((1 << C_EVR_CTRL_MAP_RAM_ENABLE) | (1 << C_EVR_CTRL_MAP_RAM_SELECT));
  if (ram == 1)
    result |= (1 << C_EVR_CTRL_MAP_RAM_SELECT);
  if (enable == 1)
    result |= (1 << C_EVR_CTRL_MAP_RAM_ENABLE);
  Control = be32_to_cpu(result);

  return result;
}

int Evr::SetPulseMap(int ram, int code, int trig,
		   int set, int clear)
{
  if (ram < 0 || ram >= EVR_MAPRAMS)
    return -1;

  if (code <= 0 || code > EVR_MAX_EVENT_CODE)
    return -1;

   if (trig >= 0 && trig < EVR_MAX_PULSES)
     MapRam[ram][code].PulseTrigger |= be32_to_cpu(1 << trig);
   if (set >= 0 && set < EVR_MAX_PULSES)
     MapRam[ram][code].PulseSet |= be32_to_cpu(1 << set);
   if (clear >= 0 && clear < EVR_MAX_PULSES)
     MapRam[ram][code].PulseClear |= be32_to_cpu(1 << clear);

  return 0;
}

int Evr::SetForwardEvent(int ram, int code, int enable)
{
  if (ram < 0 || ram >= EVR_MAPRAMS)
    return -1;

  if (code <= 0 || code > EVR_MAX_EVENT_CODE)
    return -1;

  if (!enable)
    MapRam[ram][code].IntEvent &= be32_to_cpu(~(1 << C_EVR_MAP_FORWARD_EVENT));
  if (enable)
    MapRam[ram][code].IntEvent |= be32_to_cpu(1 << C_EVR_MAP_FORWARD_EVENT);
    
  return 0;
}

int Evr::EnableEventForwarding(int enable)
{
  if (enable)
    Control |= be32_to_cpu(1 << C_EVR_CTRL_EVENT_FWD_ENA);
  else
    Control &= be32_to_cpu(~(1 << C_EVR_CTRL_EVENT_FWD_ENA));
  
  return GetEventForwarding();
}

int Evr::GetEventForwarding()
{
  return be32_to_cpu(Control & be32_to_cpu(1 << C_EVR_CTRL_EVENT_FWD_ENA));
}

int Evr::SetLedEvent(int ram, int code, int enable)
{
  if (ram < 0 || ram >= EVR_MAPRAMS)
    return -1;

  if (code <= 0 || code > EVR_MAX_EVENT_CODE)
    return -1;

  if (!enable)
    MapRam[ram][code].IntEvent &= be32_to_cpu(~(1 << C_EVR_MAP_LED_EVENT));
  if (enable)
    MapRam[ram][code].IntEvent |= be32_to_cpu(1 << C_EVR_MAP_LED_EVENT);
    
  return 0;
}

int Evr::SetFIFOEvent(int ram, int code, int enable)
{
  if (ram < 0 || ram >= EVR_MAPRAMS)
    return -1;

  if (code <= 0 || code > EVR_MAX_EVENT_CODE)
    return -1;

  if (!enable)
    MapRam[ram][code].IntEvent &= be32_to_cpu(~(1 << C_EVR_MAP_SAVE_EVENT));
  if (enable)
    MapRam[ram][code].IntEvent |= be32_to_cpu(1 << C_EVR_MAP_SAVE_EVENT);
    
  return 0;
}

int Evr::SetLatchEvent(int ram, int code, int enable)
{
  if (ram < 0 || ram >= EVR_MAPRAMS)
    return -1;

  if (code <= 0 || code > EVR_MAX_EVENT_CODE)
    return -1;

  if (!enable)
    MapRam[ram][code].IntEvent &= be32_to_cpu(~(1 << C_EVR_MAP_LATCH_TIMESTAMP));
  if (enable)
    MapRam[ram][code].IntEvent |= be32_to_cpu(1 << C_EVR_MAP_LATCH_TIMESTAMP);
    
  return 0;
}

int Evr::SetFIFOStopEvent(int ram, int code, int enable)
{
  if (ram < 0 || ram >= EVR_MAPRAMS)
    return -1;

  if (code <= 0 || code > EVR_MAX_EVENT_CODE)
    return -1;

  if (!enable)
    MapRam[ram][code].IntEvent &= be32_to_cpu(~(1 << C_EVR_MAP_STOP_FIFO));
  if (enable)
    MapRam[ram][code].IntEvent |= be32_to_cpu(1 << C_EVR_MAP_STOP_FIFO);
    
  return 0;
}

int Evr::ClearFIFO()
{
  int ctrl;

  ctrl = be32_to_cpu(Control);
  ctrl |= (1 << C_EVR_CTRL_RESET_EVENTFIFO);
  Control = be32_to_cpu(ctrl);

  return be32_to_cpu(Control);
}

int Evr::GetFIFOEvent(FIFOEvent *fe)
{
  int stat;

  stat = be32_to_cpu(IrqFlag);
  if (stat & (1 << C_EVR_IRQFLAG_EVENT))
    {
      fe->EventCode = be32_to_cpu(FIFOEvt);
      fe->TimestampHigh = be32_to_cpu(FIFOSeconds);
      fe->TimestampLow = be32_to_cpu(FIFOTimestamp);
      return 0;
    }
  else
    return -1;
}

int Evr::EnableFIFO(int enable)
{
  if (enable)
    Control |= be32_to_cpu(1 << C_EVR_CTRL_FIFO_ENABLE);
  else
    Control |= be32_to_cpu(1 << C_EVR_CTRL_FIFO_DISABLE);
  
  return GetFIFOState();
}

int Evr::GetFIFOState()
{
  return be32_to_cpu(Status & be32_to_cpu(1 << C_EVR_STATUS_FIFO_STOPPED));
}

int Evr::EnableFIFOStopEvent(int enable)
{
  if (enable)
    Control |= be32_to_cpu(1 << C_EVR_CTRL_FIFO_STOP_EV_EN);
  else
    Control &= be32_to_cpu(~(1 << C_EVR_CTRL_FIFO_STOP_EV_EN));
  
  return GetFIFOStopEvent();
}

int Evr::GetFIFOStopEvent()
{
  return be32_to_cpu(Control & be32_to_cpu(1 << C_EVR_CTRL_FIFO_STOP_EV_EN));
}

int Evr::DumpFIFO()
{
  FIFOEvent fe;
  int i;

  do
    {
      i = GetFIFOEvent(&fe);
      if (!i)
	{
	  printf("Code %08x, %08x:%08x\n",
		 fe.EventCode, fe.TimestampHigh, fe.TimestampLow);
	}
    }
  while (!i);

  return 0;
}

int Evr::ClearPulseMap(int ram, int code, int trig,
		     int set, int clear)
{
  if (ram < 0 || ram >= EVR_MAPRAMS)
    return -1;

  if (code <= 0 || code > EVR_MAX_EVENT_CODE)
    return -1;

  if (trig >= 0 && trig < EVR_MAX_PULSES)
    MapRam[ram][code].PulseTrigger &= be32_to_cpu(~(1 << trig));
  if (set >= 0 && set < EVR_MAX_PULSES)
    MapRam[ram][code].PulseSet &= be32_to_cpu(~(1 << set));
  if (clear >= 0 && clear < EVR_MAX_PULSES)
    MapRam[ram][code].PulseClear &= be32_to_cpu(~(1 << clear));

  return 0;
}

int Evr::SetPulseParams(int pulse, int presc,
		      int delay, int width)
{
  //if (pulse < 0 || pulse >= EVR_MAX_PULSES)
    //return -1;

  //PulseOut[pulse].Prescaler = be32_to_cpu(presc);
  //PulseOut[pulse].Delay = be32_to_cpu(delay);
  PulseOut[pulse].Width = be32_to_cpu(width);
  PulseOut[pulse].Delay = be32_to_cpu(delay);
  return 0;
}

void Evr::DumpPulses(int pulses)
{
  int i, control;

  for (i = 0; i < pulses; i++)
    {
      DEBUG_PRINTF("Pulse %02x Presc %08x Delay %08x Width %08x", i,
		   be32_to_cpu(PulseOut[i].Prescaler), 
		   be32_to_cpu(PulseOut[i].Delay), 
		   be32_to_cpu(PulseOut[i].Width));
      control = be32_to_cpu(PulseOut[i].Control);
      DEBUG_PRINTF(" Output %d", control & (1 << C_EVR_PULSE_OUT) ? 1 : 0);
      if (control & (1 << C_EVR_PULSE_POLARITY))
	DEBUG_PRINTF(" NEG");
      if (control & (1 << C_EVR_PULSE_MAP_RESET_ENA))
	DEBUG_PRINTF(" MAPRES");
      if (control & (1 << C_EVR_PULSE_MAP_SET_ENA))
	DEBUG_PRINTF(" MAPSET");
      if (control & (1 << C_EVR_PULSE_MAP_TRIG_ENA))
	DEBUG_PRINTF(" MAPTRIG");
      if (control & (1 << C_EVR_PULSE_ENA))
	DEBUG_PRINTF(" ENA");
      DEBUG_PRINTF("\n");
    }
}

int Evr::SetPulseProperties(int pulse, int polarity,
			  int map_reset_ena, int map_set_ena, int map_trigger_ena,
			  int enable)
{
  int result;

  if (pulse < 0 || pulse >= EVR_MAX_PULSES)
    return -1;

  result = be32_to_cpu(PulseOut[pulse].Control);

  /* 0 clears, 1 sets, others don't change */
  if (polarity == 0)
    result &= ~(1 << C_EVR_PULSE_POLARITY);
  if (polarity == 1)
    result |= (1 << C_EVR_PULSE_POLARITY);

  if (map_reset_ena == 0)
    result &= ~(1 << C_EVR_PULSE_MAP_RESET_ENA);
  if (map_reset_ena == 1)
    result |= (1 << C_EVR_PULSE_MAP_RESET_ENA);

  if (map_set_ena == 0)
    result &= ~(1 << C_EVR_PULSE_MAP_SET_ENA);
  if (map_set_ena == 1)
    result |= (1 << C_EVR_PULSE_MAP_SET_ENA);

  if (map_trigger_ena == 0)
    result &= ~(1 << C_EVR_PULSE_MAP_TRIG_ENA);
  if (map_trigger_ena == 1)
    result |= (1 << C_EVR_PULSE_MAP_TRIG_ENA);

  if (enable == 0)
    result &= ~(1 << C_EVR_PULSE_ENA);
  if (enable == 1)
    result |= (1 << C_EVR_PULSE_ENA);

#ifdef DEBUG
  DEBUG_PRINTF("PulseOut[%d].Control %08x\n", pulse, result);
#endif

  PulseOut[pulse].Control = be32_to_cpu(result);

  return 0;
}

int Evr::SetUnivOutMap(int output, int map)
{
  if (output < 0 || output >= EVR_MAX_UNIVOUT_MAP)
    return -1;

  UnivOutMap[output] = be16_to_cpu(map);
  return 0;
}

void Evr::DumpUnivOutMap(int outputs)
{
  int i;

  for (i = 0; i < outputs; i++)
    DEBUG_PRINTF("UnivOut[%d] %02x\n", i, be16_to_cpu(UnivOutMap[i]));
}

int Evr::SetFPOutMap(int output, int map)
{
  if (output < 0 || output >= EVR_MAX_FPOUT_MAP)
    return -1;

  FPOutMap[output] = be16_to_cpu(map);
  return 0;
}

void Evr::DumpFPOutMap(int outputs)
{
  int i;

  for (i = 0; i < outputs; i++)
    DEBUG_PRINTF("FPOut[%d] %02x\n", i, be16_to_cpu(FPOutMap[i]));
}

int Evr::SetTBOutMap(int output, int map)
{
  if (output < 0 || output >= EVR_MAX_TBOUT_MAP)
    return -1;

  TBOutMap[output] = be16_to_cpu(map);
  return 0;
}

void Evr::DumpTBOutMap(int outputs)
{
  int i;

  for (i = 0; i < outputs; i++)
    DEBUG_PRINTF("TBOut[%d] %02x\n", i, be16_to_cpu(TBOutMap[i]));
}


void Evr::IrqAssignHandler(int fd,
			 void (*handler)(int))
{
  struct sigaction act;
  int oflags;
  int result;

  act.sa_handler = handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  result = sigaction(SIGIO, &act, NULL);
  fcntl(fd, F_SETOWN, getpid());
  oflags = fcntl(fd, F_GETFL);
  fcntl(fd, F_SETFL, oflags | FASYNC);
  /* Now enable handler */
  IrqHandled(fd);
}

int Evr::IrqEnable(int mask)
{
  IrqEn = be32_to_cpu(mask);
  return be32_to_cpu(IrqEn);
}

int Evr::GetIrqFlags()
{
  return be32_to_cpu(IrqFlag);
}

int Evr::ClearIrqFlags(int mask)
{
  IrqFlag = be32_to_cpu(mask);
  return be32_to_cpu(IrqFlag);
}

void Evr::IrqHandled(int fd)
{
  ioctl(fd, EV_IOCIRQEN);
}

int Evr::SetPulseIrqMap(int map)
{
  PulseIrqMap = be32_to_cpu(map);
  return 0;
}

void Evr::ClearDiagCounters()
{
  DiagReset = 0xffffffff;
  DiagReset = 0x0;
}

void Evr::EnableDiagCounters(int enable)
{
  if (enable)
    DiagCE = 0xffffffff;
  else
    DiagCE = 0;
}

uint32_t Evr::GetDiagCounter(int idx)
{
  return be32_to_cpu(DiagCounter[idx]);
}

int Evr::UnivDlyEnable(int dlymod, int enable)
{
  uint32_t gpio;
  int sh = 0;

  switch (dlymod)
    {
    case 0:
      sh = 0;
      break;
    case 1:
      sh = 4;
      break;
    default:
      return -1;
    }
  
  /* Setup outputs for both slots */
  GPIODir = be32_to_cpu(((EVR_UNIV_DLY_DIN | EVR_UNIV_DLY_SCLK |
	    EVR_UNIV_DLY_LCLK | EVR_UNIV_DLY_DIS) |
	   ((EVR_UNIV_DLY_DIN | EVR_UNIV_DLY_SCLK |
	     EVR_UNIV_DLY_LCLK | EVR_UNIV_DLY_DIS) << 4)));
  gpio = be32_to_cpu(GPIOOut) & ~(EVR_UNIV_DLY_DIS << sh);
  if (!enable)
    gpio |= (EVR_UNIV_DLY_DIS << sh);
  GPIOOut = be32_to_cpu(gpio);

  return 0;
}

int Evr::UnivDlySetDelay(int dlymod, int dly0, int dly1)
{
  uint32_t gpio;
  int sh = 0;
  int sd;
  int sr, i, din, sclk, lclk, dbit;

  switch (dlymod)
    {
    case 0:
      sh = 0;
      break;
    case 1:
      sh = 4;
      break;
    default:
      return -1;
    }
  
  din = EVR_UNIV_DLY_DIN << sh;
  sclk = EVR_UNIV_DLY_SCLK << sh;
  lclk = EVR_UNIV_DLY_LCLK << sh;

  gpio = be32_to_cpu(GPIOOut) & ~((EVR_UNIV_DLY_DIN | EVR_UNIV_DLY_SCLK |
					EVR_UNIV_DLY_LCLK) | 
				       ((EVR_UNIV_DLY_DIN | EVR_UNIV_DLY_SCLK |
					 EVR_UNIV_DLY_LCLK) << 4));
  /* Limit delay values */
  dly0 &= 0x03ff;
  dly1 &= 0x03ff;

  /* We have to shift in the bits in following order:
     DA7, DA6, DA5, DA4, DA3, DA2, DA1, DA0,
     DB3, DB2, DB1, DB0, LENA, 0, DA9, DA8,
     LENB, 0, DB9, DB8, DB7, DB6, DB5, DB4 */

  sd = ((dly1 & 0x0ff) << 16) |
    ((dly0 & 0x00f) << 12) | (dly1 & 0x300) | 
    (dly0 >> 4);

  sr = sd;
  for (i = 24; i; i--)
    {
      dbit = 0;
      if (sr & 0x00800000)
	dbit = din;
      GPIOOut = be32_to_cpu(gpio | dbit);
      GPIOOut = be32_to_cpu(gpio | dbit | sclk);
      GPIOOut = be32_to_cpu(gpio | dbit);
      sr <<= 1;
    }

  GPIOOut = be32_to_cpu(gpio | lclk);
  GPIOOut = be32_to_cpu(gpio);

  /* Latch enables active */
  sr = sd | 0x000880;
  for (i = 24; i; i--)
    {
      dbit = 0;
      if (sr & 0x00800000)
	dbit = din;
      GPIOOut = be32_to_cpu(gpio | dbit);
      GPIOOut = be32_to_cpu(gpio | dbit | sclk);
      GPIOOut = be32_to_cpu(gpio | dbit);
      sr <<= 1;
    }

  GPIOOut = be32_to_cpu(gpio | lclk);
  GPIOOut = be32_to_cpu(gpio);

  sr = sd;
  for (i = 24; i; i--)
    {
      dbit = 0;
      if (sr & 0x00800000)
	dbit = din;
      GPIOOut = be32_to_cpu(gpio | dbit);
      GPIOOut = be32_to_cpu(gpio | dbit | sclk);
      GPIOOut = be32_to_cpu(gpio | dbit);
      sr <<= 1;
    }

  GPIOOut = be32_to_cpu(gpio | lclk);
  GPIOOut = be32_to_cpu(gpio);

  return 0;
}

void Evr::DumpHex()
{
  uint32_t *p = (uint32_t *) this;
  int i,j;

  for (i = 0; i < 0x600; i += 0x20)
    {
      printf("%08x: ", i);
      for (j = 0; j < 8; j++)
	printf("%08x ", be32_to_cpu(*p++));
      printf("\n");
    }
}

int Evr::SetFracDiv(int fracdiv)
{
  return FracDiv = be32_to_cpu(fracdiv);
}

int Evr::GetFracDiv()
{
  return be32_to_cpu(FracDiv);
}

int Evr::SetDBufMode(int enable)
{
  if (enable)
    DataBufControl = be32_to_cpu(1 << C_EVR_DATABUF_MODE);
  else
    DataBufControl = 0;

  return GetDBufStatus();
}

int Evr::GetDBufStatus()
{
  return be32_to_cpu(DataBufControl);
}

int Evr::ReceiveDBuf(int enable)
{
  if (enable)
    DataBufControl |= be32_to_cpu(1 << C_EVR_DATABUF_LOAD);
  else
    DataBufControl |= be32_to_cpu(1 << C_EVR_DATABUF_STOP);

  return GetDBufStatus();
}

int Evr::GetDBuf(char *dbuf, int size)
{
  int stat, rxsize;

  stat = GetDBufStatus();
  /* Check that DBUF mode enabled */
  if (!(stat & (1 << C_EVR_DATABUF_MODE)))
    return -1;
  /* Check that transfer is completed */
  if (!(stat & (1 << C_EVR_DATABUF_RXREADY)))
    return -1;

  rxsize = stat & (EVR_MAX_BUFFER-1);

  if (size < rxsize)
    return -1;

  memcpy((void *) dbuf, (void *) &Databuf[0], rxsize);

  if (stat & (1 << C_EVR_DATABUF_CHECKSUM))
    return -1;

  return rxsize;
}

int Evr::SetTimestampDivider(int div)
{
  EvCntPresc = be32_to_cpu(div);

  return be32_to_cpu(EvCntPresc);
}

int Evr::GetTimestampCounter()
{
  return be32_to_cpu(TimestampEventCounter);
}

int Evr::GetSecondsCounter()
{
  return be32_to_cpu(SecondsCounter);
}

int Evr::GetTimestampLatch()
{
  return be32_to_cpu(TimestampLatch);
}

int Evr::GetSecondsLatch()
{
  return be32_to_cpu(SecondsLatch);
}

int Evr::SetTimestampDBus(int enable)
{
  int ctrl;

  ctrl = be32_to_cpu(Control);
  if (enable)
    ctrl |= (1 << C_EVR_CTRL_TS_CLOCK_DBUS);
  else
    ctrl &= ~(1 << C_EVR_CTRL_TS_CLOCK_DBUS);
  Control = be32_to_cpu(ctrl);

  return be32_to_cpu(Control);  
}

int Evr::SetPrescaler(int presc, int div)
{
  if (presc >= 0 && presc < EVR_MAX_PRESCALERS)
    {
      Prescaler[presc] = be32_to_cpu(div);

      return be32_to_cpu(Prescaler[presc]);
    }
  return -1;
}

int Evr::SetExtEvent(int ttlin, int code, int enable)
{
  int fpctrl;

  if (ttlin < 0 || ttlin > EVR_MAX_FPIN_MAP)
    return -1;

  fpctrl = be32_to_cpu(FPInMap[ttlin]);
  if (code >= 0 && code <= EVR_MAX_EVENT_CODE)
    {
      fpctrl &= ~(EVR_MAX_EVENT_CODE << C_EVR_FPIN_EXTEVENT_BASE);
      fpctrl |= code << C_EVR_FPIN_EXTEVENT_BASE;
    }
  fpctrl &= ~(1 << C_EVR_FPIN_EXT_ENABLE);
  if (enable)
    fpctrl |= (1 << C_EVR_FPIN_EXT_ENABLE);

  FPInMap[ttlin] = be32_to_cpu(fpctrl);
  if (FPInMap[ttlin] == be32_to_cpu(fpctrl))
    return 0;
  return -1;
}

int Evr::SetBackEvent(int ttlin, int code, int enable)
{
  int fpctrl;

  if (ttlin < 0 || ttlin > EVR_MAX_FPIN_MAP)
    return -1;

  fpctrl = be32_to_cpu(FPInMap[ttlin]);
  if (code >= 0 && code <= EVR_MAX_EVENT_CODE)
    {
      fpctrl &= ~(EVR_MAX_EVENT_CODE << C_EVR_FPIN_BACKEVENT_BASE);
      fpctrl |= code << C_EVR_FPIN_BACKEVENT_BASE;
    }
  fpctrl &= ~(1 << C_EVR_FPIN_BACKEV_ENABLE);
  if (enable)
    fpctrl |= (1 << C_EVR_FPIN_BACKEV_ENABLE);

  FPInMap[ttlin] = be32_to_cpu(fpctrl);
  if (FPInMap[ttlin] == be32_to_cpu(fpctrl))
    return 0;
  return -1;
}

int Evr::SetBackDBus(int ttlin, int dbus)
{
  int fpctrl;

  if (ttlin < 0 || ttlin > EVR_MAX_FPIN_MAP)
    return -1;

  if (dbus < 0 || dbus > 255)
    return -1;

  fpctrl = be32_to_cpu(FPInMap[ttlin]);
  fpctrl &= ~(255 << C_EVR_FPIN_BACKDBUS_BASE);
  fpctrl |= dbus << C_EVR_FPIN_BACKDBUS_BASE;

  FPInMap[ttlin] = be32_to_cpu(fpctrl);
  if (FPInMap[ttlin] == be32_to_cpu(fpctrl))
    return 0;
  return -1;

}

int Evr::SetTxDBufMode(int enable)
{
  if (enable)
    TxDataBufControl = be32_to_cpu(1 << C_EVR_TXDATABUF_MODE);
  else
    TxDataBufControl = 0;

  return GetTxDBufStatus();
}

int Evr::GetTxDBufStatus()
{
  return be32_to_cpu(TxDataBufControl);
}

int Evr::SendTxDBuf(char *dbuf, int size)
{
  int stat;

  stat = GetTxDBufStatus();
  //  printf("EvgSendDBuf: stat %08x\n", stat);
  /* Check that DBUF mode enabled */
  if (!(stat & (1 << C_EVR_TXDATABUF_MODE)))
    return -1;
  /* Check that previous transfer is completed */
  if (!(stat & (1 << C_EVR_TXDATABUF_COMPLETE)))
    return -1;
  /* Check that size is valid */
  if (size & 3 || size > EVR_MAX_BUFFER || size < 4)
    return -1;

  memcpy((void *) &TxDatabuf[0], (void *) dbuf, size);

  /* Enable and set size */
  stat &= ~((EVR_MAX_BUFFER-1) | (1 << C_EVR_TXDATABUF_TRIGGER));
  stat |= (1 << C_EVR_TXDATABUF_ENA) | size;
  //  printf("EvgSendDBuf: stat %08x\n", stat);
  TxDataBufControl = be32_to_cpu(stat);
  //  printf("EvgSendDBuf: stat %08x\n", be32_to_cpu(DataBufControl));

  /* Trigger */
  TxDataBufControl = be32_to_cpu(stat | (1 << C_EVR_TXDATABUF_TRIGGER));
  //  printf("EvgSendDBuf: stat %08x\n", be32_to_cpu(DataBufControl));

  return size;
}

int Evr::GetFormFactor()
{
  int stat;
  
  stat = be32_to_cpu(FPGAVersion);
  return ((stat >> 24) & 0x0f);
}
