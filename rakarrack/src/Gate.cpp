// Based in gate_1410.c LADSPA Swh-plugins


/*
  rakarrack - a guitar effects software

 Gate.C  -  Noise Gate Effect
 Based on Steve Harris LADSPA gate.

  Copyright (C) 2008 Josep Andreu
  Author: Josep Andreu

 This program is free software; you can redistribute it and/or modify
 it under the terms of version 2 of the GNU General Public License
 as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License (version 2) for more details.

 You should have received a copy of the GNU General Public License
 (version2)  along with this program; if not, write to the Free Software
 Foundation,
 Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include <math.h>
#include "Gate.h"


Gate::Gate (Parameters *param, float * efxoutl_, float * efxoutr_)
	:Effect(None)
{
	this->param = param;
    efxoutl = efxoutl_;
    efxoutr = efxoutr_;


    lpfl = new AnalogFilter (param,2, 22000, 1, 0);
    lpfr = new AnalogFilter (param,2, 22000, 1, 0);
    hpfl = new AnalogFilter (param,3, 20, 1, 0);
    hpfr = new AnalogFilter (param,3, 20, 1, 0);

    env = 0.0;
    gate = 0.0;
    fs = fSAMPLE_RATE;
    state = CLOSED;
    hold_count = 0;

}

Gate::~Gate ()
{
}



void
Gate::cleanup ()
{
    lpfl->cleanup ();
    hpfl->cleanup ();
    lpfr->cleanup ();
    hpfr->cleanup ();
}




void
Gate::setlpf (int value)
{
    Plpf = value;
    float fr = (float)Plpf;
    lpfl->setfreq (fr);
    lpfr->setfreq (fr);
};

void
Gate::sethpf (int value)
{
    Phpf = value;
    float fr = (float)Phpf;
    hpfl->setfreq (fr);
    hpfr->setfreq (fr);
};


void
Gate::Gate_Change (int np, int value)
{

    switch (np) {

    case 0:
        Pthreshold = value;
        t_level = dB2rap ((float)Pthreshold);
        break;
    case 1:
        Prange = value;
        cut = dB2rap ((float)Prange);
        break;
    case 2:
        Pattack = value;
        a_rate = 1000.0f / ((float)Pattack * fs);
        break;
    case 3:
        Pdecay = value;
        d_rate = 1000.0f / ((float)Pdecay * fs);
        break;
    case 4:
        setlpf(value);
        break;
    case 5:
        sethpf(value);
        break;
    case 6:
        Phold = value;
        hold = (float)Phold;
        break;

    }


}

int
Gate::getpar (int np)
{

    switch (np)

    {
    case 0:
        return (Pthreshold);
        break;
    case 1:
        return (Prange);
        break;
    case 2:
        return (Pattack);
        break;
    case 3:
        return (Pdecay);
        break;
    case 4:
        return (Plpf);
        break;
    case 5:
        return (Phpf);
        break;
    case 6:
        return (Phold);
        break;

    }

    return (0);

}


void
Gate::Gate_Change_Preset (int npreset)
{

    const int PRESET_SIZE = 7;
    const int NUM_PRESETS = 3;
    int presets[NUM_PRESETS][PRESET_SIZE] = {
        //0dB
        {0, 0, 1, 2, 6703, 76, 2},
        //-10dB
        {0, -10, 1, 2, 6703, 76, 2},
        //-20dB
        {0, -20, 1, 2, 6703, 76, 2}
    };

    if(npreset>NUM_PRESETS-1) {

        Fpre->ReadPreset(16,npreset-NUM_PRESETS+1);
        for (int n = 0; n < PRESET_SIZE; n++)
            Gate_Change(n, pdata[n]);
    } else {
        for (int n = 0; n < PRESET_SIZE; n++)
            Gate_Change (n, presets[npreset][n]);
    }

}



void
Gate::out (float *efxoutl, float *efxoutr)
{


    int i;
    float sum;


    lpfl->filterout (efxoutl);
    hpfl->filterout (efxoutl);
    lpfr->filterout (efxoutr);
    hpfr->filterout (efxoutr);


    for (i = 0; i < param->PERIOD; i++) {

        sum = fabsf (efxoutl[i]) + fabsf (efxoutr[i]);


        if (sum > env)
            env = sum;
        else
            env = sum * ENV_TR + env * (1.0f - ENV_TR);

        if (state == CLOSED) {
            if (env >= t_level)
                state = OPENING;
        } else if (state == OPENING) {
            gate += a_rate;
            if (gate >= 1.0) {
                gate = 1.0f;
                state = OPEN;
                hold_count = lrintf (hold * fs * 0.001f);
            }
        } else if (state == OPEN) {
            if (hold_count <= 0) {
                if (env < t_level) {
                    state = CLOSING;
                }
            } else
                hold_count--;

        } else if (state == CLOSING) {
            gate -= d_rate;
            if (env >= t_level)
                state = OPENING;
            else if (gate <= 0.0) {
                gate = 0.0;
                state = CLOSED;
            }
        }

        efxoutl[i] *= (cut * (1.0f - gate) + gate);
        efxoutr[i] *= (cut * (1.0f - gate) + gate);

    }



};

		
void
Gate::processReplacing (float **inputs,
								float **outputs,
								int sampleFrames)
{


    int i;
    float sum;
	param->PERIOD = sampleFrames;
	param->fPERIOD = sampleFrames;
	memcpy(outputs[0], inputs[0], sampleFrames*sizeof(float));
	memcpy(outputs[1], inputs[1], sampleFrames*sizeof(float));

    lpfl->filterout (outputs[0]);
    hpfl->filterout (outputs[0]);
    lpfr->filterout (outputs[1]);
    hpfr->filterout (outputs[1]);


    for (i = 0; i < param->PERIOD; i++) {

        sum = fabsf (outputs[0][i]) + fabsf (outputs[1][i]);


        if (sum > env)
            env = sum;
        else
            env = sum * ENV_TR + env * (1.0f - ENV_TR);

        if (state == CLOSED) {
            if (env >= t_level)
                state = OPENING;
        } else if (state == OPENING) {
            gate += a_rate;
            if (gate >= 1.0) {
                gate = 1.0f;
                state = OPEN;
                hold_count = lrintf (hold * fs * 0.001f);
            }
        } else if (state == OPEN) {
            if (hold_count <= 0) {
                if (env < t_level) {
                    state = CLOSING;
                }
            } else
                hold_count--;

        } else if (state == CLOSING) {
            gate -= d_rate;
            if (env >= t_level)
                state = OPENING;
            else if (gate <= 0.0) {
                gate = 0.0;
                state = CLOSED;
            }
        }

        outputs[0][i] *= (cut * (1.0f - gate) + gate);
        outputs[1][i] *= (cut * (1.0f - gate) + gate);

    }



};