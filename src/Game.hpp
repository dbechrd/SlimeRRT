#pragma once
#include "SoundBufferCatalog.hpp"
#include "SoundSystem.hpp"
#include "TextureCatalog.hpp"
#include "SFML/Graphics.hpp"
#include <string>

class Game
{
public:
    Game(unsigned int width, unsigned int height, const std::string &title, bool fullscreen);

    bool Init();

    void ToggleFullscreen();
    void ToggleVSync();
    void ToggleMouseCapture();
    void ToggleCameraBounded();
    void ToggleCameraFollowing();
    void TogglePlayerIsAGhost();

    inline bool IsFullscreen()      { return fullscreen; }
    inline bool IsVsyncEnabled()    { return vsync; }
    inline bool IsMouseCaptured()   { return mouseCaptured; }
    inline bool IsCameraBounded()   { return cameraBounded; }
    inline bool IsCameraFollowing() { return cameraFollowing; }
    inline bool IsPlayerAGhost()    { return playerIsAGhost; }
    bool IsMouseInBounds();

    const sf::Font &Font()                              { return font; }
    const sf::SoundBuffer &SoundBuffer(std::string id)  { return soundBufferCatalog[id]; }
    const sf::Texture &Texture(std::string id)          { return textureCatalog[id]; }
    sf::RenderWindow &Window()                          { return window; }

    void PlaySound(std::string soundId);

private:
    unsigned int windowWidth;
    unsigned int windowHeight;
    const std::string title;
    bool fullscreen;
    bool vsync = true;
    bool mouseCaptured = false; //true;
    bool cameraBounded = true;
    bool cameraFollowing = true;
    bool playerIsAGhost = false;

    sf::Font font;
    SoundBufferCatalog soundBufferCatalog;
    TextureCatalog textureCatalog;
    sf::RenderWindow window;

    class SoundSystem soundSystem;

    void CreateWindow();
    void RecreateWindow();
};