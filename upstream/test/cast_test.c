/* The shareware demos don't exercise the DOOM II cast finale. Check its
   early-return timing explicitly, using the actual amalgamated code. */
#define main doom_main
#include "../../doom.c"
#undef main
#include <assert.h>

void DG_Init(void) {}
void DG_DrawFrame(void) {}
void DG_SleepMs(uint32_t ms) { (void)ms; }
uint32_t DG_GetTicksMs(void) { return 0; }
int DG_GetKey(int *pressed, unsigned char *key)
{
    (void)pressed;
    (void)key;
    return 0;
}
void DG_SetWindowTitle(const char *title) { (void)title; }
int DG_MakeDirectory(const char *path) { (void)path; return -1; }

static void expect_stopped(void)
{
    assert(!castattacking);
    assert(castframes == 0);
    assert(caststate == &states[mobjinfo[castorder[castnum].type].seestate]);
    assert(casttics == 0); /* Do not reload the new state's duration. */
}

int main(void)
{
    state_t previous = {0};
    castnum = 0;

    /* Player attack special case skips normal state advancement. */
    caststate = &states[S_PLAY_ATK1];
    casttics = 1;
    castframes = 11;
    castattacking = true;
    F_CastTicker();
    expect_stopped();

    /* An attack also stops after 24 frames, or upon reaching seestate. */
    previous.tics = 1;
    previous.nextstate = S_POSS_RUN2;
    caststate = &previous;
    casttics = 1;
    castframes = 23;
    castattacking = true;
    F_CastTicker();
    expect_stopped();

    previous.nextstate = mobjinfo[castorder[castnum].type].seestate;
    caststate = &previous;
    casttics = 1;
    castframes = 3;
    castattacking = true;
    F_CastTicker();
    expect_stopped();

    /* Ordinary advancement still reloads the next state's duration. */
    previous.nextstate = S_POSS_RUN2;
    caststate = &previous;
    casttics = 1;
    castframes = 3;
    castattacking = false;
    F_CastTicker();
    assert(caststate == &states[S_POSS_RUN2]);
    assert(castframes == 4);
    assert(casttics == states[S_POSS_RUN2].tics);
    puts("cast timing: PASS");
    return 0;
}
