#ifndef EFFECT_HPP
#define EFFECT_HPP

#include <SFML/Graphics.hpp>
#include <cassert>
#include <string>

class Effect : public sf::Drawable
{
public:
    virtual ~Effect() {}

    const std::string& getName() const
    {
        return name;
    }

    void load(const sf::FloatRect &levelRect)
    {
        isLoaded = onLoad(levelRect);
    }

    void update(float time, const sf::Vector2f &viewSize)
    {
        if (isLoaded) {
            onUpdate(time, viewSize);
        }
    }

    void draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        if (isLoaded) {
            onDraw(target, states);
        }
    }

protected:
    Effect(const std::string& name) : name(name), isLoaded(false) {}

private:
    virtual bool onLoad(const sf::FloatRect &levelRect) = 0;
    virtual void onUpdate(float time, const sf::Vector2f &viewSize) = 0;
    virtual void onDraw(sf::RenderTarget& target, sf::RenderStates states) const = 0;

private:
    std::string name;
    bool isLoaded;
};

#endif // EFFECT_HPP
