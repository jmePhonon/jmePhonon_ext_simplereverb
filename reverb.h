//////////////////
// This is a slightly modified version of Sean Connelly (@voidqk) code that
// uses jni types.
//////////////////


// (c) Copyright 2016, Sean Connelly (@voidqk), http://syntheti.cc
// MIT License
// Project Home: https://github.com/voidqk/sndfilter

// reverb algorithm is based on Progenitor2 reverb effect from freeverb3:
//   http://www.nongnu.org/freeverb3/

#ifndef SNDFILTER_REVERB__H
#define SNDFILTER_REVERB__H
#include <jni.h>
#define true 1
#define false 0

typedef struct {
	jfloat L; // left channel sample
	jfloat R; // right channel sample
} sf_sample_st;

typedef struct {
	sf_sample_st *samples;
	jint size; // number of samples
	jint rate; // samples per second
} sf_snd_st, *sf_snd;

// this API works by first initializing an sf_reverb_state_st structure, then using it to process a
// sample in chunks
//
// for example, say you're processing a stream in 128 samples per chunk:
//
//   sf_reverb_state_st rv;
//   sf_presetreverb(&rv, 44100, SF_REVERB_PRESET_DEFAULT);
//
//   for each 128 length sample:
//     sf_reverb_process(&rv, 128, input, output);
//
// notice that sf_reverb_process will change a lot of the member variables inside of the state
// structure, since these values must be carried over across chunk boundaries
//
// also notice that the choice to divide the sound into chunks of 128 samples is completely
// arbitrary from the reverb's perspective
//
// ---
//
// non-convolution based reverb effects are made up from a lot of smaller effects
//
// each reverb algorithm's sound is based on how the designers setup these smaller effects and
// chained them together
//
// this particular setup is based on Progenitor2 from Freeverb3, and uses the following components:
//    1. Delay
//    2. 1st order IIR filter (lowpass filter, highpass filter)
//    3. Biquad filter (lowpass filter, all-pass filter)
//    4. Early reflection
//    5. Oversampling
//    6. DC cut
//    7. Fractal noise
//    8. Low-frequency oscilator (LFO)
//    9. All-pass filter
//   10. 2nd order All-pass filter
//   11. 3rd order All-pass filter with modulation
//   12. Modulated all-pass filter
//   13. Delayed feedforward comb filter
//
// each of these components is broken into their own structures (sf_rv_*), and the reverb effect
// uses these in the final state structure (sf_reverb_state_st)
//
// each component is designed to work one step at a time, so any size sample can be streamed through
// in one pass

// delay
// delay buffer size; maximum size allowed for a delay
#define SF_REVERB_DS        9814
typedef struct {
	jint pos;                 // current write position
	jint size;                // delay size
	jfloat buf[SF_REVERB_DS]; // delay buffer
} sf_rv_delay_st;

// 1st order IIR filter
typedef struct {
	jfloat a2; // coefficients
	jfloat b1;
	jfloat b2;
	jfloat y1; // state
} sf_rv_iir1_st;

// biquad
// note: we don't use biquad.c because we want to step through the sound one sample at a time, one
//       channel at a time
typedef struct {
	jfloat b0; // biquad coefficients
	jfloat b1;
	jfloat b2;
	jfloat a1;
	jfloat a2;
	jfloat xn1; // input[n - 1]
	jfloat xn2; // input[n - 2]
	jfloat yn1; // output[n - 1]
	jfloat yn2; // output[n - 2]
} sf_rv_biquad_st;

// early reflection
typedef struct {
	jint             delaytblL[18], delaytblR[18];
	sf_rv_delay_st  delayPWL     , delayPWR     ;
	sf_rv_delay_st  delayRL      , delayLR      ;
	sf_rv_biquad_st allpassXL    , allpassXR    ;
	sf_rv_biquad_st allpassL     , allpassR     ;
	sf_rv_iir1_st   lpfL         , lpfR         ;
	sf_rv_iir1_st   hpfL         , hpfR         ;
	jfloat wet1, wet2;
} sf_rv_earlyref_st;

// oversampling
// maximum oversampling factor
#define SF_REVERB_OF        4
typedef struct {
	jint factor;           // oversampling factor [1 to SF_REVERB_OF]
	sf_rv_biquad_st lpfU; // lowpass filter used for upsampling
	sf_rv_biquad_st lpfD; // lowpass filter used for downsampling
} sf_rv_oversample_st;

// dc cut
typedef struct {
	jfloat gain;
	jfloat y1;
	jfloat y2;
} sf_rv_dccut_st;

// fractal noise cache
// noise buffer size; must be a power of 2 because it's generated via fractal generator
#define SF_REVERB_NS        (1<<15)
typedef struct {
	jint pos;                 // current read position in the buffer
	jfloat buf[SF_REVERB_NS]; // buffer filled with noise
} sf_rv_noise_st;

// low-frequency oscilator (LFO)
typedef struct {
	jfloat re;  // real part
	jfloat im;  // imaginary part
	jfloat sn;  // sin of angle increment per sample
	jfloat co;  // cos of angle increment per sample
	jint count; // number of samples generated so far (used to apply small corrections over time)
} sf_rv_lfo_st;

// all-pass filter
// maximum size
#define SF_REVERB_APS       6299
typedef struct {
	jint pos;
	jint size;
	jfloat feedback;
	jfloat decay;
	jfloat buf[SF_REVERB_APS];
} sf_rv_allpass_st;

// 2nd order all-pass filter
// maximum sizes of the two buffers
#define SF_REVERB_AP2S1     11437
#define SF_REVERB_AP2S2     3449
typedef struct {
	//    line 1                 line 2
	jint   pos1                 , pos2                 ;
	jint   size1                , size2                ;
	jfloat feedback1            , feedback2            ;
	jfloat decay1               , decay2               ;
	jfloat buf1[SF_REVERB_AP2S1], buf2[SF_REVERB_AP2S2];
} sf_rv_allpass2_st;

// 3rd order all-pass filter with modulation
// maximum sizes of the three buffers and maximum mod size of the first line
#define SF_REVERB_AP3S1     8171
#define SF_REVERB_AP3M1     683
#define SF_REVERB_AP3S2     4597
#define SF_REVERB_AP3S3     7541
typedef struct {
	//    line 1 (with modulation)                 line 2                 line 3
	jint   rpos1, wpos1                           , pos2                 , pos3                 ;
	jint   size1, msize1                          , size2                , size3                ;
	jfloat feedback1                              , feedback2            , feedback3            ;
	jfloat decay1                                 , decay2               , decay3               ;
	jfloat buf1[SF_REVERB_AP3S1 + SF_REVERB_AP3M1], buf2[SF_REVERB_AP3S2], buf3[SF_REVERB_AP3S3];
} sf_rv_allpass3_st;

// modulated all-pass filter
// maximum size and maximum mod size
#define SF_REVERB_APMS      8681
#define SF_REVERB_APMM      137
typedef struct {
	jint rpos, wpos;
	jint size, msize;
	jfloat feedback;
	jfloat decay;
	jfloat z1;
	jfloat buf[SF_REVERB_APMS + SF_REVERB_APMM];
} sf_rv_allpassm_st;

// comb filter
// maximum size of the buffer
#define SF_REVERB_CS        4229
typedef struct {
	jint pos;
	jint size;
	jfloat buf[SF_REVERB_CS];
} sf_rv_comb_st;

//
// the final reverb state structure
//
// note: this is about 2megs, so you might not want to throw these around willy-nilly
typedef struct {
	sf_rv_earlyref_st   earlyref;
	sf_rv_oversample_st oversampleL, oversampleR;
	sf_rv_dccut_st      dccutL     , dccutR     ;
	sf_rv_noise_st      noise;
	sf_rv_lfo_st        lfo1;
	sf_rv_iir1_st       lfo1_lpf;
	sf_rv_allpassm_st   diffL[10]  , diffR[10]  ;
	sf_rv_allpass_st    crossL[4]  , crossR[4]  ;
	sf_rv_iir1_st       clpfL      , clpfR      ; // cross LPF
	sf_rv_delay_st      cdelayL    , cdelayR    ; // cross delay
	sf_rv_biquad_st     bassapL    , bassapR    ; // bass all-pass
	sf_rv_biquad_st     basslpL    , basslpR    ; // bass lowpass
	sf_rv_iir1_st       damplpL    , damplpR    ; // dampening lowpass
	sf_rv_allpassm_st   dampap1L   , dampap1R   ; // dampening all-pass (1)
	sf_rv_delay_st      dampdL     , dampdR     ; // dampening delay
	sf_rv_allpassm_st   dampap2L   , dampap2R   ; // dampening all-pass (2)
	sf_rv_delay_st      cbassd1L   , cbassd1R   ; // cross-fade bass delay (1)
	sf_rv_allpass2_st   cbassap1L  , cbassap1R  ; // cross-fade bass allpass (1)
	sf_rv_delay_st      cbassd2L   , cbassd2R   ; // cross-fade bass delay (2)
	sf_rv_allpass3_st   cbassap2L  , cbassap2R  ; // cross-fade bass allpass (2)
	sf_rv_lfo_st        lfo2;
	sf_rv_iir1_st       lfo2_lpf;
	sf_rv_comb_st       combL      , combR      ;
	sf_rv_biquad_st     lastlpfL   , lastlpfR   ;
	sf_rv_delay_st      lastdelayL , lastdelayR ;
	sf_rv_delay_st      inpdelayL  , inpdelayR  ;
	jint outco[32];
	jfloat loopdecay;
	jfloat wet1, wet2;
	jfloat wander;
	jfloat bassb;
	jfloat ertolate; // early reflection mix parameters
	jfloat erefwet;
	jfloat dry;
} sf_reverb_state_st;

typedef enum {
	SF_REVERB_PRESET_DEFAULT,
	SF_REVERB_PRESET_SMALLHALL1,
	SF_REVERB_PRESET_SMALLHALL2,
	SF_REVERB_PRESET_MEDIUMHALL1,
	SF_REVERB_PRESET_MEDIUMHALL2,
	SF_REVERB_PRESET_LARGEHALL1,
	SF_REVERB_PRESET_LARGEHALL2,
	SF_REVERB_PRESET_SMALLROOM1,
	SF_REVERB_PRESET_SMALLROOM2,
	SF_REVERB_PRESET_MEDIUMROOM1,
	SF_REVERB_PRESET_MEDIUMROOM2,
	SF_REVERB_PRESET_LARGEROOM1,
	SF_REVERB_PRESET_LARGEROOM2,
	SF_REVERB_PRESET_MEDIUMER1,
	SF_REVERB_PRESET_MEDIUMER2,
	SF_REVERB_PRESET_PLATEHIGH,
	SF_REVERB_PRESET_PLATELOW,
	SF_REVERB_PRESET_LONGREVERB1,
	SF_REVERB_PRESET_LONGREVERB2
} sf_reverb_preset;

// populate a reverb state with a preset
void sf_presetreverb(sf_reverb_state_st *state, jint rate, sf_reverb_preset preset);

// populate a reverb state with advanced parameters
void sf_advancereverb(sf_reverb_state_st *rv,
	jint rate,             // input sample rate (samples per second)
	jint oversamplefactor, // how much to oversample [1 to 4]
	jfloat ertolate,       // early reflection amount [0 to 1]
	jfloat erefwet,        // dB, final wet mix [-70 to 10]
	jfloat dry,            // dB, final dry mix [-70 to 10]
	jfloat ereffactor,     // early reflection factor [0.5 to 2.5]
	jfloat erefwidth,      // early reflection width [-1 to 1]
	jfloat width,          // width of reverb L/R mix [0 to 1]
	jfloat wet,            // dB, reverb wetness [-70 to 10]
	jfloat wander,         // LFO wander amount [0.1 to 0.6]
	jfloat bassb,          // bass boost [0 to 0.5]
	jfloat spin,           // LFO spin amount [0 to 10]
	jfloat inputlpf,       // Hz, lowpass cutoff for input [200 to 18000]
	jfloat basslpf,        // Hz, lowpass cutoff for bass [50 to 1050]
	jfloat damplpf,        // Hz, lowpass cutoff for dampening [200 to 18000]
	jfloat outputlpf,      // Hz, lowpass cutoff for output [200 to 18000]
	jfloat rt60,           // reverb time decay [0.1 to 30]
	jfloat delay           // seconds, amount of delay [-0.5 to 0.5]
);

// this function will process the input sound based on the state passed
// the input and output buffers should be the same size
void sf_reverb_process(sf_reverb_state_st *state, jint size, sf_sample_st *input,
	sf_sample_st *output);

#endif // SNDFILTER_REVERB__H
