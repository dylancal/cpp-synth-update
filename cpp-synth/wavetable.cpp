#include <algorithm>
#include "wavetable.h"

float Oscillator::interpolate_at(float idx) {
    float wl, fl;
    fl = std::modf(idx, &wl);
    return std::lerp(table[(int)wl % TABLE_SIZE], table[(int)(wl + 1) % TABLE_SIZE], fl);
}

float Oscillator::interpolate_right() {
    float wl, fl;
    fl = std::modf(right_phase, &wl);
    return std::lerp(table[(int)wl % TABLE_SIZE], table[(int)(wl + 1) % TABLE_SIZE], fl);
}

float Oscillator::interpolate_left() {
    float wl, fl;
    fl = std::modf(left_phase, &wl);
    return std::lerp(table[(int)wl % TABLE_SIZE], table[(int)(wl + 1) % TABLE_SIZE], fl);
}

void gen_sin_wave(Oscillator& table) {
    for (int i = 0; i < TABLE_SIZE; i++) 
        table[i] = (float)std::sin((i / (double)TABLE_SIZE) * M_PI * 2.);
}

void gen_sin_wave(Oscillator* table) {
    for (int i = 0; i < TABLE_SIZE; i++) 
        (*table)[i] = (float)std::sin((i / (double)TABLE_SIZE) * M_PI * 2.);
}

void gen_saw_wave(Oscillator& table) {
    for (int i = 0; i < TABLE_SIZE; i++) 
        table[i] = 2 * ((i + TABLE_SIZE / 2) % TABLE_SIZE) / (float)TABLE_SIZE - 1.0f;
}

void gen_saw_wave(Oscillator* table) {
    for (int i = 0; i < TABLE_SIZE; i++) 
        (*table)[i] = 2 * ((i + TABLE_SIZE / 2) % TABLE_SIZE) / (float)TABLE_SIZE - 1.0f;
}

void gen_sqr_wave(Oscillator& table, float pw) {
    for (int i = 0; i < (int)(TABLE_SIZE * pw); i++) 
        table[i] = 1.0f; 
    for (int i = (int)(TABLE_SIZE * pw); i < TABLE_SIZE; i++) 
        table[i] = -1.0f; 
}

void gen_sqr_wave(Oscillator* table) {
    for (int i = 0; i < (int)(TABLE_SIZE * table->pulse_width); i++) 
        (*table)[i] = 1.0f; 
    for (int i = (int)(TABLE_SIZE * table->pulse_width); i < TABLE_SIZE; i++) 
        (*table)[i] = -1.0f; 
}

void gen_tri_wave(Oscillator& table, float pw) {
    for (int i = 0; i < (int)(TABLE_SIZE * pw); i++) 
        table[i] = 2.0*i / TABLE_SIZE * pw - 1;
    for (int i = (int)(TABLE_SIZE * pw); i < TABLE_SIZE; i++) 
        table[i] = -2.0*(i-TABLE_SIZE)/(TABLE_SIZE-pw*TABLE_SIZE);
}

void gen_tri_wave(Oscillator* table, float pw) {
    for (int i = 0; i < (int)(TABLE_SIZE * table->pulse_width); i++)
        (*table)[i] = 2.0 * i / (TABLE_SIZE * pw) - 1;
    for (int i = (int)(TABLE_SIZE * table->pulse_width); i < TABLE_SIZE; i++)
        (*table)[i] = -2.0 * (i - TABLE_SIZE) / (TABLE_SIZE - pw * TABLE_SIZE) - 1;
}

void gen_silence(Oscillator* table)
{
    for (int i = 0; i < TABLE_SIZE; i++) 
        (*table)[i] = 0;
}

float clip(float amp) {
    return std::clamp(amp, 0.0f, 1.0f);
}

float half_f_add_one(float amp) {
    return 0.5f * amp + 1;
}

inline std::chrono::milliseconds get_time()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch());
}

float ADSR::get_amp(ms sample)
{
    float out_amp = 0;
    ms life = get_time() - on_time;
    auto lifec = life.count();
    if (note_on)
    {
        if (lifec <= attack_time)
        {
            out_amp = (lifec / attack_time) * 1.0f;
        }
        else if (lifec > attack_time && lifec <= (attack_time + decay_time))
        {
            out_amp = ((lifec - attack_time) / decay_time) * (sustain_amp - 1.0f) + 1.0f;
        }
        else if (lifec > (attack_time + decay_time))
        {
            out_amp = sustain_amp;
        }
    }
    else
    {
        out_amp = ((sample - off_time).count() / release_time) * (0.0f - keyoff_amp) + keyoff_amp;
    }
    if (out_amp <= 0.0001)
    {
        out_amp = 0.0;
    }
    return out_amp;
}

void ADSR::key_on(ms time)
{
    if (!lock)
    {
        note_on = true;
        on_time = get_time();
        lock = true;
    }

}

void ADSR::key_off(ms time)
{
    note_on = false;
    off_time = get_time();
    lock = false;
}