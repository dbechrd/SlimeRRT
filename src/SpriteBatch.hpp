#ifndef SPRITEBATCH_HPP
#define SPRITEBATCH_HPP

#include <SFML\Graphics.hpp>

// Credit to user "krzat" on the SFML forums for the rough draft of this class
// https://en.sfml-dev.org/forums/index.php?topic=10205#msg70258
class SpriteBatch
{
public:
	SpriteBatch(const size_t capacity = 16);
	~SpriteBatch(void);

	void display(bool reset = true, bool flush = true);
	void draw(const sf::Sprite &sprite);
	void draw(const sf::Texture *texture, const sf::Vector2f &position,
		const sf::IntRect &rec, const sf::Color &color, const sf::Vector2f &scale,
		const sf::Vector2f &origin, float rotation = 0);

	void draw(const sf::Texture *texture, const sf::FloatRect &dest, const sf::IntRect &rec, const sf::Color &color);

	void flush()
	{
		vertices.clear();
	}
	void setRenderStates(const sf::RenderStates &states)
	{
		display(false);
		this->state = states;
	}
	void setRenderTarget(sf::RenderTarget &render_target)
	{
		this->render_target = &render_target;
	}

private:
	sf::RenderTarget *render_target;
	sf::RenderStates state;
	std::vector<sf::Vertex> vertices;
};

#endif // SPRITEBATCH_HPP