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

#define MIN(a, b) (((a)<(b))?(a):(b))
#define MAX(a, b) (((a)>(b))?(a):(b))
#define CLAMP(x, min, max) (MAX((min), MIN((x), (max))))

static FILE *logFile;

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

int SlimeCompareDepth(void const *slimeA, void const *slimeB)
{
    float aDepth = slime_get_bottom_center(slimeA).y;
    float bDepth = slime_get_bottom_center(slimeB).y;
    if (aDepth < bDepth) {
        return -1;
    } else if( aDepth > bDepth) {
        return 1;
    } else {
        return 0;
    }
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

void traceLogCallback(int logType, const char *text, va_list args)
{
    vfprintf(logFile, text, args);
    fputs("\n", logFile);
    fflush(logFile);
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

    Font defaultFont = GetFontDefault();

    Music mus_background;
    mus_background = LoadMusicStream("resources/fluquor_copyright.ogg");
    mus_background.looping = true;
    PlayMusicStream(mus_background);
    SetMusicVolume(mus_background, 0.01f);
    UpdateMusicStream(mus_background);

    Sound snd_whoosh = LoadSound("resources/whoosh1.ogg");
    Sound snd_squish1 = LoadSound("resources/squish1.ogg");
    Sound snd_squish2 = LoadSound("resources/squish2.ogg");
    Sound snd_slime_stab1 = LoadSound("resources/slime_stab1.ogg");

    Image checkerboardImage = GenImageChecked(monitorWidth, monitorHeight, 32, 32, LIGHTGRAY, GRAY);
    Texture checkboardTexture = LoadTextureFromImage(checkerboardImage);
    UnloadImage(checkerboardImage);

    Texture tex_charlie = LoadTexture("resources/charlie.png");
    assert(tex_charlie.width);

    Spritesheet charlieSpritesheet = { 0 };
    charlieSpritesheet.spriteCount = 1;
    charlieSpritesheet.sprites = calloc(charlieSpritesheet.spriteCount, sizeof(*charlieSpritesheet.sprites));
    charlieSpritesheet.texture = &tex_charlie;

    Sprite *charlieSprite = &charlieSpritesheet.sprites[0];
    charlieSprite->spritesheet = &charlieSpritesheet;
    const int charlieFramesInRow = 9;
    charlieSprite->frameCount = (size_t)charlieFramesInRow * 3;
    charlieSprite->frames = calloc(charlieSprite->frameCount, sizeof(*charlieSprite->frames));
    for (size_t frameIdx = 0; frameIdx < charlieSprite->frameCount; frameIdx++) {
        SpriteFrame *frame = &charlieSprite->frames[frameIdx];
        frame->x = 54 * ((int)frameIdx % charlieFramesInRow);
        frame->y = 94 * ((int)frameIdx / charlieFramesInRow);
        frame->width = 54;
        frame->height = 94;
    }

    Player charlie = { 0 };
    player_init(&charlie, "Charlie", charlieSprite);

    Texture tex_slime = LoadTexture("resources/peter48.png");
    assert(tex_slime.width);

    Spritesheet slimeSpritesheet = { 0 };
    slimeSpritesheet.spriteCount = 1;
    slimeSpritesheet.sprites = calloc(slimeSpritesheet.spriteCount, sizeof(*slimeSpritesheet.sprites));
    slimeSpritesheet.texture = &tex_slime;

    Sprite *slimeSprite = &slimeSpritesheet.sprites[0];
    slimeSprite->spritesheet = &slimeSpritesheet;
    const int slimeFramesInRow = 4;
    slimeSprite->frameCount = (size_t)slimeFramesInRow * 2;
    slimeSprite->frames = calloc(slimeSprite->frameCount, sizeof(*slimeSprite->frames));
    for (size_t frameIdx = 0; frameIdx < slimeSprite->frameCount; frameIdx++) {
        SpriteFrame *frame = &slimeSprite->frames[frameIdx];
        frame->x = 48 * ((int)frameIdx % slimeFramesInRow);
        frame->y = 48 * ((int)frameIdx / slimeFramesInRow);
        frame->width = 48;
        frame->height = 48;
    }

#define SLIMES_COUNT 10
    Slime slimes[SLIMES_COUNT] = { 0 };
    int slimesByDepth[SLIMES_COUNT] = { 0 };
    for (int i = 0; i < SLIMES_COUNT; i++) {
        slime_init(&slimes[i], 0, slimeSprite);
        slimes[i].position.x = (float)GetRandomValue(0, 800);
        slimes[i].position.y = (float)GetRandomValue(0, 800);
        slimesByDepth[i] = i;
    }

    Texture tilesetTex = LoadTexture("resources/tiles64.png");
    assert(tilesetTex.width);

    Tileset tileset = { 0 };
    tileset_init_ex(&tileset, &tilesetTex, 64, 64, Tile_Count);

    Tilemap tilemap = { 0 };
    tilemap_generate_ex(&tilemap, 128, 128, &tileset);
    //tilemap_generate_ex(&tilemap, 512, 512, &tileset);

    Color tileColors[Tile_Count] = {
        GREEN,
        SKYBLUE,
        DARKGREEN,
        BROWN,
        GRAY
    };

    Camera2D camera = { 0 };
    camera.target = (Vector2){ (float)tilemap.widthTiles / 2.0f * tilemap.tileset->tileWidth, (float)tilemap.heightTiles / 2.0f * tilemap.tileset->tileHeight };
    camera.offset = (Vector2){ screenWidth / 2.0f, screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    int cameraReset = 0;
    int cameraFollowPlayer = 1;

    ParticleEffect bloodParticles = { 0 };
    particle_effect_generate(&bloodParticles, ParticleEffectType_Blood, 40, 1.2);

    SetTargetFPS(60);
    //---------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())
    {
        if (IsWindowResized()) {
            screenWidth = GetScreenWidth();
            screenHeight = GetScreenHeight();
            cameraReset = 1;
        }

        UpdateMusicStream(mus_background);

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
            const float playerSpeed = 5.0f;
            Vector2 moveOffset = { 0 };
            if (IsKeyDown(KEY_A)) moveOffset.x -= playerSpeed;
            if (IsKeyDown(KEY_D)) moveOffset.x += playerSpeed;
            if (IsKeyDown(KEY_W)) moveOffset.y -= playerSpeed;
            if (IsKeyDown(KEY_S)) moveOffset.y += playerSpeed;
            player_move(&charlie, moveOffset);

            camera.target = player_get_center(&charlie);
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
            charlie.equippedWeapon = PlayerWeapon_None;
        } else if (IsKeyPressed(KEY_TWO)) {
            charlie.equippedWeapon = PlayerWeapon_Sword;
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            const float playerAttackReach = 50.0f;
            const float playerAttackDamage = 2.0f;

            if (player_attack(&charlie)) {
                size_t slimesHit = 0;
                for (size_t slimeIdx = 0; slimeIdx < SLIMES_COUNT; slimeIdx++) {
                    if (!slimes[slimeIdx].hitPoints)
                        continue;

                    Vector2 playerToSlime = v2_sub(slimes[slimeIdx].position, charlie.position);
                    if (v2_length_sq(playerToSlime) <= playerAttackReach * playerAttackReach) {
                        slimes[slimeIdx].hitPoints = MAX(0.0f, slimes[slimeIdx].hitPoints - playerAttackDamage);
                        slimesHit++;
                    }
                }

                if (slimesHit) {
                    PlaySound(snd_slime_stab1);
                } else {
                    PlaySound(snd_whoosh);
                }
            }
        }

        player_update(&charlie);

        {
            const float slimeMoveSpeed = 3.0f;
            const float slimeAttackReach = 32.0f;
            const float slimeAttackDamage = 1.0f;
            const float slimeRadius = 32.0f;

            static double lastSquish = 0;
            double timeSinceLastSquish = GetTime() - lastSquish;

            for (size_t slimeIdx = 0; slimeIdx < SLIMES_COUNT; slimeIdx++) {
                if (!slimes[slimeIdx].hitPoints)
                    continue;

                Vector2 slimeToPlayer = v2_sub(player_get_bottom_center(&charlie), slime_get_bottom_center(&slimes[slimeIdx]));
                if (v2_length_sq(slimeToPlayer) > slimeAttackReach * slimeAttackReach) {
                    Vector2 slimeMoveDir = v2_normalize(slimeToPlayer);
                    Vector2 slimeMove = v2_scale(slimeMoveDir, slimeMoveSpeed + (float)GetRandomValue(0, 2));
                    Vector2 newPos = v2_add(slimes[slimeIdx].position, slimeMove);

                    int willCollide = 0;
                    for (size_t collideIdx = 0; collideIdx < SLIMES_COUNT; collideIdx++) {
                        if (collideIdx == slimeIdx)
                            continue;

                        if (v2_length_sq(v2_sub(newPos, slimes[collideIdx].position)) < slimeRadius * slimeRadius) {
                            willCollide = 1;
                            break;
                        }
                    }

                    if (!willCollide) {
                        slime_move(&slimes[slimeIdx], slimeMove);
                        if (!IsSoundPlaying(snd_squish1) && !IsSoundPlaying(snd_squish2) && timeSinceLastSquish > 1.0) {
                            //PlaySound(GetRandomValue(0, 1) ? snd_squish1 : snd_squish2);
                            lastSquish = GetTime();
                        }
                    }
                }

                slimeToPlayer = v2_sub(player_get_bottom_center(&charlie), slime_get_bottom_center(&slimes[slimeIdx]));
                if (v2_length_sq(slimeToPlayer) <= slimeAttackReach * slimeAttackReach) {
                    if (slime_attack(&slimes[slimeIdx])) {
                        charlie.hitPoints = MAX(0.0f, charlie.hitPoints - slimeAttackDamage);
                        double time = GetTime();
                        Vector2 charlieGut = player_get_attach_point(&charlie, PlayerAttachPoint_Gut);
                        particle_effect_start(&bloodParticles, time, charlieGut);
                        // TODO: Kill peter if he dies
                    }
                }

                slime_update(&slimes[slimeIdx]);
            }
        }

        {
            double time = GetTime();
            Vector2 charlieGut = player_get_attach_point(&charlie, PlayerAttachPoint_Gut);
            bloodParticles.origin = charlieGut;
            particle_effect_update(&bloodParticles, time);
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

#if 0
        const float cameraRectPad = 50.0f * invZoom;
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
                float depth = slime_get_bottom_center(&slimes[index]).y;
                int j = i - 1;
                while (j >= 0 && slime_get_bottom_center(&slimes[slimesByDepth[j]]).y > depth) {
                    slimesByDepth[j + 1] = slimesByDepth[j];
                    j--;
                }
                slimesByDepth[j + 1] = index;
            }

            for (int i = 0; i < SLIMES_COUNT; i++) {
                int slimeIdx = slimesByDepth[i];
                if (!slimes[slimeIdx].hitPoints)
                    continue;

                slime_draw(&slimes[slimeIdx]);
            }

            // TODO: Sort player into depth list as well
            player_draw(&charlie);
        }

        for (size_t slimeIdx = 0; slimeIdx < SLIMES_COUNT; slimeIdx++) {
            if (!slimes[slimeIdx].hitPoints) continue;

            DrawHealthBar((int)slime_get_center(&slimes[slimeIdx]).x, (int)slimes[slimeIdx].position.y,
                slimes[slimeIdx].hitPoints, slimes[slimeIdx].maxHitPoints);
        }
        DrawHealthBar((int)player_get_center(&charlie).x, (int)charlie.position.y, charlie.hitPoints, charlie.maxHitPoints);

        {
            double time = GetTime();
            particle_effect_draw(&bloodParticles, time);
        }

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

        const char *cameraZoomText = TextFormat("camera.zoom: %.03f (inv: %.03f)", camera.zoom, invZoom);
        DrawText(cameraZoomText, 10, 30, 10, BLACK);

        const char *mipText = TextFormat("zoopMipLevel: %zu", zoomMipLevel);
        DrawText(mipText, 10, 40, 10, BLACK);

        const char *tilesDrawnText = TextFormat("tilesDrawn: %zu", tilesDrawn);
        DrawText(tilesDrawnText, 10, 50, 10, BLACK);

#if 1
        Vector2 pos = { 10.0f, 10.0f };
        const int fpsPad = 4;

        int fontSize = MAX(10, defaultFont.baseSize);
        const int spacing = fontSize / defaultFont.baseSize;

        const char *fpsText = TextFormat("%.02f ms (%2i fps)", GetFrameTime() * 1000.0f, GetFPS());
        Vector2 fpsSize = MeasureTextEx(defaultFont, fpsText, (float)fontSize, (float)spacing);

        Rectangle fpsRect = (Rectangle){ pos.x - fpsPad, pos.y - fpsPad, fpsSize.x + fpsPad*2, fpsSize.y + fpsPad*2 };
        DrawRectangleRec(fpsRect, Fade(RAYWHITE, 0.8f));
        DrawRectangleLinesEx(fpsRect, 1, Fade(BLACK, 0.8f));
        //DrawRectangleRec();
        DrawTextEx(defaultFont, fpsText, pos, (float)fontSize, (float)spacing, DARKGRAY);
#else
        DrawFPS(10, 10);
#endif

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    //--------------------------------------------------------------------------------------
    // Clean up
    //--------------------------------------------------------------------------------------
    particle_effect_free(&bloodParticles);
    tilemap_free(&tilemap);
    tileset_free(&tileset);
    UnloadTexture(tilesetTex);
    free(charlieSprite->frames);
    free(charlieSpritesheet.sprites);
    UnloadTexture(tex_slime);
    UnloadTexture(tex_charlie);
    UnloadTexture(checkboardTexture);
    UnloadSound(snd_slime_stab1);
    UnloadSound(snd_squish2);
    UnloadSound(snd_squish1);
    UnloadSound(snd_whoosh);
    UnloadMusicStream(mus_background);

    CloseAudioDevice();
    CloseWindow();

    fclose(logFile);
    return 0;
}