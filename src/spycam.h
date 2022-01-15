#pragma once
#include "raylib/raylib.h"

#define CAMERA_SPEED_DEFAULT PIXELS_TO_UNITS(5)

typedef struct Camera2Di {
    Vector3i offset;  // Camera offset (displacement from target)
    Vector3i target;  // Camera target (rotation and zoom origin)
    int rotation;     // Camera rotation in degrees
    float zoom;       // Camera zoom (scaling), should be 1.0f by default
} Camera2Di;

struct Spycam {
    void Init(Vector3i offset);
    const Camera2D GetCamera(void) const;
    void SetZoom(float zoom);
    void Reset(void);
    void Update(const PlayerControllerState &input, double dt);

    float GetZoom(void) const { return invZoom; }
    float GetInvZoom(void) const { return camera.zoom; }
    int   GetZoomMipLevel(void) const { return zoomMipLevel; }
    const Recti &Spycam::GetRect(void) const { return cameraRect; }

    Vector3i cameraGoal {};
    int      cameraSpeed = CAMERA_SPEED_DEFAULT;
    bool     freeRoam {};

private:
    Camera2Di camera       {};
    Recti     cameraRect   {};
    float     invZoom      {};
    int       zoomMipLevel {};
};