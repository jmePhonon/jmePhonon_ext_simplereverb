/*
This file is released into the public domain.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/
#include "ext/ext_SimpleReverb.h"
#include "reverb.h"
#include <stdlib.h>
#ifdef INCLUDE_SIMPLE_REVERB
struct {
    sf_reverb_state_st reverbState;
    sf_sample_st* reverbInputFrame;
    sf_sample_st* reverbOutputFrame;
    jboolean validEnv;
} SrContext;

void srInit(struct GlobalSettings *settings){
    SrContext.reverbInputFrame = malloc(sizeof(sf_sample_st) * settings->frameSize);
    SrContext.reverbOutputFrame = malloc(sizeof(sf_sample_st) * settings->frameSize);
    // jfloat s[2] = {-1.f,0.f};
    SrContext.validEnv = false;
    // srSetEnvironment(settings, s); // set default env
}

void srDestroy(struct GlobalSettings *settings){
    free(SrContext.reverbInputFrame);
    free( SrContext.reverbOutputFrame);
}

jboolean srHasValidEnvironment(struct GlobalSettings *settings){
    return SrContext.validEnv;
}

void srSetEnvironment(struct GlobalSettings *settings,jfloat *env){
    if(env[0]==-2){
        SrContext.validEnv = false;
        return;
    }
    SrContext.validEnv = true;
    if (env[0] == -1) {
        sf_presetreverb(&SrContext.reverbState, settings->sampleRate, (jint)env[1]);
        return;
    }
    sf_advancereverb(&SrContext.reverbState,
                settings->sampleRate,             // input sample rate (samples per second)
                env[0], // how much to oversample [1 to 4]
                env[1],       // early reflection amount [0 to 1]
                env[2],        // dB, final wet mix [-70 to 10]
                env[3],            // dB, final dry mix [-70 to 10]
                env[4],     // early reflection factor [0.5 to 2.5]
                env[5],      // early reflection width [-1 to 1]
                env[6],          // width of reverb L/R mix [0 to 1]
                env[7],            // dB, reverb wetness [-70 to 10]
                env[8],         // LFO wander amount [0.1 to 0.6]
                env[9],          // bass boost [0 to 0.5]
                env[10],           // LFO spin amount [0 to 10]
                env[11],       // Hz, lowpass cutoff for input [200 to 18000]
                env[12],        // Hz, lowpass cutoff for bass [50 to 1050]
                env[13],        // Hz, lowpass cutoff for dampening [200 to 18000]
                env[14],      // Hz, lowpass cutoff for output [200 to 18000]
                env[15],           // reverb time decay [0.1 to 30]
                env[16]           // seconds, amount of delay [-0.5 to 0.5]
    );
}

 static inline void srInterleavedToSfFrame(struct GlobalSettings *settings,jfloat *frame,sf_sample_st *store){
    if(settings->nOutputChannels!=2){
        printf("Sfx: Error, this works only with stereo audio \n");
    }
    jint ii = 0;
    for(jint i=0;i<settings->frameSize;i++){
        store[i].L = frame[ii++];
	    store[i].R = frame[ii++];	
    }
}

static inline  void srSfToInterleavedFrame(struct GlobalSettings *settings,sf_sample_st *frame,jfloat *store){
    if(settings->nOutputChannels!=2){
        printf("Sfx: Error, this works only with stereo audio \n");
    }
    jint ii = 0;
    for (jint i = 0; i < settings->frameSize; i++) {
        store[ii++] = frame[i].L;
	    store[ii++] = frame[i].R;
    }
}

// #define TEST_CONVERSION 1
// #define TEST_COPY_INPUT 1
/**
 * Both input and output must be stereo
 */
void srApplyReverb(struct GlobalSettings *settings,jfloat *inframe,jfloat *outframe){
    srInterleavedToSfFrame(settings, inframe, SrContext.reverbInputFrame);
    #ifndef TEST_CONVERSION
        sf_reverb_process(&SrContext.reverbState, settings->frameSize, SrContext.reverbInputFrame, SrContext.reverbOutputFrame);
        srSfToInterleavedFrame(settings, SrContext.reverbOutputFrame, outframe);
    #else
        srSfToInterleavedFrame(settings, SrContext.reverbInputFrame, outframe);
    #endif
        #ifdef TEST_COPY_INPUT
                jint ii = 0;
                for (jint i = 0; i < settings->frameSize; i++) {
                outframe[ii]=inframe[ii++];
                        outframe[ii]=inframe[ii++];

                }
        #endif
}
#endif