#include "vec.h"
#include "sfxd.h"

void SFXD_PlaySample();

namespace audio {

	SFXD_Params enemyParams;
	SFXD_Params xpParams;

	void init()
	{
		SFXD_Init(2);

		enemyParams.wave_type = 2;
		enemyParams.p_base_freq = 0.3f;
		enemyParams.p_freq_limit = 0.0f;
		enemyParams.p_freq_ramp = 0.0f;
		enemyParams.p_freq_dramp = 0.1f;
		enemyParams.p_duty = 0.0f;
		enemyParams.p_duty_ramp = 0.0f;
		enemyParams.p_vib_strength = 0.0f;
		enemyParams.p_vib_speed = 0.0f;
		enemyParams.p_vib_delay = 0.0f;
		enemyParams.p_env_attack = 0.0f;
		enemyParams.p_env_sustain = 0.3f;
		enemyParams.p_env_decay = 0.4f;
		enemyParams.p_env_punch = 0.0f;
		enemyParams.filter_on = false;
		enemyParams.p_lpf_resonance = 0.0f;
		enemyParams.p_lpf_freq = 1.0f;
		enemyParams.p_lpf_ramp = 0.0f;
		enemyParams.p_hpf_freq = 0.0f;
		enemyParams.p_hpf_ramp = 0.0f;
		enemyParams.p_pha_offset = 0.0f;
		enemyParams.p_pha_ramp = 0.0f;
		enemyParams.p_repeat_speed = 0.0f;
		enemyParams.p_arp_speed = 0.0f;
		enemyParams.p_arp_mod = 0.0f;

		xpParams.wave_type = 1;
		xpParams.p_base_freq = 0.45f;
		xpParams.p_freq_limit = 0.0f;
		xpParams.p_freq_ramp = 0.0f;
		xpParams.p_freq_dramp = 0.1f;
		xpParams.p_duty = 0.0f;
		xpParams.p_duty_ramp = 0.0f;
		xpParams.p_vib_strength = 0.0f;
		xpParams.p_vib_speed = 0.0f;
		xpParams.p_vib_delay = 0.0f;
		xpParams.p_env_attack = 0.1f;
		xpParams.p_env_sustain = 0.2f;
		xpParams.p_env_decay = 0.1f;
		xpParams.p_env_punch = 0.0f;
		xpParams.filter_on = false;
		xpParams.p_lpf_resonance = 0.0f;
		xpParams.p_lpf_freq = 1.0f;
		xpParams.p_lpf_ramp = 0.0f;
		xpParams.p_hpf_freq = 0.0f;
		xpParams.p_hpf_ramp = 0.0f;
		xpParams.p_pha_offset = 0.0f;
		xpParams.p_pha_ramp = 0.0f;
		xpParams.p_repeat_speed = 0.0f;
		xpParams.p_arp_speed = 0.0f;
		xpParams.p_arp_mod = 0.0f;

		SFXD_SetParams(0, enemyParams);
		SFXD_SetParams(1, xpParams);
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
				case E_ENEMY: SFXD_PlaySample(0); mutate_samples(); break;
				case E_XPCHUNK: SFXD_PlaySample(1); break;
				}
			}
		}
	}
}