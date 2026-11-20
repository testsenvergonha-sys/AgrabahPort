#ifndef ALADDIN_COOP_H
#define ALADDIN_COOP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int      active;           /* 1 if Player 2 is participating */
    int16_t  world_x;          /* 16-bit world X coordinate */
    int16_t  world_y;          /* 16-bit world Y coordinate */
    int16_t  screen_x;         /* screen X (0..255) */
    int16_t  screen_y;         /* screen Y (0..224) */
    int16_t  vx;               /* horizontal velocity */
    int16_t  vy;               /* vertical velocity */
    uint8_t  facing;           /* 0 = facing right, 1 = facing left */
    uint8_t  state;            /* 0=idle, 1=run, 2=jump, 3=slash, 4=crouch */
    uint8_t  anim_frame;       /* animation step */
    uint8_t  anim_timer;       /* frame counter for animation ticks */
    uint8_t  health;           /* player 2 health (hearts, 1..3) */
    uint8_t  apples;           /* player 2 apple ammo */
    uint8_t  attack_timer;     /* sword swing frames remaining */
    uint8_t  invuln_timer;     /* damage flash timer */
    uint8_t  grounded;         /* 1 if on solid ground */
    uint8_t  spawned;          /* 1 once placed in level */
} AladdinGuestPlayer;

/* Initialize / reset co-op system */
void AladdinCoop_Init(void);
void AladdinCoop_Reset(void);

/* Called at the end of each simulated frame (in GameRunOneFrame) */
void AladdinCoop_PostFrame(void);

/* Called right before drawing each frame (in GameDrawPpuFrame) */
void AladdinCoop_PrepareDraw(void);

/* Check if 2-player co-op is currently active in gameplay */
int AladdinCoop_IsActive(void);

/* Enable / disable co-op */
void AladdinCoop_SetEnabled(int enabled);

/* Access Player 2 guest state */
AladdinGuestPlayer *AladdinCoop_GetPlayer2(void);

#ifdef __cplusplus
}
#endif

#endif /* ALADDIN_COOP_H */
