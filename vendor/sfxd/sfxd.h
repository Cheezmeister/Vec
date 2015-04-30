/*
* sfxd. Procedural sound effects for games.

  Original notice for sfxr follows.
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

struct SFXD_Params
{
    int wave_type;

    float p_base_freq;
    float p_freq_limit;
    float p_freq_ramp;
    float p_freq_dramp;
    float p_duty;
    float p_duty_ramp;

    float p_vib_strength;
    float p_vib_speed;
    float p_vib_delay;

    float p_env_attack;
    float p_env_sustain;
    float p_env_decay;
    float p_env_punch;

    bool filter_on;
    float p_lpf_resonance;
    float p_lpf_freq;
    float p_lpf_ramp;
    float p_hpf_freq;
    float p_hpf_ramp;

    float p_pha_offset;
    float p_pha_ramp;

    float p_repeat_speed;

    float p_arp_speed;
    float p_arp_mod;

    //float master_vol;

    float sound_vol;
};

void SFXD_Init(int numChannels = 1);
void SFXD_MutateParams(SFXD_Params& params); 
void SFXD_MutateChannel(int channel = 0);
void SFXD_SetParams(int channel, const SFXD_Params& params);
void SFXD_PlaySample(int channel = 0);
