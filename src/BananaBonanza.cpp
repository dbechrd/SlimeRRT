
////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Effect.hpp"
#include "Globals.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <cassert>
#include <string>
#include <vector>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <random>

std::random_device randDevice;
std::mt19937 mersenne(randDevice());

namespace Vector2f {
    const sf::Vector2f Zero(0.0f, 0.0f);

    float LengthSq(const sf::Vector2f &vector)
    {
        float lengthSq = vector.x * vector.x + vector.y * vector.y;
        return lengthSq;
    }

    float Length(const sf::Vector2f &vector)
    {
        float length = std::sqrtf(LengthSq(vector));
        return length;
    }

    bool IsZero(const sf::Vector2f &vector)
    {
        return vector.x == 0.0f && vector.y == 0.0f;
    }

    sf::Vector2f Normalize(const sf::Vector2f &vector)
    {
        if (IsZero(vector)) {
            return vector;
        }

        float length = Length(vector);
        return sf::Vector2f(vector.x / length, vector.y / length);
    }

    float Dot(const sf::Vector2f &left, const sf::Vector2f &right)
    {
        float dot = left.x * right.x + left.y * right.y;
        return dot;
    }
};

class Game {
public:
    Game(unsigned int width, unsigned int height, const std::string &title, bool fullscreen = false)
        : windowWidth(width), windowHeight(height), title(title), fullscreen(fullscreen) {}

    bool Init()
    {
        if (!font.loadFromFile("resources/sansation.ttf")) {
            return false;
        }

        CreateWindow();
        window.setVerticalSyncEnabled(vsync);
        window.setMouseCursorGrabbed(mouseCaptured);
        window.setKeyRepeatEnabled(false);

        if (!fullscreen) {
            // HACK(dlb): Subtract half of assumed taskbar height to center rendering area on overall monitor (a hack to match
            // the behavior of SDL_WINDOWPOS_CENTERED. Correct solution would be for SFML to call GetMonitorInfo instead of
            // EnumDisplaySettings internally, at least for the purposes of window positioning.
            sf::Vector2i windowPos = window.getPosition();
            windowPos.y -= 32;
            window.setPosition(windowPos);
        }

        initialized = true;
        return true;
    }

    sf::Font &Font()
    {
        assert(initialized);
        return font;
    }

    sf::RenderWindow &Window()
    {
        assert(initialized);
        return window;
    }

    bool IsFullscreen() { return fullscreen; }
    void ToggleFullscreen()
    {
        fullscreen = !fullscreen;
        RecreateWindow();
    }

    bool IsVsyncEnabled() { return vsync; }
    void ToggleVSync()
    {
        vsync = !vsync;
        window.setVerticalSyncEnabled(vsync);
    }

    bool IsMouseCaptured() { return mouseCaptured; }
    void ToggleMouseCapture()
    {
        mouseCaptured = !mouseCaptured;
        window.setMouseCursorGrabbed(mouseCaptured);
    }

    bool IsMouseInBounds()
    {
        // NOTE: Gets mouse position relative to window position in display coordinates
        sf::Vector2i mouse = sf::Mouse::getPosition(window);
        sf::Vector2i windowSize(window.getSize());

        bool inBounds = (mouse.x >= 0) && (mouse.x < windowSize.x) &&
                        (mouse.y >= 0) && (mouse.y < windowSize.y);
        return inBounds;
    }

    bool IsCameraBounded() { return cameraBounded; }
    void ToggleCameraBounded()
    {
        cameraBounded = !cameraBounded;
    }

    bool IsCameraFollowing() { return cameraFollowing; }
    void ToggleCameraFollowing()
    {
        cameraFollowing = !cameraFollowing;
    }

    bool IsPlayerAGhost() { return playerIsAGhost; }
    void TogglePlayerIsAGhost()
    {
        playerIsAGhost = !playerIsAGhost;
    }

private:
    void CreateWindow()
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

    void RecreateWindow()
    {
        if (initialized) {
            window.close();
        }
        CreateWindow();
    }

    unsigned int windowWidth;
    unsigned int windowHeight;
    const std::string title;
    bool fullscreen;
    bool vsync = true;
    bool mouseCaptured = false; //true;
    bool cameraBounded = true;
    bool cameraFollowing = true;
    bool playerIsAGhost = false;

    bool initialized = false;
    sf::Font font;
    sf::RenderWindow window;
};

class ShadedSprite
{
public:
    ShadedSprite(const ShadedSprite &shadedSprite)
        : sprite(shadedSprite.sprite), shader(shadedSprite.shader) {}

    ShadedSprite(const sf::Texture &texture, const sf::Shader &shader)
        : sprite(texture), shader(&shader) {}

    ShadedSprite(const sf::Texture &texture, const sf::IntRect &rect, const sf::Shader &shader)
        : sprite(texture, rect), shader(&shader) {}

    void draw(sf::RenderTarget& target) const
    {
        sf::RenderStates states = sf::RenderStates::Default;
        states.shader = shader;
        target.draw(sprite, states);
    }

//private:
    sf::Sprite sprite;
    const sf::Shader *shader;
};

class ShadedText
{
public:
    ShadedText(float x, float y, const std::string &str, const sf::Font &font, const sf::Shader &shader,
        const sf::Color &color = sf::Color::White, unsigned int characterSize = 20U)
        : text(str, font, characterSize), shader(&shader)
    {
        text.setPosition(x, y);
        text.setFillColor(color);
        text.setOutlineColor(sf::Color(255U - color.r, 255U - color.g, 255U - color.b, color.a));
        text.setOutlineThickness(1);
    }

    ShadedText(const ShadedText &shadedText) : text(shadedText.text), shader(shadedText.shader) {}

    void draw(sf::RenderTarget& target) const
    {
        sf::RenderStates states = sf::RenderStates::Default;
        states.shader = shader;
        target.draw(text, states);
    }

//private:
    sf::Text text;
    const sf::Shader *shader;
};

class Player
{
public:
    enum Direction {
        Idle      = 0,
        West      = 1,
        East      = 2,
        South     = 3,
        North     = 4,
        NorthEast = 5,
        NorthWest = 6,
        SouthWest = 7,
        SouthEast = 8
    };

    Player(const std::string &name, ShadedSprite &sprite)
        : name(name), shadedSprite(&sprite)
    {
        updateDirection(0.0f, 0.0f);
    }

    void move(const sf::Vector2f &offset)
    {
        shadedSprite->sprite.move(offset);
        updateDirection(offset.x, offset.y);
    }

    void move(float offsetX, float offsetY)
    {
        shadedSprite->sprite.move(offsetX, offsetY);
        updateDirection(offsetX, offsetY);
    }

    void draw(sf::RenderTarget& target) const
    {
        shadedSprite->draw(target);
    }

//private:
    const std::string &name;
    ShadedSprite *shadedSprite;
    Direction direction;

private:
    void updateDirection(float offsetX, float offsetY)
    {
        // NOTE: Branching could be removed by putting the sprites in a more logical order.. doesn't matter if this
        // only applies to players since there would be so few.
        if (offsetX > 0.0f) {
            if (offsetY > 0.0f) {
                direction = SouthEast;
            } else if (offsetY < 0.0f) {
                direction = NorthEast;
            } else {
                direction = East;
            }
        } else if (offsetX < 0.0f) {
            if (offsetY > 0.0f) {
                direction = SouthWest;
            } else if (offsetY < 0.0f) {
                direction = NorthWest;
            } else {
                direction = West;
            }
        } else {
            if (offsetY > 0.0f) {
                direction = South;
            } else if (offsetY < 0.0f) {
                direction = North;
            } else {
                direction = Idle;
            }
        }
        // TODO: Load rect info from config file or something?
        shadedSprite->sprite.setTextureRect(sf::IntRect(54 * direction, 0, 54, 93));
    }
};

class BananaParticles : public Effect
{
public:

    BananaParticles() : Effect("banana_particles"), pointCloud(sf::Points, 10000) {}

    bool onLoad(const sf::FloatRect &levelRect)
    {
        // Check if geometry shaders are supported
        if (!sf::Shader::isGeometryAvailable()) {
            return false;
        }

        // Load the texture
        sf::Image img;
        if (!img.loadFromFile("resources/banana_npot.png")) {
            return false;
        }
        img.flipVertically();
        if (!texture.loadFromImage(img)) {
            return false;
        }

        sf::Vector2u textureSize = texture.getSize();
        assert((float)textureSize.x < levelRect.width);
        assert((float)textureSize.x < levelRect.height);

        sf::Vector2f halfTextureSize = sf::Vector2f(textureSize) / 2.0f;

        // Move the points in the point cloud to random positions in the level
        std::uniform_real_distribution<> randomX(halfTextureSize.x, levelRect.width  - halfTextureSize.x);
        std::uniform_real_distribution<> randomY(halfTextureSize.y, levelRect.height - halfTextureSize.y);
        for (std::size_t i = 0; i < 10000; i++) {
            pointCloud[i].position.x = (float)randomX(mersenne);
            pointCloud[i].position.y = (float)randomY(mersenne);
        }

        // Load the shader
        if (!shader.loadFromFile("resources/billboard.vert", "resources/billboard.geom", "resources/billboard.frag")) {
            return false;
        }
        shader.setUniform("texture", sf::Shader::CurrentTexture);

        transform = sf::Transform::Identity;

        return true;
    }

    void onUpdate(float time, const sf::Vector2f &viewSize)
    {
        //transform.translate(400, 300);

        // Set the render resolution (used for proper scaling)
        shader.setUniform("resolution", viewSize);

        sf::Vector2f textureSize(texture.getSize());
        shader.setUniform("size", textureSize);

        float angle = std::fmodf(time * 90.0f, 360.0f);
        float angleRad = (float)(angle * PI / 180.0f);
        shader.setUniform("angle", angleRad);
    }

    void onDraw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        // Prepare the render state
        states.shader = &shader;
        states.texture = &texture;
        states.transform = transform;

        // Draw the point cloud
        target.draw(pointCloud, states);
    }

private:
    sf::Texture texture;
    sf::Transform transform;
    sf::Shader shader;
    sf::VertexArray pointCloud;
};

////////////////////////////////////////////////////////////
// Entry point of application
////////////////////////////////////////////////////////////
int main()
{
    bool shaderCheck = sf::Shader::isAvailable();
    if (!shaderCheck) {
        return EXIT_FAILURE;
    }

    bool vertexBufferCheck = sf::VertexBuffer::isAvailable();
    if (!vertexBufferCheck) {
        return EXIT_FAILURE;
    }

    Game game(1600, 900, "Banana Bonanza", false);
    if (!game.Init()) {
        return EXIT_FAILURE;
    }
    sf::RenderWindow &window = game.Window();

    // Load textures
    sf::Texture textBackgroundTexture;
    if (!textBackgroundTexture.loadFromFile("resources/text-background.png")) {
        return EXIT_FAILURE;
    }

    sf::Texture grassTexture;
    if (!grassTexture.loadFromFile("resources/grass2.jpg")) {
        return EXIT_FAILURE;
    }
    grassTexture.setRepeated(true);

    sf::Texture playerTexture;
    if (!playerTexture.loadFromFile("resources/charlie.png")) {
        return EXIT_FAILURE;
    }

    // Load shaders
    sf::Shader textureColorShader;
    if (!textureColorShader.loadFromFile("resources/texture_color.frag", sf::Shader::Fragment)) {
        return EXIT_FAILURE;
    }
    textureColorShader.setUniform("texture", sf::Shader::CurrentTexture);

    // Load audio buffers
    sf::FileInputStream backgroundMusicStream;
    backgroundMusicStream.open("resources/orchestral.ogg");
    sf::Music backgroundMusic;
    backgroundMusic.openFromStream(backgroundMusicStream);
    backgroundMusic.setLoop(true);
    backgroundMusic.setVolume(2);
    backgroundMusic.play();

    sf::Music whistleMusic;
    whistleMusic.openFromFile("resources/whistle.ogg");
    whistleMusic.setLoop(true);
    whistleMusic.setVolume(50);

    sf::SoundBuffer bananaSoundBuffer;
    bananaSoundBuffer.loadFromFile("resources/banana.ogg");
    const size_t bananaSoundsCount = 5;
    sf::Sound bananaSounds[bananaSoundsCount];
    for (int i = 0; i < bananaSoundsCount; ++i) {
        bananaSounds[i].setBuffer(bananaSoundBuffer);
    }
    size_t bananaSoundIdx = 0;

    // Create sprites
    // NOTE: If this is smaller than the resolution of the camera, then bounding breaks miserably
    sf::IntRect levelRect(0, 0, 4096, 4096);
    sf::FloatRect levelRectf(levelRect);

    ShadedSprite grassSprite(grassTexture, levelRect, textureColorShader);

    ShadedSprite playerSprite(playerTexture, textureColorShader);
    Player player("Charlie", playerSprite);
    const float playerSpeed = 5.0f;
    sf::Vector2f playerVelocity;

    sf::View camera = sf::View(window.getDefaultView());
    const float keyboardCameraSpeed = 20.0f;
    const float mouseCameraSpeed = 30.0f;
    sf::Vector2f cameraVelocity;

    sf::Sprite textBackground(textBackgroundTexture);
    textBackground.setColor(sf::Color(255, 255, 255, 255));

    sf::Color navyColor(80, 80, 80, 255);
    ShadedText testText(0.0f, 0.0f, "Owl is the best!", game.Font(), textureColorShader, navyColor, 20U);
    testText.text.setPosition(10, textBackground.getPosition().y + 10);
    testText.text.setOutlineThickness(0.0f);

    std::vector<Effect *> effects;
    effects.push_back(new BananaParticles());

    for (auto effect : effects) {
        effect->load(levelRectf);
    }

    char buf[128] = { 0 };
    size_t len = 0;

    // Start the game loop
    sf::Clock clock;
    sf::Clock idleClock;
    bool quit = false;
    while (!quit) {

        // TODO: Frame rate timer stuff
        // Update the current example
        //float x = static_cast<float>(sf::Mouse::getPosition(window).x) / window.getSize().x;
        //float y = static_cast<float>(sf::Mouse::getPosition(window).y) / window.getSize().y;
        //for (std::size_t i = 0; i < drawables.size(); ++i)
        //    drawables[i]->update(clock.getElapsedTime().asSeconds(), x, y);

        // Handle events
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed: {
                    quit = true;
                    break;
                }
                case sf::Event::Resized: {
                    // ???
                    break;
                }
                case sf::Event::KeyPressed: {
                    switch (event.key.code) {
                        case sf::Keyboard::Escape: {
                            quit = true;
                            break;
                        }
                        case sf::Keyboard::F11: {
                            game.ToggleFullscreen();
                            break;
                        }
                        case sf::Keyboard::M: {
                            game.ToggleMouseCapture();
                            break;
                        }
#if _DEBUG
                        case sf::Keyboard::V: {
                            game.ToggleVSync();
                            break;
                        }
                        case sf::Keyboard::B: {
                            game.ToggleCameraBounded();
                            break;
                        }
                        case sf::Keyboard::G: {
                            game.TogglePlayerIsAGhost();
                            break;
                        }
                        case sf::Keyboard::F: {
                            game.ToggleCameraFollowing();
                            break;
                        }
#endif
                    }
                    break;
                }
                case sf::Event::MouseWheelScrolled: {
                    if (!game.IsCameraBounded()) {
                        camera.zoom(1.0f - event.mouseWheelScroll.delta * 0.1f);
                    }
                    break;
                }
                case sf::Event::MouseButtonPressed: {
                    switch (event.mouseButton.button) {
                        case sf::Mouse::Button::Middle: {
                            //resetCameraZoom = true;
                            break;
                        }
                    }
                    break;
                }
            }
        }

        //================================================================================
        // Player update
        //================================================================================
        sf::Vector2f playerMoveAccum;
        playerMoveAccum.x -= sf::Keyboard::isKeyPressed(sf::Keyboard::A); // Left
        playerMoveAccum.x += sf::Keyboard::isKeyPressed(sf::Keyboard::D); // Right
        playerMoveAccum.y -= sf::Keyboard::isKeyPressed(sf::Keyboard::W); // Up
        playerMoveAccum.y += sf::Keyboard::isKeyPressed(sf::Keyboard::S); // Down
        playerVelocity = Vector2f::Normalize(playerMoveAccum) * playerSpeed;
        player.move(playerVelocity);
        //if (!Vector2f::IsZero(playerVelocity)) {
        //    player.move(playerVelocity);
        //}

        // Constrain player to level bounds
        sf::FloatRect playerRect = player.shadedSprite->sprite.getGlobalBounds();
        sf::FloatRect playerIntersect;
        sf::Vector2f boundPlayerDelta;
        if (!game.IsPlayerAGhost()) {
            // NOTE: This will break if rect entirely outside of level, rather than intersection with edge
            if (levelRectf.intersects(playerRect, playerIntersect)) {
                boundPlayerDelta = sf::Vector2f(
                    (playerIntersect.left == 0.0f ? 1.0f : -1.0f) * (playerRect.width - playerIntersect.width),
                    (playerIntersect.top == 0.0f ? 1.0f : -1.0f) * (playerRect.height - playerIntersect.height)
                );
                if (!Vector2f::IsZero(boundPlayerDelta)) {
                    player.move(boundPlayerDelta);
                }
            } else {
                // NOTE: If player entirely outside of level, move to 0,0. Moving to closest edge might be better?
                player.shadedSprite->sprite.setPosition(0.0f, 0.0f);
            }
        }

        bool playerIdle = player.direction == Player::Direction::Idle;
        sf::SoundSource::Status whistleStatus = whistleMusic.getStatus();
        if (!playerIdle) {
            if (whistleStatus != sf::SoundSource::Status::Stopped) {
                whistleMusic.stop();
            }
            idleClock.restart();
        } else if (idleClock.getElapsedTime().asSeconds() > 10.0f) {
            if (whistleStatus != sf::SoundSource::Status::Playing) {
                whistleMusic.play();
            }
        }

        //================================================================================
        // Camera input/update
        //================================================================================
        const sf::Vector2f viewSize = window.getView().getSize();

        // TODO: Only when window size changes.. also something is broken when user manually resizes window
        // Update camera when window size changes
        sf::Vector2f cameraSize = camera.getSize();
        if (game.IsCameraBounded() && viewSize != cameraSize) {
            camera.setSize(viewSize);
            camera.setCenter(camera.getCenter() - ((cameraSize - viewSize) / 2.0f));
            cameraSize = camera.getSize();
        }

        if (game.IsCameraFollowing()) {
            sf::Vector2f cameraTarget(
                playerRect.left + playerRect.width / 2.0f,
                playerRect.top + playerRect.height / 2.0f
            );
            sf::Vector2f cameraCenter = camera.getCenter();
            camera.setCenter(cameraCenter + (cameraTarget - cameraCenter) * 0.1f);
        } else {
            sf::Vector2f cameraMoveAccum;
            cameraMoveAccum.x -= sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
            cameraMoveAccum.x += sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
            cameraMoveAccum.y -= sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
            cameraMoveAccum.y += sf::Keyboard::isKeyPressed(sf::Keyboard::Down);

            if (Vector2f::IsZero(cameraMoveAccum) && game.IsMouseCaptured()) {
                sf::Vector2f mousePos = sf::Vector2f(sf::Mouse::getPosition(window));
                const float edgeScrollSize = 20.0f;
                const sf::Vector2f windowSize = sf::Vector2f(window.getSize());

                bool scrollLeft  = mousePos.x < edgeScrollSize;
                bool scrollRight = mousePos.x > (windowSize.x - edgeScrollSize);
                bool scrollUp    = mousePos.y < edgeScrollSize;
                bool scrollDown  = mousePos.y > (windowSize.y - edgeScrollSize);

                cameraMoveAccum.x -= 1.0f * scrollLeft;
                cameraMoveAccum.x += 1.0f * scrollRight;
                cameraMoveAccum.y -= 1.0f * scrollUp;
                cameraMoveAccum.y += 1.0f * scrollDown;

                cameraVelocity = Vector2f::Normalize(cameraMoveAccum) * mouseCameraSpeed;
            } else {
                cameraVelocity = Vector2f::Normalize(cameraMoveAccum) * keyboardCameraSpeed;
            }

            camera.move(cameraVelocity);
        }

        // Constrain camera to level bounds
        sf::FloatRect cameraRect(camera.getCenter() - (cameraSize / 2.0f), cameraSize);
        sf::FloatRect cameraIntersect;
        sf::Vector2f boundCameraDelta;
        if (game.IsCameraBounded()) {
            if (levelRectf.intersects(cameraRect, cameraIntersect)) {
                boundCameraDelta = sf::Vector2f(
                    (cameraIntersect.left == 0.0f ? 1.0f : -1.0f) * (cameraRect.width - cameraIntersect.width),
                    (cameraIntersect.top == 0.0f ? 1.0f : -1.0f) * (cameraRect.height - cameraIntersect.height)
                );
                if (!Vector2f::IsZero(boundCameraDelta)) {
                    camera.move(boundCameraDelta);
                }
            } else {
                // NOTE: If camera entirely outside of level, move to 0,0. Moving to closest edge might be better?
                camera.setCenter(cameraSize / 2.0f);
            }
        }

        //================================================================================
        // Other stuff update
        //================================================================================

        // TODO: Check for player collision with BananaParticles
        //for (auto bananaIter = bananas.begin(); bananaIter != bananas.end();) {
        //    if (player.shadedSprite->sprite.getGlobalBounds().intersects(bananaIter->sprite.getGlobalBounds())) {
        //        bananaIter = bananas.erase(bananaIter);
        //        bananaSounds[bananaSoundIdx].play();
        //        bananaSoundIdx = (bananaSoundIdx + 1) % bananaSoundsCount;
        //    } else {
        //        ++bananaIter;
        //    }
        //}

        for (auto effect : effects) {
            effect->update(clock.getElapsedTime().asSeconds(), viewSize);
        }

        // TODO: Only when window changes size
        // Update textbox position
        textBackground.setPosition(0, (float)viewSize.y - textBackgroundTexture.getSize().y);
        textBackground.setScale(sf::Vector2f((float)viewSize.x / textBackgroundTexture.getSize().x, 1.0f));
        testText.text.setPosition(10, textBackground.getPosition().y + 10);

        // Clear the window
        window.clear(sf::Color(255, 128, 0));

        //================================================================================
        // Draw (world)
        //================================================================================
        window.setView(camera);

        grassSprite.draw(window);
        for (auto effect : effects) {
            effect->draw(window, sf::RenderStates::Default);
        }
        playerSprite.draw(window);

        //================================================================================
        // Draw (screen)
        //================================================================================
        window.setView(window.getDefaultView());

        window.draw(textBackground);
        testText.draw(window);

        float text_x = 10.0f;
        float text_y = 10.0f;
        const unsigned int font_h = 16U;

        //const sf::Vector2f cameraCenter = camera.getCenter();
        //len = snprintf(buf, sizeof(buf), "Camera x: %f, y: %f", cameraCenter.x, cameraCenter.y);
        //ShadedText cameraCenterText(text_x, text_y, std::string(buf, len), game.Font(), textureColorShader, sf::Color::White, font_h);
        //cameraCenterText.draw(window);
        //memset(buf, 0, sizeof(buf));
        //text_y += font_h;

        len = snprintf(buf, sizeof(buf), "Player (%.f x %.f) @ %.02f, %.02f", playerRect.width, playerRect.height, playerRect.left, playerRect.top);
        ShadedText playerRectText(text_x, text_y, std::string(buf, len), game.Font(), textureColorShader, sf::Color::White, font_h);
        playerRectText.draw(window);
        memset(buf, 0, sizeof(buf));
        text_y += font_h;

        len = snprintf(buf, sizeof(buf), "Player Intersect (%.f x %.f) @ %.02f, %.02f", playerIntersect.width, playerIntersect.height, playerIntersect.left, playerIntersect.top);
        ShadedText playerIntersectText(text_x, text_y, std::string(buf, len), game.Font(), textureColorShader, sf::Color::White, font_h);
        playerIntersectText.draw(window);
        memset(buf, 0, sizeof(buf));
        text_y += font_h;

        len = snprintf(buf, sizeof(buf), "Player Delta     %.02f, %.02f", boundPlayerDelta.x, boundPlayerDelta.y);
        ShadedText playerDeltaText(text_x, text_y, std::string(buf, len), game.Font(), textureColorShader, sf::Color::White, font_h);
        playerDeltaText.draw(window);
        memset(buf, 0, sizeof(buf));
        text_y += font_h;

        len = snprintf(buf, sizeof(buf), "Camera Intersect (%.f x %.f) @ %.02f, %.02f", cameraIntersect.width, cameraIntersect.height, cameraIntersect.left, cameraIntersect.top);
        ShadedText cameraIntersectText(text_x, text_y, std::string(buf, len), game.Font(), textureColorShader, sf::Color::White, font_h);
        cameraIntersectText.draw(window);
        memset(buf, 0, sizeof(buf));
        text_y += font_h;

        len = snprintf(buf, sizeof(buf), "Camera Delta     %.02f, %.02f", boundCameraDelta.x, boundCameraDelta.y);
        ShadedText cameraDeltaText(text_x, text_y, std::string(buf, len), game.Font(), textureColorShader, sf::Color::White, font_h);
        cameraDeltaText.draw(window);
        memset(buf, 0, sizeof(buf));
        text_y += font_h;

        // Finally, display the rendered frame on screen
        window.display();
    }

    window.close();
    return EXIT_SUCCESS;
}
