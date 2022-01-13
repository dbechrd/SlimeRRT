#include "player.h"
#include "controller.h"
#include "draw_command.h"
#include "healthbar.h"
#include "catalog/items.h"
#include "maths.h"
#include "shadow.h"
#include "sprite.h"
#include "spritesheet.h"
#include <cassert>

void Player::Init(const SpriteDef *spriteDef)
{
    assert(!sprite.spriteDef);
    printf("Init player\n");

    combat.hitPoints = 100.0f;
    combat.hitPointsMax = 100.0f;
    combat.meleeDamage = 1.0f;

    sprite.scale = 1.0f;
    sprite.spriteDef = spriteDef;

    // TODO: Load selected slot from save file / server
    inventory.selectedSlot = PlayerInventorySlot::Slot_1;

    // TODO: Load inventory from save file / server
    inventory.slots[(int)PlayerInventorySlot::Slot_1] = { Catalog::ItemID::Weapon_Sword ,  1 };
    inventory.slots[(int)PlayerInventorySlot::Coins ] = { Catalog::ItemID::Currency_Coin,  0 };

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

Vector3 Player::GetAttachPoint(AttachPoint attachPoint) const
{
    Vector3 attach = {};
    switch (attachPoint) {
        case AttachPoint::Gut: {
            Vector3 playerC = sprite_world_center(sprite, body.position, sprite.scale);
            attach = v3_add(playerC, { 0.0f, 0.0f, -10.0f });
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

    body.position.x += offset.x;
    body.position.y += offset.y;
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
        body.lastMoved = glfwGetTime();
        combat.attackStartedAt = glfwGetTime();
        combat.attackDuration = 0.2;

        const ItemStack &selectedStack = GetSelectedItem();
        switch (selectedStack.id) {
            case Catalog::ItemID::Weapon_Sword: {
                stats.timesSwordSwung++;
                break;
            }
            default: {
                stats.timesFistSwung++;
                break;
            }
        }

        return true;
    }
    return false;
}

void Player::Update(double dt, InputSample &input, const Tilemap &map)
{
    assert(id);
    assert(input.ownerId == id);

    if (input.selectSlot) {
        inventory.selectedSlot = (PlayerInventorySlot)input.selectSlot;
    }

    float playerSpeed = 4.0f;
    Vector2 move{};

    if (input.walkNorth || input.walkEast || input.walkSouth || input.walkWest) {
        if (input.walkNorth) {
            move.y -= 1.0f;
        }
        if (input.walkEast) {
            move.x += 1.0f;
        }
        if (input.walkSouth) {
            move.y += 1.0f;
        }
        if (input.walkWest) {
            move.x -= 1.0f;
        }
        if (input.run) {
            moveState = Player::MoveState::Running;
            playerSpeed += 2.0f;
        } else {
            moveState = Player::MoveState::Walking;
        }
    } else {
        moveState = Player::MoveState::Idle;
    }

    Vector2 moveOffset = v2_scale(v2_normalize(move), METERS_TO_PIXELS(playerSpeed) * (float)dt);
    moveBuffer = v2_add(moveBuffer, moveOffset);

    if (!input.skipFx && input.attack && Attack()) {
        Catalog::g_sounds.Play(Catalog::SoundID::Whoosh, 1.0f + dlb_rand32f_variance(0.1f));
    }

    if (!v2_is_zero(moveBuffer)) {
        const Vector2 curPos = body.GroundPosition();
        const Tile *curTile = map.TileAtWorldTry(curPos.x, curPos.y, 0, 0);
        const bool curWalkable = curTile && curTile->IsWalkable();

        Vector2 newPos = v2_add(curPos, moveBuffer);
        Tile *newTile = map.TileAtWorldTry(newPos.x, newPos.y, 0, 0);

        // NOTE: This extra logic allows the player to slide when attempting to move diagonally against a wall
        // NOTE: If current tile isn't walkable, allow player to walk off it. This may not be the best solution
        // if the player can accidentally end up on unwalkable tiles through gameplay, but currently the only
        // way to end up on an unwalkable tile is to spawn there.
        // TODO: We should fix spawning to ensure player spawns on walkable tile (can probably just manually
        // generate something interesting in the center of the world that overwrites procgen, like Don't
        // Starve's fancy arrival portal.
        if (curWalkable) {
            if (!newTile || !newTile->IsWalkable()) {
                // XY unwalkable, try only X offset
                newPos = curPos;
                newPos.x += moveBuffer.x;
                newTile = map.TileAtWorldTry(newPos.x, newPos.y, 0, 0);
                if (!newTile || !newTile->IsWalkable()) {
                    // X unwalkable, try only Y offset
                    newPos = curPos;
                    newPos.y += moveBuffer.y;
                    newTile = map.TileAtWorldTry(newPos.x, newPos.y, 0, 0);
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
            static double lastFootstep = 0;
            double timeSinceLastFootstep = glfwGetTime() - lastFootstep;
            float distanceMoved = v2_length(moveBuffer);
            if (!input.skipFx && timeSinceLastFootstep > 1.0f / distanceMoved) {
                Catalog::g_sounds.Play(Catalog::SoundID::Footstep, 1.0f + dlb_rand32f_variance(0.5f));
                lastFootstep = glfwGetTime();
            }
        }

        if (body.position.x < 0.0f) {
            assert(!"wtf?");
        }

        moveBuffer = {};
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
        if (selectedStack.id == Catalog::ItemID::Weapon_Sword) {
            switch (actionState) {
                case ActionState::None: {
                    switch (body.idle) {
                        // TODO: sprite_by_name("player_sword");
                        case false: sprite.spriteDef = &sheet->sprites[2]; break;
                        // TODO: sprite_by_name("player_sword_idle");
                        case true:  sprite.spriteDef = &sheet->sprites[3]; break;
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
            switch (body.idle) {
                // TODO: sprite_by_name("player_melee");
                case false: sprite.spriteDef = &sheet->sprites[0]; break;
                // TODO: sprite_by_name("player_melee_idle");
                case true:  sprite.spriteDef = &sheet->sprites[1]; break;
            }
        }
    }

    body.Update(dt);
    sprite_update(sprite, dt);

    if (!input.skipFx && body.idleChanged) {
        // TODO: PushEvent(BODY_IDLE_CHANGED, body.idle);

        //fadeTo(music, target, speed = 1.0) { music.targetVol = float }
        //fadeIn(music, speed = 1.0) { fade(Music, 1.0, speed) }
        //fadeOut(music, speed = 1.0) { fadeTo(Music, 0.0, speed) }

        // TODO: HandleEvent, body.idle ? (fadeIn(idleMusic), fadeTo(bgMusic, 0.1)) : (fadeOut(idleMusic), fadeIn(bgMusic))
    }

    // Skip sounds/particles etc. next time this input is used (e.g. during reconciliation)
    input.skipFx = true;
}

float Player::Depth(void) const
{
    return body.position.y;
}

bool Player::Cull(const Rectangle &cullRect) const
{
    bool cull = sprite_cull_body(sprite, body, cullRect);
    return cull;
}

void Player::Draw(void) const
{
    // Player shadow
    // TODO: Shadow size based on height from ground
    // https://yal.cc/top-down-bouncing-loot-effects/
    //const float shadowScale = 1.0f + slime->transform.position.z / 20.0f;
    const Vector2 playerGroundPos = body.GroundPosition();
    Shadow::Draw((int)playerGroundPos.x, (int)playerGroundPos.y, 16.0f, -6.0f);

    sprite_draw_body(sprite, body, WHITE);
    HealthBar::Draw(10, sprite, body, name, combat.hitPoints, combat.hitPointsMax);
}