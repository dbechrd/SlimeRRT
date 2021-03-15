#include "helpers.h"
#include "particles.h"
#include "player.h"
#include "slime.h"
#include "spritesheet.h"
#include "tileset.h"
#include "tilemap.h"
#include "raylib.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static FILE *logFile;

void traceLogCallback(int logType, const char *text, va_list args)
{
    vfprintf(logFile, text, args);
    fputs("\n", logFile);
    vfprintf(stdout, text, args);
    fputs("\n", stdout);
    fflush(logFile);
}

Rectangle RectPad(const Rectangle rec, float pad)
{
    Rectangle padded = (Rectangle){ rec.x - pad, rec.y - pad, rec.width + pad * 2.0f, rec.height + pad * 2.0f };
    return padded;
}

Rectangle RectPadXY(const Rectangle rec, float padX, float padY)
{
    Rectangle padded = (Rectangle){ rec.x - padX, rec.y - padY, rec.width + padX * 2.0f, rec.height + padY * 2.0f };
    return padded;
}

void DrawShadow(int x, int y, float radius, float yOffset)
{
    DrawEllipse(x, y + (int)yOffset, radius, radius / 2.0f, Fade(BLACK, 0.5f));
}

void DrawHealthBar(int x, int y, float hitPoints, float maxHitPoints)
{
    Font font = GetFontDefault();
    const float hpPercent = hitPoints / maxHitPoints;
    const char *hpText = TextFormat("HP: %.02f / %.02f", hitPoints, maxHitPoints);

    const int hp_w = MeasureText(hpText, font.baseSize);
    const int hp_h = font.baseSize;
    const int hp_x = x - hp_w / 2;
    //const int hp_y = y - hp_h / 2;
    const int hp_y = y - font.baseSize;

    const int pad_x = 4;
    const int pad_y = 2;
    const int bg_x = hp_x - pad_x;
    const int bg_y = hp_y - pad_y;
    const int bg_w = hp_w + pad_x * 2;
    const int bg_h = hp_h + pad_y * 2;

    // Draw hitpoint indicators
    DrawRectangle(bg_x, bg_y, bg_w, bg_h, DARKGRAY);
    DrawRectangle(bg_x, bg_y, (int)(bg_w * hpPercent), bg_h, RED);
    DrawRectangleLines(bg_x, bg_y, bg_w, bg_h, BLACK);
    DrawText(hpText, hp_x, hp_y, hp_h, WHITE);
}

void GoldParticlesStarted(ParticleEffect *effect, void *userData)
{
    Sprite *coinSprite = (Sprite *)userData;
    for (size_t i = 0; i < effect->particleCount; i++) {
        effect->particles[i].body.sprite = coinSprite;
    }
}

// TODO: Remove global variable, look up sound by event type.. e.g. PlaySound(sounds[AudioEvent_GoldDropped])
static Sound snd_gold;
void GoldParticlesDying(ParticleEffect *effect, void *userData)
{
    assert(effect);
    assert(effect->type == ParticleEffectType_Gold);
    assert(snd_gold.sampleCount);

    SetSoundPitch(snd_gold, 1.0f + random_normalized_signed(6) * 0.1f);
    PlaySound(snd_gold);
}

void BloodParticlesFollowPlayer(ParticleEffect *effect, void *userData)
{
    assert(effect);
    assert(effect->type == ParticleEffectType_Blood);
    assert(userData);

    Player *charlie = (Player *)userData;
    effect->origin = player_get_attach_point(charlie, PlayerAttachPoint_Gut);
}

int main(void)
{
    //--------------------------------------------------------------------------------------
    // Initialization
    //--------------------------------------------------------------------------------------
    logFile = fopen("log.txt", "w");

    InitWindow(800, 600, "Attack the slimes!");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // NOTE: There could be other, bigger monitors
    const int monitorWidth = GetMonitorWidth(0);
    const int monitorHeight = GetMonitorHeight(0);

    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        printf("ERROR: Failed to initialized audio device\n");
    }
    // NOTE: Minimum of 0.001 seems reasonable (0.0001 is still audible on max volume)
    //SetMasterVolume(0.01f);

    SetTraceLogCallback(traceLogCallback);

    Music mus_background;
    mus_background = LoadMusicStream("resources/fluquor_copyright.ogg");
    mus_background.looping = true;
    PlayMusicStream(mus_background);
    SetMusicVolume(mus_background, 0.02f);
    UpdateMusicStream(mus_background);

    Music mus_whistle = LoadMusicStream("resources/whistle.ogg");
    mus_whistle.looping = true;

    Sound snd_whoosh        = LoadSound("resources/whoosh1.ogg");
    Sound snd_squish1       = LoadSound("resources/squish1.ogg");
    Sound snd_squish2       = LoadSound("resources/squish2.ogg");
    Sound snd_slime_stab1   = LoadSound("resources/slime_stab1.ogg");
    Sound snd_squeak        = LoadSound("resources/squeak1.ogg");
    Sound snd_footstep      = LoadSound("resources/footstep1.ogg");
    snd_gold                = LoadSound("resources/gold1.ogg");
    assert(snd_whoosh       .sampleCount);
    assert(snd_squish1      .sampleCount);
    assert(snd_squish2      .sampleCount);
    assert(snd_slime_stab1  .sampleCount);
    assert(snd_squeak       .sampleCount);
    assert(snd_footstep     .sampleCount);
    assert(snd_gold         .sampleCount);

    Image checkerboardImage = GenImageChecked(monitorWidth, monitorHeight, 32, 32, LIGHTGRAY, GRAY);
    Texture checkboardTexture = LoadTextureFromImage(checkerboardImage);
    UnloadImage(checkerboardImage);

    Texture tilesetTex = LoadTexture("resources/tiles32.png");
    assert(tilesetTex.width);

    Tileset tileset = { 0 };
    tileset_init_ex(&tileset, &tilesetTex, 32, 32, Tile_Count);

    Tilemap tilemap = { 0 };
    tilemap_generate_ex(&tilemap, 128, 128, &tileset);
    //tilemap_generate_ex(&tilemap, 256, 256, &tileset);
    //tilemap_generate_ex(&tilemap, 512, 512, &tileset);

    Color tileColors[Tile_Count] = {
        GREEN,
        SKYBLUE,
        DARKGREEN,
        BROWN,
        GRAY
    };

    Camera2D camera = { 0 };
    //camera.target = (Vector2){ (float)tilemap.widthTiles / 2.0f * tilemap.tileset->tileWidth, (float)tilemap.heightTiles / 2.0f * tilemap.tileset->tileHeight };
    camera.offset = (Vector2){ screenWidth / 2.0f, screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    int cameraReset = 0;
    int cameraFollowPlayer = 1;

    Spritesheet *charlieSpritesheet = LoadSpritesheet("resources/charlie.txt");
    assert(charlieSpritesheet->sprites);
    // TODO: These shouldn't hard-coded.. find_by_name maybe?
    Sprite *charlieSprite = &charlieSpritesheet->sprites[0];

    Spritesheet *slimeSpritesheet = LoadSpritesheet("resources/slime.txt");
    assert(slimeSpritesheet->sprites);
    Sprite *slimeSprite = &slimeSpritesheet->sprites[0];

    Spritesheet *coinSpritesheet = LoadSpritesheet("resources/coin_gold.txt");
    assert(coinSpritesheet->sprites);
    Sprite *coinSprite = &coinSpritesheet->sprites[0];

    Weapon weaponFist = { 0 };
    weaponFist.damage = 1.0f;

    Weapon weaponSword = { 0 };
    weaponSword.damage = 2.0f;

    Player charlie = { 0 };
    player_init(&charlie, "Charlie", charlieSprite);
    charlie.body.position = (Vector3){
        (float)tilemap.widthTiles / 2.0f * tilemap.tileset->tileWidth,
        (float)tilemap.heightTiles / 2.0f * tilemap.tileset->tileHeight,
        0.0f
    };
    charlie.combat.weapon = &weaponSword;


#define SLIMES_COUNT 100
    Slime slimes[SLIMES_COUNT] = { 0 };
    int slimesByDepth[SLIMES_COUNT] = { 0 };

    {
        // TODO: Slime radius should probably be determined base on largest frame, no an arbitrary frame. Or, it could
        // be specified in the config file.
        int southAnimIdx = slimeSprite->animations[Facing_South];
        assert(southAnimIdx >= 0);
        assert(southAnimIdx < slimeSprite->spritesheet->animationCount);

        int firstFrameIdx = slimeSprite->spritesheet->animations[southAnimIdx].frames[0];
        assert(firstFrameIdx >= 0);
        assert(firstFrameIdx < slimeSprite->spritesheet->frameCount);

        SpriteFrame *firstFrame = &slimeSpritesheet->frames[firstFrameIdx];
        const int slimeRadiusX = firstFrame->width / 2;
        const int slimeRadiusY = firstFrame->height / 2;
        const size_t mapPixelsX = tilemap.widthTiles * tilemap.tileset->tileWidth;
        const size_t mapPixelsY = tilemap.heightTiles * tilemap.tileset->tileHeight;
        const int maxX = (int)mapPixelsX - slimeRadiusX;
        const int maxY = (int)mapPixelsY - slimeRadiusY;
        for (int i = 0; i < SLIMES_COUNT; i++) {
            slime_init(&slimes[i], 0, slimeSprite);
            slimes[i].body.position.x = (float)GetRandomValue(slimeRadiusX, maxX);
            slimes[i].body.position.y = (float)GetRandomValue(slimeRadiusY, maxY);
            slimesByDepth[i] = i;
        }
    }

    int slimesKilled = 0;
    int coinsCollected = 0;

    double frameStart = GetTime();
    double dt = 0;
    const int targetFPS = 60;
    SetTargetFPS(targetFPS);
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())
    {
        // NOTE: Limit delta time to 2 frames worth of updates to prevent chaos for large dt (e.g. when debugging)
        const double dtMax = (1.0 / targetFPS) * 2;
        const double now = GetTime();
        dt = MIN(now - frameStart, dtMax);
        frameStart = now;

        if (IsWindowResized()) {
            screenWidth = GetScreenWidth();
            screenHeight = GetScreenHeight();
            cameraReset = 1;
        }

        UpdateMusicStream(mus_background);
        UpdateMusicStream(mus_whistle);

        // TODO: This should be on its own thread probably
        if (IsKeyPressed(KEY_F11)) {
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            char screenshotName[64] = { 0 };
            int len = snprintf(screenshotName, sizeof(screenshotName),
                "screenshots/%d-%02d-%02d_%02d-%02d-%02d_screenshot.png",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            assert(len < sizeof(screenshotName));
            TakeScreenshot(screenshotName);
        }

        if (IsKeyPressed(KEY_F)) {
            cameraFollowPlayer = !cameraFollowPlayer;
        }

        // Camera reset (zoom and rotation)
        if (cameraReset || IsKeyPressed(KEY_R)) {
            camera.target = (Vector2){ roundf(camera.target.x), roundf(camera.target.y) };
            camera.offset = (Vector2){ roundf(screenWidth / 2.0f), roundf(screenHeight / 2.0f) };
            camera.rotation = 0.0f;
            camera.zoom = 1.0f;
            cameraReset = 0;
        }

        if (cameraFollowPlayer) {
            float playerSpeed = 4.0f;
            Vector2 moveBuffer = { 0 };
            if (IsKeyDown(KEY_LEFT_SHIFT)) {
                playerSpeed += 2.0f;
            }
            if (IsKeyDown(KEY_A)) moveBuffer.x -= 1.0f;
            if (IsKeyDown(KEY_D)) moveBuffer.x += 1.0f;
            if (IsKeyDown(KEY_W)) moveBuffer.y -= 1.0f;
            if (IsKeyDown(KEY_S)) moveBuffer.y += 1.0f;

            Vector2 moveOffset = v2_round(v2_scale(v2_normalize(moveBuffer), playerSpeed));
            if (!v2_is_zero(moveOffset)) {
                Vector2 curPos = body_ground_position(&charlie.body);
                Tile *curTile = tilemap_at_world_try(&tilemap, (int)curPos.x, (int)curPos.y);
                int curWalkable = tile_is_walkable(curTile);

                Vector2 newPos = v2_add(curPos, moveOffset);
                Tile *newTile = tilemap_at_world_try(&tilemap, (int)newPos.x, (int)newPos.y);

                // NOTE: This extra logic allows the player to slide when attempting to move diagonally against a wall
                // NOTE: If current tile isn't walkable, allow player to walk off it. This may not be the best solution
                // if the player can accidentally end up on unwalkable tiles through gameplay, but currently the only
                // way to end up on an unwalkable tile is to spawn there.
                // TODO: We should fix spawning to ensure player spawns on walkable tile (can probably just manually
                // generate something interesting in the center of the world that overwrites procgen, like Don't
                // Starve's fancy arrival portal.
                if (curWalkable) {
                    if (!tile_is_walkable(newTile)) {
                        // XY unwalkable, try only X offset
                        newPos = curPos;
                        newPos.x += moveOffset.x;
                        newTile = tilemap_at_world_try(&tilemap, (int)newPos.x, (int)newPos.y);
                        if (tile_is_walkable(newTile)) {
                            // X offset is walkable
                            moveOffset.y = 0.0f;
                        } else {
                            // X unwalkable, try only Y offset
                            newPos = curPos;
                            newPos.y += moveOffset.y;
                            newTile = tilemap_at_world_try(&tilemap, (int)newPos.x, (int)newPos.y);
                            if (tile_is_walkable(newTile)) {
                                // Y offset is walkable
                                moveOffset.x = 0.0f;
                            } else {
                                // XY, and both slide directions are all unwalkable
                                moveOffset.x = 0.0f;
                                moveOffset.y = 0.0f;

                                // TODO: Play wall bonk sound (or splash for water? heh)
                                // TODO: Maybe bounce the player against the wall? This code doesn't do that nicely..
                                //player_move(&charlie, v2_scale(v2_negate(moveOffset), 10.0f));
                            }
                        }
                    }
                }

                if (player_move(&charlie, now, dt, moveOffset)) {
                    static double lastFootstep = 0;
                    double timeSinceLastFootstep = now - lastFootstep;
                    if (timeSinceLastFootstep > 1.0 / (double)playerSpeed) {
                        SetSoundPitch(snd_footstep, 1.0f + random_normalized_signed(6) * 0.5f);
                        PlaySound(snd_footstep);
                        lastFootstep = now;
                    }
                }
            }

            camera.target = body_ground_position(&charlie.body);
        } else {
            const int cameraSpeed = 5;
            if (IsKeyDown(KEY_A)) camera.target.x -= cameraSpeed / camera.zoom;
            if (IsKeyDown(KEY_D)) camera.target.x += cameraSpeed / camera.zoom;
            if (IsKeyDown(KEY_W)) camera.target.y -= cameraSpeed / camera.zoom;
            if (IsKeyDown(KEY_S)) camera.target.y += cameraSpeed / camera.zoom;
        }

        // Camera rotation controls
        //if (IsKeyPressed(KEY_Q)) {
        //    camera.rotation -= 45.0f;
        //    if (camera.rotation < 0.0f) camera.rotation += 360.0f;
        //}
        //else if (IsKeyPressed(KEY_E)) {
        //    camera.rotation += 45.0f;
        //    if (camera.rotation >= 360.0f) camera.rotation -= 360.0f;
        //}

        // Camera zoom controls
#if 0
        camera.zoom += GetMouseWheelMove() * 0.1f * camera.zoom;
#else
        const float mouseWheelMove = GetMouseWheelMove();
        if (mouseWheelMove) {
            //printf("zoom: %f, log: %f\n", camera.zoom, log10f(camera.zoom));
            camera.zoom *= mouseWheelMove > 0.0f ? 2.0f : 0.5f;
        }
#endif

#if 1
        const int negZoomMultiplier = 7; // 7x negative zoom (out)
        const int posZoomMultiplier = 1; // 2x positive zoom (in)
        const float minZoom = 1.0f / (float)(1 << (negZoomMultiplier - 1));
        const float maxZoom = 1.0f * (float)(1 << (posZoomMultiplier - 1));
        camera.zoom = CLAMP(camera.zoom, minZoom, maxZoom);
        const float invZoom = 1.0f / camera.zoom;
#else
        const int negZoomMultiplier = 7; // 7x negative zoom (out)
        const int posZoomMultiplier = 2; // 2x positive zoom (in)
        const float minZoomPow2 = (float)(1 << (negZoomMultiplier - 1));
        const float maxZoomPow2 = (float)(1 << (posZoomMultiplier - 1));
        const float minZoom = 1.0f / minZoomPow2;
        const float maxZoom = 1.0f * maxZoomPow2;
        const float roundedZoom = roundf(camera.zoom * minZoomPow2) / minZoomPow2;
        camera.zoom = CLAMP(roundedZoom, minZoom, maxZoom);
        const float invZoom = 1.0f / camera.zoom;
#endif
        if (IsKeyPressed(KEY_ONE)) {
            charlie.combat.weapon = &weaponFist;
        } else if (IsKeyPressed(KEY_TWO)) {
            charlie.combat.weapon = &weaponSword;
        }

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            const float playerAttackReach = METERS(1.0f);

            if (player_attack(&charlie, now, dt)) {
                size_t slimesHit = 0;
                for (size_t slimeIdx = 0; slimeIdx < SLIMES_COUNT; slimeIdx++) {
                    if (!slimes[slimeIdx].combat.hitPoints)
                        continue;

                    Vector3 playerToSlime = v3_sub(slimes[slimeIdx].body.position, charlie.body.position);
                    if (v3_length_sq(playerToSlime) <= playerAttackReach * playerAttackReach) {
                        slimes[slimeIdx].combat.hitPoints = MAX(0.0f, slimes[slimeIdx].combat.hitPoints - charlie.combat.weapon->damage);
                        if (!slimes[slimeIdx].combat.hitPoints) {
                            SetSoundPitch(snd_squeak, 1.0f + random_normalized_signed(6) * 0.05f);
                            PlaySound(snd_squeak);

                            int coins = GetRandomValue(1, 4) * (int)slimes[slimeIdx].body.scale;
                            coinsCollected += coins;

                            ParticleEffect *gooParticles = particle_effect_alloc(ParticleEffectType_Goo, 20);
                            //gooParticles->callbacks[ParticleEffectEvent_Started].function = GooParticlesStarted;
                            //gooParticles->callbacks[ParticleEffectEvent_Started].userData = gooSprite;
                            //gooParticles->callbacks[ParticleEffectEvent_Dying].function = GooParticlesDying;
                            Vector3 deadCenter = body_center(&slimes[slimeIdx].body);
                            particle_effect_start(gooParticles, now, 2.0, deadCenter);

                            ParticleEffect *goldParticles = particle_effect_alloc(ParticleEffectType_Gold, (size_t)coins);
                            goldParticles->callbacks[ParticleEffectEvent_Started].function = GoldParticlesStarted;
                            goldParticles->callbacks[ParticleEffectEvent_Started].userData = coinSprite;
                            goldParticles->callbacks[ParticleEffectEvent_Dying].function = GoldParticlesDying;
                            Vector3 slimeBC = body_center(&slimes[slimeIdx].body);
                            particle_effect_start(goldParticles, now, 2.0, slimeBC);

                            slimesKilled++;
                        } else {
                            Sound *squish = GetRandomValue(0, 1) ? &snd_squish1 : &snd_squish2;
                            SetSoundPitch(*squish, 1.0f + random_normalized_signed(6) * 0.2f);
                            PlaySound(*squish);
                        }
                        slimesHit++;
                    }
                }

                if (slimesHit) {
                    //PlaySound(snd_slime_stab1);
                    //PlaySound(GetRandomValue(0, 1) ? snd_squish1 : snd_squish2);
                } else {
                    SetSoundPitch(snd_whoosh, 1.0f + random_normalized_signed(12) * 0.1f);
                    PlaySound(snd_whoosh);
                }
            }
        }

        player_update(&charlie, now, dt);
        if (charlie.body.facing == Facing_Idle && !IsMusicPlaying(mus_whistle)) {
            PlayMusicStream(mus_whistle);
        } else if (charlie.body.facing != Facing_Idle && IsMusicPlaying(mus_whistle)) {
            StopMusicStream(mus_whistle);
        }

        {
            // TODO: Move these to Body3D
            const float slimeMoveSpeed = METERS(2.0f);
            const float slimeAttackReach = METERS(0.5f);
            const float slimeAttackTrack = METERS(32.0f);
            const float slimeAttackDamage = 1.0f;
            const float slimeRadius = METERS(0.5f);

            static double lastSquish = 0;
            double timeSinceLastSquish = now - lastSquish;

            for (size_t slimeIdx = 0; slimeIdx < SLIMES_COUNT; slimeIdx++) {
                if (!slimes[slimeIdx].combat.hitPoints)
                    continue;

                Vector2 slimeToPlayer = v2_sub(body_ground_position(&charlie.body), body_ground_position(&slimes[slimeIdx].body));
                const float slimeToPlayerDistSq = v2_length_sq(slimeToPlayer);
                if (slimeToPlayerDistSq > SQUARED(slimeAttackReach) &&
                    slimeToPlayerDistSq <= SQUARED(slimeAttackTrack))
                {
                    const float slimeToPlayerDist = sqrtf(slimeToPlayerDistSq);
                    const float moveDist = MIN(slimeToPlayerDist, slimeMoveSpeed * slimes[slimeIdx].body.scale);
                    // 25% -1.0, 75% +1.0f
                    const float moveRandMult = (random_normalized_signed(100) + 0.5f) > 0.0f ? 1.0f : -1.0f;
                    const Vector2 slimeMoveDir = v2_scale(slimeToPlayer, 1.0f / slimeToPlayerDist);
                    const Vector2 slimeMove = v2_scale(slimeMoveDir, moveDist * moveRandMult);
                    const Vector2 slimePos = body_ground_position(&slimes[slimeIdx].body);
                    const Vector2 slimePosNew = v2_add(slimePos, slimeMove);

                    int willCollide = 0;
                    for (size_t collideIdx = 0; collideIdx < SLIMES_COUNT; collideIdx++) {
                        if (collideIdx == slimeIdx)
                            continue;

                        if (!slimes[collideIdx].combat.hitPoints)
                            continue;

                        Vector2 otherSlimePos = body_ground_position(&slimes[collideIdx].body);
                        const float zDist = fabsf(slimes[slimeIdx].body.position.z - slimes[collideIdx].body.position.z);
                        const float radiusScaled = slimeRadius * slimes[slimeIdx].body.scale;
                        if (v2_length_sq(v2_sub(slimePos, otherSlimePos)) < SQUARED(radiusScaled) && zDist < radiusScaled) {
                            Slime *a = &slimes[slimeIdx];
                            Slime *b = &slimes[collideIdx];
                            if (slime_combine(a, b)) {
                                Slime *dead = a->combat.hitPoints == 0.0f ? a : b;
                                ParticleEffect *gooParticles = particle_effect_alloc(ParticleEffectType_Goo, 20);
                                //gooParticles->callbacks[ParticleEffectEvent_Started].function = GooParticlesStarted;
                                //gooParticles->callbacks[ParticleEffectEvent_Started].userData = gooSprite;
                                //gooParticles->callbacks[ParticleEffectEvent_Dying].function = GooParticlesDying;
                                Vector3 deadCenter = body_center(&dead->body);
                                particle_effect_start(gooParticles, now, 2.0, deadCenter);
                            }
                        }
                        if (v2_length_sq(v2_sub(slimePosNew, otherSlimePos)) < SQUARED(radiusScaled) && zDist < radiusScaled) {
                            willCollide = 1;
                        }
                    }

                    if (!willCollide) {
                        slime_move(&slimes[slimeIdx], now, dt, slimeMove);
                        if (!IsSoundPlaying(snd_squish1) && !IsSoundPlaying(snd_squish2) && timeSinceLastSquish > 1.0) {
                            //PlaySound(GetRandomValue(0, 1) ? snd_squish1 : snd_squish2);
                            lastSquish = GetTime();
                        }
                    }
                }

                // Allow slime to attack if on the ground and close enough to the player
                slimeToPlayer = v2_sub(body_ground_position(&charlie.body), body_ground_position(&slimes[slimeIdx].body));
                if (v2_length_sq(slimeToPlayer) <= SQUARED(slimeAttackReach)) {
                    if (slime_attack(&slimes[slimeIdx], now, dt)) {
                        charlie.combat.hitPoints = MAX(0.0f, charlie.combat.hitPoints - (slimeAttackDamage * slimes[slimeIdx].body.scale));

                        static double lastBleed = 0;
                        const double bleedDuration = 1.2;

                        if (now - lastBleed > bleedDuration) {
                            ParticleEffect *bloodParticles = particle_effect_alloc(ParticleEffectType_Blood, 40);
                            bloodParticles->callbacks[ParticleEffectEvent_BeforeUpdate].function = BloodParticlesFollowPlayer;
                            bloodParticles->callbacks[ParticleEffectEvent_BeforeUpdate].userData = &charlie;

                            Vector3 playerGut = player_get_attach_point(&charlie, PlayerAttachPoint_Gut);
                            particle_effect_start(bloodParticles, now, bleedDuration, playerGut);
                            lastBleed = now;
                        }

                        // TODO: Kill peter if he dies
                    }
                }

                slime_update(&slimes[slimeIdx], now, dt);
            }
        }

        {
            particle_effects_update(now, dt);
        }


        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(ORANGE);
        DrawTexture(checkboardTexture, 0, 0, WHITE);

        Rectangle cameraRect = { 0 };
        cameraRect.x = camera.target.x - camera.offset.x * invZoom;
        cameraRect.y = camera.target.y - camera.offset.y * invZoom;
        cameraRect.width  = camera.offset.x * 2.0f * invZoom;
        cameraRect.height = camera.offset.y * 2.0f * invZoom;

#if DEMO_VIEW_CULLING
        const float cameraRectPad = 100.0f * invZoom;
        cameraRect.x += cameraRectPad;
        cameraRect.y += cameraRectPad;
        cameraRect.width  -= cameraRectPad * 2.0f;
        cameraRect.height -= cameraRectPad * 2.0f;
#endif

        // TODO: Calculate this based on how many tiles will appear on the screen, rather than camera zoom
        // Alternatively, we could group nearby tiles of the same type together into large quads?
        const int zoomMipLevel = MAX(1, (int)invZoom / 8);

        const Vector2 mousePosScreen = GetMousePosition();
        Vector2 mousePosWorld = { 0 };
        mousePosWorld.x += mousePosScreen.x * invZoom + cameraRect.x;
        mousePosWorld.y += mousePosScreen.y * invZoom + cameraRect.y;

        const int findMouseTile = IsKeyDown(KEY_LEFT_CONTROL);
        Tile *mouseTile = 0;
        Rectangle mouseTileRect;

        BeginMode2D(camera);
        size_t tilesDrawn = 0;
        for (size_t y = 0; y < tilemap.heightTiles; y += zoomMipLevel) {
            for (size_t x = 0; x < tilemap.widthTiles; x += zoomMipLevel) {
                Tile *tile = &tilemap.tiles[y * tilemap.widthTiles + x];
                if (tile->position.x + tilemap.tileset->tileWidth  * zoomMipLevel >= cameraRect.x &&
                    tile->position.y + tilemap.tileset->tileHeight * zoomMipLevel >= cameraRect.y &&
                    tile->position.x < cameraRect.x + cameraRect.width &&
                    tile->position.y < cameraRect.y + cameraRect.height)
                {
                    Rectangle bounds = (Rectangle){
                        tile->position.x,
                        tile->position.y,
                        (float)tilemap.tileset->tileWidth  * zoomMipLevel,
                        (float)tilemap.tileset->tileHeight * zoomMipLevel
                    };
#if 0
                    // Draw solid color rectangles
                    DrawRectangle((int)tile->position.x, (int)tile->position.y, (int)tilemap.tileset->tileWidth * zoomMipLevel,
                        (int)tilemap.tileset->tileHeight * zoomMipLevel, tileColors[tile->tileType]);
#elif 0
                    // Draw all tiles as textured rects (looks best, performs worst)
                    Rectangle textureRect = tilemap.tileset->textureRects[tile->tileType];
                    DrawTextureRec(tilemap.tileset->texture, textureRect, tile->position, WHITE);
#else
                    // Draw repeating texture of top-left mip tile when zoomed out
                    Rectangle source = tilemap.tileset->textureRects[tile->tileType];
                    DrawTexturePro(*tilemap.tileset->texture, source, bounds, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
#endif
                    tilesDrawn++;

                    // TODO: This can be done deterministically outside of the loop, no need to run it per-tile
                    if (findMouseTile &&
                        mousePosWorld.x >= bounds.x &&
                        mousePosWorld.y >= bounds.y &&
                        mousePosWorld.x < bounds.x + bounds.width &&
                        mousePosWorld.y < bounds.y + bounds.height)
                    {
                        mouseTile = tile;
                        mouseTileRect = bounds;
                    }
                }
            }
        }
        //DrawRectangleRec(cameraRect, Fade(PINK, 0.5f));

        if (findMouseTile && mouseTile) {
            // Draw red outline on hovered tile
#if 0
            DrawRectangleLines
                (int)mouseTile->position.x, (int)mouseTile->position.y,
                (int)tilemap.tileset->tileWidth, (int)tilemap.tileset->tileHeight, RED);
#else
            DrawRectangleLinesEx(mouseTileRect, 1 * (int)invZoom, RED);
#endif
        }

        {
            // Sort by y value of bottom of sprite rect
            // TODO(perf): Sort indices instead to prevent copying Slimes around in array?
            //qsort(slimes, sizeof(slimes)/sizeof(*slimes), sizeof(*slimes), SlimeCompareDepth);
            for (int i = 1; i < SLIMES_COUNT; i++) {
                int index = slimesByDepth[i];
                Vector2 groundPosA = body_ground_position(&slimes[index].body);
                int j = i - 1;
                Vector2 groundPosB = body_ground_position(&slimes[slimesByDepth[j]].body);
                while (groundPosB.y > groundPosA.y) {
                    slimesByDepth[j + 1] = slimesByDepth[j];
                    j--;
                    if (j < 0) break;
                    groundPosB = body_ground_position(&slimes[slimesByDepth[j]].body);
                }
                slimesByDepth[j + 1] = index;
            }

            const Vector2 playerGroundPos = body_ground_position(&charlie.body);

            // TODO: Shadow size based on height from ground
            // https://yal.cc/top-down-bouncing-loot-effects/

            //const float shadowScale = 1.0f + slimes[slimeIdx].transform.position.z / 20.0f;

            // Draw shadows
            for (int i = 0; i < SLIMES_COUNT; i++) {
                int slimeIdx = slimesByDepth[i];
                if (!slimes[slimeIdx].combat.hitPoints)
                    continue;

                const Vector2 slimeBC = body_ground_position(&slimes[slimeIdx].body);
                DrawShadow((int)slimeBC.x, (int)slimeBC.y, 16.0f * slimes[slimeIdx].body.scale, -8.0f * slimes[slimeIdx].body.scale);
            }

            // Player shadow
            DrawShadow((int)playerGroundPos.x, (int)playerGroundPos.y, 16.0f, -6.0f);

            // Draw entities
            bool charlieDrawn = false;

            for (int i = 0; i < SLIMES_COUNT; i++) {
                int slimeIdx = slimesByDepth[i];
                if (!slimes[slimeIdx].combat.hitPoints) {
                    continue;
                }

                const Vector2 slimeGroundPos = body_ground_position(&slimes[slimeIdx].body);

                // HACK: Draw charlie in sorted order
                if (!charlieDrawn && playerGroundPos.y < slimeGroundPos.y) {
                    player_draw(&charlie);
                    charlieDrawn = true;
                }

                slime_draw(&slimes[slimeIdx]);
            }

            if (!charlieDrawn) {
                player_draw(&charlie);
            }
        }

        for (size_t slimeIdx = 0; slimeIdx < SLIMES_COUNT; slimeIdx++) {
            if (!slimes[slimeIdx].combat.hitPoints) {
                continue;
            }

            // TODO: slime_get_top_center (after unify player/slime code)
            Rectangle slimeRect = body_rect(&slimes[slimeIdx].body);
            const int x = (int)(slimeRect.x + slimeRect.width / 2.0f);
            const int y = (int)slimeRect.y;
            DrawHealthBar(x, y, slimes[slimeIdx].combat.hitPoints, slimes[slimeIdx].combat.maxHitPoints);
        }
        // TODO: player_get_top_center (after unify player/slime code)
        Rectangle playerRect = body_rect(&charlie.body);
        const int x = (int)(playerRect.x + playerRect.width / 2.0f);
        const int y = (int)playerRect.y;
        DrawHealthBar(x, y, charlie.combat.hitPoints, charlie.combat.maxHitPoints);

        {
            particle_effects_draw(dt);
        }

#if DEMO_VIEW_CULLING
        DrawRectangleRec(cameraRect, Fade(PINK, 0.5f));
#endif

#if DEMO_AI_TRACKING
        {
            Vector2 charlieCenter = player_get_bottom_center(&charlie);
            DrawCircle((int)charlieCenter.x, (int)charlieCenter.y, 640.0f, Fade(ORANGE, 0.5f));
        }
#endif

        EndMode2D();

        if (findMouseTile && mouseTile) {
            const int tooltipOffsetX = 10;
            const int tooltipOffsetY = 10;
            const int tooltipPad = 4;

            int tooltipX = (int)mousePosScreen.x + tooltipOffsetX;
            int tooltipY = (int)mousePosScreen.y + tooltipOffsetY;
            const int tooltipW = 150 + tooltipPad * 2;
            const int tooltipH = 30  + tooltipPad * 2;

            if (tooltipX + tooltipW > screenWidth ) tooltipX = screenWidth  - tooltipW;
            if (tooltipY + tooltipH > screenHeight) tooltipY = screenHeight - tooltipH;

            Rectangle tooltipRect = (Rectangle){ (float)tooltipX, (float)tooltipY, (float)tooltipW, (float)tooltipH };
            DrawRectangleRec(tooltipRect, Fade(RAYWHITE, 0.8f));
            DrawRectangleLinesEx(tooltipRect, 1, Fade(BLACK, 0.8f));

            const int fontHeight = 10;
            int lineOffset = 0;
            DrawText(TextFormat("tilePos : %.02f, %.02f", mouseTile->position.x, mouseTile->position.y),
                tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, fontHeight, BLACK);
            lineOffset += fontHeight;
            DrawText(TextFormat("tileSize: %zu, %zu", tilemap.tileset->tileWidth, tilemap.tileset->tileHeight),
                tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, fontHeight, BLACK);
            lineOffset += fontHeight;
            DrawText(TextFormat("tileType: %d", mouseTile->tileType),
                tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, fontHeight, BLACK);
            lineOffset += fontHeight;
        }

        int yOffset = 10;

        {
            Vector2 pos = { 10.0f, (float)yOffset };
            const int pad = 4;

            const char *text = TextFormat("%2i fps (%.02f ms)", GetFPS(), GetFrameTime() * 1000.0f);
            int textWidth = 100; //MeasureText(text, 10);

            Rectangle fpsRect = (Rectangle){ pos.x - pad, pos.y - pad, (float)textWidth + pad*2, 10.0f + pad*2 };
            DrawRectangleRec(fpsRect, Fade(DARKGRAY, 0.8f));
            DrawRectangleLinesEx(fpsRect, 1, Fade(BLACK, 0.8f));
            DrawText(text, (int)pos.x, yOffset, 10, WHITE);
            yOffset += 10 + pad*2 - 1;
        }

        {
            Vector2 pos = { 10.0f, (float)yOffset };
            const int pad = 4;

            const char *text = TextFormat("Slimes slain: %d", slimesKilled);
            int textWidth = 100; //MeasureText(text, 10);

            Rectangle fpsRect = (Rectangle){ pos.x - pad, pos.y - pad, (float)textWidth + pad*2, 10.0f + pad*2 };
            DrawRectangleRec(fpsRect, Fade(DARKGRAY, 0.8f));
            DrawRectangleLinesEx(fpsRect, 1, Fade(BLACK, 0.8f));
            DrawText(text, (int)pos.x, yOffset, 10, RED);
            yOffset += 10 + pad*2;
        }

        {
            Vector2 pos = { 10.0f, (float)yOffset };
            const int pad = 4;

            const char *text = TextFormat("Coins: %d", coinsCollected);
            int textWidth = 100; //MeasureText(text, 10);

            Rectangle fpsRect = (Rectangle){ pos.x - pad, pos.y - pad, (float)textWidth + pad*2, 10.0f + pad*2 };
            DrawRectangleRec(fpsRect, Fade(DARKGRAY, 0.8f));
            DrawRectangleLinesEx(fpsRect, 1, Fade(BLACK, 0.8f));
            DrawText(text, (int)pos.x, yOffset, 10, YELLOW);
            yOffset += 10 + pad*2;
        }

#if _DEBUG
        {
            const char *cameraZoomText = TextFormat("camera.zoom: %.03f (inv: %.03f)", camera.zoom, invZoom);
            DrawText(cameraZoomText, 10, yOffset, 10, BLACK);
            yOffset += 10;

            const char *mipText = TextFormat("zoopMipLevel: %zu", zoomMipLevel);
            DrawText(mipText, 10, yOffset, 10, BLACK);
            yOffset += 10;

            const char *tilesDrawnText = TextFormat("tilesDrawn: %zu", tilesDrawn);
            DrawText(tilesDrawnText, 10, yOffset, 10, BLACK);
            yOffset += 10;
        }
#endif

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    //--------------------------------------------------------------------------------------
    // Clean up
    //--------------------------------------------------------------------------------------
    particle_effects_free();
    tilemap_free(&tilemap);
    tileset_free(&tileset);
    UnloadTexture(tilesetTex);
    UnloadSpritesheet(coinSpritesheet);
    UnloadSpritesheet(slimeSpritesheet);
    UnloadSpritesheet(charlieSpritesheet);
    UnloadTexture(checkboardTexture);
    UnloadSound(snd_gold);
    UnloadSound(snd_footstep);
    UnloadSound(snd_squeak);
    UnloadSound(snd_slime_stab1);
    UnloadSound(snd_squish2);
    UnloadSound(snd_squish1);
    UnloadSound(snd_whoosh);
    UnloadMusicStream(mus_whistle);
    UnloadMusicStream(mus_background);

    CloseAudioDevice();
    CloseWindow();

    fclose(logFile);
    return 0;
}