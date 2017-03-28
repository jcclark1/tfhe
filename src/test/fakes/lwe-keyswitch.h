#ifndef FAKE_LWE_KEYSWITCH_H
#define FAKE_LWE_KEYSWITCH_H

#include "tfhe.h"
#include "tfhe_core.h"
#include "lwesamples.h"
#include "./lwe.h"



/*
<<<<<<< HEAD

// Fake LWE structure 
struct FakeLweKeySwitchKey {
    static const int FAKE_KS_UID=25789314; // precaution: do not confuse fakes with trues
    const int fake_uid;
    LweSample*** ks;
    double current_variance;

    // construct
    FakeLweKeySwitchKey(int n, int t, int basebit, const LweParams* out_params):fake_uid(FAKE_KS_UID) {
        int base = 1<<basebit;
        ks0_raw = new_LweSample_array(n*t*base,out_params);
        ks1_raw = new LweSample*[n*t];
        ks = new LweSample**[n];

        for (int p = 0; p < n*t; ++p)
            ks1_raw[p] = ks0_raw + base*p;
        for (int p = 0; p < n; ++p)
            ks[p] = ks1_raw + t*p;
        
        current_variance = 0.;
    }

    // delete
    ~FakeLweKeySwitchKey() {
        if (fake_uid!=FAKE_KS_UID) abort();
        delete_LweSample_array(ks0_raw);
        delete[] ks1_raw;
        delete[] ks;
    }
    FakeLweKeySwitchKey(const FakeLweKeySwitchKey&)=delete;
    void operator=(const FakeLweKeySwitchKey&)=delete;
};


*/
class FakeLweKeyswitchKey {
    public:
    static const long FAKE_LWEKEYSWITCH_UID = 956475132024584l; // precaution: distinguish fakes from trues
    const long fake_uid;
    const double variance_overhead;

    //this padding is here to make sure that FakeTLwe and TLweSample have the same size
    char unused_padding[sizeof(LweKeySwitchKey)-sizeof(long)-sizeof(double)];

    FakeLweKeyswitchKey(): fake_uid(FAKE_LWE_KEYSWITCH_H) {}
};

// At compile time, we verify that the two structures have exactly the same size
static_assert (sizeof(FakeLweKeySwitchKey) == sizeof(LweKeySwitchKey), "Error: Size is not correct");



inline FakeLweKeySwitchKey* fake(LweKeySwitchKey* key) {
    FakeLweKeyswitchKey* reps = (FakeLweKeyswitchKey*) key;
    if (reps->fake_uid!=FakeLweKeyswitchKey::FAKE_LWEKEYSWITCH_UID) abort();
    return reps;
}

inline const FakeLweKeySwitchKey* fake(const LweKeySwitchKey* key) {
    const FakeLweKeyswitchKey* reps = (const FakeLweKeyswitchKey*) key;
    if (reps->fake_uid!=FakeLweKeyswitchKey::FAKE_LWEKEYSWITCH_UID) abort();
    return reps;
}



/**
 * fills the KeySwitching key array
 * @param result The (n x t x base) array of samples. 
 *        result[i][j][k] encodes k.s[i]/base^(j+1)
 * @param out_key The LWE key to encode all the output samples 
 * @param out_alpha The standard deviation of all output samples
 * @param in_key The (binary) input key
 * @param n The size of the input key
 * @param t The precision of the keyswitch (technically, 1/2.base^t)
 * @param basebit Log_2 of base
 */
inline void fake_lweCreateKeySwitchKey_fromArray(LweSample*** result, 
	const LweKey* out_key, const double out_alpha, 
	const int* in_key, const int n, const int t, const int basebit){
    const int base=1<<basebit;       // base=2 in [CGGI16]

    for(int i=0;i<n;i++) {
	for(int j=0;j<t;j++){
	    for(int k=0;k<base;k++){
		Torus32 x=(in_key[i]*k)*(1<<(32-(j+1)*basebit));
		fake_lweSymEncrypt(&result[i][j][k],x,out_alpha,out_key);
	    }
	}
    }
}

#define USE_FAKE_lweCreateKeySwitchKey_fromArray \
    inline void lweCreateKeySwitchKey_fromArray(LweSample*** result, \
	    const LweKey* out_key, const double out_alpha, \
	    const int* in_key, const int n, const int t, const int basebit){ \
	fake_lweCreateKeySwitchKey_fromArray(result, \
		out_key, out_alpha, \
		in_key, n, t, basebit); \
    }



/**
 * translates the message of the result sample by -sum(a[i].s[i]) where s is the secret
 * embedded in ks.
 * @param result the LWE sample to translate by -sum(ai.si). 
 * @param ks The (n x t x base) key switching key 
 *        ks[i][j][k] encodes k.s[i]/base^(j+1)
 * @param params The common LWE parameters of ks and result
 * @param ai The input torus array
 * @param n The size of the input key
 * @param t The precision of the keyswitch (technically, 1/2.base^t)
 * @param basebit Log_2 of base
 */
void fake_lweKeySwitchTranslate_fromArray(LweSample* result, 
	const LweSample*** ks, const LweParams* params, 
	const Torus32* ai, 
	const int n, const int t, const int basebit){
    const int base=1<<basebit;       // base=2 in [CGGI16]
    const int32_t prec_offset=1<<(32-(1+basebit*t)); //precision
    const int mask=base-1;

    for (int i=0;i<n;i++){
	const uint32_t aibar=ai[i]+prec_offset;
	for (int j=0;j<t;j++){
	    const uint32_t aij=(aibar>>(32-(j+1)*basebit)) & mask;
	    fake_lweSubTo(result,&ks[i][j][aij],params);
	}
    }
}

#define USE_FAKE_lweKeySwitchTranslate_fromArray \
    inline void lweKeySwitchTranslate_fromArray(LweSample* result, \
	    const LweSample*** ks, const LweParams* params, \
	    const Torus32* ai, \
	    const int n, const int t, const int basebit){ \
	fake_lweKeySwitchTranslate_fromArray(result, \
		ks, params, \
		ai, \
		n, t, basebit); \
    }


inline void fake_lweCreateKeySwitchKey(LweKeySwitchKey* result, const LweKey* in_key, const LweKey* out_key){
    const double variance = out_key->params->alpha_min*out_key->params->alpha_min;
    const int n=ks->n;
    const int basebit=ks->basebit;
    const int t=ks->t;
    const double epsilon2 = pow(0.5,2*(basebit*t+1));

    const double variance_overhead = n*t*variance+n*epsilon2;

    FakeLweKeyswitchKey* fres = fake(result);
    fres->variance_overhead = variance_overhead;
}

#define USE_FAKE_lweCreateKeySwitchKey \
    inline void lweCreateKeySwitchKey(LweKeySwitchKey* result, const LweKey* in_key, const LweKey* out_key) { \
	fake_lweCreateKeySwitchKey(result, in_key, out_key); \
    }


//sample=(a',b')
inline void fake_lweKeySwitch(LweSample* result, const LweKeySwitchKey* ks, const LweSample* sample){
    FakeLwe* fres = fake(result);
    const FakeLweKeyswitchKey* fks = fake(ks);
    const FakeLwe* fsample = fake(sample);

    fres->message = fsample->message;
    fres->current_variance = fsample->current_variance+fks->variance_overhead;
}

#define USE_FAKE_lweKeySwitch \
    inline void lweKeySwitch(LweSample* result, const LweKeySwitchKey* ks, const LweSample* sample) { \
	fake_lweKeySwitch(result, ks, sample); \
    } 


#endif // FAKE_LWE_KEYSWITCH_H
