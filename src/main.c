#include "chat.h"
#include "controller.h"
#include "draw_command.h"
#include "healthbar.h"
#include "helpers.h"
#include "item_catalog.h"
#include "maths.h"
#include "network_client.h"
#include "network_server.h"
#include "particles.h"
#include "particle_fx.h"
#include "player.h"
#include "slime.h"
#include "sound_catalog.h"
#include "spritesheet.h"
#include "spritesheet_catalog.h"
#include "tileset.h"
#include "tilemap.h"
#include "sim.h"
#include "dlb_rand.h"
#include "raygui.h"
#include "raylib.h"
#include "zed_net.h"
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
    if (logType == LOG_FATAL) {
        assert(!"Catch in debugger");
        exit(-1);
    }
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

void BloodParticlesFollowPlayer(ParticleEffect *effect, void *userData)
{
    assert(effect);
    assert(effect->type == ParticleEffectType_Blood);
    assert(userData);

    Player *charlie = (Player *)userData;
    effect->origin = player_get_attach_point(charlie, PlayerAttachPoint_Gut);
}

#define DLB_RAND_TEST
#include "dlb_rand.h"

int main(void)
{
    //--------------------------------------------------------------------------------------
    // Initialization
    //--------------------------------------------------------------------------------------
    logFile = fopen("log.txt", "w");
    SetTraceLogCallback(traceLogCallback);

    //dlb_rand32_seed(42);
    dlb_rand32_seed(time(NULL));

    //dlb_rand_test();

#if ALPHA_NETWORKING
    if (zed_net_init() < 0) {
        const char *err = zed_net_get_error();
        TraceLog(LOG_FATAL, "Failed to initialize network utilities. Error: %s\n", err);
    }

    NetworkServer server = { 0 };
    if (!network_server_init(&server)) {
        TraceLog(LOG_FATAL, "Failed to initialize network server system.\n");
    }

    const unsigned int SERVER_PORT = 4040;
    if (!network_server_open_socket(&server, SERVER_PORT)) {
        TraceLog(LOG_ERROR, "Failed to open server socket on port %hu.\n", SERVER_PORT);
        // TODO: Handle gracefully if local server to allow singleplayer mode
        exit(-1);
    }

    NetworkClient client = { 0 };
    if (!network_client_init(&client)) {
        TraceLog(LOG_FATAL, "Failed to initialize network client system.\n");
    }

    if (!network_client_open_socket(&client)) {
        TraceLog(LOG_ERROR, "Failed to open client socket.\n");
        // TODO: Handle "offline mode" gracefully. This shouldn't prevent people people from playing single player
        exit(-1);
    }

    if (!network_client_connect(&client, "127.0.0.1", 4040)) {
    //if (!network_client_connect(&client, "slime.theprogrammingjunkie.com", 4040)) {
        // TODO: Handle connection failure.
        TraceLog(LOG_FATAL, "Failed to connect client.\n");
    }
#endif

    if (!chat_init()) {
        TraceLog(LOG_FATAL, "Failed to initialize chat system.\n");
    }

    InitWindow(1600, 900, "Attack the slimes!");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetExitKey(0);  // Disable default Escape exit key, we'll handle escape ourselves

    const int fontHeight = 14;
    Font fonts[3] = {
        GetFontDefault(),
        LoadFontEx("C:/Windows/Fonts/consola.ttf", fontHeight, 0, 0),
        LoadFontEx("resources/UbuntuMono-Regular.ttf", fontHeight, 0, 0),
    };
    for (size_t i = 0; i < ARRAY_SIZE(fonts); i++) {
        assert(fonts[i].texture.id);
    }
    size_t fontIdx = 1;
    GuiSetFont(fonts[fontIdx]);

    healthbars_set_font(fonts[0]);

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

    draw_commands_init();
    sound_catalog_init();
    spritesheet_catalog_init();
    item_catalog_init();
    particles_init();

    Music mus_background;
    mus_background = LoadMusicStream("resources/fluquor_copyright.ogg");
    mus_background.looping = true;
    PlayMusicStream(mus_background);
    SetMusicVolume(mus_background, 0.02f);
    UpdateMusicStream(mus_background);

    Music mus_whistle = LoadMusicStream("resources/whistle.ogg");
    mus_whistle.looping = true;

    Image checkerboardImage = GenImageChecked(monitorWidth, monitorHeight, 32, 32, LIGHTGRAY, GRAY);
    Texture checkboardTexture = LoadTextureFromImage(checkerboardImage);
    UnloadImage(checkerboardImage);

    Texture tilesetTex = LoadTexture("resources/tiles32.png");
    assert(tilesetTex.width);

    Tileset tileset = { 0 };
    tileset_init_ex(&tileset, &tilesetTex, 32, 32, TileType_Count);

    Tilemap tilemap = { 0 };
    //tilemap_generate_ex(&tilemap, 128, 128, &tileset);
    tilemap_generate_ex(&tilemap, 256, 256, &tileset);
    //tilemap_generate_ex(&tilemap, 512, 512, &tileset);

    Texture minimapTex = { 0 };
    {
        Image minimapImg = { 0 };
        minimapImg.width = (int)tilemap.widthTiles;
        minimapImg.height = (int)tilemap.heightTiles;
        minimapImg.mipmaps = 1;
        minimapImg.format = UNCOMPRESSED_R8G8B8A8;
        assert(sizeof(Color) == 4);
        minimapImg.data = calloc((size_t)minimapImg.width * minimapImg.height, sizeof(Color));
        assert(minimapImg.data);

        Color tileColors[TileType_Count] = {
            [TileType_Grass   ] = GREEN,
            [TileType_Water   ] = SKYBLUE,
            [TileType_Forest  ] = DARKGREEN,
            [TileType_Wood    ] = BROWN,
            [TileType_Concrete] = GRAY
        };

        const size_t heightTiles = tilemap.heightTiles;
        const size_t widthTiles = tilemap.widthTiles;

        Color *minimapPixel = minimapImg.data;
        for (size_t y = 0; y < heightTiles; y += 1) {
            for (size_t x = 0; x < widthTiles; x += 1) {
                const Tile *tile = &tilemap.tiles[y * tilemap.widthTiles + x];
                // Draw all tiles as different colored pixels
                assert(tile->tileType >= 0);
                assert(tile->tileType < TileType_Count);
                *minimapPixel = tileColors[tile->tileType];
                minimapPixel++;
            }
        }

        minimapTex = LoadTextureFromImage(minimapImg);
        free(minimapImg.data);
    }

    const Vector3 worldSpawn = (Vector3){
        (float)tilemap.widthTiles / 2.0f * tilemap.tileset->tileWidth,
        (float)tilemap.heightTiles / 2.0f * tilemap.tileset->tileHeight,
        0.0f
    };

    Camera2D camera = { 0 };
    //camera.target = (Vector2){
    //    (float)tilemap.widthTiles / 2.0f * tilemap.tileset->tileWidth,
    //    (float)tilemap.heightTiles / 2.0f * tilemap.tileset->tileHeight
    //};
    camera.offset = (Vector2){ screenWidth / 2.0f, screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    int cameraReset = 0;
    int cameraFollowPlayer = 1;

    // TODO: Move sprite loading to somewhere more sane
    const Spritesheet *charlieSpritesheet = spritesheet_catalog_find(SpritesheetID_Charlie);
    const SpriteDef *charlieSpriteDef = spritesheet_find_sprite(charlieSpritesheet, "player_sword");

    const Spritesheet *slimeSpritesheet = spritesheet_catalog_find(SpritesheetID_Slime);
    const SpriteDef *slimeSpriteDef = spritesheet_find_sprite(slimeSpritesheet, "slime");

    const Spritesheet *coinSpritesheet = spritesheet_catalog_find(SpritesheetID_Coin);
    const SpriteDef *coinSpriteDef = spritesheet_find_sprite(coinSpritesheet, "coin");

    Player charlie = { 0 };
    player_init(&charlie, "Charlie", charlieSpriteDef);
    charlie.body.position = worldSpawn;

    Slime slimes[MAX_SLIMES] = { 0 };

    {
        // TODO: Slime radius should probably be determined base on largest frame, no an arbitrary frame. Or, it could
        // be specified in the config file.
        assert(slimeSpriteDef);
        int firstAnimIndex = slimeSpriteDef->animations[0];
        assert(firstAnimIndex >= 0);
        assert(firstAnimIndex < slimeSpriteDef->spritesheet->animationCount);

        int firstFrameIdx = slimeSpriteDef->spritesheet->animations[firstAnimIndex].frames[0];
        assert(firstFrameIdx >= 0);
        assert(firstFrameIdx < slimeSpriteDef->spritesheet->frameCount);

        SpriteFrame *firstFrame = &slimeSpritesheet->frames[firstFrameIdx];
        const float slimeRadiusX = firstFrame->width / 2.0f;
        const float slimeRadiusY = firstFrame->height / 2.0f;
        const size_t mapPixelsX = tilemap.widthTiles * tilemap.tileset->tileWidth;
        const size_t mapPixelsY = tilemap.heightTiles * tilemap.tileset->tileHeight;
        const float maxX = mapPixelsX - slimeRadiusX;
        const float maxY = mapPixelsY - slimeRadiusY;
        for (int i = 0; i < MAX_SLIMES; i++) {
            slime_init(&slimes[i], 0, slimeSpriteDef);
            slimes[i].body.position.x = dlb_rand32f_range(slimeRadiusX, maxX);
            slimes[i].body.position.y = dlb_rand32f_range(slimeRadiusY, maxY);
        }
    }

    double frameStart = GetTime();
    double dt = 0;
    const int targetFPS = 60;
    SetTargetFPS(targetFPS);
    bool gifRecording = false;
    bool chatActive = false;
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())
    {
        bool escape = IsKeyPressed(KEY_ESCAPE);
        bool findMouseTile = false;

        // NOTE: Limit delta time to 2 frames worth of updates to prevent chaos for large dt (e.g. when debugging)
        const double dtMax = (1.0 / targetFPS) * 2;
        const double now = GetTime();
        dt = MIN(now - frameStart, dtMax);
        frameStart = now;

        // HACK: No way to check if Raylib is currently recording.. :(
        if (gifRecording) {
            dt = 1.0 / 10.0;
        }

#if ALPHA_NETWORKING
        network_server_receive(&server);
        network_client_receive(&client);
#endif

        if (IsWindowResized()) {
            screenWidth = GetScreenWidth();
            screenHeight = GetScreenHeight();
            cameraReset = 1;
        }

        UpdateMusicStream(mus_background);
        UpdateMusicStream(mus_whistle);

        if (!chatActive) {
            findMouseTile = IsKeyDown(KEY_LEFT_ALT);

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

            if (IsKeyPressed(KEY_SEVEN)) {
                fontIdx++;
                if (fontIdx >= ARRAY_SIZE(fonts)) {
                    fontIdx = 0;
                    GuiSetFont(fonts[fontIdx]);
                }
            }

#if ALPHA_NETWORKING
            if (IsKeyPressed(KEY_C)) {
                network_client_send(&client, CSTR("User pressed the C key."));
            }
#endif

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
                PlayerControllerState input = QueryPlayerController();
                sim(now, dt, input, &charlie, &tilemap, slimes, coinSpriteDef);
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
        }

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

        player_update(&charlie, now, dt);
        if (charlie.body.idle && !IsMusicPlaying(mus_whistle)) {
            PlayMusicStream(mus_whistle);
        } else if (!charlie.body.idle && IsMusicPlaying(mus_whistle)) {
            StopMusicStream(mus_whistle);
        }

        {
            // TODO: Move these to somewhere
            const float slimeMoveSpeed = METERS_TO_PIXELS(2.0f);
            const float slimeAttackReach = METERS_TO_PIXELS(0.5f);
            const float slimeAttackTrack = METERS_TO_PIXELS(10.0f);
            const float slimeAttackDamage = 1.0f;
            const float slimeRadius = METERS_TO_PIXELS(0.5f);

            //static double lastSquish = 0;
            //double timeSinceLastSquish = now - lastSquish;

            for (size_t slimeIdx = 0; slimeIdx < MAX_SLIMES; slimeIdx++) {
                Slime *slime = &slimes[slimeIdx];
                if (!slime->combat.hitPoints)
                    continue;

                Vector2 slimeToPlayer = v2_sub(body_ground_position(&charlie.body), body_ground_position(&slime->body));
                const float slimeToPlayerDistSq = v2_length_sq(slimeToPlayer);
                if (slimeToPlayerDistSq > SQUARED(slimeAttackReach) &&
                    slimeToPlayerDistSq <= SQUARED(slimeAttackTrack))
                {
                    const float slimeToPlayerDist = sqrtf(slimeToPlayerDistSq);
                    const float moveDist = MIN(slimeToPlayerDist, slimeMoveSpeed * slime->sprite.scale);
                    // 25% -1.0, 75% +1.0f
                    const float moveRandMult = dlb_rand32i_range(1, 4) > 1 ? 1.0f : -1.0f;
                    const Vector2 slimeMoveDir = v2_scale(slimeToPlayer, 1.0f / slimeToPlayerDist);
                    const Vector2 slimeMove = v2_scale(slimeMoveDir, moveDist * moveRandMult);
                    const Vector2 slimePos = body_ground_position(&slime->body);
                    const Vector2 slimePosNew = v2_add(slimePos, slimeMove);

                    int willCollide = 0;
                    for (size_t collideIdx = 0; collideIdx < MAX_SLIMES; collideIdx++) {
                        if (collideIdx == slimeIdx)
                            continue;

                        if (!slimes[collideIdx].combat.hitPoints)
                            continue;

                        Vector2 otherSlimePos = body_ground_position(&slimes[collideIdx].body);
                        const float zDist = fabsf(slime->body.position.z - slimes[collideIdx].body.position.z);
                        const float radiusScaled = slimeRadius * slime->sprite.scale;
                        if (v2_length_sq(v2_sub(slimePos, otherSlimePos)) < SQUARED(radiusScaled) && zDist < radiusScaled) {
                            Slime *a = &slimes[slimeIdx];
                            Slime *b = &slimes[collideIdx];
                            if (slime_combine(a, b)) {
                                const Slime *dead = a->combat.hitPoints == 0.0f ? a : b;
                                const Vector3 slimeBC = sprite_world_center(
                                    &dead->sprite, dead->body.position, dead->sprite.scale
                                );
                                particle_effect_create(ParticleEffectType_Goo, 20, slimeBC, 2.0, now, 0);
                            }
                        }
                        if (v2_length_sq(v2_sub(slimePosNew, otherSlimePos)) < SQUARED(radiusScaled) && zDist < radiusScaled) {
                            willCollide = 1;
                        }
                    }

                    if (!willCollide && slime_move(&slimes[slimeIdx], now, dt, slimeMove)) {
                        SoundID squish = dlb_rand32i_range(0, 1) ? SoundID_Squish1 : SoundID_Squish2;
                        sound_catalog_play(squish, 1.0f + dlb_rand32f_variance(0.2f));
                    }
                }

                // Allow slime to attack if on the ground and close enough to the player
                slimeToPlayer = v2_sub(body_ground_position(&charlie.body), body_ground_position(&slime->body));
                if (v2_length_sq(slimeToPlayer) <= SQUARED(slimeAttackReach)) {
                    if (slime_attack(&slimes[slimeIdx], now, dt)) {
                        charlie.combat.hitPoints = MAX(
                            0.0f,
                            charlie.combat.hitPoints - (slimeAttackDamage * slime->sprite.scale)
                        );

                        static double lastBleed = 0;
                        const double bleedDuration = 3.0;

                        if (now - lastBleed > bleedDuration / 3.0) {
                            Vector3 playerGut = player_get_attach_point(&charlie, PlayerAttachPoint_Gut);
                            ParticleEffect *bloodParticles = particle_effect_create(
                                ParticleEffectType_Blood, 32, playerGut, bleedDuration, now, 0
                            );
                            if (bloodParticles) {
                                bloodParticles->callbacks[ParticleEffectEvent_BeforeUpdate].function = BloodParticlesFollowPlayer;
                                bloodParticles->callbacks[ParticleEffectEvent_BeforeUpdate].userData = &charlie;
                            }
                            lastBleed = now;
                        }
                    }
                }

                slime_update(&slimes[slimeIdx], now, dt);
            }
        }

        particles_update(now, dt);

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

        BeginMode2D(camera);
        size_t tilesDrawn = 0;

        {
            const float tileWidthMip  = (float)(tilemap.tileset->tileWidth * zoomMipLevel);
            const float tileHeightMip = (float)(tilemap.tileset->tileHeight * zoomMipLevel);
            const size_t heightTiles = tilemap.heightTiles;
            const size_t widthTiles = tilemap.widthTiles;
            const Rectangle *tileRects = tilemap.tileset->textureRects;
            const float camLeft   = cameraRect.x;
            const float camTop    = cameraRect.y;
            const float camRight  = cameraRect.x + cameraRect.width;
            const float camBottom = cameraRect.y + cameraRect.height;

            for (size_t y = 0; y < heightTiles; y += zoomMipLevel) {
                for (size_t x = 0; x < widthTiles; x += zoomMipLevel) {
                    const Tile *tile = &tilemap.tiles[y * tilemap.widthTiles + x];
                    const Vector2 tilePos = tile->position;
                    if (tilePos.x + tileWidthMip >= camLeft &&
                        tilePos.y + tileHeightMip >= camTop &&
                        tilePos.x < camRight &&
                        tilePos.y < camBottom)
                    {
                        // Draw all tiles as textured rects (looks best, performs worst)
                        Rectangle textureRect = tileRects[tile->tileType];
                        DrawTextureRec(*tilemap.tileset->texture, textureRect, tile->position, WHITE);
                        tilesDrawn++;
                    }
                }
            }
        }
        //DrawRectangleRec(cameraRect, Fade(PINK, 0.5f));

        const Vector2 mousePosScreen = GetMousePosition();
        Vector2 mousePosWorld = { 0 };
        mousePosWorld.x += mousePosScreen.x * invZoom + cameraRect.x;
        mousePosWorld.y += mousePosScreen.y * invZoom + cameraRect.y;

        Tile *mouseTile = 0;
        if (findMouseTile) {
            mouseTile = tilemap_at_world_try(&tilemap, (int)mousePosWorld.x, (int)mousePosWorld.y);
            if (mouseTile) {
                // Draw red outline on hovered tile
                Rectangle mouseTileRect = (Rectangle){
                    mouseTile->position.x,
                    mouseTile->position.y,
                    (float)tilemap.tileset->tileWidth  * zoomMipLevel,
                    (float)tilemap.tileset->tileHeight * zoomMipLevel
                };
                DrawRectangleLinesEx(mouseTileRect, 1 * (int)invZoom, RED);
            }
        }

        {
            draw_commands_enable_culling(cameraRect);

            // Queue player for drawing
            player_push(&charlie);

            // Queue slimes for drawing
            for (int i = 0; i < MAX_SLIMES; i++) {
                const Slime *slime = &slimes[i];
                if (!slime->combat.hitPoints) {
                    continue;
                }

                slime_push(slime);
            }

            // Queue particles for drawing
            particles_push();

            draw_commands_flush();
        }

#if DEMO_VIEW_CULLING
        DrawRectangleLinesEx(cameraRect, 3, Fade(PINK, 0.8f));
#endif

#if DEMO_AI_TRACKING
        {
            Vector2 charlieCenter = player_get_bottom_center(&charlie);
            DrawCircle((int)charlieCenter.x, (int)charlieCenter.y, 640.0f, Fade(ORANGE, 0.5f));
        }
#endif

        EndMode2D();

        if (findMouseTile && mouseTile) {
            const float tooltipOffsetX = 10.0f;
            const float tooltipOffsetY = 10.0f;
            const float tooltipPad = 4.0f;

            float tooltipX = mousePosScreen.x + tooltipOffsetX;
            float tooltipY = mousePosScreen.y + tooltipOffsetY;
            const float tooltipW = 220.0f + tooltipPad * 2.0f;
            const float tooltipH = 40.0f  + tooltipPad * 2.0f;

            if (tooltipX + tooltipW > screenWidth ) tooltipX = screenWidth  - tooltipW;
            if (tooltipY + tooltipH > screenHeight) tooltipY = screenHeight - tooltipH;

            Rectangle tooltipRect = (Rectangle){ tooltipX, tooltipY, tooltipW, tooltipH };
            DrawRectangleRec(tooltipRect, Fade(RAYWHITE, 0.8f));
            DrawRectangleLinesEx(tooltipRect, 1, Fade(BLACK, 0.8f));

            int lineOffset = 0;
            DrawTextFont(fonts[fontIdx], TextFormat("tilePos : %.02f, %.02f", mouseTile->position.x, mouseTile->position.y),
                tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, fontHeight, BLACK);
            lineOffset += fontHeight;
            DrawTextFont(fonts[fontIdx], TextFormat("tileSize: %zu, %zu", tilemap.tileset->tileWidth, tilemap.tileset->tileHeight),
                tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, fontHeight, BLACK);
            lineOffset += fontHeight;
            DrawTextFont(fonts[fontIdx], TextFormat("tileType: %d", mouseTile->tileType),
                tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, fontHeight, BLACK);
            lineOffset += fontHeight;
        }

        const int minimapMargin = 6;
        const int minimapBorderWidth = 1;
        const int minimapX = screenWidth - minimapMargin - minimapTex.width - minimapBorderWidth * 2;
        const int minimapY = minimapMargin;
        const int minimapW = minimapTex.width + minimapBorderWidth * 2;
        const int minimapH = minimapTex.height + minimapBorderWidth * 2;
        const int minimapTexX = minimapX + minimapBorderWidth;
        const int minimapTexY = minimapY + minimapBorderWidth;
        DrawRectangleLines(minimapX, minimapY, minimapW, minimapH, BLACK);
        DrawTexture(minimapTex, minimapTexX, minimapTexY, WHITE);

        const char *text = 0;
        float hudCursorY = 0;

#define PUSH_TEXT(text, color) \
    DrawTextFont(fonts[fontIdx], text, margin + pad, hudCursorY, fontHeight, color); \
    hudCursorY += fontHeight + pad; \

        // Render HUD
        {
            int linesOfText = 8;
#if SHOW_DEBUG_STATS
            linesOfText += 7;
#endif
            const float margin = 6.0f;   // left/top margin
            const float pad = 4.0f;      // left/top pad
            const float hudWidth = 200.0f;
            const float hudHeight = linesOfText * (fontHeight + pad) + pad;

            hudCursorY += margin;

            const Color darkerGray = (Color){ 40, 40, 40, 255 };
            UNUSED(darkerGray);
            DrawRectangle((int)margin, (int)hudCursorY, (int)hudWidth, (int)hudHeight, DARKBLUE);
            DrawRectangleLines((int)margin, (int)hudCursorY, (int)hudWidth, (int)hudHeight, BLACK);

            hudCursorY += pad;

            if (gifRecording) {
                text = TextFormat("GIF RECORDING");
            } else {
                text = TextFormat("%2i fps (%.02f ms)", GetFPS(), GetFrameTime() * 1000.0f);
            }
            PUSH_TEXT(text, WHITE);
            text = TextFormat("Coins: %d", charlie.inventory.slots[PlayerInventorySlot_Coins].stackCount);
            PUSH_TEXT(text, YELLOW);

            text = TextFormat("Coins collected   %u", charlie.stats.coinsCollected);
            PUSH_TEXT(text, LIGHTGRAY);
            text = TextFormat("Damage dealt      %.2f", charlie.stats.damageDealt);
            PUSH_TEXT(text, LIGHTGRAY);
            text = TextFormat("Kilometers walked %.2f", charlie.stats.kmWalked);
            PUSH_TEXT(text, LIGHTGRAY);
            text = TextFormat("Slimes slain      %u", charlie.stats.slimesSlain);
            PUSH_TEXT(text, LIGHTGRAY);
            text = TextFormat("Times fist swung  %u", charlie.stats.timesFistSwung);
            PUSH_TEXT(text, LIGHTGRAY);
            text = TextFormat("Times sword swung %u", charlie.stats.timesSwordSwung);
            PUSH_TEXT(text, LIGHTGRAY);

#if SHOW_DEBUG_STATS
            text = TextFormat("Zoom          %.03f", camera.zoom);
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Zoom inverse  %.03f", invZoom);
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Zoom mip      %zu", zoomMipLevel);
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Tiles visible %zu", tilesDrawn);
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Font index    %zu", fontIdx);
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Particle FX   %zu", particle_effects_active());
            PUSH_TEXT(text, GRAY);
            text = TextFormat("Particles     %zu", particles_active());
            PUSH_TEXT(text, GRAY);
#endif
        }

#if ALPHA_NETWORKING
        // Render connected clients
        {
            int linesOfText = 1 + (int)server.clientsConnected;

            const float margin = 6.0f;   // left/top margin
            const float pad = 4.0f;      // left/top pad
            const float hudWidth = 200.0f;
            const float hudHeight = linesOfText * (fontHeight + pad) + pad;

            hudCursorY += margin;

            const Color darkerGray = (Color){ 40, 40, 40, 255 };
            UNUSED(darkerGray);
            DrawRectangle((int)margin, (int)hudCursorY, (int)hudWidth, (int)hudHeight, DARKBLUE);
            DrawRectangleLines((int)margin, (int)hudCursorY, (int)hudWidth, (int)hudHeight, BLACK);

            hudCursorY += pad;

            PUSH_TEXT("Connected clients:", WHITE);

            for (size_t i = 0; i < NETWORK_SERVER_MAX_CLIENTS; i++) {
                const NetworkServerClient *serverClient = &server.clients[i];
                if (serverClient->address.host) {
                    text = TextFormatIP(serverClient->address);
                    PUSH_TEXT(text, WHITE);
                }
            }
        }

        // Render chat history
        {
            if (chatActive) {
                const int linesOfText = (int)client.packetHistory.count;
                const float margin = 6.0f;   // left/bottom margin
                const float pad = 4.0f;      // left/bottom pad
                const float chatWidth = 320.0f;
                const float chatHeight = linesOfText * (fontHeight + pad) + pad;
                const float inputBoxHeight = fontHeight + pad * 2.0f;
                const float chatX = margin;
                const float chatY = screenHeight - margin - inputBoxHeight - chatHeight;

                // NOTE: The chat history renders from the bottom up (most recent message first)
                float cursorY = (chatY + chatHeight) - pad - fontHeight;
                const char *text = 0;

                DrawRectangle((int)chatX, (int)chatY, (int)chatWidth, (int)chatHeight, Fade(DARKGRAY, 0.8f));
                DrawRectangleLines((int)chatX, (int)chatY, (int)chatWidth, (int)chatHeight, Fade(BLACK, 0.8f));

                for (int i = (int)client.packetHistory.count - 1; i >= 0; i--) {
                    size_t packetIdx = (client.packetHistory.first + i) & (NETWORK_CLIENT_MAX_PACKETS - 1);
                    const Packet *packet = &client.packetHistory.packets[packetIdx];
                    assert(packet->data[0]);

                    text = TextFormat("[%s][%s]: %s", packet->timestampStr, TextFormatIP(packet->srcAddress), packet->data);
                    DrawTextFont(fonts[fontIdx], text, margin + pad, cursorY, fontHeight, WHITE);
                    cursorY -= fontHeight + pad;
                }

                static char chatInputText[MAX_CHAT_MESSAGE_LEN];
                Rectangle inputBox = { margin, screenHeight - margin - inputBoxHeight, chatWidth, inputBoxHeight };
                GuiTextBox(inputBox, chatInputText, sizeof(chatInputText), true);
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
                    network_client_send(&client, chatInputText, strnlen(chatInputText, sizeof(chatInputText)));
                    memset(chatInputText, 0, sizeof(chatInputText));
                }

                if (escape) {
                    chatActive = false;
                    escape = false;
                    memset(chatInputText, 0, sizeof(chatInputText));
                }
            }

            // HACK: Check for this *after* rendering chat to avoid pressing "t" causing it to show up in the chat box
            if (!chatActive && IsKeyDown(KEY_T)) {
                chatActive = true;
            }
        }
#endif
#undef PUSH_TEXT

        EndDrawing();
        //----------------------------------------------------------------------------------

        // If nobody else handled the escape key, time to exit!
        if (escape) {
            break;
        }
    }

    //--------------------------------------------------------------------------------------
    // Clean up
    //--------------------------------------------------------------------------------------
#if ALPHA_NETWORKING
    network_client_free(&client);
    network_server_free(&server);
    zed_net_shutdown();
#endif
    chat_free();
    tilemap_free(&tilemap);
    tileset_free(&tileset);
    UnloadTexture(minimapTex);
    UnloadTexture(tilesetTex);
    UnloadTexture(checkboardTexture);
    sound_catalog_free();
    spritesheet_catalog_free();
    particles_free();
    draw_commands_free();
    UnloadMusicStream(mus_whistle);
    UnloadMusicStream(mus_background);
    CloseAudioDevice();
    CloseWindow();
    for (size_t i = 0 ; i < ARRAY_SIZE(fonts); i++) {
        UnloadFont(fonts[i]);
    }

    fclose(logFile);
    return 0;
}