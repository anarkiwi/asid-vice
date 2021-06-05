/*
 * soundasid.c - Implementation of the asid protocol midi sound device.
 *
 * Written by
 *  aTc <aTc@k-n-p.org>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#define __LINUX_ALSASEQ__
#include "RtMidi.cpp"

extern "C" {


#include "vice.h"
#include "debug.h"
#include "log.h"

#include <stdio.h>

#include "sound.h"
#include "types.h"

const uint8_t regmap[]={0,1,2,3,5,6,7,8,9,10,12,13,14,15,16,17,19,20,21,22,23,24,4,11,18,25,26,27};
uint8_t sid_register[sizeof(regmap)];
bool sid_modified[sizeof(regmap)];
bool sid_modified_flag = false;

RtMidiOut *midiout = NULL;
std::vector<unsigned char> message;


  static int asid_init(const char *param, int *speed,
		       int *fragsize, int *fragnr, int *channels)
  {
    /* No stereo capability. */
    *channels = 1;
    int i,nports,asidport=0;

    midiout = new RtMidiOut();
    nports=midiout->getPortCount();

    if (nports == 0) {
      log_message(LOG_DEFAULT, "No MIDI ports available");
      return -1;
    }

    log_message(LOG_DEFAULT,"asid open, available ports:");
    for(i=0;i<nports;i++)
      log_message(LOG_DEFAULT,"Port %d : %s",i,midiout->getPortName(i).c_str());

    asidport=atoi(param);
    if (asidport < 0 or asidport > (nports - 1)) {
      log_message(LOG_DEFAULT, "invalid MIDI port");
      return -1;
    }

    log_message(LOG_DEFAULT,"Using port: %d %s",asidport,midiout->getPortName(asidport).c_str());
    midiout->openPort(asidport);

    memset(sid_register, 0, sizeof(sid_register));
    memset(sid_modified, false, sizeof(sid_modified));
    sid_modified_flag = false;
    message.clear();
    message.push_back(0xf0);
    message.push_back(0x2d);
    message.push_back(0x4c);
    message.push_back(0xf7);
    midiout->sendMessage(&message); //start sid play mode

    return 0;
  }

  static int asid_write(int16_t *pbuf, size_t nr)
  {
    return 0;
  }

  static void _set_reg(uint8_t reg, uint8_t byte) {
    if (sid_register[reg] != byte) {
       sid_register[reg] = byte;
       sid_modified[reg] = true;
       sid_modified_flag = true;
    }
  }

  static void _set_sec_reg(uint8_t sec_reg, uint8_t reg, uint8_t byte) {
    if (sid_modified[sec_reg]) {
       _set_reg(reg, sid_register[sec_reg]);
    }
    _set_reg(sec_reg, byte);
  }

  static int asid_dump(uint16_t addr, uint8_t byte, CLOCK clks)
  {
    int reg = addr & 0x1f;

    if (sid_modified[reg]) {
	switch(reg)
	  {
	  case 0x04:
            _set_sec_reg(0x19, reg, byte);
	    break;
	  case 0x0b:
            _set_sec_reg(0x1a, reg, byte);
	    break;
	  case 0x12:
            _set_sec_reg(0x1b, reg, byte);
	    break;
	  default:
            _set_reg(reg, byte);
	  }
    } else {
       _set_reg(reg, byte);
    }

    return 0;
  }

  static int asid_flush(char *state)
  {
    if (!sid_modified_flag) {
      return 0;
    }

    uint8_t i,j;
    unsigned int mask=0;
    unsigned int msb=0;

    message.clear();
    message.push_back(0xf0);
    message.push_back(0x2d);
    message.push_back(0x4e);
    // set bits in mask for each register that has been written to
    // write last bit of each register into msb
    for(i=0;i<sizeof(regmap);i++)
      {
	j=regmap[i];
	if(sid_modified[j]!=0)
	  {
	    mask=mask | (1<<i);
	  }
	if(sid_register[j]>0x7f)
	  {
	    msb=msb | (1<<i);
	  }
      }
    message.push_back(mask & 0x7f);
    message.push_back((mask>>7)&0x7f);
    message.push_back((mask>>14)&0x7f);
    message.push_back((mask>>21)&0x7f);
    message.push_back(msb & 0x7f);
    message.push_back((msb>>7)&0x7f);
    message.push_back((msb>>14)&0x7f);
    message.push_back((msb>>21)&0x7f);
    for(i=0;i<sizeof(regmap);i++)
      {
	j=regmap[i];
	if(sid_modified[j]!=0)
	  {
	    message.push_back(sid_register[j]&0x7f);
	  }
      }
    message.push_back(0xf7);
    midiout->sendMessage(&message);
    memset(sid_modified, false, sizeof(sid_modified));
    sid_modified_flag = false;
    return 0;
  }

  static void asid_close(void)
  {
    message.clear();
    message.push_back(0xf0);
    message.push_back(0x2d);
    message.push_back(0x4d);
    message.push_back(0xf7);
    midiout->sendMessage(&message);
    delete midiout;
  }

  static sound_device_t asid_device =
    {
     "asid",
     asid_init,
     asid_write,
     asid_dump,
     asid_flush,
     NULL,
     asid_close,
     NULL,
     NULL,
     0
    };

  int sound_init_asid_device(void)
  {
    return sound_register_device(&asid_device);
  }

} // extern "C"
