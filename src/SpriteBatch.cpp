#include "SpriteBatch.hpp"
#include "Globals.hpp"
#include <cmath>

const int LookupSize = 512;

float getSin[LookupSize];
float getCos[LookupSize];
bool initialized = false;

void create_lookup()
{
	for (int i = 0; i < LookupSize; i++) {
		getSin[i] = static_cast<float>(sin(i * PI / LookupSize * 2));
		getCos[i] = static_cast<float>(cos(i * PI / LookupSize * 2));
	}
	initialized = true;
}

SpriteBatch::SpriteBatch(const size_t capacity)
{
	vertices.reserve(capacity * 4);

	if (!initialized) {
		create_lookup();
	}
}

SpriteBatch::~SpriteBatch(void)
{
}

void SpriteBatch::draw(const sf::Sprite &sprite)
{
	draw(sprite.getTexture(), sprite.getPosition(), sprite.getTextureRect(), sprite.getColor(), sprite.getScale(), sprite.getOrigin(), sprite.getRotation());
}

void SpriteBatch::display(bool reset, bool flush)
{
	const size_t count = vertices.size();
	rt->draw(&vertices[0], count, sf::PrimitiveType::Quads, state);
	if (flush) vertices.clear();
	if (reset) state = sf::RenderStates::Default;
}

void SpriteBatch::draw(
	const sf::Texture *texture, const sf::Vector2f &position,
	const sf::IntRect &rec, const sf::Color &color, const sf::Vector2f &scale,
	const sf::Vector2f &origin, float rotation)
{
	if (texture != state.texture) {
		display(false);
		state.texture = texture;
	}

	int rot = static_cast<int>(rotation / 360 * LookupSize + 0.5) & (LookupSize -1);
	float _sin = getSin[rot];
	float _cos = getCos[rot];

	//float _sin = sinf(rotation);
	//float _cos = cosf(rotation);

	auto scalex = rec.width * scale.x;
	auto scaley = rec.height * scale.y;

	auto pX = -origin.x * scale.x;
	auto pY = -origin.y * scale.y;

	vertices.resize(vertices.size() + 4);
	sf::Vertex *ptr = &vertices.end()[-4];

	ptr->position.x = pX * _cos - pY * _sin + position.x;
	ptr->position.y = pX * _sin + pY * _cos + position.y;
	ptr->texCoords.x = static_cast<float>(rec.left);
	ptr->texCoords.y = static_cast<float>(rec.top);
	ptr->color = color;
	ptr++;

	pX += scalex;
	ptr->position.x = pX * _cos - pY * _sin + position.x;
	ptr->position.y = pX * _sin + pY * _cos + position.y;
	ptr->texCoords.x = static_cast<float>(rec.left + rec.width);
	ptr->texCoords.y = static_cast<float>(rec.top);
	ptr->color = color;
	ptr++;

	pY += scaley;
	ptr->position.x = pX * _cos - pY * _sin + position.x;
	ptr->position.y = pX * _sin + pY * _cos + position.y;
	ptr->texCoords.x = static_cast<float>(rec.left + rec.width);
	ptr->texCoords.y = static_cast<float>(rec.top + rec.height);
	ptr->color = color;
	ptr++;

	pX -= scalex;
	ptr->position.x = pX * _cos - pY * _sin + position.x;
	ptr->position.y = pX * _sin + pY * _cos + position.y;
	ptr->texCoords.x = static_cast<float>(rec.left);
	ptr->texCoords.y = static_cast<float>(rec.top + rec.height);
	ptr->color = color;
}

void SpriteBatch::draw(const sf::Texture *texture, const sf::FloatRect &dest, const sf::IntRect &rec, const sf::Color &color)
{
	draw(texture, sf::Vector2f(dest.left, dest.top), rec, color, sf::Vector2f(1, 1), sf::Vector2f(0, 0), 0);
}