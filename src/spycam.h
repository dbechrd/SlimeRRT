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
    const Rectangle &Spycam::GetRect(void) const { return cameraRect; }
    const Camera2D &GetCamera(void) const { return camera; }

    Vector2 cameraGoal {};
    float   cameraSpeed = CAMERA_SPEED_DEFAULT;
    bool    freeRoam {};

private:
    Camera2D  camera       {};
    Rectangle cameraRect   {};
    float     invZoom      {};
    int       zoomMipLevel {};
};