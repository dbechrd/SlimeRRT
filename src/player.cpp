#include "player.h"
#include "controller.h"
#include "draw_command.h"
#include "healthbar.h"
#include "item_catalog.h"
#include "maths.h"
#include "shadow.h"
#include "sprite.h"
#include "spritesheet.h"
#include <cassert>

void Player::Init()
{
    assert(!sprite.spriteDef);
    printf("Init player\n");

    combat.hitPoints = 100.0f;
    combat.hitPointsMax = 100.0f;
    combat.meleeDamage = 1.0f;

    sprite.scale = 1.0f;
    const Spritesheet &spritesheet = SpritesheetCatalog::spritesheets[(int)SpritesheetID::Charlie];
    const SpriteDef *spriteDef = spritesheet.FindSprite("player_sword");
    if (spriteDef) {
        sprite.spriteDef = spriteDef;
    }

    // TODO: Load selected slot from save file / server
    inventory.selectedSlot = PlayerInventorySlot::Slot_1;

    // TODO: Load inventory from save file / server
    inventory.slots[(int)PlayerInventorySlot::Slot_1] = { ItemID::Weapon_Sword ,  1 };
    inventory.slots[(int)PlayerInventorySlot::Coins ] = { ItemID::Currency_Coin,  0 };

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

const Item &Player::GetSelectedItem() const
{
    const ItemStack &selectedStack = inventory.slots[(int)inventory.selectedSlot];
    const Item &selectedItem = ItemCatalog::instance.FindById(selectedStack.id);
    return selectedItem;
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

bool Player::Move(double now, double dt, Vector2 offset)
{
    UNUSED(now);
    UNUSED(dt);  // offset is already in time slice space

    if (v2_is_zero(offset))
        return false;

    // Don't allow player to move if they're not touching the ground (currently no jump/fall, so just assert)
    assert(body.position.z == 0.0f);

    body.position.x += offset.x;
    body.position.y += offset.y;
    UpdateDirection(offset);

    const float pixelsMoved = v2_length(offset);
    const float metersMoved = PIXELS_TO_METERS(pixelsMoved);
    stats.kmWalked += metersMoved / 1000.0f;
    return true;
}

bool Player::Attack(double now, double dt)
{
    UNUSED(dt);

    if (actionState == ActionState::None) {
        actionState = ActionState::Attacking;
        //body.lastUpdated = now;
        body.lastMoved = now;
        combat.attackStartedAt = now;
        combat.attackDuration = 0.1;

        const Item &selectedItem = GetSelectedItem();
        switch (selectedItem.id) {
            case ItemID::Weapon_Sword: {
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

void Player::ProcessInput(InputSnapshot &input, const Tilemap &map)
{
    assert(id);
    assert(input.ownerId == id);

    if (input.selectSlot) {
        inventory.selectedSlot = (PlayerInventorySlot)input.selectSlot;
    }

    float playerSpeed = 4.0f;
    Vector2 moveBuffer = {};

    if (input.walkNorth || input.walkEast || input.walkSouth || input.walkWest) {
        if (input.walkNorth) {
            moveBuffer.y -= 1.0f;
        }
        if (input.walkEast) {
            moveBuffer.x += 1.0f;
        }
        if (input.walkSouth) {
            moveBuffer.y += 1.0f;
        }
        if (input.walkWest) {
            moveBuffer.x -= 1.0f;
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

    Vector2 moveOffset = v2_scale(v2_normalize(moveBuffer), METERS_TO_PIXELS(playerSpeed) * (float)input.frameDt);
    if (!v2_is_zero(moveOffset)) {
        const Vector2 curPos = body.GroundPosition();
        const Tile *curTile = map.TileAtWorldTry(curPos.x, curPos.y, 0, 0);
        const bool curWalkable = curTile && curTile->IsWalkable();

        Vector2 newPos = v2_add(curPos, moveOffset);
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
                newPos.x += moveOffset.x;
                newTile = map.TileAtWorldTry(newPos.x, newPos.y, 0, 0);
                if (!newTile || !newTile->IsWalkable()) {
                    // X unwalkable, try only Y offset
                    newPos = curPos;
                    newPos.y += moveOffset.y;
                    newTile = map.TileAtWorldTry(newPos.x, newPos.y, 0, 0);
                    if (!newTile || !newTile->IsWalkable()) {
                        // XY, and both slide directions are all unwalkable
                        moveOffset.x = 0.0f;
                        moveOffset.y = 0.0f;

                        // TODO: Play wall bonk sound (or splash for water? heh)
                        // TODO: Maybe bounce the player against the wall? This code doesn't do that nicely..
                        //player_move(&charlie, v2_scale(v2_negate(moveOffset), 10.0f));
                    } else {
                        // Y offset is walkable
                        moveOffset.x = 0.0f;
                    }
                } else {
                    // X offset is walkable
                    moveOffset.y = 0.0f;
                }
            }
        }

        if (Move(input.frameTime, input.frameDt, moveOffset)) {
            static double lastFootstep = 0;
            double timeSinceLastFootstep = input.frameTime - lastFootstep;
            if (timeSinceLastFootstep > 1.0f / playerSpeed) {
                sound_catalog_play(SoundID::Footstep, 1.0f + dlb_rand32f_variance(0.5f));
                lastFootstep = input.frameTime;
            }
        }

        if (body.position.x < 0.0f) {
            assert(!"wtf?");
        }
    }

    if (input.attack) {
        Attack(input.frameTime, input.frameDt);
    }

    // Skip sounds/particles etc. next time this input is used (e.g. during reconciliation)
    input.skipFx = true;
}

void Player::Update(double now, double dt)
{
    assert(id);

    const double timeSinceAttackStarted = now - combat.attackStartedAt;
    if (timeSinceAttackStarted > combat.attackDuration) {
        actionState = ActionState::None;
        combat.attackStartedAt = 0;
        combat.attackDuration = 0;
    }

    if (sprite.spriteDef) {
        // TODO: Less hard-coded way to look up player sprite based on selected item id
        const Spritesheet *sheet = sprite.spriteDef->spritesheet;
        assert(sheet->sprites.size() == 5);

        const Item &selectedItem = GetSelectedItem();
        if (selectedItem.id == ItemID::Weapon_Sword) {
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
                    sprite.spriteDef = &sheet->sprites[4];
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

    body.Update(now, dt);
    sprite_update(sprite, now, dt);
}

void Player::Push(DrawList &drawList) const
{
    Drawable drawable{ DrawableType::Player };
    drawable.player = this;
    drawList.Push(drawable);
}

float Player_Depth(const Drawable &drawable)
{
    assert(drawable.type == DrawableType::Player);
    const Player &player = *drawable.player;
    return player.body.position.y;
}

bool Player_Cull(const Drawable &drawable, const Rectangle &cullRect)
{
    assert(drawable.type == DrawableType::Player);
    const Player &player = *drawable.player;
    bool cull = sprite_cull_body(player.sprite, player.body, cullRect);
    return cull;
}

void Player_Draw(const Drawable &drawable)
{
    assert(drawable.type == DrawableType::Player);
    const Player &player = *drawable.player;
    // Player shadow
    // TODO: Shadow size based on height from ground
    // https://yal.cc/top-down-bouncing-loot-effects/
    //const float shadowScale = 1.0f + slime->transform.position.z / 20.0f;
    const Vector2 playerGroundPos = player.body.GroundPosition();
    Shadow::Draw((int)playerGroundPos.x, (int)playerGroundPos.y, 16.0f, -6.0f);

    sprite_draw_body(player.sprite, player.body, WHITE);
    HealthBar::Draw(10, player.sprite, player.body, player.name, player.combat.hitPoints, player.combat.hitPointsMax);
}