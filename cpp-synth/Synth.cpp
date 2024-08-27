#include "Synth.h"
#include "wavetable.h"

Synth::Synth() 
{
     sprintf(message, "Synth End ");
     oscA.label = 'A';
     oscB.label = 'B';
     oscC.label = 'C';
}

bool Synth::open(PaDeviceIndex index) {
    PaStreamParameters outputParameters{ };

    outputParameters.device = index;
    if (outputParameters.device == paNoDevice) {
        return false;
    }

    const PaDeviceInfo* pInfo = Pa_GetDeviceInfo(index);
    if (pInfo != 0)
    {
        printf("Output device name: %s\r", pInfo->name);
    }

    outputParameters.channelCount = 2;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(&stream, NULL, &outputParameters, SAMPLE_RATE, 512, 0, &Synth::paCallback, this);

    if (err != paNoError)
    {
        return false;
    }

    err = Pa_SetStreamFinishedCallback(stream, &Synth::paStreamFinished);

    if (err != paNoError)
    {
        Pa_CloseStream(stream);
        stream = 0;

        return false;
    }

    return true;
}

bool Synth::close() {
    if (stream == 0)
        return false;
    PaError err = Pa_CloseStream(stream);
    stream = 0;
    return (err == paNoError);
}

bool Synth::start() {
    if (stream == 0)
        return false;
    PaError err = Pa_StartStream(stream);
    return (err == paNoError);
}

bool Synth::stop() {
    if (stream == 0)
        return false;
    PaError err = Pa_StopStream(stream);
    return (err == paNoError);
}

int Synth::paCallbackMethod(const void* inputBuffer, 
                            void* outputBuffer, 
                            unsigned long framesPerBuffer, 
                            const PaStreamCallbackTimeInfo* timeInfo, 
                            PaStreamCallbackFlags statusFlags) {

    float* out = (float*)outputBuffer;
    (void)timeInfo;
    (void)statusFlags;
    (void)inputBuffer;

    for (std::size_t i = 0; i < framesPerBuffer; i++) {
        *out++ = amplitude * (
            oscA.env.get_amp(get_time()) * oscA.interpolate_left() +
            oscB.env.get_amp(get_time()) * oscB.interpolate_left() +
            oscC.env.get_amp(get_time()) * oscC.interpolate_left());
        *out++ = amplitude * (
            oscA.env.get_amp(get_time()) * oscA.interpolate_right() +
            oscB.env.get_amp(get_time()) * oscB.interpolate_right() +
            oscC.env.get_amp(get_time()) * oscC.interpolate_right());

        for (std::size_t j = 0; j < 3; ++j) {
            oscs[j]->left_phase += oscs[j]->left_phase_inc;
            if (oscs[j]->left_phase >= TABLE_SIZE) oscs[j]->left_phase -= TABLE_SIZE;
            oscs[j]->right_phase += oscs[j]->right_phase_inc;
            if (oscs[j]->right_phase >= TABLE_SIZE) oscs[j]->right_phase -= TABLE_SIZE; 
        }
    }
    return paContinue;
}

int Synth::paCallback(const void* inputBuffer, 
                      void* outputBuffer, 
                      unsigned long framesPerBuffer, 
                      const PaStreamCallbackTimeInfo* timeInfo, 
                      PaStreamCallbackFlags statusFlags, 
                      void* userData) {

    return ((Synth*)userData)->paCallbackMethod(inputBuffer, 
                                                outputBuffer,
                                                framesPerBuffer,
                                                timeInfo,
                                                statusFlags);
}

void Synth::paStreamFinishedMethod() {
    printf("Stream Completed: %s\n", message);
}

void Synth::paStreamFinished(void* userData) {
    return ((Synth*)userData)->paStreamFinishedMethod();
}


