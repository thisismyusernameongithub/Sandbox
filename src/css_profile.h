#ifndef CSS_PROFILE_H_
#define CSS_PROFILE_H_

#include <stdint.h> //Types
#include <stdio.h> //sprintf()
#include <string.h> //memcpy()
#include <stdbool.h> //bool


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
    TIMETYPE lastTime;   //Latest profile time
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
        css_profile.profiles[profIndex].lastTime = timeDiff;         \
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


static inline float normalizef(float value, float min, float max)
{
	return (value - min) / (max - min);
}

static void drawProfile(Layer layer, int x, int y)
{
	#define PROF_HISTORY_SIZE 50

	int profX = x;
	int profY = y;
	int margin = 5;
	int barWidth = 3;
	int profW = 2 * margin + (barWidth + 1) * PROF_HISTORY_SIZE - 1;
	int profH = 100;
	Layer profLayer = layer;
	static bool paused = false;

	static struct{
		double dTime_max;
		double dTime_min;
		double dTime_avg;
		double dTime[PROF_HISTORY_SIZE];
		double profile[PROFILE_MAX_PROFILES * PROF_HISTORY_SIZE];
		int head;
	}history = {.dTime_min = 0.0, .dTime_avg = 20.0, .dTime_max = 30.0};
	
	if(!paused)
	{

		// Sample current dTime
		history.head = (history.head + 1) % PROF_HISTORY_SIZE;
		history.dTime[history.head] = window.time.dTime * 1000.f; // Store dTime in ms
		// printf("dTime: %.2f, min: %.2f, max: %.2f\n", window.time.dTime * 1000.f, history.dTime_min, history.dTime_max);
		
		// Calculate moving average
		double avg = 0.0;
		for(int i = 0; i < PROF_HISTORY_SIZE; i++)
		{
			avg += history.dTime[i];
		}
		history.dTime_avg = avg / PROF_HISTORY_SIZE;

		
		
		// Increase max dTime if needed, decrease it if it is too high
		if(history.dTime_avg > history.dTime_max - 10.0)
		{
			history.dTime_max += 10.0;
		}
		else if(history.dTime_avg < history.dTime_max - 20.0)
		{
			history.dTime_max -= 10.0;
		}
		// Increase min dTime if needed, decrease it if it is too low
		if(history.dTime_avg < history.dTime_min + 10.0)
		{
			history.dTime_min -= 10.0;
		}
		else if(history.dTime_avg > history.dTime_min + 20.0)
		{
			history.dTime_min += 10.0;
		}
	
		// Sample profiles
		for(int i = 0; i < css_profile.numberOfProfiles; i++)
		{
			history.profile[history.head + i * PROF_HISTORY_SIZE] = css_profile.profiles[i].lastTime * 1000.f; // Store dTime in ms
		}
	}
	

	{ // Draw dTime widget

		// Draw profile containter
		drawSquare(profLayer, profX, profY, profW, profH, rgba(255, 255, 255, 0.25));
		drawLine(profLayer, profX, profY, profX + profW - 1, profY, rgba(255, 255, 255, 0.75));
		drawLine(profLayer, profX + profW - 1, profY, profX + profW - 1, profY + profH - 1, rgba(255, 255, 255, 0.75));
		drawLine(profLayer, profX, profY + profH - 1, profX + profW - 1, profY + profH - 1, rgba(255, 255, 255, 0.75));
		drawLine(profLayer, profX, profY + profH - 1, profX, profY, rgba(255, 255, 255, 0.75));

		// Draw fps and ms
		dw_text.size = 10;
		dw_text.color = rgb(255, 255, 255);
		drawText(profLayer, profX + margin, profY + margin, printfLocal("fps: %.2f, ms: %.2f", window.time.fps, 1000.f*window.time.dTime));
		
		// Draw Average line
		float avgLineY = profY + margin + profH - 2*margin - (profH - 2*margin) * normalizef(history.dTime_avg, history.dTime_min, history.dTime_max); 
		drawText(profLayer, profX + margin, avgLineY - 2*margin, printfLocal("avg: %.2f ms", history.dTime_avg));
		drawLine(profLayer, profX + margin, avgLineY, profX + profW - margin - 1, avgLineY, rgb(255, 0, 0));

		// Draw dTime as a histogram
		for(int i = 0; i < PROF_HISTORY_SIZE; i++)
		{
			int x = profX + margin + i * (barWidth + 1);
			int y = profY + margin;
			// int h = (int)(profH - 10) * (history.dTime[(history.head + 1 + i) % PROF_HISTORY_SIZE] - history.dTime_min) / (history.dTime_max - history.dTime_min);
			int h = (int)(profH - 2*margin) * normalizef(history.dTime[(history.head + 1 + i) % PROF_HISTORY_SIZE], history.dTime_min, history.dTime_max);
			int w = barWidth;
			drawSquare(profLayer, x, y + (profH - 2*margin - h), w, h, rgba(255, 255, 255, 0.8));
		}

	}

	{ // Draw profiles container

		static argb_t profilesColorPallete[16] = {
			rgb(231, 76, 60),    // Strong Red
			rgb(46, 204, 113),   // Emerald Green
			rgb(52, 152, 219),   // Bright Blue
			rgb(241, 196, 15),   // Vivid Yellow
			rgb(230, 126, 34),   // Orange
			rgb(26, 188, 156),   // Teal
			rgb(9, 132, 227),    // Royal Blue
			rgb(214, 48, 49),    // Crimson
			rgb(0, 184, 148),    // Aqua Green
			rgb(253, 203, 110),  // Saffron
			rgb(108, 92, 231),   // Light Violet
			rgb(255, 121, 63),   // Coral
			rgb(0, 206, 201),    // Cyan
			rgb(232, 67, 147),   // Pink
			rgb(30, 144, 255),   // Dodger Blue
			rgb(155, 89, 182),   // Purple
		};
		
		// Line graphs are drawn separately in its own widget box
		int profGraphX = profX + profW + 2*margin;
		int profGraphY = profY;
		int profGraphW = 120;
		int profGraphH = profH;
		drawSquare(profLayer, profGraphX, profGraphY, profGraphW, profGraphH, rgba(255, 255, 255, 0.25));
		drawLine(profLayer, profGraphX, profGraphY, profGraphX + profGraphW - 1, profGraphY, rgba(255, 255, 255, 0.75));
		drawLine(profLayer, profGraphX + profGraphW - 1, profGraphY, profGraphX + profGraphW - 1, profGraphY + profGraphH - 1, rgba(255, 255, 255, 0.75));
		drawLine(profLayer, profGraphX, profGraphY + profGraphH - 1, profGraphX + profGraphW - 1, profGraphY + profGraphH - 1, rgba(255, 255, 255, 0.75));
		drawLine(profLayer, profGraphX, profGraphY + profGraphH - 1, profGraphX, profGraphY, rgba(255, 255, 255, 0.75));

		for(int i = 0; i < css_profile.numberOfProfiles; i++)
		{
			int x = profGraphX + margin;
			int y = profGraphY + margin;
			argb_t color = profilesColorPallete[i % 16];
			profile_t profile = css_profile.profiles[i];
			for(int j = 0; j < PROF_HISTORY_SIZE - 1; j++) // Don't draw latest sample because then we need to handle the endpoint of that line which wraps back to the oldest sample
			{
				int x1 = x + (j * (profGraphW - 2*margin)) / (PROF_HISTORY_SIZE - 1);
				int y1 = y + (profGraphH - 2*margin) - (profGraphH - 2*margin) * (history.profile[(history.head + 1 + j) % PROF_HISTORY_SIZE + i * PROF_HISTORY_SIZE] / history.dTime_max);
				int x2 = x + ((j + 1) * (profGraphW - 2*margin)) / (PROF_HISTORY_SIZE - 1) - 1;
				int y2 = y + (profGraphH - 2*margin) - (profGraphH - 2*margin) * (history.profile[(history.head + 1 + (j + 1)) % PROF_HISTORY_SIZE + i * PROF_HISTORY_SIZE] / history.dTime_max);
				// int y1 = y + (profH - 20 - (profH - 20) * (history.profile[(history.head + 1 + j) % PROF_HISTORY_SIZE + i * PROF_HISTORY_SIZE] - history.dTime_min) / (history.dTime_max - history.dTime_min));
				// int y1 = profY + margin + profH - 2*margin -  (profH - 2*margin) * normalizef(history.profile[(history.head + 1 + j) % PROF_HISTORY_SIZE + i * PROF_HISTORY_SIZE], history.dTime_min, history.dTime_max);
				// int y2 = y + (profH - 20 - (profH - 20) * (history.profile[(history.head + 1 + (j + 1)) % PROF_HISTORY_SIZE + i * PROF_HISTORY_SIZE] - history.dTime_min) / (history.dTime_max - history.dTime_min));
				// int y2 = profY + margin + profH - 2*margin -  (profH - 2*margin) * normalizef(history.profile[(history.head + 1 + (j + 1)) % PROF_HISTORY_SIZE + i * PROF_HISTORY_SIZE], history.dTime_min, history.dTime_max);
				drawLine(profLayer, x1, y1, x2, y2, color);
			}
		}

		int profilesX = profGraphX + profGraphW + 2*margin;
		int profilesY = profGraphY;
		int profilesW = 429;
		int profilesH = profGraphH;
		drawSquare(profLayer, profilesX, profilesY, profilesW, profilesH, rgba(255, 255, 255, 0.25));
		drawLine(profLayer, profilesX, profilesY, profilesX + profilesW - 1, profilesY, rgba(255, 255, 255, 0.75));
		drawLine(profLayer, profilesX + profilesW - 1, profilesY, profilesX + profilesW - 1, profilesY + profilesH - 1, rgba(255, 255, 255, 0.75));
		drawLine(profLayer, profilesX, profilesY + profilesH - 1, profilesX + profilesW - 1, profilesY + profilesH - 1, rgba(255, 255, 255, 0.75));
		drawLine(profLayer, profilesX, profilesY + profilesH - 1, profilesX, profilesY, rgba(255, 255, 255, 0.75));

		// Draw profiles as line diagrams

		drawText(profLayer, profilesX + margin, profilesY + margin, "     ID    |   last  |   low   |   avg   |   high  |  calls  |  total");
		for(int i = 0; i < css_profile.numberOfProfiles; i++)
		{
			int x = profilesX + margin;
			int y = profilesY + margin;
			argb_t color = profilesColorPallete[i % 16];
			// Draw profile name and stats
			dw_text.color = lerpargb(color, rgb(0, 0, 0), 0.1f);
			dw_text.size = 10;
			profile_t profile = css_profile.profiles[i];
			drawText(profLayer, x, y + (i + 1) * dw_text.size, printfLocal("%10.10s | %01.5f | %01.5f | %01.5f | %01.5f | %7d | %07.3f", 
																								css_profile.profiles[i].id, 
																								profile.lastTime,
																								profile.lowTime,
																								profile.avgTime,
																								profile.highTime,
																								profile.numberOfCalls, 
																								profile.totalTime));
		}

	}

} 

#endif /* CSS_PROFILE_H_ */
