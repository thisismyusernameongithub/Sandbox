#ifndef CSS_PROFILE_H_
#define CSS_PROFILE_H_

#include <stdint.h> //Types
#include <stdio.h> //sprintf()
#include <string.h> //memcpy()


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

static void printProfileResults(void);

typedef struct{
    char id[PROFILE_MAX_NAME]; //Name of profile
    uint8_t initialized; //If profile has been initialized
    uint32_t numberOfCalls; //Times profiled
    TIMETYPE lowTime;    //Shortest time for profile to run
    TIMETYPE avgTime;    //Average time for profile to run
    TIMETYPE highTime;   //Longest time for profile to run
    TIMETYPE totalTime;  //Total time profile has ran
}profile_t;

typedef struct{
    int initialized;
    uint32_t counterBase;
    int numberOfProfiles;
    profile_t profiles[PROFILE_MAX_PROFILES];
    void (*print)(void);  // Function pointer for printing results
} css_profile_t;
    
static css_profile_t css_profile = {
    .counterBase = __COUNTER__,
    .initialized = 1,
    .numberOfProfiles = 0,
    .print = printProfileResults
};

//TODO: Add filename to printout, it will print the name of the file this was included in.
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

static void printProfileResults()
{
    //Print profile result
    int nameWidth = 20; //Minimum 20
    for(int i=0;i<css_profile.numberOfProfiles;i++){
        int len = strlen(css_profile.profiles[i].id) + 1;
        nameWidth = MAX(nameWidth, len);
    }
    


    printf("\n Total runtime: %f seconds \n", (double)window.time.ms1 / 1000.0);

    printf("\n");
    printf("┏━┫Profiling results┣━"); for(int i=0;i<nameWidth - 20;i++){ printf("━");}                       printf("┳━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━┓\n");
    printf("┃ Name "); for(int i=0;i<nameWidth - 5;i++){ printf(" ");}                                       printf("┃ noCalls ┃   low   ┃   avg   ┃   high  ┃       total       ┃\n");
    printf("┣━"); for(int i=0;i<nameWidth;i++){ printf("━");}                                                printf("╇━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━━━━━━━━━━━┫\n");
    for(int i=0;i<=css_profile.numberOfProfiles;i++){
        printf("┃ %s", css_profile.profiles[i].id); for(unsigned int j=0;j<nameWidth - strlen(css_profile.profiles[i].id);j++){ printf(" ");}
        printf("│ %7d ", css_profile.profiles[i].numberOfCalls);
        printf("│ %01.5f ", css_profile.profiles[i].lowTime);
        printf("│ %01.5f ", css_profile.profiles[i].avgTime);
        printf("│ %01.5f ", css_profile.profiles[i].highTime);
        printf("│ %07.3f ", css_profile.profiles[i].totalTime);
        double percentage = css_profile.profiles[i].totalTime / ((double)window.time.ms1 / 100000.0);
        printf("(%06.2f%%) ", percentage);
        printf("┃ \n");
        if(i == css_profile.numberOfProfiles){ printf("┗━"); for(int i=0;i<nameWidth;i++){ printf("━");} printf("┷━━━━━━━━━┷━━━━━━━━━┷━━━━━━━━━┷━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━┛\n"); }
        else{ printf("┠─"); for(int i=0;i<nameWidth;i++){ printf("─");}                                      printf("┼─────────┼─────────┼─────────┼─────────┼───────────────────┨\n"); }
        
    }
}

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

