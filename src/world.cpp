#include "world.h"
#include "body.h"
#include "catalog/sounds.h"
#include "catalog/spritesheets.h"
#include "controller.h"
#include "direction.h"
#include "helpers.h"
#include "maths.h"
#include "particles.h"
#include "spritesheet.h"
#include "tilemap.h"
#include "dlb_rand.h"
#include <unordered_set>

World::World(void)
{
    rtt_seed = 16;
    //rtt_seed = time(NULL);
    dlb_rand32_seed_r(&rtt_rand, rtt_seed, rtt_seed);
    g_noise.Seed(rtt_seed);
}

World::~World(void)
{
    // HUH
}

const Vector3 World::GetWorldSpawn(void)
{
    Vector3 worldSpawn{ 0, 0, 0 };
    return worldSpawn;
};

ErrorType World::AddPlayerInfo(const char *name, uint32_t nameLength, PlayerInfo **result)
{
    DLB_ASSERT(name);
    DLB_ASSERT(nameLength >= USERNAME_LENGTH_MIN);
    DLB_ASSERT(nameLength <= USERNAME_LENGTH_MAX);
    DLB_ASSERT(result);

    PlayerInfo *existingPlayer = FindPlayerInfoByName(name, nameLength);
    if (existingPlayer) {
        return ErrorType::UserAccountInUse;
    }

    for (int i = 0; i < (int)ARRAY_SIZE(playerInfos); i++) {
        PlayerInfo &playerInfo = playerInfos[i];
        if (!playerInfo.nameLength) {
            // We found a free slot
            playerInfo.entityId = i + 1;
            playerInfo.nameLength = (uint32_t)MIN(nameLength, USERNAME_LENGTH_MAX);
            memcpy(playerInfo.name, name, playerInfo.nameLength);
            *result = &playerInfo;
            return ErrorType::Success;
        }
    }
    return ErrorType::ServerFull;
}

PlayerInfo *World::FindPlayerInfo(EntityID entityId)
{
    for (PlayerInfo &info : playerInfos) {
        if (info.entityId == entityId) {
            return &info;
        }
    }
    return 0;
}

PlayerInfo *World::FindPlayerInfoByName(const char *name, size_t nameLength)
{
    if (!name || !nameLength) {
        return 0;
    }

    for (PlayerInfo &playerInfo : playerInfos) {
        if (nameLength == playerInfo.nameLength && !strncmp(name, playerInfo.name, playerInfo.nameLength)) {
            return &playerInfo;
        }
    }
    return 0;
}


void World::RemovePlayerInfo(EntityID playerId)
{
    RemovePlayer(playerId);

    PlayerInfo *playerInfo = FindPlayerInfo(playerId);
    if (playerInfo) {
        *playerInfo = {};
    }
}

EntityID World::PlayerFindOrCreate(EntityID entityId)
{
    Entity *player = facetDepot.EntityFind(entityId);
    if (!player) {
        Player::Init(*this, entityId);
    } else {
        E_WARN("PlayerFindOrCreate: eid %u is already in use!", entityId);
    }
    return entityId;
}

EntityID World::PlayerFindByName(const char *name, size_t nameLength)
{
    DLB_ASSERT(name);
    DLB_ASSERT(nameLength);

    EntityID result = 0;
    for (EntityID playerId : facetDepot.entityIdsByType[Entity_Player]) {
        Entity *entity = facetDepot.EntityFind(playerId);
        DLB_ASSERT(entity);
        if (entity &&
            entity->nameLength == nameLength &&
            !strncmp(entity->name, name, nameLength))
        {
            result = playerId;
            break;
        }
    }
    return result;
}

EntityID World::PlayerFindNearest(Vector2 worldPos, float maxDist, Vector2 *toPlayer, bool includeDead)
{
    EntityID result = 0;
    for (EntityID playerId : facetDepot.entityIdsByType[Entity_Player]) {
        Combat *combat = (Combat *)facetDepot.FacetFind(playerId, Facet_Combat);
        DLB_ASSERT(combat);
        if (!includeDead && combat->diedAt) {
            continue;
        }

        Body3D *body3d = (Body3D *)facetDepot.FacetFind(playerId, Facet_Body3D);
        DLB_ASSERT(body3d);
        Vector2 toPlayerVec = v2_sub(body3d->GroundPosition(), worldPos);
        const float toPlayerDistSq = v2_length_sq(toPlayerVec);
        if (toPlayerDistSq <= SQUARED(maxDist)) {
            if (toPlayer) *toPlayer = toPlayerVec;
            result = playerId;
            break;
        }
    }
    return result;
}

void World::RemovePlayer(EntityID entityId)
{
    E_DEBUG("Remove player %u", entityId);

    Entity *entity = facetDepot.EntityFind(entityId);
    if (!entity) {
        E_WARN("Cannot remove a player that doesn't exist (entityId: %u).", entityId);
    }
    DLB_ASSERT(entity->entityType == Entity_Player);

    facetDepot.EntityFree(entityId);
}

ErrorType World::SpawnSam(void)
{
    //NPC *sam = 0;
    //E_ERROR_RETURN(SpawnNpc(0, NPC::Type_Slime, { 0, 0, 0 }, &sam), "Failed to spawn Sam", 0);

    //DLB_ASSERT(sam);
    //sam->SetName(CSTR("Sam"));
    //sam->combat.hitPointsMax = 20.0f; //100000.0f;
    //sam->combat.hitPoints = sam->combat.hitPointsMax;
    //sam->combat.meleeDamage = -1.0f;
    //sam->combat.lootTableId = LootTableID::LT_Sam;
    //sam->body.Teleport(v3_add(GetWorldSpawn(), { -200.0f, 0, 0 }));
    //sam->sprite.scale = 2.0f;

    return ErrorType::Success;
}

ErrorType World::SpawnEntity(EntityID entityId, EntityType type, Vector3 worldPos, Entity **result)
{
    DLB_ASSERT(type > 0);
    DLB_ASSERT(type < Entity_Count);

    // Only the server should be spawning new npcs; the client merely replicates them by targetId
    if (g_clock.server) {
        DLB_ASSERT(entityId == 0);
    } else {
        DLB_ASSERT(entityId);
    }

    if (!entityId) {
        thread_local static uint32_t nextId = 0;
        nextId = MAX(1, nextId + 1); // Prevent ID zero from being used on overflow
        entityId = nextId;
    }

    Entity *entity = 0;
    E_ERROR_RETURN(facetDepot.EntityAlloc(entityId, type, &entity), "Entity alloc failed");
    DLB_ASSERT(entity);

    entity->Init(*this);

    Body3D *body3d = (Body3D *)facetDepot.FacetFind(entityId, Facet_Body3D);
    if (body3d) {
        body3d->Teleport({ worldPos.x, worldPos.y, 0 });
        E_DEBUG("Spawning entity [%u] @ %.f, %.f", entityId, worldPos.x, worldPos.y);
    } else {
        E_DEBUG("Spawning entity [%u]", entityId);
    }

    if (result) *result = entity;
    return ErrorType::Success;
}

void World::SV_Simulate(double dt)
{
    SV_SimPlayers(dt);
    SV_SimNpcs(dt);
    SV_SimItems(dt);
}

void World::SV_SimPlayers(double dt)
{
    UNUSED(dt);

    for (EntityID playerId : facetDepot.entityIdsByType[Entity_Player]) {
        Entity *playerEntity = facetDepot.EntityFind(playerId);
        Body3D *playerBody3d = (Body3D *)facetDepot.FacetFind(playerId, Facet_Body3D);
        Combat *playerCombat = (Combat *)facetDepot.FacetFind(playerId, Facet_Combat);
        Inventory *playerInventory = (Inventory *)facetDepot.FacetFind(playerId, Facet_Inventory);
        DLB_ASSERT(playerEntity);
        DLB_ASSERT(playerBody3d);
        DLB_ASSERT(playerCombat);
        DLB_ASSERT(playerInventory);

        // Use < to allow two players to kill each other in the same tick
        if (playerCombat->diedAt < g_clock.now) {
            if (g_clock.now - playerCombat->diedAt < SV_RESPAWN_TIMER) {
                continue;
            } else {
                playerCombat->diedAt = 0;
                playerCombat->hitPoints = playerCombat->hitPointsMax;
                playerBody3d->Teleport(GetWorldSpawn());
            }
        }

        //switch (player.actionState) {
        //    case Player::ActionState::AttackBegin  : printf("%u !!!!\n", tick); break;
        //    case Player::ActionState::AttackSustain: printf("%u **\n", tick); break;
        //    case Player::ActionState::AttackRecover: printf("%u .\n", tick); break;
        //}
        if (playerEntity->actionState == Action_Attack) {
            const float playerAttackReach = METERS_TO_PIXELS(1.0f);

            const ItemStack selectedStack = playerInventory->GetSelectedStack();
            const Item &selectedItem = g_item_db.Find(selectedStack.uid);
            float playerDamage = selectedItem.FindAffix(ItemAffix_DamageFlat).rollValue();
            float playerKnockback = selectedItem.FindAffix(ItemAffix_KnockbackFlat).rollValue();
            if (selectedItem.type == ItemType_Empty) {
                playerDamage = playerCombat->meleeDamage;
            }

            // TODO: Store list of EntityIDs in each chunk. Find nearby entities by combining
            // the the 3x3 chunks around the entity being processed. In the future, we will
            // also be finding entities nearby projectiles, not just nearby players, so it
            // be smart to have a generic FindEntitiesNearby(EntityID entityId) function.
            for (int targetEntityType = 0; targetEntityType < Entity_Count; targetEntityType++) {
                // When pvp is disabled, skip all attacks targeting other players
                // TODO: We may want to allow healing attacks in the future?
                if (targetEntityType == Entity_Player && !pvp) {
                    continue;
                }

                for (EntityID targetId : facetDepot.entityIdsByType[targetEntityType]) {
                    // Don't attack oneself
                    if (targetId == playerId) {
                        continue;
                    }
                    // TODO: Also check if target is a player and on your team

                    Body3D *targetBody3d = (Body3D *)facetDepot.FacetFind(targetId, Facet_Body3D);
                    Combat *targetCombat = (Combat *)facetDepot.FacetFind(targetId, Facet_Combat);
                    DLB_ASSERT(targetBody3d);
                    DLB_ASSERT(targetCombat);

                    // Don't let the player beat a dead horse
                    if (targetCombat->diedAt) {
                        continue;
                    }

                    // Apply damage to nearby NPCs
                    Vector3 playerToTarget = v3_sub(targetBody3d->WorldPosition(), playerBody3d->WorldPosition());
                    if (v3_length_sq(playerToTarget) <= SQUARED(playerAttackReach)) {
                        // TODO: Stats
                        //player.stats.damageDealt += combat->TakeDamage(playerDamage);
                        targetCombat->TakeDamage(playerDamage);
                        if (targetCombat->hitPoints) {
                            // Still alive, apply effects
                            if (playerKnockback) {
                                Vector3 knockbackDir = v3_normalize(playerToTarget);
                                Vector3 knockbackVec = v3_scale(knockbackDir, METERS_TO_PIXELS(playerKnockback));
                                targetBody3d->ApplyForce(knockbackVec);
                            }
                        } else {
                            // Dead, do all the dead stuff
                            bool spawnMerchantOnLevelUp = playerCombat->level == 1;
                            if (playerCombat->GrantXP(targetCombat->xp)) {
                                // Spawn a merchant for the player
                                if (spawnMerchantOnLevelUp) {
                                    Vector3 spawnPos = playerBody3d->WorldPosition();
                                    spawnPos.y -= 40;
                                    spawnPos.z = 0;
                                    Entity *level1Merchant = 0;
                                    SpawnEntity(0, Entity_Townfolk, spawnPos, &level1Merchant);
                                    DLB_ASSERT(level1Merchant);
                                    if (level1Merchant) {
                                        char nameBuf[USERNAME_LENGTH_MAX + 32]{};
                                        int nameLen = snprintf(CSTR0(nameBuf), "%.*s's Merchant", playerEntity->nameLength, playerEntity->name);
                                        level1Merchant->SetName(nameBuf, nameLen);
                                    }
                                }
                            }
                            // TODO: Stats
                            //player.stats.entitiesSlainByType[entityType]++;
                        }
                    }
                }
            }
        }

        // Try to spawn enemies near player
        if (!peaceful && dlb_rand32f() < 0.1f) {
            Vector2 spawnPos{};
            spawnPos.x = dlb_rand32f_variance(1.0f);
            spawnPos.y = dlb_rand32f_variance(1.0f);
            spawnPos = v2_normalize(spawnPos);

            // Scale into correct range for valid spawn ring
            float mult = dlb_rand32f_range(SV_ENEMY_MIN_SPAWN_DIST, SV_ENEMY_DESPAWN_RADIUS);
            spawnPos = v2_scale(spawnPos, mult);

            // Translate to whatever point we want to spawn
            Vector2 playerPos = playerBody3d->GroundPosition();
            spawnPos.x += playerPos.x;
            spawnPos.y += playerPos.y;

            const Tile *spawnTile = map.TileAtWorld(spawnPos.x, spawnPos.y);
            if (spawnTile && spawnTile->IsSpawnable()) {
                EntityID anyPlayerWithinRange = PlayerFindNearest(spawnPos, SV_ENEMY_MIN_SPAWN_DIST);
                if (!anyPlayerWithinRange) {
                    SpawnEntity(0, Entity_Slime, { spawnPos.x, spawnPos.y, 0 }, 0);
                }
            }
        }
    }
}

void World::SV_SimNpcs(double dt)
{
    // TODO: We need e.g. EntityClass_NPC or EntityTag_NPC or some better way
    // of encapsulating sets of entity types under various categories.
    std::unordered_set<int> npcTypes{
        Entity_Slime,
        Entity_Townfolk
    };

    for (int entityType = 0; entityType < Entity_Count; entityType++) {
        if (npcTypes.contains(entityType)) {
            for (EntityID entityId : facetDepot.entityIdsByType[entityType]) {
                Entity *entity = facetDepot.EntityFind(entityId);
                DLB_ASSERT(entity);

                if (entity->despawnedAt) {
                    continue;
                }

                entity->Update(*this, entityId, dt);
            }
        }
    }
}

void World::SV_SimItems(double dt)
{
    // Move items toward nearby players (attraction zone auto-pickup)
    for (WorldItem &item : itemSystem.worldItems) {
        if (!item.euid || item.despawnedAt || g_clock.now < item.spawnedAt + SV_ITEM_PICKUP_DELAY) {
            continue;
        }

        DLB_ASSERT(item.stack.uid);
        DLB_ASSERT(item.stack.count);

        EntityID closestPlayerId = PlayerFindNearest(item.body.GroundPosition(), SV_ITEM_ATTRACT_DIST);

        // No players nearby
        if (!closestPlayerId) {
            continue;
        }

        // Don't attract items you just dropped a moment ago, it's annnoying
        if (item.droppedByPlayerId == closestPlayerId &&
            g_clock.now < item.spawnedAt + SV_ITEM_REPICKUP_DELAY)
        {
            continue;
        }

        Body3D *playerBody3d = (Body3D *)facetDepot.FacetFind(closestPlayerId, Facet_Body3D);
        DLB_ASSERT(playerBody3d);

        Vector2 itemToPlayer = v2_sub(playerBody3d->GroundPosition(), item.body.GroundPosition());
        const float itemToPlayerDistSq = v2_length_sq(itemToPlayer);
        if (itemToPlayerDistSq < SQUARED(SV_ITEM_PICKUP_DIST)) {
            Inventory *playerInventory = (Inventory *)facetDepot.FacetFind(closestPlayerId, Facet_Inventory);
            DLB_ASSERT(playerInventory);

            if (playerInventory->PickUp(item.stack)) {
                if (!item.stack.count) {
                    item.despawnedAt = g_clock.now;
#if SV_DEBUG_WORLD_ITEMS
                    E_DEBUG("Sim: Item picked up %u", item.type);
#endif
                } else {
                    // TODO: Send item update message so other players know stack was partially picked up
                    // to update their label.
                }
            }
        } else {
            const Vector2 itemToPlayerDir = v2_normalize(itemToPlayer);
            const float speed = MAX(0, 5.0f / PIXELS_TO_METERS(sqrtf(itemToPlayerDistSq)));
            const Vector2 itemVel = v2_scale(itemToPlayerDir, METERS_TO_PIXELS(speed));
            item.body.velocity.x = itemVel.x;
            item.body.velocity.y = itemVel.y;
            //item.body.velocity.z = MAX(item.body.velocity.z, itemVel.z);
        }
    }

    itemSystem.Update(dt);
}

void World::SV_DespawnDeadEntities(void)
{
    for (int entityType = 0; entityType < Entity_Count; entityType++) {
        std::vector<EntityID> entitiesByType = facetDepot.entityIdsByType[entityType];
        for (EntityID entityId : entitiesByType) {
            Entity *entity = facetDepot.EntityFind(entityId);
            DLB_ASSERT(entity);

            // Remove entity if they've been despawned for awhile
            if (entity->despawnedAt) {
                const double sinceDespawn = g_clock.now - entity->despawnedAt;
                if (sinceDespawn > SV_NPC_DESPAWN_LIFETIME) {
                    E_DEBUG("Remove despawned npc %u", entityId);
                    facetDepot.EntityFree(entityId);
                    continue;
                }
            } else {
#if 0
                // NOTE: We really only need to do this if the # of corpses gets really large,
                // i.e. garbage collection of corpses, to prevent using too much memory.
                Combat *combat = (Combat *)facetDepot.FacetFind(targetId, Facet_Combat);
                if (combat) {
                    const double sinceDeath = combat->diedAt ? (g_clock.now - combat->diedAt) : 0;
                    if (sinceDeath > SV_NPC_CORPSE_LIFETIME) {
                        E_DEBUG("Despawn dead npc %u", targetId);
                        entity->despawnedAt = g_clock.now;
                        continue;
                    }
                }
#endif
            }
        }
    }

    itemSystem.DespawnDeadEntities(1.0 / SNAPSHOT_SEND_RATE);
}

void World::CL_Interpolate(double renderAt)
{
    for (int entityType = 0; entityType < Entity_Count; entityType++) {
        std::vector<EntityID> entitiesByType = facetDepot.entityIdsByType[entityType];
        for (EntityID entityId : entitiesByType) {
            Entity *entity = facetDepot.EntityFind(entityId);
            Body3D *body3d = (Body3D *)facetDepot.FacetFind(entityId, Facet_Body3D);
            Sprite *sprite = (Sprite *)facetDepot.FacetFind(entityId, Facet_Sprite);
            DLB_ASSERT(entity);
            DLB_ASSERT(body3d);
            DLB_ASSERT(sprite);

            // TODO: It might be a better idea to just serialize direction over
            // the wire as part of positionHistory?
            body3d->CL_Interpolate(renderAt, sprite->direction);

            // HACK(cleanup): Where should sfx generation *actually* go??
            if (entity->entityType == Entity_Slime && body3d->jumped) {
                //Catalog::SoundID squish = dlb_rand32i_range(0, 1) ? Catalog::SoundID::Squish1 : Catalog::SoundID::Squish2;
                Catalog::SoundID squish = Catalog::SoundID::Squish1;
                Catalog::g_sounds.Play(squish, 1.0f + dlb_rand32f_variance(0.2f), true);
            }
        }
    }

    for (WorldItem &item : itemSystem.worldItems) {
        if (!item.euid) {
            continue;
        }
        item.body.CL_Interpolate(renderAt, item.sprite.direction);
    }
}

void World::CL_Extrapolate(double dt)
{
#if 0
    // TODO: Collision detection is in Simulate() for enemies, but Player::Update() for players..
    // this makes no sense. It should probably not be in either place. We need a ResolveCollisions, eh?

    for (Player &player : players) {
        if (!player.id || player.id == playerId) {
            continue;
        }
        // TODO: Use future inputs we've received from the server to predict other players more reliability
        InputSample input{};
        input.dt = (float)dt;
        player.Update(input, map);
    }
    for (int type = NPC::Type_None + 1; type < NPC::Type_Count; type++) {
        NpcList npcList = npcs.byType[type];
        for (size_t i = 0; i < npcList.length; i++) {
            NPC &npc = npcList.data[i];
            if (!npc.id) {
                continue;
            }
            npc.Update(*this, dt);
        }
    }

    for (WorldItem &item : itemSystem.worldItems) {
        if (!item.euid || item.despawnedAt || g_clock.now < item.spawnedAt + SV_ITEM_PICKUP_DELAY) {
            continue;
        }

        DLB_ASSERT(item.stack.uid);
        DLB_ASSERT(item.stack.count);
        item.Update(dt);
    }
#endif
}

void World::CL_Animate(double dt)
{
    for (int entityType = 0; entityType < Entity_Count; entityType++) {
        std::vector<EntityID> entitiesByType = facetDepot.entityIdsByType[entityType];
        for (EntityID entityId : entitiesByType) {
            Combat *combat = (Combat *)facetDepot.FacetFind(entityId, Facet_Combat);
            DLB_ASSERT(combat);
            combat->Update(dt);
        }
    }
}

void World::CL_FreeDespawnedEntities(void)
{
    for (int entityType = 0; entityType < Entity_Count; entityType++) {
        std::vector<EntityID> entitiesByType = facetDepot.entityIdsByType[entityType];
        for (EntityID entityId : entitiesByType) {
            Entity *entity = facetDepot.EntityFind(entityId);
            DLB_ASSERT(entity);
            if (entity->despawnedAt) {
                // Remove npc immediately if requested by server
                E_DEBUG("Remove despawned entity %u", entityId);
                facetDepot.EntityFree(entityId);
            }
        }
    }

    itemSystem.DespawnDeadEntities(0);
}

void World::EnableCulling(Rectangle cullRect)
{
    drawList.EnableCulling(cullRect);
}

bool World::CullTile(Vector2 tilePos, int zoomMipLevel)
{
    if (!drawList.cullEnabled) {
        return false;
    }

    const float tileWidthMip = (float)(TILE_W * zoomMipLevel);
    const float tileHeightMip = (float)(TILE_W * zoomMipLevel);

    if (tilePos.x + tileWidthMip < drawList.cullRect.x
     || tilePos.y + tileHeightMip < drawList.cullRect.y
     || tilePos.x >= drawList.cullRect.x + drawList.cullRect.width
     || tilePos.y >= drawList.cullRect.y + drawList.cullRect.height) {
        return true;
    }

    return false;
}

size_t World::DrawMap(const Spycam &spycam)
{
    const int zoomMipLevel = spycam.GetZoomMipLevel();
    DLB_ASSERT(zoomMipLevel > 0);
    if (zoomMipLevel <= 0) {
        return 0;
    }

    //ImDrawList *bgDrawList = ImGui::GetBackgroundDrawList();

    size_t tilesDrawn = 0;
    const Rectangle &camRect = spycam.GetRect();
    const float cx = camRect.x;
    const float cy = camRect.y;
    const float tilesW = camRect.width / TILE_W;
    const float tilesH = camRect.height / TILE_W;
    for (float y = -1; y < tilesH + 2; y += zoomMipLevel) {
        for (float x = -1; x < tilesW + 2; x += zoomMipLevel) {
            float xx = cx + x * TILE_W;
            float yy = cy + y * TILE_W;
            xx = floorf(xx / TILE_W) * TILE_W;
            yy = floorf(yy / TILE_W) * TILE_W;

            const Tile *tile = map.TileAtWorld(xx, yy);
            if (tile) {
                tileset_draw_tile(map.tilesetId, tile->type, { xx, yy }, WHITE);

                //uint8_t r = tile->base;
                //uint8_t g = tile->baseNoise;
                //const int xxi = (int)xx;
                //const int yyi = (int)yy;
                //DrawRectangle(xxi    , yyi     , SUBTILE_W, SUBTILE_H, { r, 0, 0, 255 });  // Top left
                //DrawRectangle(xxi + 8, yyi     , SUBTILE_W, SUBTILE_H, { 0, g, 0, 255 });  // Top right

                //DrawRectangle(xxi    , yyi + 16, SUBTILE_W, SUBTILE_H, { 0, 0, 0, 255 });  // Bottom left
                //DrawRectangle(xxi + 8, yyi + 16, SUBTILE_W, SUBTILE_H, { 0, 0, 0, 255 });  // Bottom right
            } else {
                tileset_draw_tile(map.tilesetId, TileType_Void, { xx, yy }, WHITE);
            }
#if 0
            const Tile *tile = map.TileAtWorld(xx, yy);
            Tileset &tileset = g_tilesets[(size_t)map.tilesetId];
            Rectangle tileRect = tileset_tile_rect(map.tilesetId, tile ? tile->type : TileType_Void);

            ImVec2 uv0 = {
                tileRect.x / tileset.texture.width,
                tileRect.y / tileset.texture.height,
            };
            ImVec2 uv1 = {
                (tileRect.x + tileRect.width ) / tileset.texture.width,
                (tileRect.y + tileRect.height) / tileset.texture.height,
            };

            xx -= cx;
            yy -= cy;

            bgDrawList->AddImage(
                (ImTextureID)(size_t)tileset.texture.type,
                ImVec2(xx, yy),
                ImVec2(xx + tileRect.width, yy + tileRect.height),
                ImVec2{ uv0.x, uv0.y },
                ImVec2{ uv1.x, uv1.y }
            );
#endif
#if 0
            if (xx > 20 && yy > 20) {
                char buf[16]{};
                char *bufPtr = buf + sizeof(buf) - 1;
                int xxp = xx;
                int rem;
                while (xxp) {
                    rem = xxp % 10;
                    *(--bufPtr) = '0' + rem;
                    xxp /= 10;
                }
                //snprintf(buf, sizeof(buf), "%d,\n%d", (int)xx, (int)yy);
                bgDrawList->AddText({ xx - cx, yy - cy }, IM_COL32_WHITE, bufPtr);
            }
#endif
            tilesDrawn++;
        }
    }

    for (float y = -1; y < tilesH + 2; y += zoomMipLevel) {
        for (float x = -1; x < tilesW + 2; x += zoomMipLevel) {
            float xx = cx + x * TILE_W;
            float yy = cy + y * TILE_W;
            xx = floorf(xx / TILE_W) * TILE_W;
            yy = floorf(yy / TILE_W) * TILE_W;

            const Vector2 at = { xx, yy };

            const Tile *tile = map.TileAtWorld(xx, yy);
            if (tile && tile->object.type) {
                ObjectType effectiveType = tile->object.EffectiveType();
                if (effectiveType < ObjectType_SpritesheetCount) {
                    tileset_draw_tile(TilesetID::TS_Objects, effectiveType, at, WHITE);
                } else {
                    // TODO: Make this lookup more general somehow
                    switch (effectiveType) {
                        case ObjectType_Tree01: {
                            Spritesheet &spritesheet = Catalog::g_spritesheets.FindById(Catalog::SpritesheetID::Environment_Forest);
                            DLB_ASSERT(spritesheet.texture.id);
                            SpriteDef *spriteDef = spritesheet.FindSprite("tree_01");
                            SpriteAnim &spriteAnim = spritesheet.animations[spriteDef->animations[0]];
                            DLB_ASSERT(spriteAnim.frameCount == 1);
                            SpriteFrame &spriteFrame = spritesheet.frames[spriteAnim.frames[0]];
                            DLB_ASSERT(spriteFrame.width);
                            DLB_ASSERT(spriteFrame.height);

                            drawList.Push(spriteFrame, at.y + TILE_H, false, at);
                            break;
                        }
                    }
                }
            }
        }
    }

    return tilesDrawn;
}

void World::DrawItems(void)
{
    itemSystem.PushAll(drawList);
}

void World::DrawEntities(void)
{
    for (int entityType = 0; entityType < Entity_Count; entityType++) {
        std::vector<EntityID> entitiesByType = facetDepot.entityIdsByType[entityType];
        for (EntityID entityId : entitiesByType) {
            Entity *entity = facetDepot.EntityFind(entityId);
            DLB_ASSERT(entity);
            // NOTE: CL_FreeDespawnedEntities() should've been called right before drawing
            DLB_ASSERT(!entity->despawnedAt);

            drawList.Push(*entity, entity->Depth(*this, entityId),
                entity->Cull(*this, entityId, drawList.cullRect));
        }
    }
}

void World::DrawParticles(void)
{
    // Queue particles for drawing
    particleSystem.PushAll(drawList);
}

void World::DrawFlush(void)
{
    drawList.Flush(*this);
}