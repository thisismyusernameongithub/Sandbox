#ifndef CSS_PROFILE_H_
#define CSS_PROFILE_H_

#include <stdint.h> //Types
#include <stdio.h> //sprintf()


#define PROFILE_MAX_NAME (100) //Max length of a profile name
#define PROFILE_MAX_PROFILES (50) //Max number of profiles

#define MAX(a,b)              \
({                            \
    __typeof__ (a) _aa = (a); \
    __typeof__ (b) _bb = (b); \
    _aa > _bb ? _aa : _bb;    \
})

#define MIN(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b;       \
})

#define TIMESAMPLE (getWindowTime()) //Should contain or return the current time since start in a suitable resolution
#define TIMETYPE double //The type of TIMESAMPLE

typedef struct{
    char id[PROFILE_MAX_NAME]; //Name of profile
    uint8_t initialized; //If profile has been initialized
    uint32_t numberOfCalls; //Times profiled
    TIMETYPE lowTime;    //Shortest time for profile to run
    TIMETYPE avgTime;    //Average time for profile to run
    TIMETYPE highTime;   //Longest time for profile to run
    TIMETYPE totalTime;  //Total time profile has ran
}profile_t;

struct{
    int initialized;
    uint32_t counterBase;
    int numberOfProfiles;
    profile_t profiles[PROFILE_MAX_PROFILES];
}css_profile = {
    .counterBase = __COUNTER__,
    .initialized = 1,
    .numberOfProfiles = 0
};

//If __COUNTER__ Is used in the application then PROFILE_INIT is needed before any PROFILE() calls.
//Don't use __COUNTER__ between PROFILE() calls
//TODO: Add zeroing of __COUNTER__ to first PROFILE() call, use css_profile.initialized to detect and store base in css_profile.counterBase
#define PROFILE_INIT {css_profile.counterBase = __COUNTER__ ; css_profile.initialized = 1;}

#define PROFILE(func) \
{ \
        int profIndex =  __COUNTER__ - css_profile.counterBase - 1;  \
        TIMETYPE timeBefore = TIMESAMPLE;                            \
        func                                                         \
        TIMETYPE timeDiff = TIMESAMPLE - timeBefore;                 \
        if(!css_profile.profiles[profIndex].initialized)             \
        {                                                            \
            if(strlen(#func) < 24)                                   \
            {                                                        \
                sprintf(css_profile.profiles[profIndex].id, #func); \
            }                                                        \
            else                                                   \
            {                                                            \
                memcpy(css_profile.profiles[profIndex].id, #func, 20); \
                memcpy(&(css_profile.profiles[profIndex].id[20]), "...", 4); \
            }                                                             \
            css_profile.profiles[profIndex].initialized = 1;       \
            css_profile.profiles[profIndex].lowTime = 999.0;         \
            css_profile.numberOfProfiles++;                          \
        }                                                           \
        css_profile.profiles[profIndex].numberOfCalls++;           \
        css_profile.profiles[profIndex].totalTime += timeDiff;       \
        css_profile.profiles[profIndex].avgTime = css_profile.profiles[profIndex].totalTime / css_profile.profiles[profIndex].numberOfCalls; \
        css_profile.profiles[profIndex].lowTime = MIN(css_profile.profiles[profIndex].lowTime, timeDiff);              \
        css_profile.profiles[profIndex].highTime = MAX(css_profile.profiles[profIndex].highTime, timeDiff);              \
}\

//TODO: Add zeroing of __COUNTER__ to first PROFILE() call, use css_profile.initialized to detect and store base in css_profile.counterBase
//TODO: Add printout funciton that looks something like this
/*
┏━┫Profiling results┣━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━┓
┃ Name                        ┃ noCalls ┃   low   ┃   avg   ┃   high  ┃  total  ┃
┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━┫
┃ clearLayer(botLayer);       │      94 │ 0.00011 │ 0.00013 │ 0.00032 │ 000.013 ┃
┠─────────────────────────────┼─────────┼─────────┼─────────┼─────────┼─────────┨
┃ updateInput();              │      94 │ 0.00000 │ 0.00000 │ 0.00001 │ 000.000 ┃
┠─────────────────────────────┼─────────┼─────────┼─────────┼─────────┼─────────┨
┃ process(window.time.dTime); │      94 │ 0.00415 │ 0.00457 │ 0.00692 │ 000.429 ┃
┠─────────────────────────────┼─────────┼─────────┼─────────┼─────────┼─────────┨
┃ generateShadowMap();        │      94 │ 0.00060 │ 0.00080 │ 0.00122 │ 000.075 ┃
┠─────────────────────────────┼─────────┼─────────┼─────────┼─────────┼─────────┨
┃ generateColorMap();         │      94 │ 0.00041 │ 0.00044 │ 0.00057 │ 000.041 ┃
┠─────────────────────────────┼─────────┼─────────┼─────────┼─────────┼─────────┨
┃ render();                   │      94 │ 0.00333 │ 0.00381 │ 0.00538 │ 000.359 ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━┷━━━━━━━━━┷━━━━━━━━━┷━━━━━━━━━┷━━━━━━━━━┛
*/

#endif /* CSS_PROFILE_H_ */

