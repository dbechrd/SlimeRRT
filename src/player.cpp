#include "player.h"
#include "catalog/items.h"
#include "controller.h"
#include "draw_command.h"
#include "healthbar.h"
#include "helpers.h"
#include "maths.h"
#include "shadow.h"
#include "sprite.h"
#include "spritesheet.h"
#include <cassert>

void Player::Init(const SpriteDef *spriteDef)
{
    assert(!sprite.spriteDef);
    printf("Init player\n");

    combat.level = 1;
    combat.hitPoints = 100.0f;
    combat.hitPointsMax = 100.0f;
    combat.meleeDamage = 1.0f;

    sprite.scale = 1.0f;
    sprite.spriteDef = spriteDef;

    // TODO: Load selected slot from save file / server
    inventory.selectedSlot = PlayerInventorySlot::Hotbar_1;

    // TODO: Load inventory from save file / server
    inventory.slots[(int)PlayerInventorySlot::Hotbar_1] = { Catalog::ItemID::Weapon_Long_Sword , 1 };
    inventory.slots[(int)PlayerInventorySlot::Coin_Copper] = { Catalog::ItemID::Currency_Copper, 0 };
    inventory.slots[(int)PlayerInventorySlot::Coin_Silver] = { Catalog::ItemID::Currency_Silver, 0 };
    inventory.slots[(int)PlayerInventorySlot::Coin_Gilded] = { Catalog::ItemID::Currency_Gilded, 0 };

    // TODO: Load stats from save file / server
    //stats.coinsCollected = 33;
    //stats.slimesSlain = 44;
    stats = {};
}

void Player::SetName(const char *playerName, uint32_t playerNameLength)
{
    nameLength = MIN(playerNameLength, USERNAME_LENGTH_MAX);
    memcpy(name, playerName, nameLength);
}

Vector3 Player::WorldCenter(void) const
{
    Vector3 worldC = v3_add(body.WorldPosition(), sprite_center(sprite));
    return worldC;
}

Vector3 Player::WorldTopCenter(void) const
{
    Vector3 worldTopC = v3_add(body.WorldPosition(), sprite_top_center(sprite));
    return worldTopC;
}

Vector3 Player::GetAttachPoint(AttachPoint attachPoint) const
{
    Vector3 attach = {};
    switch (attachPoint) {
        case AttachPoint::Gut: {
            attach = v3_add(WorldCenter(), { 0.0f, 0.0f, -16.0f });
            break;
        } default: {
            assert(!"That's not a valid attachment point identifier");
            break;
        }
    }
    return attach;
}

const ItemStack &Player::GetSelectedItem() const
{
    const ItemStack &selectedStack = inventory.slots[(size_t)inventory.selectedSlot];
    return selectedStack;
}

void Player::UpdateDirection(Vector2 offset)
{
    // NOTE: Branching could be removed by putting the sprites in a more logical order.. doesn't matter if this
    // only applies to players since there would be so few.
    Direction prevDirection = sprite.direction;
    if (offset.x > 0.0f) {
        if (offset.y > 0.0f) {
            sprite.direction = Direction::SouthEast;
        } else if (offset.y < 0.0f) {
            sprite.direction = Direction::NorthEast;
        } else {
            sprite.direction = Direction::East;
        }
    } else if (offset.x < 0.0f) {
        if (offset.y > 0.0f) {
            sprite.direction = Direction::SouthWest;
        } else if (offset.y < 0.0f) {
            sprite.direction = Direction::NorthWest;
        } else {
            sprite.direction = Direction::West;
        }
    } else {
        if (offset.y > 0.0f) {
            sprite.direction = Direction::South;
        } else if (offset.y < 0.0f) {
            sprite.direction = Direction::North;
        }
    }
    if (sprite.direction != prevDirection) {
        sprite.animFrameIdx = 0;
    }
}

bool Player::Move(Vector2 offset)
{
    if (v2_is_zero(offset))
        return false;

    // Don't allow player to move if they're not touching the ground (currently no jump/fall, so just assert)
    //assert(body.position.z == 0.0f);

    body.Move(offset);
    UpdateDirection(offset);

    const float pixelsMoved = v2_length(offset);
    const float metersMoved = PIXELS_TO_METERS(pixelsMoved);
    stats.kmWalked += metersMoved / 1000.0f;
    return true;
}

bool Player::Attack(void)
{
    if (actionState == ActionState::None) {
        actionState = ActionState::Attacking;
        body.Move({});  // update lastMoved to stop idle animation
        combat.attackStartedAt = glfwGetTime();
        combat.attackDuration = 0.2;

        const ItemStack &selectedStack = GetSelectedItem();
        switch (selectedStack.id) {
            case Catalog::ItemID::Weapon_Long_Sword: {
                stats.timesSwordSwung++;
                break;
            } default: {
                stats.timesFistSwung++;
                break;
            }
        }

        return true;
    }
    return false;
}

void Player::Update(InputSample &input, const Tilemap &map)
{
    assert(id);
    assert(input.ownerId == id);

    float dt = input.msec / 1000.0f;

    if (input.selectSlot) {
        inventory.selectedSlot = (PlayerInventorySlot)input.selectSlot;
    }

    if (combat.hitPoints) {
        // TODO(cleanup): jump
        //if (body.OnGround() && input.dbgJump) {
        //    body.ApplyForce({ 0, 0, METERS_TO_PIXELS(4.0f) });
        //}

        float playerSpeed = 3.0f;
        Vector2 move{};

        if (input.walkNorth || input.walkEast || input.walkSouth || input.walkWest) {
            move.y -= 1.0f * input.walkNorth;
            move.x += 1.0f * input.walkEast;
            move.y += 1.0f * input.walkSouth;
            move.x -= 1.0f * input.walkWest;
            if (input.run) {
                moveState = Player::MoveState::Running;
                playerSpeed += 1.0f;
            } else {
                moveState = Player::MoveState::Walking;
            }
        } else {
            moveState = Player::MoveState::Idle;
        }

        const Vector2 pos = body.GroundPosition();
        const Tile *tile = map.TileAtWorld(pos.x, pos.y);
        if (tile && tile->type == Tile::Type::Water) {
            playerSpeed *= 0.5f;
            // TODO: moveState = Player::MoveState::Swimming;
        }

        Vector2 moveOffset = v2_scale(v2_normalize(move), METERS_TO_PIXELS(playerSpeed) * dt);
        moveBuffer = v2_add(moveBuffer, moveOffset);

        if (!input.skipFx && input.attack && Attack()) {
            Catalog::g_sounds.Play(Catalog::SoundID::Whoosh, 1.0f + dlb_rand32f_variance(0.1f));
        }

        if (!v2_is_zero(moveBuffer)) {

            const bool walkable = tile && tile->IsWalkable();

            Vector2 newPos = v2_add(pos, moveBuffer);
            const Tile *newTile = map.TileAtWorld(newPos.x, newPos.y);

            // NOTE: This extra logic allows the player to slide when attempting to move diagonally against a wall
            // NOTE: If current tile isn't walkable, allow player to walk off it. This may not be the best solution
            // if the player can accidentally end up on unwalkable tiles through gameplay, but currently the only
            // way to end up on an unwalkable tile is to spawn there.
            // TODO: We should fix spawning to ensure player spawns on walkable tile (can probably just manually
            // generate something interesting in the center of the world that overwrites procgen, like Don't
            // Starve's fancy arrival portal.
            if (walkable) {
                if (!newTile || !newTile->IsWalkable()) {
                    // XY unwalkable, try only X offset
                    newPos = pos;
                    newPos.x += moveBuffer.x;
                    newTile = map.TileAtWorld(newPos.x, newPos.y);
                    if (!newTile || !newTile->IsWalkable()) {
                        // X unwalkable, try only Y offset
                        newPos = pos;
                        newPos.y += moveBuffer.y;
                        newTile = map.TileAtWorld(newPos.x, newPos.y);
                        if (!newTile || !newTile->IsWalkable()) {
                            // XY, and both slide directions are all unwalkable
                            moveBuffer.x = 0.0f;
                            moveBuffer.y = 0.0f;

                            // TODO: Play wall bonk sound (or splash for water? heh)
                            // TODO: Maybe bounce the player against the wall? This code doesn't do that nicely..
                            //player_move(&charlie, v2_scale(v2_negate(moveBuffer), 10.0f));
                        } else {
                            // Y offset is walkable
                            moveBuffer.x = 0.0f;
                        }
                    } else {
                        // X offset is walkable
                        moveBuffer.y = 0.0f;
                    }
                }
            }

            if (Move(moveBuffer)) {
                thread_local double lastFootstep = 0;
                double timeSinceLastFootstep = glfwGetTime() - lastFootstep;
                float distanceMoved = v2_length(moveBuffer);
                if (!input.skipFx && timeSinceLastFootstep > 1.0f / distanceMoved) {
                    Catalog::g_sounds.Play(Catalog::SoundID::Footstep, 1.0f + dlb_rand32f_variance(0.5f));
                    lastFootstep = glfwGetTime();
                }
            }

            moveBuffer = {};
        }
    }

    switch (actionState) {
        case ActionState::Attacking: {
            const double timeSinceAttackStarted = glfwGetTime() - combat.attackStartedAt;
            if (timeSinceAttackStarted > combat.attackDuration) {
                actionState = ActionState::None;
                combat.attackStartedAt = 0;
                combat.attackDuration = 0;
                combat.attackFrame = 0;
            } else {
                combat.attackFrame++;
            }
            break;
        }
    }

    if (sprite.spriteDef) {
        // TODO: Less hard-coded way to look up player sprite based on selected item id
        const Spritesheet *sheet = sprite.spriteDef->spritesheet;
        assert(sheet->sprites.size() == 5);

        const ItemStack &selectedStack = GetSelectedItem();
        if (selectedStack.id == Catalog::ItemID::Weapon_Long_Sword) {
            switch (actionState) {
                case ActionState::None: {
                    switch ((int)body.Idle()) {
                        // TODO: sprite_by_name("player_sword");
                        case 0: sprite.spriteDef = &sheet->sprites[2]; break;
                        // TODO: sprite_by_name("player_sword_idle");
                        case 1: sprite.spriteDef = &sheet->sprites[3]; break;
                    }
                    break;
                }
                case ActionState::Attacking: {
                    // sprite_by_name("player_sword_attack");
                    if (combat.attackFrame < 10) {
                        sprite.spriteDef = &sheet->sprites[4];
                    } else {
                        sprite.spriteDef = &sheet->sprites[2];
                    }
                    break;
                }
            }
        } else {
            switch ((int)body.Idle()) {
                // TODO: sprite_by_name("player_melee");
                case 0: sprite.spriteDef = &sheet->sprites[0]; break;
                // TODO: sprite_by_name("player_melee_idle");
                case 1: sprite.spriteDef = &sheet->sprites[1]; break;
            }
        }
    }

    body.Update(dt);
    sprite_update(sprite, dt);

#if 0
    // TODO: PushEvent(BODY_IDLE_CHANGED, body.idle);
    if (!input.skipFx && body.idleChanged) {
        fadeTo(music, target, speed = 1.0) { music.targetVol = float }
        fadeIn(music, speed = 1.0) { fade(Music, 1.0, speed) }
        fadeOut(music, speed = 1.0) { fadeTo(Music, 0.0, speed) }
        // TODO: HandleEvent, body.idle ? (fadeIn(idleMusic), fadeTo(bgMusic, 0.1)) : (fadeOut(idleMusic), fadeIn(bgMusic))
    }
#endif

    // Skip sounds/particles etc. next time this input is used (e.g. during reconciliation)
    input.skipFx = true;
}

float Player::Depth(void) const
{
    return body.GroundPosition().y;
}

bool Player::Cull(const Rectangle &cullRect) const
{
    bool cull = sprite_cull_body(sprite, body, cullRect);
    return cull;
}

void Player::DrawSwimOverlay(const World &world) const
{
    Vector2 groundPos = body.GroundPosition();
    const Tile *tileLeft = world.map->TileAtWorld(groundPos.x - 15.0f, groundPos.y);
    const Tile *tileRight = world.map->TileAtWorld(groundPos.x + 15.0f, groundPos.y);
    if (tileLeft && tileLeft->type == Tile::Type::Water ||
        tileRight && tileRight->type == Tile::Type::Water) {
        auto tilesetId = world.map->tilesetId;
        Tileset &tileset = g_tilesets[(size_t)tilesetId];
        Rectangle tileRect = tileset_tile_rect(tilesetId, Tile::Type::Water);
        assert(tileRect.width == TILE_W);
        assert(tileRect.height == TILE_W);

        Vector3 gut3d = GetAttachPoint(Player::AttachPoint::Gut);
        Vector2 gut2d = { gut3d.x, gut3d.y - gut3d.z + 10.0f };
        float offsetX = fmodf(gut2d.x, TILE_W);
        float offsetY = fmodf(gut2d.y, TILE_W);
        if (offsetX < 0) offsetX += TILE_W;
        if (offsetY < 0) offsetY += TILE_W;

        Rectangle dstTopLeft{
            gut2d.x - offsetX - ((offsetX < (TILE_W / 2)) ? TILE_W : 0),
            gut2d.y,
            TILE_W,
            TILE_W - offsetY
        };

        Rectangle dstTopRight = dstTopLeft;
        dstTopRight.x += TILE_W;

        Rectangle srcTop = tileRect;
        srcTop.y += offsetY;
        srcTop.height -= offsetY;

        Rectangle dstBotLeft{
            gut2d.x - offsetX - ((offsetX < (TILE_W / 2)) ? TILE_W : 0),
            gut2d.y - offsetY + TILE_W,
            TILE_W,
            TILE_W
        };

        Rectangle dstBotRight = dstBotLeft;
        dstBotRight.x += TILE_W;

        float minXWater = FLT_MAX;
        float maxXWater = FLT_MIN;

#define CHECK_AND_DRAW(src, dst) \
            tile = world.map->TileAtWorld((dst).x, (dst).y);                                  \
            if (tile && tile->type == Tile::Type::Water) {                                   \
                DrawTexturePro(tileset.texture, (src), (dst), { 0, 0 }, 0, Fade(WHITE, 0.8f)); \
                minXWater = MIN(minXWater, (dst).x);                                           \
                maxXWater = MAX(maxXWater, (dst).x + (dst).width);                             \
            }

        const Tile *tile = 0;
        CHECK_AND_DRAW(srcTop, dstTopLeft);
        CHECK_AND_DRAW(srcTop, dstTopRight);
        CHECK_AND_DRAW(tileRect, dstBotLeft);
        CHECK_AND_DRAW(tileRect, dstBotRight);

#undef CHECK_AND_DRAW

        const Tile *playerGutTile = world.map->TileAtWorld(gut2d.x, gut2d.y);
        if (playerGutTile && playerGutTile->type == Tile::Type::Water) {
            Rectangle bubblesDstTopMid{
                gut2d.x - 20.0f,
                gut2d.y,
                40.0f,
                8.0f
            };

            Rectangle bubblesSrcTop = tileRect;
            bubblesSrcTop.y += offsetY;
            bubblesSrcTop.height -= offsetY;

            //DrawCircleV({ minXWater, playerGut2D.y }, 2.0f, PINK);
            //DrawCircleV({ maxXWater, playerGut2D.y }, 2.0f, PINK);

            const double now = glfwGetTime();
            const float radiusDelta = (moveState != Player::MoveState::Idle) ? (sinf((float)now * 8) * 3) : 0.0f;
            const float radius = 20.0f + radiusDelta;
            DrawEllipse((int)gut2d.x, (int)gut2d.y, radius, radius * 0.5f, Fade(SKYBLUE, 0.4f));
        }
    }
}

void Player::Draw(const World &world) const
{
    // Player shadow
    // TODO: Shadow size based on height from ground
    // https://yal.cc/top-down-bouncing-loot-effects/
    //const float shadowScale = 1.0f + slime->transform.position.z / 20.0f;
    Vector2 playerGroundPos = body.GroundPosition();
    Shadow::Draw((int)playerGroundPos.x, (int)playerGroundPos.y, 16.0f, -6.0f);

    sprite_draw_body(sprite, body, combat.hitPoints ? WHITE : GRAY);

    DrawSwimOverlay(world);

#if CL_DEMO_SNAPSHOT_RADII
    {
        const Vector2 visPos = body.VisualPosition();
        DrawCircleLines((int)visPos.x, (int)visPos.y, SV_ENEMY_NEARBY_THRESHOLD, GREEN);
        DrawCircleLines((int)visPos.x, (int)visPos.y, CL_ENEMY_FARAWAY_THRESHOLD, RED);
    }
#endif

#if CL_DEMO_SPAWN_RADII
    {
        const Vector2 visPos = body.VisualPosition();
        DrawCircleLines((int)visPos.x, (int)visPos.y, SV_ENEMY_DESPAWN_RADIUS, GREEN);
        DrawCircleLines((int)visPos.x, (int)visPos.y, SV_ENEMY_MIN_SPAWN_DIST, RED);
    }
#endif

    Vector3 topCenter = WorldTopCenter();
    HealthBar::Draw({ topCenter.x, topCenter.y - topCenter.z }, name, combat);
}