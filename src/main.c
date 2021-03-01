#include "particles.h"
#include "player.h"
#include "spritesheet.h"
#include "tileset.h"
#include "tilemap.h"
#include "raylib.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MIN(a, b) (((a)<(b))?(a):(b))
#define MAX(a, b) (((a)>(b))?(a):(b))
#define CLAMP(x, min, max) (MAX((min), MIN((x), (max))))

static FILE *logFile;

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

    InitWindow(800, 450, "raylib [textures] example - procedural images generation");
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

    Music backgroundMusic;
    backgroundMusic = LoadMusicStream("resources/fluquor_copyright.ogg");
    backgroundMusic.looping = true;
    PlayMusicStream(backgroundMusic);
    SetMusicVolume(backgroundMusic, 0.01f);
    UpdateMusicStream(backgroundMusic);

    Sound whooshSound;
    whooshSound = LoadSound("resources/whoosh1.ogg");
    //PlaySound(whooshSound);

    Image checkerboardImage = GenImageChecked(monitorWidth, monitorHeight, 32, 32, LIGHTGRAY, GRAY);
    Texture checkboardTexture = LoadTextureFromImage(checkerboardImage);
    UnloadImage(checkerboardImage);

    Texture charlieTex = LoadTexture("resources/charlie.png");
    assert(charlieTex.width);

    Spritesheet charlieSpritesheet = { 0 };

    charlieSpritesheet.spriteCount = 1;
    charlieSpritesheet.sprites = calloc(charlieSpritesheet.spriteCount, sizeof(*charlieSpritesheet.sprites));
    charlieSpritesheet.texture = &charlieTex;

    Sprite *charlieSprite = &charlieSpritesheet.sprites[0];
    charlieSprite->spritesheet = &charlieSpritesheet;
    charlieSprite->frameCount = 9;
    charlieSprite->frames = calloc(charlieSprite->frameCount, sizeof(*charlieSprite->frames));
    for (size_t frameIdx = 0; frameIdx < charlieSprite->frameCount; frameIdx++) {
        SpriteFrame *frame = &charlieSprite->frames[frameIdx];
        frame->x = 54 * (int)frameIdx;
        frame->y = 0;
        frame->width = 54;
        frame->height = 94;
    }

    Player charlie = { 0 };
    player_init(&charlie, "Charlie", charlieSprite);

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

        if (IsKeyPressed(KEY_P) && bloodParticles.state == ParticleEffectState_Dead) {
            double time = GetTime();
            Vector2 charlieGut = player_get_attach_point(&charlie, PlayerAttachPoint_Gut);
            particle_effect_start(&bloodParticles, time, charlieGut);
        }

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

        UpdateMusicStream(backgroundMusic);

        player_update(&charlie);

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
            player_draw(&charlie);
        }

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

        //Player player;
        //player.

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
    UnloadTexture(charlieTex);
    UnloadTexture(checkboardTexture);
    UnloadSound(whooshSound);
    UnloadMusicStream(backgroundMusic);

    CloseAudioDevice();
    CloseWindow();

    fclose(logFile);
    return 0;
}