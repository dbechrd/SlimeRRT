#include "Game.hpp"
#include "Globals.hpp"
#include "Effect.hpp"
#include "SpriteBatch.hpp"
#include "SoundBufferCatalog.hpp"
#include "SoundSystem.hpp"
#include "TextureCatalog.hpp"
#include "Vector2f.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <cassert>
#include <string>
#include <vector>
#include <cmath>
#include <iostream>
#include <unordered_map>

class ShadedSprite
{
public:
    ShadedSprite(const ShadedSprite &shadedSprite)
        : sprite(shadedSprite.sprite), shader(shadedSprite.shader) {}

    ShadedSprite(const sf::Texture &texture, const sf::Shader &shader)
        : sprite(texture), shader(&shader) {}

    ShadedSprite(const sf::Texture &texture, const sf::IntRect &rect, const sf::Shader &shader)
        : sprite(texture, rect), shader(&shader) {}

    void draw(sf::RenderTarget& target)
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
        shadedSprite->sprite.setTextureRect(sf::IntRect(54 * direction, 0, 54, 94));
    }
};

class BananaParticles : public Effect
{
public:

    BananaParticles() : Effect("banana_particles"), pointCloud(sf::Points, 10000) {}

    bool onLoad(const sf::IntRect &levelRect)
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

        // Only support evenly sized textures to prevent rounding/truncation problems
        assert(textureSize.x % 2 == 0);
        assert(textureSize.y % 2 == 0);

        sf::Vector2u halfTextureSize(textureSize.x / 2, textureSize.y / 2);

        // Move the points in the point cloud to random positions in the level
        std::uniform_real_distribution<> randomX(halfTextureSize.x, levelRect.width  - halfTextureSize.x);
        std::uniform_real_distribution<> randomY(halfTextureSize.y, levelRect.height - halfTextureSize.y);
        for (std::size_t i = 0; i < 1000; i++) {
            pointCloud[i].position.x = (float)randomX(g_mersenne);
            pointCloud[i].position.y = (float)randomY(g_mersenne);
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
        std::cerr << "Failed to init game" << std::endl;
        return EXIT_FAILURE;
    }
    sf::RenderWindow &window = game.Window();

    // Load shaders
    sf::Shader textureColorShader;
    if (!textureColorShader.loadFromFile("resources/texture_color.frag", sf::Shader::Fragment)) {
        return EXIT_FAILURE;
    }
    textureColorShader.setUniform("texture", sf::Shader::CurrentTexture);

    sf::Listener::setGlobalVolume(20.0f);

    // Load audio buffers
    sf::Music backgroundMusic;
    backgroundMusic.openFromFile("resources/fluquor_copyright.ogg");
    backgroundMusic.setLoop(true);
    backgroundMusic.setVolume(25.0f);
    backgroundMusic.play();

    sf::Music whistleMusic;
    whistleMusic.openFromFile("resources/whistle.ogg");
    whistleMusic.setLoop(true);

    // Create sprites
    // NOTE: If this is smaller than the resolution of the camera, then bounding breaks miserably
    sf::IntRect levelRect(0, 0, 4096, 4096);
    sf::FloatRect levelRectf(levelRect);

    ShadedSprite grassSprite(game.Texture(TexID::Grass), levelRect, textureColorShader);

    ShadedSprite playerSprite(game.Texture(TexID::Player), textureColorShader);
    Player player("Charlie", playerSprite);
    const float playerSpeed = 5.0f;
    sf::Vector2f playerVelocity;
    player.shadedSprite->sprite.setPosition(800.0f, 800.0f);

    size_t score = 0;

    const float keyboardCameraSpeed = 20.0f;
    const float mouseCameraSpeed = 30.0f;
    sf::Vector2f cameraVelocity;
    const float cameraSmoothingFactor = 0.05f;
    sf::View camera = sf::View(window.getDefaultView());

    sf::Sprite textBackground(game.Texture(TexID::TextBackground));
    textBackground.setColor(sf::Color(255, 255, 255, 255));

    sf::Color navyColor(80, 80, 80, 255);
    ShadedText testText(0.0f, 0.0f, "Ayples and Banaynays!", game.Font(), textureColorShader, navyColor, 20U);
    testText.text.setPosition(10, textBackground.getPosition().y + 10);
    testText.text.setOutlineThickness(0.0f);

    std::vector<Effect *> effects;
    effects.push_back(new BananaParticles);

    SpriteBatch appleSpriteBatch(32);
    appleSpriteBatch.setRenderTarget(window);
    std::vector<sf::Sprite> appleSprites;
    appleSprites.resize(500);
    std::uniform_real_distribution<> randomX(16, levelRect.width  - 16);
    std::uniform_real_distribution<> randomY(16, levelRect.height - 16);
    for (auto &appleSprite : appleSprites) {
        appleSprite.setPosition((float)randomX(g_mersenne), (float)randomY(g_mersenne));
        appleSprite.setTexture(game.Texture(TexID::Items));
        appleSprite.setTextureRect(sf::IntRect(0, 0, 32, 32));
    }

    for (auto effect : effects) {
        effect->load(levelRect);
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
#if _DEBUG
                case sf::Event::MouseWheelScrolled: {
                    if (!game.IsCameraBounded()) {
                        camera.zoom(1.0f - event.mouseWheelScroll.delta * 0.1f);
                    }
                    break;
                }
#endif
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

        sf::Vector2f pos = player.shadedSprite->sprite.getPosition();
        sf::Vector2f evenPosDelta(-(pos.x - floorf(pos.x)), -(pos.y - floorf(pos.y)));
        player.shadedSprite->sprite.setPosition(pos + evenPosDelta);

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
            camera.setCenter(cameraCenter + (cameraTarget - cameraCenter) * cameraSmoothingFactor);
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

        camera.setCenter(floorf(camera.getCenter().x), floorf(camera.getCenter().y));

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

        // TODO: Check for player collision with apples
        for (auto appleIter = appleSprites.begin(); appleIter != appleSprites.end();) {
            if (player.shadedSprite->sprite.getGlobalBounds().intersects(appleIter->getGlobalBounds())) {
                static std::string nomsSoundBuffers[] = {
                    SndID::OmNomNom1,
                    SndID::OmNomNom2,
                    SndID::OmNomNom3,
                };

                appleIter = appleSprites.erase(appleIter);
                std::uniform_int_distribution<> randomNoms(0, ARRAY_COUNT(nomsSoundBuffers) - 1);
                size_t randomNomIdx = (size_t)randomNoms(g_mersenne);
                game.PlaySound(nomsSoundBuffers[randomNomIdx]);
                score++;
            } else {
                ++appleIter;
            }
        }

        for (auto effect : effects) {
            effect->update(clock.getElapsedTime().asSeconds(), viewSize);
        }

        {
            // TODO: Only when window changes size
            // Update textbox position
            const sf::Texture &textBackgroundTexture = game.Texture(TexID::TextBackground);
            textBackground.setPosition(0, (float)viewSize.y - textBackgroundTexture.getSize().y);
            textBackground.setScale(sf::Vector2f((float)viewSize.x / textBackgroundTexture.getSize().x, 1.0f));
            testText.text.setPosition(10, textBackground.getPosition().y + 10);
        }

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
        for (auto appleSprite : appleSprites) {
            appleSpriteBatch.draw(appleSprite);
        }
        appleSpriteBatch.display();
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

        len = snprintf(buf, sizeof(buf), "Apples OmNomNom'd: %zu", score);
        ShadedText scoreText(text_x, text_y, std::string(buf, len), game.Font(), textureColorShader, sf::Color::White, font_h);
        scoreText.draw(window);
        memset(buf, 0, sizeof(buf));
        text_y += font_h;

#if _DEBUG
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
#endif

        // Finally, display the rendered frame on screen
        window.display();
    }

    window.close();

    for (auto effect : effects) {
        delete effect;
    }

    return EXIT_SUCCESS;
}
