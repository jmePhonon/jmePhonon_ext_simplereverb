# jmePhonon_ext_simplereverb

Optional support for simple parametric reverberation in jmePhonon.

The scope of this extension is to provide some sort of backward compatibility with jme's audio mechanics.

Due to uncertainties about the license of reverb.c and reverb.h ( https://github.com/voidqk/sndfilter/issues/7 ), this code has not been included in the main repo.



# Usage

1. Clone the main jmePhonon repo
2. Create an empty directory in src/ext
3. Copy the content of this repo in src/ext
4. Set this env var 
```
export BUILD_ARGS="-DINCLUDE_SIMPLE_REVERB"
```
4. Compile as usual
