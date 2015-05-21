#include <cmath>
#include "vec.h"
#include "sfxd.h"

void SFXD_PlaySample();

using namespace std;

// Twelfth root of two
const float trot = 0.0594630943592952645;

namespace audio {


/// Types

typedef struct Channel {
    int id; // SFXD channel num
    SFXD_Params params;
    int octave;
    u32 pattern; // This is a bitfield, one bit per beat to play
    u32 nextnote; // Next ticks to play a note at
} Channel;

typedef float Scale[7];

typedef enum Melody {
    TONIC,
    SUPERTONIC,
    MEDIANT,
    SUBDOMINANT,
    DOMINANT,
    SUBMEDIANT,
    LEADING,
    OCTAVE,
} Melody;

/// Utils

void make_mode(int index, Scale destination)
{
    Scale ionian = {
        1,
        9 / 8.0,
        81 / 64.0,
        4 / 3.0,
        3 / 2.0,
        27 / 16.0,
        243 / 128.0,
    };

    for (int i = 0; i < 7; ++i)
    {
        destination[i] = ionian[(index + i) % 7] / ionian[index];
        if (index + i >= 7)
            destination[i] *= 2;
    }
}

void play_sample(Channel& channel)
{
    SFXD_PlaySample(channel.id);
}

void update_params(Channel& channel)
{
    SFXD_SetParams(channel.id, channel.params);
}

// Data

float baseNote = 0.3;

Scale ionian, dorian, phrygian, lydian, mixolydian, aeolian, locrian;
Scale scales[7];

Channel bass  = {0};
Channel bell  = {0};
Channel xylo  = {0};
Channel clink = {0};
Channel enemy = {0};
Channel hat   = {0};
Channel moog  = {0};
Channel perc  = {0};
Channel xp    = {0};

Scale& currentMode = dorian;


// bullets fired and consumed
int fired = 0;
int consumed = 0;

// rolling channel ids
int nextId = -1;

// Forward
void init_channels();
void set_note(Channel& channel, int note, float tonic = baseNote);


void init(u32 ticks)
{
    SFXD_Init(9);

    make_mode(0, ionian);
    make_mode(1, dorian);
    make_mode(2, phrygian);
    make_mode(3, lydian);
    make_mode(4, mixolydian);
    make_mode(5, aeolian);
    make_mode(6, locrian);

    init_channels();
    bass.nextnote = ticks;
    bell.nextnote = ticks;
    hat.nextnote = ticks;
    perc.nextnote = ticks;

    update_params(enemy);
    update_params(xp);
    update_params(clink);
    update_params(xylo);
    update_params(moog);

    update_params(bass);
    update_params(bell);
    update_params(perc);
    update_params(hat);
}

// TODO trim down
void init_channels()
{

    enemy.id = ++nextId;
    enemy.params.wave_type = WAVE_SINE;
    enemy.params.p_base_freq = 0.3f;
    enemy.params.p_env_attack = 0.1f;
    enemy.params.p_env_sustain = 0.2f;
    enemy.params.p_env_decay = 0.7f;
    enemy.params.p_lpf_freq = 1.0f;

    xp.id = ++nextId;
    xp.octave = -1;
    xp.params.wave_type = WAVE_SAWTOOTH;
    xp.params.p_base_freq = 0.45f;
    xp.params.p_freq_ramp = 0.01f;
    xp.params.p_env_attack = 0.3f;
    xp.params.p_env_sustain = 0.2f;
    xp.params.p_env_decay = 0.3f;
    xp.params.p_lpf_freq = 1.0f;

    bass.id = ++nextId;
    bass.pattern = 0x10000000;
    bass.octave = -6;
    set_note(bass, TONIC);
    bass.params.wave_type = WAVE_SQUARE;
    bass.params.p_freq_ramp = 0.02f;
    bass.params.p_env_attack = 0.1f;
    bass.params.p_env_sustain = 0.2f;
    bass.params.p_env_decay = 1.0f;
    bass.params.p_lpf_freq = 1.0f;
    bass.params.p_arp_speed = 0.28f;
    bass.params.p_arp_mod = 0.05;

    bell.id = ++nextId;
    bell.pattern = 0x00001000;
    bell.octave = -2;
    bell.params.wave_type = WAVE_SINE;
    set_note(bell, DOMINANT);
    bell.params.p_freq_ramp = 0.02f;
    bell.params.p_env_attack = 0.0f;
    bell.params.p_env_sustain = 0.0f;
    bell.params.p_env_decay = 0.9f;
    bell.params.p_lpf_freq = 1.0f;
    bell.params.p_lpf_ramp = -1.0f;
    bell.params.p_arp_speed = 0.28f;
    bell.params.p_arp_mod = 0.05;

    clink.id = ++nextId;
    clink.params.wave_type = WAVE_SQUARE;
    clink.params.p_base_freq = 0; // melodic
    clink.params.p_freq_ramp = 0;
    clink.params.p_env_attack = 0.0f;
    clink.params.p_env_sustain = 0.0f;
    clink.params.p_env_decay = 0.3f;
    clink.params.filter_on = false;
    clink.params.p_lpf_freq = 1.0f;
    clink.params.p_lpf_ramp = -1.0f;
    clink.params.p_arp_speed = 0.28f;
    clink.params.p_arp_mod = 0.05;

    moog.id = ++nextId;
    moog.params.wave_type = WAVE_SQUARE;
    moog.octave = -5;
    moog.params.p_freq_ramp = 0;
    moog.params.p_env_attack = 0.0f;
    moog.params.p_env_sustain = 0.1f;
    moog.params.p_env_decay = 0.3f;
    moog.params.filter_on = false;
    moog.params.p_lpf_freq = 0.5;
    moog.params.p_lpf_resonance = 1.5;
    moog.params.p_lpf_ramp = -0.9f;

    xylo.id = ++nextId;
    xylo.params.wave_type = WAVE_SINE;
    xylo.params.p_base_freq = 0; // melodic
    xylo.params.p_freq_ramp = 0;
    xylo.params.p_env_attack = 0.1f;
    xylo.params.p_env_sustain = 0.0f;
    xylo.params.p_env_decay = 0.2f;
    xylo.params.p_lpf_freq = 1.0f;
    xylo.params.p_lpf_ramp = -1.0f;
    xylo.params.p_arp_speed = 0.28f;
    xylo.params.p_arp_mod = 0.05;

    perc.id = ++nextId;
    perc.pattern = 0x10101010;
    perc.params.wave_type = WAVE_NOISE; // noise
    perc.params.p_base_freq = 0.8f;
    perc.params.p_env_attack = 0.1f;
    perc.params.p_env_punch = 0.8f;
    perc.params.p_env_sustain = 0.0f;
    perc.params.p_env_decay = 0.25f;
    perc.params.p_lpf_freq = 1.0f;
    perc.params.p_arp_speed = 0.28f;
    perc.params.p_arp_mod = 0.05;

    hat.id = ++nextId;
    hat.pattern = 0x11111111;
    hat.params.wave_type = WAVE_NOISE; // noise
    hat.params.p_base_freq = 1.5f;
    hat.params.p_env_attack = 0.0f;
    hat.params.p_env_punch = 0.0f;
    hat.params.p_env_sustain = 0.05f;
    hat.params.p_env_decay = 0.15f;
    hat.params.p_lpf_freq = 1.0f;
    hat.params.p_arp_speed = 0.28f;
    hat.params.p_arp_mod = 0.05;
}

void mutate_samples()
{
    SFXD_MutateChannel(0);
    SFXD_MutateChannel(1);
    SFXD_MutateChannel(2);
    SFXD_MutateChannel(3);
    SFXD_MutateChannel(4);
    SFXD_MutateChannel(5);
    SFXD_MutateChannel(6);
}

void update(const GameState& state, u32 ticks)
{
    if (state.player.size < 1.0)
    {
        baseNote = 0.3 - 0.2 * state.player.size;
    }

    for (int i = 0; i < state.next_event; ++i)
    {
        if (state.events[i].type == Event::T_ENT_CREATED)
        {
            switch (state.events[i].entity)
            {
            case E_NOVA:
            {
                set_note(moog, OCTAVE - fired++ % 8);
                update_params(moog);
                play_sample(moog);
            } // fallthrough
            case E_BULLET:
            {
                set_note(clink, LEADING - ++fired % 7);
                update_params(clink);
                play_sample(clink);
            }
            break;
            }
        }
        if (state.events[i].type == Event::T_ENT_DESTROYED)
        {
            switch (state.events[i].entity)
            {
            case E_ENEMY:
            {
                int m = state.player.killcount % 7;
                int p = state.player.killcount / 7;

                enemy.octave = p;
                bass.octave = p - 5;
                bell.octave = -p - 2;
                set_note(enemy, m);
                set_note(xp, MAX_ENEMIES - m);
                set_note(bass, TONIC);
                set_note(bell, DOMINANT);

                update_params(enemy);
                update_params(xp);
                update_params(bell);
                update_params(bass);

                play_sample(enemy);
            }
            break;
            case E_XPCHUNK:
            {
                play_sample(xp);
            }
            break;
            case E_BULLET:
            {
                set_note(xylo, LEADING - ++consumed % 7);
                update_params(xylo);
                play_sample(xylo);
            }
            break;
            }
        }
    }


    float bpm = beats_per_minute(state);
    if (ticks > hat.nextnote)
    {
        hat.nextnote += 60000.0 / (bpm * 4);
        play_sample(hat);
    }
    if (ticks > perc.nextnote)
    {
        perc.nextnote += 60000.0 / (bpm);
        play_sample(perc);
    }
    if (ticks > bass.nextnote)
    {
        bass.nextnote += 60000.0 / (bpm / 4);
        play_sample(bass);
    }
    if (ticks > bell.nextnote)
    {
        bell.nextnote += 60000.0 / (bpm * 3);
        set_note(bell, DOMINANT + state.player.killcount % 3 - 1);
        update_params(bell);
        if (rand() % 7 < state.player.killcount) play_sample(bell);
    }
}

void set_note(Channel& channel, int note, float tonic)
{
    int octave = channel.octave;
    while (note >= 7)
    {
        ++octave;
        note -= 7;
    }
    channel.params.p_base_freq = tonic * currentMode[note] * pow(2, octave);
}

}
