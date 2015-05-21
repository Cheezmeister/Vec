/*
 * sfxd. Original notice for sfxr follows.
 */

/*
   Copyright (c) 2007 Tomas Pettersson <drpetter@gmail.com>

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.

*/

#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>

#include "SDL.h"

#include "sfxd.h"

#define rnd(n) (rand()%(n+1))
#define PI 3.14159265f

// SFXD Globals
const int MAX_CHANNELS = 12;
int num_channels = 0;
float master_vol = 0.05f;
bool mute_stream;


float frnd(float range)
{
	return (float)rnd(10000)/10000*range;
}

// forward
struct SFXD_Sample;
void ResetSample(SFXD_Sample& channel, bool restart);


typedef struct SFXD_Sample {
	SFXD_Params params;

	bool playing_sample = false;
	int phase;
	double fperiod;
	double fmaxperiod;
	double fslide;
	double fdslide;
	int period;
	float square_duty;
	float square_slide;
	int env_stage;
	int env_time;
	int env_length[3];
	float env_vol;
	float fphase;
	float fdphase;
	int iphase;
	float phaser_buffer[1024];
	int ipp;
	float noise_buffer[32];
	float fltp;
	float fltdp;
	float fltw;
	float fltw_d;
	float fltdmp;
	float fltphp;
	float flthp;
	float flthp_d;
	float vib_phase;
	float vib_speed;
	float vib_amp;
	int rep_time;
	int rep_limit;
	int arp_time;
	int arp_limit;
	double arp_mod;

	float sound_vol = 0.5f;


	// TODO this is a method to save me typing a bunch of qualifiers. More consistent to make it an oldschool function.
	void SynthSample(int length, float* buffer)
	{
		int i;
		for (i = 0; i<length; i++)
		{
			if (!playing_sample)
				break;

			rep_time++;
			if (rep_limit != 0 && rep_time >= rep_limit)
			{
				rep_time = 0;
				ResetSample(*this, true);
			}

			// frequency envelopes/arpeggios
			arp_time++;
			if (arp_limit != 0 && arp_time >= arp_limit)
			{
				arp_limit = 0;
				fperiod *= arp_mod;
			}
			fslide += fdslide;
			fperiod *= fslide;
			if (fperiod>fmaxperiod)
			{
				fperiod = fmaxperiod;
				if (params.p_freq_limit>0.0f)
					playing_sample = false;
			}
			float rfperiod = fperiod;
			if (vib_amp>0.0f)
			{
				vib_phase += vib_speed;
				rfperiod = fperiod*(1.0 + sin(vib_phase)*vib_amp);
			}
			period = (int)rfperiod;
			if (period<8) period = 8;
			square_duty += square_slide;
			if (square_duty<0.0f) square_duty = 0.0f;
			if (square_duty>0.5f) square_duty = 0.5f;
			// volume envelope
			env_time++;
			if (env_time>env_length[env_stage])
			{
				env_time = 0;
				env_stage++;
				if (env_stage == 3)
					playing_sample = false;
			}
			if (env_stage == 0)
				env_vol = (float)env_time / env_length[0];
			if (env_stage == 1)
				env_vol = 1.0f + pow(1.0f - (float)env_time / env_length[1], 1.0f)*2.0f*params.p_env_punch;
			if (env_stage == 2)
				env_vol = 1.0f - (float)env_time / env_length[2];

			// phaser step
			fphase += fdphase;
			iphase = abs((int)fphase);
			if (iphase>1023) iphase = 1023;

			if (flthp_d != 0.0f)
			{
				flthp *= flthp_d;
				if (flthp<0.00001f) flthp = 0.00001f;
				if (flthp>0.1f) flthp = 0.1f;
			}

			float ssample = 0.0f;
			for (int si = 0; si<8; si++) // 8x supersampling
			{
				float sample = 0.0f;
				phase++;
				if (phase >= period)
				{
					//				phase=0;
					phase %= period;
					if (params.wave_type == 3)
						for (int i = 0; i<32; i++)
							noise_buffer[i] = frnd(2.0f) - 1.0f;
				}
				// base waveform
				float fp = (float)phase / period;
				switch (params.wave_type)
				{
				case 0: // square
					if (fp<square_duty)
						sample = 0.5f;
					else
						sample = -0.5f;
					break;
				case 1: // sawtooth
					sample = 1.0f - fp * 2;
					break;
				case 2: // sine
					sample = (float)sin(fp * 2 * PI);
					break;
				case 3: // noise
					sample = noise_buffer[phase * 32 / period];
					break;
				}
				// lp filter
				float pp = fltp;
				fltw *= fltw_d;
				if (fltw<0.0f) fltw = 0.0f;
				if (fltw>0.1f) fltw = 0.1f;
				if (params.p_lpf_freq != 1.0f)
				{
					fltdp += (sample - fltp)*fltw;
					fltdp -= fltdp*fltdmp;
				}
				else
				{
					fltp = sample;
					fltdp = 0.0f;
				}
				fltp += fltdp;
				// hp filter
				fltphp += fltp - pp;
				fltphp -= fltphp*flthp;
				sample = fltphp;
				// phaser
				phaser_buffer[ipp & 1023] = sample;
				sample += phaser_buffer[(ipp - iphase + 1024) & 1023];
				ipp = (ipp + 1) & 1023;
				// final accumulation and envelope application
				ssample += sample*env_vol;
			}
			ssample = ssample / 8 * master_vol;

			ssample *= 2.0f*sound_vol;

			if (buffer != NULL)
			{
				if (ssample>1.0f) ssample = 1.0f;
				if (ssample<-1.0f) ssample = -1.0f;
				*buffer++ = ssample;
			}
		}
		for (int j = i; j < length; ++j)
		{
			*buffer++ = 0;
		}
	} // SynthSample

};
SFXD_Sample channels[MAX_CHANNELS];





// Set default params (sqre wave, minimal filtering)
void ResetParams(int channelNum = 0)
{
	SFXD_Params& channel = channels[channelNum].params;

	channel.wave_type=0;
	
	channel.p_base_freq=0.3f;
	channel.p_freq_limit=0.0f;
	channel.p_freq_ramp=0.0f;
	channel.p_freq_dramp=0.0f;
	channel.p_duty=0.0f;
	channel.p_duty_ramp=0.0f;
	
	channel.p_vib_strength=0.0f;
	channel.p_vib_speed=0.0f;
	channel.p_vib_delay=0.0f;
	
	channel.p_env_attack=0.0f;
	channel.p_env_sustain=0.3f;
	channel.p_env_decay=0.4f;
	channel.p_env_punch=0.0f;
	
	channel.filter_on=false;
	channel.p_lpf_resonance=0.0f;
	channel.p_lpf_freq=1.0f;
	channel.p_lpf_ramp=0.0f;
	channel.p_hpf_freq=0.0f;
	channel.p_hpf_ramp=0.0f;
	
	channel.p_pha_offset=0.0f;
	channel.p_pha_ramp=0.0f;
	
	channel.p_repeat_speed=0.0f;
	
	channel.p_arp_speed=0.0f;
	channel.p_arp_mod=0.0f;
}

void ResetSample(SFXD_Sample& channel, bool restart)
{
	SFXD_Params& params = channel.params;

	if (!restart)
		channel.phase = 0;
	channel.fperiod = 100.0 / (params.p_base_freq); // FIXME wtf units is this in
	channel.period = (int)channel.fperiod;
	channel.fmaxperiod = 100.0 / (params.p_freq_limit + 0.001);
	channel.fslide = 1.0 - pow((double)params.p_freq_ramp, 3.0)*0.01;
	channel.fdslide = -pow((double)params.p_freq_dramp, 3.0)*0.000001;
	channel.square_duty = 0.5f - params.p_duty*0.5f;
	channel.square_slide = -params.p_duty_ramp*0.00005f;
	if (channel.params.p_arp_mod >= 0.0f)
		channel.arp_mod = 1.0 - pow((double)params.p_arp_mod, 2.0)*0.9;
	else
		channel.arp_mod = 1.0 + pow((double)params.p_arp_mod, 2.0)*10.0;
	channel.arp_time = 0;
	channel.arp_limit = (int)(pow(1.0f - params.p_arp_speed, 2.0f) * 20000 + 32);
	if (params.p_arp_speed == 1.0f)
		channel.arp_limit = 0;
	if(!restart)
	{
		// reset filter
		channel.fltp = 0.0f;
		channel.fltdp = 0.0f;
		channel.fltw = pow(params.p_lpf_freq, 3.0f)*0.1f;
		channel.fltw_d = 1.0f + params.p_lpf_ramp*0.0001f;
		channel.fltdmp = 5.0f / (1.0f + pow(params.p_lpf_resonance, 2.0f)*20.0f)*(0.01f + channel.fltw);
		if(channel.fltdmp>0.8f) channel.fltdmp=0.8f;
		channel.fltphp = 0.0f;
		channel.flthp = pow(params.p_hpf_freq, 2.0f)*0.1f;
		channel.flthp_d = 1.0 + params.p_hpf_ramp*0.0003f;
		// reset vibrato
		channel.vib_phase = 0.0f;
		channel.vib_speed = pow(params.p_vib_speed, 2.0f)*0.01f;
		channel.vib_amp = params.p_vib_strength*0.5f;
		// reset envelope
		channel.env_vol = 0.0f;
		channel.env_stage = 0;
		channel.env_time = 0;
		channel.env_length[0] = (int)(params.p_env_attack*params.p_env_attack*100000.0f);
		channel.env_length[1] = (int)(params.p_env_sustain*params.p_env_sustain*100000.0f);
		channel.env_length[2] = (int)(params.p_env_decay*params.p_env_decay*100000.0f);

		channel.fphase = pow(params.p_pha_offset, 2.0f)*1020.0f;
		if (params.p_pha_offset<0.0f) channel.fphase = -channel.fphase;
		channel.fdphase = pow(params.p_pha_ramp, 2.0f)*1.0f;
		if (params.p_pha_ramp<0.0f) channel.fdphase = -channel.fdphase;
		channel.iphase = abs((int)channel.fphase);
		channel.ipp = 0;
		for(int i=0;i<1024;i++)
			channel.phaser_buffer[i] = 0.0f;

		for(int i=0;i<32;i++)
			channel.noise_buffer[i] = frnd(2.0f) - 1.0f;

		channel.rep_time = 0;
		channel.rep_limit = (int)(pow(1.0f - params.p_repeat_speed, 2.0f) * 20000 + 32);
		if (params.p_repeat_speed == 0.0f)
			channel.rep_limit = 0;
	}
}

void SFXD_PlaySample(int channelNum)
{
    SFXD_Sample& sample = channels[channelNum];
    ResetSample(sample, false);
    sample.playing_sample=true;
}


//lets use SDL instead
static void SDLAudioCallback(void *userdata, Uint8 *stream, int len)
{
	memset(stream, 0, len);
	for (int i = 0; i < num_channels; ++i)
	{
		SFXD_Sample& sample = channels[i];
		if (sample.playing_sample)
		{
			unsigned int l = len / 2;
			float* fbuf = new float[l]; // TODO buffer in sample struct
			memset(fbuf, 0, sizeof(fbuf));
			sample.SynthSample(l, fbuf);
			while (l--)
			{
				float f = fbuf[l];
				if (f < -1.0) f = -1.0;
				if (f > 1.0) f = 1.0;
				((Sint16*)stream)[l] += (Sint16)(f * 32767);
				//for (int j = 0; j < f * 80; ++j)
				//{
				//	printf("x");
				//}
				//printf("\n");
			}
			delete[] fbuf;
		}
	}
}

void SFXD_SetParams(int channelNum, const SFXD_Params& params)
{
	SFXD_Sample& channel = channels[channelNum];
	channel.params.wave_type = params.wave_type;
	channel.params.p_base_freq = params.p_base_freq;
	channel.params.p_freq_limit = params.p_freq_limit;
	channel.params.p_freq_ramp = params.p_freq_ramp;
	channel.params.p_freq_dramp = params.p_freq_dramp;
	channel.params.p_duty = params.p_duty;
	channel.params.p_duty_ramp = params.p_duty_ramp;
	channel.params.p_vib_strength = params.p_vib_strength;
	channel.params.p_vib_speed = params.p_vib_speed;
	channel.params.p_vib_delay = params.p_vib_delay;
	channel.params.p_env_attack = params.p_env_attack;
	channel.params.p_env_sustain = params.p_env_sustain;
	channel.params.p_env_decay = params.p_env_decay;
	channel.params.p_env_punch = params.p_env_punch;
	channel.params.filter_on = params.filter_on;
	channel.params.p_lpf_resonance = params.p_lpf_resonance;
	channel.params.p_lpf_freq = params.p_lpf_freq;
	channel.params.p_lpf_ramp = params.p_lpf_ramp;
	channel.params.p_hpf_freq = params.p_hpf_freq;
	channel.params.p_hpf_ramp = params.p_hpf_ramp;
	channel.params.p_pha_offset = params.p_pha_offset;
	channel.params.p_pha_ramp = params.p_pha_ramp;
	channel.params.p_repeat_speed = params.p_repeat_speed;
	channel.params.p_arp_speed = params.p_arp_speed;
	channel.params.p_arp_mod = params.p_arp_mod;
	//master_vol = params.master_vol;
	channel.params.sound_vol = params.sound_vol;
}

void SFXD_MutateChannel(int channelNum)
{
	SFXD_MutateParams(channels[channelNum].params);
}

void SFXD_MutateParams(SFXD_Params& params)
{

	/* if(rnd(1)) params.p_base_freq+=frnd(0.1f)-0.05f; */
//		if(rnd(1)) params.p_freq_limit+=frnd(0.1f)-0.05f;
	/* if(rnd(1)) params.p_freq_ramp+=frnd(0.1f)-0.05f; */
	/* if(rnd(1)) params.p_freq_dramp+=frnd(0.1f)-0.05f; */
	if(rnd(1)) params.p_duty+=frnd(0.1f)-0.05f;
	if(rnd(1)) params.p_duty_ramp+=frnd(0.1f)-0.05f;
	if(rnd(1)) params.p_vib_strength+=frnd(0.1f)-0.05f;
	if(rnd(1)) params.p_vib_speed+=frnd(0.1f)-0.05f;
	if(rnd(1)) params.p_vib_delay+=frnd(0.1f)-0.05f;
	if(rnd(1)) params.p_env_attack+=frnd(0.1f)-0.05f;
	if(rnd(1)) params.p_env_sustain+=frnd(0.1f)-0.05f;
	if(rnd(1)) params.p_env_decay+=frnd(0.1f)-0.05f;
	if(rnd(1)) params.p_env_punch+=frnd(0.1f)-0.05f;
	if(rnd(1)) params.p_lpf_resonance+=frnd(0.1f)-0.05f;
	if(rnd(1)) params.p_lpf_freq+=frnd(0.1f)-0.05f;
	if(rnd(1)) params.p_lpf_ramp+=frnd(0.1f)-0.05f;
	if(rnd(1)) params.p_hpf_freq+=frnd(0.1f)-0.05f;
	if(rnd(1)) params.p_hpf_ramp+=frnd(0.1f)-0.05f;
	if(rnd(1)) params.p_pha_offset+=frnd(0.1f)-0.05f;
	if(rnd(1)) params.p_pha_ramp+=frnd(0.1f)-0.05f;
	if(rnd(1)) params.p_repeat_speed+=frnd(0.1f)-0.05f;
	if(rnd(1)) params.p_arp_speed+=frnd(0.1f)-0.05f;
	if(rnd(1)) params.p_arp_mod+=frnd(0.1f)-0.05f;
}

void SFXD_Init(int numChannels)
{
	srand(time(NULL));

	if (numChannels > MAX_CHANNELS)
	{
		fprintf(stderr, "Cannot open more than %d channels", MAX_CHANNELS);
	}

	num_channels = numChannels;
	for (int i = 0; i < numChannels; ++i)
	{
		ResetParams(i);
	}

	SDL_AudioSpec des, got;
	des.freq = 44100;
	des.format = AUDIO_S16SYS;
	des.channels = 1;
	des.samples = 512;
	des.callback = SDLAudioCallback;
	des.userdata = NULL;
	if (SDL_OpenAudio(&des, &got))
	{
		fprintf(stderr, "Error opening SDL audio");
	}
	SDL_PauseAudio(0);
}

