#include "spycam.h"

void Spycam::Init(Vector3i offset)
{
    //cameraGoal = position;
    //camera.target = (Vector2){
    //    (float)tilemap.width / 2.0f * tilemap.tileset->tileWidth,
    //    (float)tilemap.height / 2.0f * tilemap.tileset->tileHeight
    //};
    camera.offset = offset;
    camera.rotation = 0;
    SetZoom(1.0f);
}

const Camera2D Spycam::GetCamera(void) const
{
    Camera2D cam{};
    cam.offset = { UNITS_TO_PIXELS((float)camera.offset.x), UNITS_TO_PIXELS((float)camera.offset.y) };
    cam.target = { UNITS_TO_PIXELS((float)camera.target.x), UNITS_TO_PIXELS((float)camera.target.y) };
    cam.rotation = (float)camera.rotation;
    cam.zoom = camera.zoom;
    return cam;
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
    camera.rotation = 0;
    camera.offset = { PIXELS_TO_UNITS(GetRenderWidth() / 2), PIXELS_TO_UNITS(GetRenderHeight() / 2) };
}

void Spycam::Update(const PlayerControllerState &input, double dt)
{
    if (input.cameraReset) Reset();
    if (input.dbgToggleFreecam) freeRoam = !freeRoam;

    if (!freeRoam) {
        camera.rotation = 0;
        cameraSpeed = CAMERA_SPEED_DEFAULT;
        SetZoom(1.0f);
    } else {
        cameraSpeed = CLAMP(cameraSpeed + PIXELS_TO_UNITS(input.cameraSpeedDelta), PIXELS_TO_UNITS(1), PIXELS_TO_UNITS(50));

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

        if (input.cameraWest)  cameraGoal.x -= (int)(cameraSpeed / camera.zoom);
        if (input.cameraEast)  cameraGoal.x += (int)(cameraSpeed / camera.zoom);
        if (input.cameraNorth) cameraGoal.y -= (int)(cameraSpeed / camera.zoom);
        if (input.cameraSouth) cameraGoal.y += (int)(cameraSpeed / camera.zoom);
        if (input.cameraRotateCW) {
            camera.rotation += (int)(90 * dt);
            if (camera.rotation >= 360) camera.rotation -= 360;
        } else if (input.cameraRotateCCW) {
            camera.rotation -= (int)(90 * dt);
            if (camera.rotation < 0) camera.rotation += 360;
        }
    }
    camera.target = v3_add(camera.target, v3_sub(cameraGoal, camera.target));

    cameraRect.x = (int)(camera.target.x - camera.offset.x * invZoom);
    cameraRect.y = (int)(camera.target.y - camera.offset.y * invZoom);
    cameraRect.width  = (int)(camera.offset.x * 2 * invZoom);
    cameraRect.height = (int)(camera.offset.y * 2 * invZoom);
}