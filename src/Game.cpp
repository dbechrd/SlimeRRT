#include "Game.hpp"
#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"
#include <cassert>
#include <iostream>
#include <string>

Game::Game(unsigned int width, unsigned int height, const std::string &title, bool fullscreen = false)
    : windowWidth(width), windowHeight(height), title(title), fullscreen(fullscreen)
{
}

bool Game::Init()
{
    if (!font.loadFromFile("resources/sansation.ttf")) {
        std::cerr << "Failed to load font" << std::endl;
        return false;
    }

    CreateWindow();
    window.setVerticalSyncEnabled(vsync);
    window.setMouseCursorGrabbed(mouseCaptured);
    window.setKeyRepeatEnabled(false);

    if (!fullscreen) {
        // HACK(dlb): Subtract half of assumed taskbar height to center rendering area on overall monitor (a hack to match
        // the behavior of SDL_WINDOWPOS_CENTERED. Correct solution would be for SFML to call GetMonitorInfo instead of
        // EnumDisplaySettings internally, at least for the purposes of window positioning).
        sf::Vector2i windowPos = window.getPosition();
        windowPos.y -= 32;
        window.setPosition(windowPos);
    }

    textureCatalog.LoadDefaultBank();
    soundBufferCatalog.LoadDefaultBank();
    return true;
}

void Game::ToggleFullscreen()
{
    fullscreen = !fullscreen;
    RecreateWindow();
}

void Game::ToggleVSync()
{
    vsync = !vsync;
    window.setVerticalSyncEnabled(vsync);
}

void Game::ToggleMouseCapture()
{
    mouseCaptured = !mouseCaptured;
    window.setMouseCursorGrabbed(mouseCaptured);
}

void Game::ToggleCameraBounded()
{
    cameraBounded = !cameraBounded;
}

void Game::ToggleCameraFollowing()
{
    cameraFollowing = !cameraFollowing;
}

void Game::TogglePlayerIsAGhost()
{
    playerIsAGhost = !playerIsAGhost;
}

bool Game::IsMouseInBounds()
{
    // NOTE: Gets mouse position relative to window position in display coordinates
    sf::Vector2i mouse = sf::Mouse::getPosition(window);
    sf::Vector2i windowSize(window.getSize());

    bool inBounds = (mouse.x >= 0) && (mouse.x < windowSize.x) &&
        (mouse.y >= 0) && (mouse.y < windowSize.y);
    return inBounds;
}

void Game::PlaySound(std::string soundId)
{
    soundSystem.PlaySound(soundBufferCatalog[soundId]);
}

void Game::CreateWindow()
{
    sf::VideoMode videoMode;

    std::vector<sf::VideoMode> fullscreenModes = sf::VideoMode::getFullscreenModes();
    if (fullscreenModes.empty()) {
        std::cout << "According to SFML, fullscreen mode is not supported.\n";
        fullscreen = false;
    }

    if (fullscreen) {
        videoMode = fullscreenModes.front();
    } else {
        videoMode = sf::VideoMode(windowWidth, windowHeight);
    }

    sf::Uint32 style = fullscreen ? sf::Style::Fullscreen : sf::Style::Default;
    window.create(videoMode, title, style);
    window.setVerticalSyncEnabled(vsync);
    window.setMouseCursorGrabbed(mouseCaptured);
}

void Game::RecreateWindow()
{
    window.close();
    CreateWindow();
}