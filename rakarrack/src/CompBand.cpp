/*

  CompBand.C - 4 Bands Compressor

  Using Compressor and AnalogFilters by other authors.

  Based on artscompressor.cc by Matthias Kretz <kretz@kde.org>
  Stefan Westerfeld <stefan@space.twc.de>

  Modified by Ryan Billing & Josep Andreu

  ZynAddSubFX - a software synthesizer
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "CompBand.h"

/*
 * Waveshape (this is called by OscilGen::waveshape and Distorsion::process)
 */



CompBand::CompBand (Parameters *param, float * efxoutl_, float * efxoutr_)
	:Effect(WetDry)
{
	this->param = param;
    efxoutl = efxoutl_;
    efxoutr = efxoutr_;
	param->PERIOD = 44102;
    lowl = (float *) malloc (sizeof (float) * param->PERIOD+100);
    lowr = (float *) malloc (sizeof (float) * param->PERIOD+100);
    midll = (float *) malloc (sizeof (float) * param->PERIOD+100);
    midlr = (float *) malloc (sizeof (float) * param->PERIOD+100);
    midhl = (float *) malloc (sizeof (float) * param->PERIOD+100);
    midhr = (float *) malloc (sizeof (float) * param->PERIOD+100);
    highl = (float *) malloc (sizeof (float) * param->PERIOD+100);
    highr = (float *) malloc (sizeof (float) * param->PERIOD+100);


    lpf1l = new AnalogFilter (param,2, 500.0f,.7071f, 0);
    lpf1r = new AnalogFilter (param,2, 500.0f,.7071f, 0);
    hpf1l = new AnalogFilter (param,3, 500.0f,.7071f, 0);
    hpf1r = new AnalogFilter (param,3, 500.0f,.7071f, 0);
    lpf2l = new AnalogFilter (param,2, 2500.0f,.7071f, 0);
    lpf2r = new AnalogFilter (param,2, 2500.0f,.7071f, 0);
    hpf2l = new AnalogFilter (param,3, 2500.0f,.7071f, 0);
    hpf2r = new AnalogFilter (param,3, 2500.0f,.7071f, 0);
    lpf3l = new AnalogFilter (param,2, 5000.0f,.7071f, 0);
    lpf3r = new AnalogFilter (param,2, 5000.0f,.7071f, 0);
    hpf3l = new AnalogFilter (param,3, 5000.0f,.7071f, 0);
    hpf3r = new AnalogFilter (param,3, 5000.0f,.7071f, 0);


    CL = new Compressor(param,efxoutl,efxoutr);
    CML = new Compressor(param,efxoutl,efxoutr);
    CMH = new Compressor(param,efxoutl,efxoutr);
    CH = new Compressor(param,efxoutl,efxoutr);

    CL->Compressor_Change_Preset(0,5);
    CML->Compressor_Change_Preset(0,5);
    CMH->Compressor_Change_Preset(0,5);
    CH->Compressor_Change_Preset(0,5);


    //default values
    Ppreset = 0;
    Pvolume = 50;

    setpreset (Ppreset);
    cleanup ();
};

CompBand::~CompBand ()
{
};

/*
 * Cleanup the effect
 */
void
CompBand::cleanup ()
{
    lpf1l->cleanup ();
    hpf1l->cleanup ();
    lpf1r->cleanup ();
    hpf1r->cleanup ();
    lpf2l->cleanup ();
    hpf2l->cleanup ();
    lpf2r->cleanup ();
    hpf2r->cleanup ();
    lpf3l->cleanup ();
    hpf3l->cleanup ();
    lpf3r->cleanup ();
    hpf3r->cleanup ();
    CL->cleanup();
    CML->cleanup();
    CMH->cleanup();
    CH->cleanup();

};
/*
 * Effect output
 */
void
CompBand::out (float * smpsl, float * smpsr)
{
    int i;


    memcpy(lowl,smpsl,sizeof(float) * param->PERIOD);
    memcpy(midll,smpsl,sizeof(float) * param->PERIOD);
    memcpy(midhl,smpsl,sizeof(float) * param->PERIOD);
    memcpy(highl,smpsl,sizeof(float) * param->PERIOD);

    lpf1l->filterout(lowl);
    hpf1l->filterout(midll);
    lpf2l->filterout(midll);
    hpf2l->filterout(midhl);
    lpf3l->filterout(midhl);
    hpf3l->filterout(highl);

    memcpy(lowr,smpsr,sizeof(float) * param->PERIOD);
    memcpy(midlr,smpsr,sizeof(float) * param->PERIOD);
    memcpy(midhr,smpsr,sizeof(float) * param->PERIOD);
    memcpy(highr,smpsr,sizeof(float) * param->PERIOD);

    lpf1r->filterout(lowr);
    hpf1r->filterout(midlr);
    lpf2r->filterout(midlr);
    hpf2r->filterout(midhr);
    lpf3r->filterout(midhr);
    hpf3r->filterout(highr);


    CL->out(lowl,lowr);
    CML->out(midll,midlr);
    CMH->out(midhl,midhr);
    CH->out(highl,highr);


    for (i = 0; i < param->PERIOD; i++) {
        efxoutl[i]=(lowl[i]+midll[i]+midhl[i]+highl[i])*level;
        efxoutr[i]=(lowr[i]+midlr[i]+midhr[i]+highr[i])*level;
    }



};

void
CompBand::processReplacing (float **inputs,
								float **outputs,
								int sampleFrames)
{
    int i;
	param->PERIOD = sampleFrames;
	param->fPERIOD = sampleFrames;

    memcpy(lowl,inputs[0],sizeof(float) * param->PERIOD);
    memcpy(midll,inputs[0],sizeof(float) * param->PERIOD);
    memcpy(midhl,inputs[0],sizeof(float) * param->PERIOD);
    memcpy(highl,inputs[0],sizeof(float) * param->PERIOD);

    lpf1l->filterout(lowl);
    hpf1l->filterout(midll);
    lpf2l->filterout(midll);
    hpf2l->filterout(midhl);
    lpf3l->filterout(midhl);
    hpf3l->filterout(highl);

    memcpy(lowr,inputs[1],sizeof(float) * param->PERIOD);
    memcpy(midlr,inputs[1],sizeof(float) * param->PERIOD);
    memcpy(midhr,inputs[1],sizeof(float) * param->PERIOD);
    memcpy(highr,inputs[1],sizeof(float) * param->PERIOD);

    lpf1r->filterout(lowr);
    hpf1r->filterout(midlr);
    lpf2r->filterout(midlr);
    hpf2r->filterout(midhr);
    lpf3r->filterout(midhr);
    hpf3r->filterout(highr);

	float *lows[2] = {lowl, lowr};
	float *midls[2] = {midll, midlr};
	float *midhs[2] = {midhl, midhr};
	float *highes[2] = {highl, highr};
    CL->processReplacing(lows,lows, param->PERIOD);
    CML->processReplacing(midls,midls, param->PERIOD);
    CMH->processReplacing(midhs,midhs, param->PERIOD);
    CH->processReplacing(highes,highes, param->PERIOD);


    for (i = 0; i < param->PERIOD; i++) {
        outputs[0][i]=(lowl[i]+midll[i]+midhl[i]+highl[i])*level;
        outputs[1][i]=(lowr[i]+midlr[i]+midhr[i]+highr[i])*level;
    }



};


/*
 * Parameter control
 */
void
CompBand::setvolume (int value)
{
    Pvolume = value;
    outvolume = (float)Pvolume / 128.0f;

};


void
CompBand::setlevel (int value)
{
    Plevel = value;
    level = dB2rap (60.0f * (float)value / 127.0f - 36.0f);


};



void
CompBand::setratio(int ch, int value)
{

    switch(ch) {
    case 0:
        CL->Compressor_Change(2,value);
        break;
    case 1:
        CML->Compressor_Change(2,value);
        break;
    case 2:
        CMH->Compressor_Change(2,value);
        break;
    case 3:
        CH->Compressor_Change(2,value);
        break;
    }
}


void
CompBand::setthres(int ch, int value)
{

    switch(ch) {
    case 0:
        CL->Compressor_Change(1,value);
        break;
    case 1:
        CML->Compressor_Change(1,value);
        break;
    case 2:
        CMH->Compressor_Change(1,value);
        break;
    case 3:
        CH->Compressor_Change(1,value);
        break;
    }
}




void
CompBand::setCross1 (int value)
{
    Cross1 = value;
    lpf1l->setfreq ((float)value);
    lpf1r->setfreq ((float)value);
    hpf1l->setfreq ((float)value);
    hpf1r->setfreq ((float)value);

};

void
CompBand::setCross2 (int value)
{
    Cross2 = value;
    hpf2l->setfreq ((float)value);
    hpf2r->setfreq ((float)value);
    lpf2l->setfreq ((float)value);
    lpf2r->setfreq ((float)value);

};

void
CompBand::setCross3 (int value)
{
    Cross3 = value;
    hpf3l->setfreq ((float)value);
    hpf3r->setfreq ((float)value);
    lpf3l->setfreq ((float)value);
    lpf3r->setfreq ((float)value);

};


void
CompBand::setpreset (int npreset)
{
    const int PRESET_SIZE = 13;
    const int NUM_PRESETS = 3;
    int presets[NUM_PRESETS][PRESET_SIZE] = {
        //Good Start
        {0, 16, 16, 16, 16, 0, 0, 0, 0, 1000, 5000, 10000, 48},

        //Loudness
        {0, 16, 2, 2, 4, -16, 24, 24, -8, 140, 1000, 5000, 48},

        //Loudness 2
        {64, 16, 2, 2, 2, -32, 24, 24, 24, 100, 1000, 5000, 48}

    };

    if(npreset>NUM_PRESETS-1) {
        Fpre->ReadPreset(43,npreset-NUM_PRESETS+1);
        for (int n = 0; n < PRESET_SIZE; n++)
            changepar (n, pdata[n]);
    } else {
        for (int n = 0; n < PRESET_SIZE; n++)
            changepar (n, presets[npreset][n]);
    }
    Ppreset = npreset;
    cleanup ();
};


void
CompBand::changepar (int npar, int value)
{
    switch (npar) {
    case 0:
        setvolume (value);
        break;
    case 1:
        PLratio = value;
        setratio(0,value);
        break;
    case 2:
        PMLratio = value;
        setratio(1,value);
        break;
    case 3:
        PMHratio = value;
        setratio(2,value);
        break;
    case 4:
        PHratio = value;
        setratio(3,value);
        break;
    case 5:
        PLthres = value;
        setthres(0,value);
        break;
    case 6:
        PMLthres = value;
        setthres(1,value);
        break;
    case 7:
        PMHthres = value;
        setthres(2,value);
        break;
    case 8:
        PHthres = value;
        setthres(3,value);
        break;
    case 9:
        setCross1 (value);
        break;
    case 10:
        setCross2 (value);
        break;
    case 11:
        setCross3(value);
        break;
    case 12:
        setlevel(value);
        break;


    };
};

int
CompBand::getpar (int npar)
{
    switch (npar) {
    case 0:
        return (Pvolume);
        break;
    case 1:
        return (PLratio);
        break;
    case 2:
        return (PMLratio);
        break;
    case 3:
        return (PMHratio);
        break;
    case 4:
        return (PHratio);
        break;
    case 5:
        return (PLthres);
        break;
    case 6:
        return (PMLthres);
        break;
    case 7:
        return (PMHthres);
        break;
    case 8:
        return (PHthres);
        break;
    case 9:
        return (Cross1);
        break;
    case 10:
        return (Cross2);
        break;
    case 11:
        return (Cross3);
        break;
    case 12:
        return (Plevel);
        break;
    };
    return (0);			//in case of bogus parameter number
};

