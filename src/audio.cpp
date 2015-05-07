#include <cmath>
#include "vec.h"
#include <sfxd.h>

void SFXD_PlaySample();

using namespace std;

// Twelfth root of two
const float trot = 0.0594630943592952645;

namespace audio {


/// Types

typedef enum _GameParams {
    GP_DEAD_ENEMIES,
    GP_LAST
} GameParams;

typedef struct Channel {
    int id; // SFXD channel num
    SFXD_Params params;
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
int gameParams[1] = {0};

Scale ionian, dorian, phrygian, lydian, mixolydian, aeolian, locrian;
Scale scales[7];

Channel enemy = {0};
Channel xp    = {0};
Channel bass  = {0};
Channel bell  = {0};
Channel perc  = {0};
Channel hat   = {0};

Scale& currentMode = locrian;

float baseNote = 0.3;

int nextId = -1;

// Forward
void init_channels();

void init(u32 ticks)
{
    SFXD_Init(6);

    make_mode(0, ionian);
    make_mode(1, dorian);
    make_mode(2, phrygian);
    make_mode(3, lydian);
    make_mode(4, mixolydian);
    make_mode(5, aeolian);
    make_mode(6, locrian);

    init_channels();
    bass.nextnote = ticks;
    bell.nextnote = ticks + 60000.0 / (beats_per_minute() / 2);
    perc.nextnote = ticks;
    hat.nextnote = ticks;

    update_params(enemy);
    update_params(xp);
    update_params(bass);
    update_params(bell);
    update_params(perc);
    update_params(hat);
}

void init_channels()
{

    enemy.id = ++nextId;
    enemy.params.wave_type = 2;
    enemy.params.p_base_freq = 0.3f;
    enemy.params.p_env_attack = 0.1f;
    enemy.params.p_env_sustain = 0.2f;
    enemy.params.p_env_decay = 0.7f;
    enemy.params.filter_on = false;
    enemy.params.p_lpf_freq = 1.0f;

    xp.id = ++nextId;
    xp.params.wave_type = 1;
    xp.params.p_base_freq = 0.45f;
    xp.params.p_freq_ramp = 0.01f;
    xp.params.p_env_attack = 0.3f;
    xp.params.p_env_sustain = 0.2f;
    xp.params.p_env_decay = 0.3f;
    xp.params.filter_on = false;
    xp.params.p_lpf_freq = 1.0f;

    bass.id = ++nextId;
    bass.pattern = 0x10000000;
    bass.params.wave_type = 0;
    /* bass.params.p_base_freq = 1.0 / pow(2, 6); */
    bass.params.p_base_freq = baseNote * currentMode[TONIC] / pow(2, 6);
    bass.params.p_freq_ramp = 0.02f;
    bass.params.p_env_attack = 0.3f;
    bass.params.p_env_sustain = 0.2f;
    bass.params.p_env_decay = 1.0f;
    bass.params.filter_on = false;
    bass.params.p_lpf_freq = 1.0f;
    bass.params.p_arp_speed = 0.28f;
    bass.params.p_arp_mod = 0.05;

    bell.id = ++nextId;
    bell.pattern = 0x10000000;
    bell.params.wave_type = 2;
    bell.params.p_base_freq = baseNote * currentMode[DOMINANT] / pow(2, 2);
    bell.params.p_freq_ramp = 0.02f;
    bell.params.p_env_attack = 0.2f;
    bell.params.p_env_sustain = 0.5f;
    bell.params.p_env_decay = 0.3f;
    bell.params.filter_on = false;
    bell.params.p_lpf_freq = 1.0f;
    bell.params.p_arp_speed = 0.28f;
    bell.params.p_arp_mod = 0.05;

    perc.id = ++nextId;
    perc.pattern = 0x10101010;
    perc.params.wave_type = 3; // noise
    perc.params.p_base_freq = 0.8f;
    perc.params.p_env_attack = 0.1f;
    perc.params.p_env_punch = 0.8f;
    perc.params.p_env_sustain = 0.0f;
    perc.params.p_env_decay = 0.25f;
    perc.params.filter_on = false;
    perc.params.p_lpf_freq = 1.0f;
    perc.params.p_arp_speed = 0.28f;
    perc.params.p_arp_mod = 0.05;

    hat.id = ++nextId;
    hat.pattern = 0x11111111;
    hat.params.wave_type = 3; // noise
    hat.params.p_base_freq = 1.5f;
    hat.params.p_env_attack = 0.0f;
    hat.params.p_env_punch = 0.0f;
    hat.params.p_env_sustain = 0.05f;
    hat.params.p_env_decay = 0.15f;
    hat.params.filter_on = false;
    hat.params.p_lpf_freq = 1.0f;
    hat.params.p_arp_speed = 0.28f;
    hat.params.p_arp_mod = 0.05;
}

void mutate_samples()
{
    SFXD_MutateChannel(0);
    SFXD_MutateChannel(1);
}

void update(const GameState& state, u32 ticks)
{
    for (int i = 0; i < state.next_event; ++i)
    {
        if (state.events[i].type == Event::T_ENT_DESTROYED)
        {
            switch (state.events[i].entity)
            {
            case E_ENEMY:
            {
                int m = gameParams[GP_DEAD_ENEMIES] % 7;
                int p = gameParams[GP_DEAD_ENEMIES] / 7;
                ++gameParams[GP_DEAD_ENEMIES];
                enemy.params.p_base_freq = baseNote * currentMode[m] * pow(2, p);
                xp.params.p_base_freq = baseNote * pow(2, p);
                bass.params.p_base_freq = baseNote * currentMode[TONIC] / pow(2, 6 - p);
                bell.params.p_base_freq = baseNote * currentMode[DOMINANT] / pow(2, 2 + p);

                update_params(enemy);
                update_params(xp);
                update_params(bass);

                play_sample(enemy);
            }
            break;
            case E_XPCHUNK:
            {
                play_sample(xp);
            }
            break;
            }
        }
    }


    if (ticks > hat.nextnote)
    {
        hat.nextnote += 60000.0 / (beats_per_minute() * 4);
        play_sample(hat);
    }
    if (ticks > perc.nextnote)
    {
        perc.nextnote += 60000.0 / (beats_per_minute());
        play_sample(perc);
    }
    if (ticks > bass.nextnote)
    {
        bass.nextnote += 60000.0 / (beats_per_minute() / 4);
        play_sample(bass);
    }
    if (ticks > bell.nextnote)
    {
        bell.nextnote += 60000.0 / (beats_per_minute() / 4);
        if (bml::normrand() > 0.5) play_sample(bell);
    }
}
}
