#ifndef ALADDIN_COOP_H
#define ALADDIN_COOP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Aladdin Player 2 Action States */
#define ALADDIN_STATE_IDLE              0
#define ALADDIN_STATE_RUN               1
#define ALADDIN_STATE_JUMP              2
#define ALADDIN_STATE_SLASH             3
#define ALADDIN_STATE_CROUCH            4
#define ALADDIN_STATE_THROW_GROUND      5
#define ALADDIN_STATE_THROW_AIR         6
#define ALADDIN_STATE_CLIMB_VERTICAL    7
#define ALADDIN_STATE_HANG_HORIZONTAL   8

/* Projectile System */
#define MAX_P2_PROJECTILES              4

typedef struct {
    int      active;           /* 1 if projectile is active in flight */
    int16_t  world_x;          /* 16-bit world X coordinate */
    int16_t  world_y;          /* 16-bit world Y coordinate */
    int16_t  screen_x;         /* screen X (-16..272) */
    int16_t  screen_y;         /* screen Y (-16..240) */
    int16_t  vx;               /* horizontal velocity */
    int16_t  vy;               /* vertical velocity */
    uint8_t  anim_timer;       /* frame counter for spin animation */
} AladdinP2Projectile;

typedef struct {
    int      active;           /* 1 if Player 2 is participating */
    int16_t  world_x;          /* 16-bit world X coordinate */
    int16_t  world_y;          /* 16-bit world Y coordinate */
    int16_t  screen_x;         /* screen X (0..255) */
    int16_t  screen_y;         /* screen Y (0..224) */
    int16_t  vx;               /* horizontal velocity */
    int16_t  vy;               /* vertical velocity */
    uint8_t  facing;           /* 0 = facing right, 1 = facing left */
    uint8_t  state;            /* ALADDIN_STATE_* */
    uint8_t  anim_frame;       /* animation step */
    uint8_t  anim_timer;       /* frame counter for animation ticks */
    uint8_t  health;           /* player 2 health (hearts, 1..3) */
    uint8_t  apples;           /* player 2 apple ammo */
    uint8_t  attack_timer;     /* throw / attack action lock frames remaining */
    uint8_t  invuln_timer;     /* damage flash timer */
    uint8_t  grounded;         /* 1 if on solid ground */
    uint8_t  spawned;          /* 1 once placed in level */
    uint16_t last_pad;         /* previous frame input for edge triggers */
    uint8_t  rope_cooldown;    /* cooldown after jumping off rope to prevent immediate re-attachment */
    int16_t  rope_anchor_x;    /* current locked rope X axis */
    int16_t  rope_anchor_y;    /* current locked rope Y axis */
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

/* Access Player 2 projectiles array */
AladdinP2Projectile *AladdinCoop_GetProjectiles(void);

#ifdef __cplusplus
}
#endif

#endif /* ALADDIN_COOP_H */
