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
//#define __WINDOWS_MM__
#include "RtMidi.cpp"

extern "C" {


#include "vice.h"
#include "debug.h"
#include "log.h"

#include <stdio.h>

#include "sound.h"
#include "types.h"

int regmap[]={0,1,2,3,5,6,7,8,9,10,12,13,14,15,16,17,19,20,21,22,23,24,4,11,18,25,26,27};
RtMidiOut *midiout;
unsigned char sid_register[28];
unsigned char sid_modified[28];
std::vector<unsigned char> message;

#define BYTE uint8_t
#define WORD uint16_t
#define SWORD int16_t



static int asid_init(const char *param, int *speed,
		     int *fragsize, int *fragnr, int *channels)
{
    /* No stereo capability. */
    *channels = 1;
    int i,nports,asidport=0;
    log_message(LOG_DEFAULT,"asid open, available ports:");
     
    asidport=atoi(param);
    midiout = new RtMidiOut();
    nports=midiout->getPortCount();
    for(i=0;i<nports;i++)
        log_message(LOG_DEFAULT,"Port %d : %s",i,midiout->getPortName(i).c_str());

    log_message(LOG_DEFAULT,"Using port: %d %s",asidport,midiout->getPortName(asidport).c_str());

	midiout->openPort(asidport);
    for (i=0;i<28;i++)
    {
	    sid_register[i]=0;
	    sid_modified[i]=0;
    }
    	 
	message.clear();
	message.push_back(0xf0);
	message.push_back(0x2d);
	message.push_back(0x4c);
	message.push_back(0xf7);
	midiout->sendMessage(&message); //start sid play mode

    return 0;
}

static int asid_write(SWORD *pbuf, size_t nr)
{
    return 0;
}

static int asid_dump(WORD addr, BYTE byte, CLOCK clks)
{
    int reg,data;
    
    reg=addr & 0x1f;
    data=byte;
    if(sid_modified[reg]==0)
    {
        sid_register[reg]=data & 0xff;
		sid_modified[reg]++;
    }
    else
    {
		switch(reg)
		{
		    case 0x04:
			    if(sid_modified[0x19]!=0) sid_register[0x04]=sid_register[0x19]; //if already written to secondary,move back to original one 
		    	sid_register[0x19]=data & 0xff;
		    	sid_modified[0x19]++;
			break;
		    case 0x0b:
			    if(sid_modified[0x1a]!=0) sid_register[0x0b]=sid_register[0x1a]; 
			    sid_register[0x1a]=data & 0xff;
 			    sid_modified[0x1a]++;
			break;
		    case 0x12:
			    if(sid_modified[0x1b]!=0) sid_register[0x12]=sid_register[0x1b]; 
		    	sid_register[0x1b]=data & 0xff;
		    	sid_modified[0x1b]++;
			break;

		    default:
		     	sid_register[reg]=data & 0xff;
			sid_modified[reg]++;
		}
	}

    
    return 0;
}

static int asid_flush(char *state)
{
    int i,j;
	unsigned int mask=0;
	unsigned int msb=0;

	message.clear();
	message.push_back(0xf0);
	message.push_back(0x2d);
	message.push_back(0x4e);
	// set bits in mask for each register that has been written to
	// write last bit of each register into msb
	for(i=0;i<28;i++)
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
	for(i=0;i<28;i++)
	{
		j=regmap[i];
		if(sid_modified[j]!=0)
		{
			message.push_back(sid_register[j]&0x7f);
		}
	}
	message.push_back(0xf7);
	midiout->sendMessage(&message);
	for(i=0;i<28;i++)
	{
		sid_modified[i]=0;
	}


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
