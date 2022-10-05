#pragma once
#include "raylib/raylib.h"

#define CAMERA_SPEED_DEFAULT METERS_TO_PIXELS(1.0f);

struct Spycam {
    void Init(Vector2 offset);
    void SetZoom(float zoom);
    void Reset(void);
    void Update(const PlayerControllerState &input, double dt);

    float GetZoom(void) const { return invZoom; }
    float GetInvZoom(void) const { return camera.zoom; }
    int   GetZoomMipLevel(void) const { return zoomMipLevel; }
    const Rectangle &GetRect(void) const { return cameraRect; }
    const Camera2D &GetCamera(void) const { return camera; }
    Vector2 WorldToScreen(Vector2 world) const {
        Vector2 screen{
            world.x - camera.target.x + camera.offset.x,
            world.y - camera.target.y + camera.offset.y
        };
        return screen;
    }
    Vector2 ScreenToWorld(Vector2 screen) const {
        Vector2 mouseWorld{
            //screen.x + camera.target.x - camera.offset.x,
            //screen.y + camera.target.y - camera.offset.y
            cameraRect.x + screen.x * GetInvZoom(),
            cameraRect.y + screen.y * GetInvZoom()
        };
        return mouseWorld;
    }

    Vector2 cameraGoal {};
    float   cameraSpeed = CAMERA_SPEED_DEFAULT
    bool    freeRoam {};

private:
    Camera2D  camera       {};
    Rectangle cameraRect   {};
    float     invZoom      {};
    int       zoomMipLevel {};
    float     smoothing    = 0.2f;
};

thread_local static Spycam g_spycam{};