#include "spycam.h"

void Spycam::Init(Vector2 offset)
{
    //cameraGoal = position;
    //camera.target = (Vector2){
    //    (float)tilemap.width / 2.0f * tilemap.tileset->tileWidth,
    //    (float)tilemap.height / 2.0f * tilemap.tileset->tileHeight
    //};
    camera.offset = offset;
    camera.rotation = 0.0f;
    SetZoom(1.0f);
}

void Spycam::SetZoom(float zoom)
{
    const int negZoomMultiplier = 7; // 7x negative zoom (out)
    const int posZoomMultiplier = 1; // 2x positive zoom (in)
#if 1
    const float minZoom = 1.0f / (float)(1 << (negZoomMultiplier - 1));
    const float maxZoom = 1.0f * (float)(1 << (posZoomMultiplier - 1));
    zoom = CLAMP(zoom, minZoom, maxZoom);
#else
    const float minZoomPow2 = (float)(1 << (negZoomMultiplier - 1));
    const float maxZoomPow2 = (float)(1 << (posZoomMultiplier - 1));
    const float minZoom = 1.0f / minZoomPow2;
    const float maxZoom = 1.0f * maxZoomPow2;
    const float roundedZoom = roundf(zoom * minZoomPow2) / minZoomPow2;
    zoom = CLAMP(roundedZoom, minZoom, maxZoom);
#endif

    camera.zoom = zoom;
    invZoom = 1.0f / camera.zoom;
    // TODO: Calculate this based on how many tiles will appear on the screen, rather than camera zoom
    // Alternatively, we could group nearby tiles of the same type together into large quads?
    zoomMipLevel = MAX(1, (int)invZoom / 8);
}

void Spycam::Reset(void)
{
    cameraSpeed = CAMERA_SPEED_DEFAULT;
    SetZoom(1.0f);
    camera.rotation = 0.0f;
    camera.offset = Vector2{ GetRenderWidth() / 2.0f, GetRenderHeight() / 2.0f };
    camera.target = Vector2{ camera.target.x, camera.target.y };
    //printf("render: %d %d\n", GetRenderWidth(), GetRenderHeight());
    //printf("camera.offset: %f %f\n", camera.offset.x, camera.offset.y);
    //printf("camera.target: %f %f\n", camera.target.x, camera.target.y);
}

void Spycam::Update(const PlayerControllerState &input, double dt)
{
    if (input.cameraReset) Reset();
    if (input.dbgToggleFreecam) freeRoam = !freeRoam;

    if (!freeRoam) {
        camera.rotation = 0.0f;
        cameraSpeed = CAMERA_SPEED_DEFAULT;
        SetZoom(1.0f);
    } else {
        cameraSpeed = CLAMP(cameraSpeed + input.cameraSpeedDelta * METERS_TO_PIXELS(0.5f), METERS_TO_PIXELS(0.5f), METERS_TO_PIXELS(20.0f));

        // Camera zoom controls
        float zoom = camera.zoom;
#if 0
        zoom += input.cameraZoomDelta * 0.1f * zoom;
#elif 0
        if (input.cameraZoomDelta) {
            //printf("zoom: %f, log: %f\n", camera.zoom, log10f(camera.zoom));
            zoom *= input.cameraZoomDelta > 0.0f ? 2.0f : 0.5f;
        }
#else
        static bool zoomOut = false;
        if (input.cameraZoomOut) zoomOut = !zoomOut;
        zoom = zoomOut ? 0.5f : 1.0f;
#endif
        SetZoom(zoom);

        const float speed = (float)(cameraSpeed * invZoom * dt);
        cameraGoal.y -= speed * input.cameraNorth;
        cameraGoal.x += speed * input.cameraEast;
        cameraGoal.y += speed * input.cameraSouth;
        cameraGoal.x -= speed * input.cameraWest;

        if (input.cameraRotateCW) {
            camera.rotation += (float)(90.0f * dt);
            if (camera.rotation >= 360.0f) camera.rotation -= 360.0f;
        } else if (input.cameraRotateCCW) {
            camera.rotation -= (float)(90.0f * dt);
            if (camera.rotation < 0.0f) camera.rotation += 360.0f;
        }
    }
    camera.target = v2_add(camera.target, v2_scale(v2_sub(cameraGoal, camera.target), 1.0f));

    cameraRect.x = camera.target.x - camera.offset.x * invZoom;
    cameraRect.y = camera.target.y - camera.offset.y * invZoom;
    cameraRect.width = camera.offset.x * 2.0f * invZoom;
    cameraRect.height = camera.offset.y * 2.0f * invZoom;
}