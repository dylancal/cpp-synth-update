#pragma once
#include <atomic>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <chrono>
constexpr auto TABLE_SIZE = (872);
#ifndef M_PI
#define M_PI  (3.14159265)
#endif



struct ADSR
{
    using ms = std::chrono::milliseconds;
    float     attack_time   = 0.0f;
    float     decay_time    = 0.0f;
    float     release_time  = 0.01f;
    float     sustain_amp   = 1.0f;
    float     keyoff_amp    = 0.0f;
    ms        on_time       = ms(0);
    ms        off_time      = ms(0);
    bool      note_on       = false;
    bool      lock          = false;
    float     get_amp(ms);
    void      key_on(ms time);
    void      key_off(ms time);
};

struct Oscillator
{
    ADSR   env;
    float  amp              = 1.0f;
    char   label            = ' ';
    float  left_phase       = 0;
    float  right_phase      = 0;
    float  left_phase_inc   = 1;
    float  right_phase_inc  = 1;
    int    current_note     = 1;
    int    current_waveform = 2;
    float  pulse_width      = 0.5f;
    float  table[TABLE_SIZE]{ 0 };
    float& operator[](int i) { return table[i]; }
    float  interpolate_at(float idx);
    float  interpolate_left();
    float  interpolate_right();
};

void gen_sin_wave(Oscillator& table);
void gen_sin_wave(Oscillator* table);
void gen_saw_wave(Oscillator& table);
void gen_saw_wave(Oscillator* table);
void gen_sqr_wave(Oscillator& table, float pw);
void gen_sqr_wave(Oscillator* table);
void gen_tri_wave(Oscillator& table, float pw);
void gen_tri_wave(Oscillator* table, float pw);
void gen_silence(Oscillator* table);

inline std::chrono::milliseconds get_time();
float clip(float amp);
float half_f_add_one(float amp);

