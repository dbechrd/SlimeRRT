namespace Player {
    ErrorType Init(World &world, EntityID id)
    {
        Attach *attach = 0;
        Body3D *body3d = 0;
        Combat *combat = 0;
        Inventory *inventory = 0;
        Sprite *sprite = 0;

        E_ERROR_RETURN(world.facetDepot.FacetAlloc(id, Facet_Attach, (Facet **)&attach), "Failed to alloc facet");
        E_ERROR_RETURN(world.facetDepot.FacetAlloc(id, Facet_Body3D, (Facet **)&body3d), "Failed to alloc facet");
        E_ERROR_RETURN(world.facetDepot.FacetAlloc(id, Facet_Combat, (Facet **)&combat), "Failed to alloc facet");
        E_ERROR_RETURN(world.facetDepot.FacetAlloc(id, Facet_Inventory, (Facet **)&inventory), "Failed to alloc facet");
        E_ERROR_RETURN(world.facetDepot.FacetAlloc(id, Facet_Sprite, (Facet **)&sprite), "Failed to alloc facet");

        attach->points[Attach_Gut] = { 0.0f, 0.0f, -16.0f };
        body3d->speed = SV_PLAYER_MOVE_SPEED;
        combat->level = 1;
        combat->hitPoints = 100;
        combat->hitPointsMax = 100;
        combat->meleeDamage = 1;
        inventory->selectedSlot = PlayerInventory::SlotId_Hotbar_0;
        sprite->scale = 1;
        if (!g_clock.server) {
            const Spritesheet &spritesheet = Catalog::g_spritesheets.FindById(Catalog::SpritesheetID::Character_Charlie);
            const SpriteDef *spriteDef = spritesheet.FindSprite("player_sword");
            sprite->spriteDef = spriteDef;
        }

        return ErrorType::Success;
    }

    bool Attack(World &world, EntityID entityId, InputSample &input)
    {
        DLB_ASSERT(entityId);
        Body3D *body3d = (Body3D *)world.facetDepot.FacetFind(entityId, Facet_Body3D);
        Combat *combat = (Combat *)world.facetDepot.FacetFind(entityId, Facet_Combat);
        Entity *entity = (Entity *)world.facetDepot.FacetFind(entityId, Facet_Entity);
        Inventory *inventory = (Inventory *)world.facetDepot.FacetFind(entityId, Facet_Inventory);
        DLB_ASSERT(body3d);
        DLB_ASSERT(combat);
        DLB_ASSERT(entity);
        DLB_ASSERT(inventory);

        const double attackAlpha = (g_clock.now - combat->attackStartedAt) / combat->attackDuration;
        switch (entity->actionState) {
            case Entity::Act_Attack: {
                if (attackAlpha > 0.5f) {
                    entity->actionState = Entity::Act_Recover;
                }
                break;
            }
            case Entity::Act_Recover: {
                if (attackAlpha > 1.0f) {
                    combat->attackStartedAt = 0;
                    combat->attackDuration = 0;
                    entity->actionState = Entity::Act_None;
                }
                break;
            }
        }

        if (input.primary && entity->actionState == Entity::Act_None) {
            entity->actionState = Entity::Act_Attack;
            body3d->Move({});  // update lastMoved to stop idle animation
            combat->attackStartedAt = g_clock.now;
            combat->attackDuration = 0.3;

            ItemStack selectedStack = inventory->GetSelectedStack();
            if (selectedStack.uid) {
                const Item &selectedItem = g_item_db.Find(selectedStack.uid);
                switch (selectedItem.Proto().itemClass) {
                    //case ItemClass_Weapon: {
                    //    stats.timesSwordSwung++;
                    //    break;
                    //}
                    //default: {
                    //    stats.timesFistSwung++;
                    //    break;
                    //}
                }
            }
            return true;
        }
        return false;
    }

    bool Move(World &world, EntityID entityId, Vector2 offset)
    {
        DLB_ASSERT(entityId);

        if (v2_is_zero(offset)) {
            return false;
        }

        Body3D *body3d = (Body3D *)world.facetDepot.FacetFind(entityId, Facet_Body3D);
        Sprite *sprite = (Sprite *)world.facetDepot.FacetFind(entityId, Facet_Sprite);
        DLB_ASSERT(body3d);
        DLB_ASSERT(sprite);

        // Don't allow player to move if they're not touching the ground (currently no jump/fall, so just assert)
        //DLB_ASSERT(body.position.z == 0.0f);

        body3d->Move(offset);
        sprite_set_direction(*sprite, offset);

        const float pixelsMoved = v2_length(offset);
        const float metersMoved = PIXELS_TO_METERS(pixelsMoved);
        //stats.kmWalked += metersMoved / 1000.0f;
        return true;
    }

    void Update(World &world, EntityID entityId, InputSample &input, Tilemap &map)
    {
        DLB_ASSERT(entityId);
        DLB_ASSERT(input.ownerId == entityId);
        Body3D *body3d = (Body3D *)world.facetDepot.FacetFind(entityId, Facet_Body3D);
        Combat *combat = (Combat *)world.facetDepot.FacetFind(entityId, Facet_Combat);
        Entity *entity = (Entity *)world.facetDepot.FacetFind(entityId, Facet_Entity);
        Inventory *inventory = (Inventory *)world.facetDepot.FacetFind(entityId, Facet_Inventory);
        Sprite *sprite = (Sprite *)world.facetDepot.FacetFind(entityId, Facet_Sprite);
        DLB_ASSERT(body3d);
        DLB_ASSERT(combat);
        DLB_ASSERT(entity);
        DLB_ASSERT(inventory);
        DLB_ASSERT(sprite);

        DLB_ASSERT(!entity->despawnedAt);

        // TODO: Do client-side prediction of inventory (probably requires tracking input.seq of last time a slot
        // changed and only syncing the server state if the input.seq >= client-side recorded seq #). This will
        // only affect people with high ping.
        if (g_clock.server && input.selectSlot) {
            inventory->selectedSlot = input.selectSlot;
        }

        if (combat->hitPoints) {
            // TODO(cleanup): jump
            //if (body.OnGround() && input.dbgJump) {
            //    body.ApplyForce({ 0, 0, METERS_TO_PIXELS(4.0f) });
            //}

            float speed = body3d->speed;

            {
                ItemUID speedSlotUid = inventory->slots[PlayerInventory::SlotId_Hotbar_9].stack.uid;
                if (speedSlotUid) {
                    const Item &speedSlotItem = g_item_db.Find(speedSlotUid);
                    ItemAffix afxMoveSpeedFlat = speedSlotItem.FindAffix(ItemAffix_MoveSpeedFlat);
                    speed += afxMoveSpeedFlat.value.min;
                }
            }

            Vector2 move{};
            if (input.walkNorth || input.walkEast || input.walkSouth || input.walkWest) {
                move.y -= 1.0f * input.walkNorth;
                move.x += 1.0f * input.walkEast;
                move.y += 1.0f * input.walkSouth;
                move.x -= 1.0f * input.walkWest;
                if (input.run) {
                    entity->moveState = Entity::Move_Run;
                    speed += 1.0f;
                } else {
                    entity->moveState = Entity::Move_Walk;
                }
            } else {
                entity->moveState = Entity::Move_Idle;
            }

            const Vector2 pos = body3d->GroundPosition();
            const Tile *tile = map.TileAtWorld(pos.x, pos.y);
            if (tile && tile->type == TileType_Water) {
                speed *= 0.5f;
                // TODO: moveState = Player::MoveState::Swimming;
            }

            Vector2 moveBuffer = v2_scale(v2_normalize(move), METERS_TO_PIXELS(speed) * input.dt);

            if (Attack(world, entityId, input) && !input.skipFx) {
                Catalog::g_sounds.Play(Catalog::SoundID::Whoosh, 1.0f + dlb_rand32f_variance(0.1f));
            }

            if (!v2_is_zero(moveBuffer)) {
                const bool walkable = tile && tile->IsWalkable();

                Vector2 newPos = v2_add(pos, moveBuffer);
                const Tile *newTile = map.TileAtWorld(newPos.x, newPos.y);
                bool newWalkable = newTile && newTile->IsWalkable();

                // NOTE: This extra logic allows the player to slide when attempting to move diagonally against a wall
                // NOTE: If current tile isn't walkable, allow player to walk off it. This may not be the best solution
                // if the player can accidentally end up on unwalkable tiles through gameplay, but currently the only
                // way to end up on an unwalkable tile is to spawn there.
                // TODO: We should fix spawning to ensure player spawns on walkable tile (can probably just manually
                // generate something interesting in the center of the world that overwrites procgen, like Don't
                // Starve's fancy arrival portal).
                if (walkable) {
                    if (!newWalkable) {
                        // XY unwalkable, try only X offset
                        newPos = pos;
                        newPos.x += moveBuffer.x;
                        newTile = map.TileAtWorld(newPos.x, newPos.y);
                        newWalkable = newTile && newTile->IsWalkable();
                        if (!newWalkable) {
                            // X unwalkable, try only Y offset
                            newPos = pos;
                            newPos.y += moveBuffer.y;
                            newTile = map.TileAtWorld(newPos.x, newPos.y);
                            newWalkable = newTile && newTile->IsWalkable();
                            if (!newWalkable) {
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

                const bool moved = Move(world, entityId, moveBuffer);
                if (moved) {
                    thread_local static double lastFootstep = 0;
                    double timeSinceLastFootstep = g_clock.now - lastFootstep;
                    float distanceMoved = v2_length(moveBuffer);
                    if (!input.skipFx && timeSinceLastFootstep > 1.0f / distanceMoved) {
                        Catalog::g_sounds.Play(Catalog::SoundID::Footstep, 1.0f + dlb_rand32f_variance(0.5f));
                        lastFootstep = g_clock.now;
                    }
                }
    #if 0
                printf("[%s] %s %s -> %s, + %.02f, %.02f %.02f, %.02f -> %.02f, %.02f\n",
                    g_clock.server ? "                                                                            SRV" : "CLI",
                    moved ? "M" : "-",
                    walkable ? "-" : "C",
                    newWalkable ? "-" : "C",
                    moveOffset.x,
                    moveOffset.y,
                    pos.x,
                    pos.y,
                    body.GroundPosition().x,
                    body.GroundPosition().y
                );
    #endif
            }
        }

        if (sprite->spriteDef) {
            // TODO: Less hard-coded way to look up player sprite based on selected item type
            const Spritesheet *sheet = sprite->spriteDef->spritesheet;
            DLB_ASSERT(sheet->sprites.size() == 5);

            ItemStack selectedStack = inventory->GetSelectedStack();
            if (selectedStack.uid) {
                const Item &selectedItem = g_item_db.Find(selectedStack.uid);
                if (selectedItem.Proto().itemClass == ItemClass_Weapon) {
                    switch (entity->actionState) {
                        case Entity::Act_None: {
                            if (body3d->idle) {
                                // TODO: sprite_by_name("player_sword");
                                sprite->spriteDef = &sheet->sprites[2];
                            } else {
                                // TODO: sprite_by_name("player_sword_idle");
                                sprite->spriteDef = &sheet->sprites[3];
                            }
                            break;
                        }
                        case Entity::Act_Attack: {
                            // sprite_by_name("player_sword_attack");
                            sprite->spriteDef = &sheet->sprites[4];
                            break;
                        }
                        case Entity::Act_Recover: {
                            sprite->spriteDef = &sheet->sprites[2];
                            break;
                        }
                    }
                } else {
                    if (body3d->idle) {
                        // TODO: sprite_by_name("player_melee");
                        sprite->spriteDef = &sheet->sprites[0];
                    } else {
                        // TODO: sprite_by_name("player_melee_idle");
                        sprite->spriteDef = &sheet->sprites[1];
                    }
                }
            }
        }

        body3d->Update(input.dt);
        sprite_update(*sprite, input.dt);
        combat->Update(input.dt);

    #if 0
        // TODO: PushEvent(BODY_IDLE_CHANGED, body.idle);
        if (!input.skipFx && body.idleChanged) {
            fadeTo(music, target, speed = 1.0) { music.targetVol = float }
            fadeIn(music, speed = 1.0) { fade(Music, 1.0, speed) }
            fadeOut(music, speed = 1.0) { fadeTo(Music, 0.0, speed) }
            // TODO: HandleEvent, body.idle ? (fadeIn(idleMusic), fadeTo(bgMusic, 0.1)) : (fadeOut(idleMusic), fadeIn(bgMusic))
        }
    #endif

    #if 0
        if (g_clock.server || !input.skipFx) {
            thread_local bool idle = false;
            Vector3 worldPos = body.WorldPosition();
            if (!body.TimeSinceLastMove()) {
                E_DEBUG("%.3f %.3f %.3f %.3f %u", worldPos.x, worldPos.y, worldPos.z, input.dt, input.seq);
                idle = false;
            } else if (!idle) {
                E_DEBUG("--------", 0);
                idle = true;
            }
        }
    #endif
        // Skip sounds/particles etc. next time this input is used (e.g. during reconciliation)
        input.skipFx = true;
    }
}