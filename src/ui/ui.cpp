﻿#include "error.h"
#include "net_client.h"
#include "../catalog/items.h"
#include "../spycam.h"
#include "../tilemap.h"
#include "../world.h"
#include "imgui/imgui.h"
#include "GLFW/glfw3.h"
#include "raylib/raylib.h"

const PlayerControllerState *UI::input;
Vector2 UI::mouseScreen;
Vector2 UI::mouseWorld;
Vector2 UI::screenSize;
Spycam  *UI::spycam;

bool UI::showMenubar = false;
bool UI::showDemoWindow = false;
bool UI::showParticleConfig = false;
bool UI::showNetstatWindow = false;
bool UI::showItemProtoEditor = false;

bool UI::disconnectRequested = false;
bool UI::quitRequested = false;

UI::MenuStack UI::menuStack {};
const char *UI::hoverLabel = 0;
const char *UI::prevHoverLabel = 0;

const ImGuiTableSortSpecs *UI::itemSortSpecs = 0;

#define SERVER_IDX_NONE -2
#define SERVER_IDX_NEW -1
static int editingServerIdx = SERVER_IDX_NONE;

void UI::Update(const PlayerControllerState &input, Vector2 screenSize, Spycam *spycam)
{
    Rectangle cameraRect = spycam->GetRect();
    const Vector2 mouseScreen = GetMousePosition();
    Vector2 mouseWorld{
        cameraRect.x + mouseScreen.x * spycam->GetInvZoom(),
        cameraRect.y + mouseScreen.y * spycam->GetInvZoom()
    };

    UI::input = &input;
    UI::mouseScreen = mouseScreen;
    UI::mouseWorld = mouseWorld;
    UI::screenSize = screenSize;
    UI::spycam = spycam;
    UI::hoverLabel = 0;
    if (input.dbgToggleNetUI) {
        UI::showNetstatWindow = !UI::showNetstatWindow;
    }
};

bool UI::DisconnectRequested(bool disconnected)
{
    if (disconnected) {
        disconnectRequested = false;
    }
    return disconnectRequested;
}

bool UI::QuitRequested()
{
    return quitRequested;
}

void UI::TileHoverOutline(Tilemap &map)
{
    DLB_ASSERT(spycam);

    const Tile *mouseTile = map.TileAtWorld(mouseWorld.x, mouseWorld.y);
    if (!mouseTile) {
        return;
    }

    const int mouseTileX = (int)floorf(mouseWorld.x / TILE_W);
    const int mouseTileY = (int)floorf(mouseWorld.y / TILE_W);

    // Draw red outline on hovered tile
    const int zoomMipLevel = spycam->GetZoomMipLevel();
    Rectangle mouseTileRect{
        (float)TILE_W * mouseTileX,
        (float)TILE_W * mouseTileY,
        (float)TILE_W * zoomMipLevel,
        (float)TILE_W * zoomMipLevel
    };
    DrawRectangleLinesEx(mouseTileRect, floorf(spycam->GetInvZoom()), RED);
}

void UI::WorldGrid()
{
    DLB_ASSERT(spycam);

    const Rectangle &camRect = spycam->GetRect();
    for (float y = 0; y <= camRect.height; y++) {
        DrawLine(
            (int)(camRect.x + 0),
            (int)(camRect.y + y * TILE_H),
            (int)(camRect.x + camRect.width * TILE_W),
            (int)(camRect.y + y * TILE_H),
            BLACK
        );
    }
    for (float x = 0; x <= camRect.width; x++) {
        DrawLine(
            (int)(camRect.x + x * TILE_W),
            (int)(camRect.y + 0),
            (int)(camRect.x + x * TILE_W),
            (int)(camRect.y + camRect.height * TILE_H),
            BLACK
        );
    }
    const float center_y = camRect.height / 2 * TILE_H;
    const float center_x = camRect.width / 2 * TILE_W;
    DrawLineEx({ 0, (float)center_y }, { camRect.width * TILE_W, center_y }, 3.0f, RED);
    DrawLineEx({ (float)center_x, 0 }, { center_x, camRect.height * TILE_H }, 3.0f, GREEN);
}

void UI::Minimap(const Font &font, World &world)
{
    DLB_ASSERT(spycam);

    // Render minimap
    const int minimapMargin = 6;
    const int minimapBorderWidth = 2;
    const int minimapX = (int)screenSize.x - minimapMargin - world.map.minimap.width - minimapBorderWidth * 2;
    const int minimapY = minimapMargin;
    const int minimapW = world.map.minimap.width + minimapBorderWidth * 2;
    const int minimapH = world.map.minimap.height + minimapBorderWidth * 2;
    const int minimapTexX = minimapX + minimapBorderWidth;
    const int minimapTexY = minimapY + minimapBorderWidth;
    DrawRectangle(minimapX, minimapY, minimapW, minimapH, Fade(BLACK, 0.6f));
    DrawRectangleLinesEx({(float)minimapX, (float)minimapY, (float)minimapW, (float)minimapH}, (float)minimapBorderWidth, BLACK);

    Player *player = world.FindPlayer(world.playerId);
    if (player) {
        world.map.GenerateMinimap(player->body.GroundPosition());
        Vector2 playerPos = player->body.GroundPosition();
        const int offsetX = world.map.CalcChunkTile(playerPos.x) - CHUNK_W / 2;
        const int offsetY = world.map.CalcChunkTile(playerPos.y) - CHUNK_W / 2;
        BeginScissorMode(minimapTexX, minimapTexY, world.map.minimap.width, world.map.minimap.height);
        DrawTexture(world.map.minimap, minimapTexX - offsetX, minimapTexY - offsetY, WHITE);
        EndScissorMode();
    }

#if 1
    const Rectangle &camRect = spycam->GetRect();

    // TODO: Fix relative positioning of the map markers now that the map scrolls

    // Draw slimes on map
    for (int i = 0; i < (int)ARRAY_SIZE(world.npcs.slimes); i++) {
        const NPC &npc = world.npcs.slimes[i];
        if (npc.type) {
            float x = (npc.body.WorldPosition().x / camRect.width) * (float)(minimapW + minimapX);
            float y = (npc.body.WorldPosition().y / camRect.height) * (float)(minimapH + minimapY);
            DrawCircle((int)x, (int)y, 2.0f, Color{ 0, 170, 80, 255 });
        }
    }

    // Draw players on map
    for (int i = 0; i < (int)ARRAY_SIZE(world.players); i++) {
        const Player &player = world.players[i];
        if (player.id) {
            world.FindPlayerInfo(player.id);
            const PlayerInfo *playerInfo = world.FindPlayerInfo(player.id);
            if (playerInfo) {
                float x = (player.body.WorldPosition().x / camRect.width) * (float)(minimapW + minimapX);
                float y = (player.body.WorldPosition().y / camRect.height) * (float)(minimapH + minimapY);
                const Color playerColor{ 220, 90, 20, 255 };
                DrawCircle((int)x, (int)y, 2.0f, playerColor);
                const char *pName = SafeTextFormat("%.*s", playerInfo->nameLength, playerInfo->name);
                int nameWidth = MeasureText(pName, font.baseSize);
                DrawTextFont(font, pName, x - (float)(nameWidth / 2), y - (float)font.baseSize - 4, 0, 0, font.baseSize, YELLOW);
            } else {
                E_ERROR(ErrorType::NotFound, "Missing player info for player id %u", player.id);
            }
        }
    }
#endif
    rlDrawRenderBatchActive();
}

void UI::Menubar(void)
{
    if (input->dbgImgui) showMenubar = !showMenubar;
    if (!showMenubar) return;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("Demo Window", 0, &showDemoWindow);
            ImGui::MenuItem("Item Proto Editor", 0, &showItemProtoEditor);
            ImGui::MenuItem("Netstat", 0, &showNetstatWindow);
            ImGui::MenuItem("Particle Config", 0, &showParticleConfig);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void UI::ShowDemoWindow(void)
{
    if (!showDemoWindow) {
        return;
    }

    ImGui::ShowDemoWindow(&showDemoWindow);
}

static void CenterNextItem(const char *text)
{
    float windowContentWidth = ImGui::GetContentRegionAvail().x;
    float textWidth = ImGui::CalcTextSize(text).x;
    float cursorX = ImGui::GetCursorPosX();
    ImGui::SetCursorPosX(cursorX + (windowContentWidth - textWidth) * 0.5f);
}

void UI::MenuTitle(const char *text, const ImVec4 &color)
{
    ImGui::PushFont(g_fonts.imFontHack64);
    ImGui::TextColored(color, text);
    ImGui::Dummy({ 0, 10 });
    ImGui::Separator();
    ImGui::Dummy({ 0, 10 });
    ImGui::PopFont();
}

void UI::SliderFloatLeft(const char *label, float *v, float min, float max)
{
    DLB_ASSERT(label[0] == '#');
    DLB_ASSERT(label[1] == '#');
    ImGui::Text(label + 2);
    ImGui::SameLine();
    float sliderMargin = 20.0f;
    //float cursorX = ImGui::GetCursorPosX();
    //ImGui::SetCursorPosX(ImGui::GetCursorPosX() + sliderMargin);
    float sliderWidth = 500.0f;
    ImGui::Dummy(ImVec2(sliderMargin, 1.0f));
    ImGui::SameLine();

    int styleVars = 0;
    const float rounding = 3.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 0.0f)); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 14.0f)); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 20.0f); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, rounding); styleVars++;
    ImGui::PushItemWidth(sliderWidth);
    ImGui::SliderFloat(label, v, min, max, "%.02f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::PopItemWidth();
    ImGui::PopStyleVar(styleVars);
}

void UI::SliderIntLeft(const char *label, int *v, int min, int max)
{
    DLB_ASSERT(label[0] == '#');
    DLB_ASSERT(label[1] == '#');
    ImGui::Text(label + 2);
    ImGui::SameLine();
    float sliderMargin = 20.0f;
    //float cursorX = ImGui::GetCursorPosX();
    //ImGui::SetCursorPosX(ImGui::GetCursorPosX() + sliderMargin);
    float sliderWidth = ImGui::GetContentRegionAvail().x - sliderMargin * 2.0f;
    ImGui::PushItemWidth(sliderWidth);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 14.0f));
    ImGui::SliderInt(label, v, min, max);
    ImGui::PopStyleVar();
    ImGui::PopItemWidth();
}

void UI::ParticleConfig(World &world)
{
    if (!showParticleConfig) {
        return;
    }

    const ImVec2 inventorySize{ 540.0, 500.0f };
    const float pad = 80.0f;
    const float left = screenSize.x - pad - inventorySize.x;
    const float top = pad;
    ImGui::SetNextWindowPos(ImVec2(left, top), ImGuiCond_Once);
    ImGui::SetNextWindowSize(inventorySize);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);

    auto bgWindow = BLACK;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(bgWindow.r, bgWindow.g, bgWindow.b, 0.7f * 255.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32_BLACK);

    ImGui::Begin("##particle_config", &showParticleConfig,
        //ImGuiWindowFlags_NoDecoration |
        //ImGuiWindowFlags_NoMove |
        //ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoSavedSettings
    );

    thread_local static ParticleEffectParams params{};
    UI::MenuTitle("Particle Effect");
    SliderIntLeft(  "##particleCountMin ", &params.particleCountMin, 1, 256);
    SliderIntLeft(  "##particleCountMax ", &params.particleCountMax, 1, 256);
    SliderFloatLeft("##durationMin      ", &params.durationMin, 0.1f, 10.0f);
    SliderFloatLeft("##durationMax      ", &params.durationMax, 0.1f, 10.0f);
    SliderFloatLeft("##spawnDelayMin    ", &params.spawnDelayMin, 0.0f, 10.0f);
    SliderFloatLeft("##spawnDelayMax    ", &params.spawnDelayMax, 0.0f, 10.0f);
    SliderFloatLeft("##lifespanMin      ", &params.lifespanMin, 0.1f, 10.0f);
    SliderFloatLeft("##lifespanMax      ", &params.lifespanMax, 0.1f, 10.0f);
    SliderFloatLeft("##spawnScaleFirst  ", &params.spawnScaleFirst, 0.1f, 10.0f);
    SliderFloatLeft("##spawnScaleLast   ", &params.spawnScaleLast, 0.1f, 10.0f);
    SliderFloatLeft("##velocityXMin     ", &params.velocityXMin, -10.0f, 10.0f);
    SliderFloatLeft("##velocityXMax     ", &params.velocityXMax, -10.0f, 10.0f);
    SliderFloatLeft("##velocityYMin     ", &params.velocityYMin, -10.0f, 10.0f);
    SliderFloatLeft("##velocityYMax     ", &params.velocityYMax, -10.0f, 10.0f);
    SliderFloatLeft("##velocityZMin     ", &params.velocityZMin, -10.0f, 10.0f);
    SliderFloatLeft("##velocityZMax     ", &params.velocityZMax, -10.0f, 10.0f);
    SliderFloatLeft("##friction         ", &params.friction, 0.0f, 10.0f);
    if (ImGui::Button("Generate")) {
        Player *player = world.FindPlayer(world.playerId);
        if (player) {
            ParticleEffect *blood = world.particleSystem.GenerateEffect(
                Catalog::ParticleEffectID::Blood,
                v3_add(player->body.WorldPosition(), { 0, 0, 40 }),
                params);
            if (blood) {
                blood->effectCallbacks[(size_t)ParticleEffect_Event::BeforeUpdate] = { ParticlesFollowPlayerGut, player };
            }
        }
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    ImGui::End();
}

void UI::Netstat(NetClient &netClient, double renderAt)
{
    if (!showNetstatWindow) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(380, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(360, 100), ImGuiCond_FirstUseEver);
    int styleCols = 0;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0.6f * 255.0f)); styleCols++;
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32_BLACK); styleCols++;
    ImGui::Begin("Network##netstat", &showNetstatWindow,
        0
        //ImGuiWindowFlags_NoTitleBar
        //ImGuiWindowFlags_NoMove |
        //ImGuiWindowFlags_NoResize |
        //ImGuiWindowFlags_NoCollapse
    );

#if CL_DEBUG_SPEEDHAX
    int msecHax = g_inputMsecHax;
    ImGui::Text("InputMsSpeedhax");
    ImGui::SameLine();
    ImGui::SliderInt("##g_inputMsecHax", &msecHax, 0, UINT8_MAX);
    g_inputMsecHax = (uint8_t)msecHax;
#endif

    ImGui::Text("Client-side prediction");
    ImGui::SameLine();
    ImGui::Checkbox("##g_cl_client_prediction", &g_cl_client_prediction);

    ImGui::Text("Show server shadow");
    ImGui::SameLine();
    ImGui::Checkbox("##g_cl_show_snapshot_shadow", &g_cl_show_snapshot_shadow);

    ImGui::Text("Smooth reconcile");
    ImGui::SameLine();
    ImGui::Checkbox("##g_cl_smooth_reconcile", &g_cl_smooth_reconcile);

    ImGui::Text("Reconcile smooth factor");
    ImGui::SameLine();
    ImGui::SliderFloat("##g_cl_smooth_reconcile_factor", &g_cl_smooth_reconcile_factor, 0.01f, 1.0f, "%.02f", ImGuiSliderFlags_AlwaysClamp);

    if (ImGui::Button("Force de-sync")) {
        Player *player = netClient.serverWorld->LocalPlayer();
        if (player) {
            Vector3 worldPos = player->body.WorldPosition();
            worldPos.x += METERS_TO_PIXELS(3.0f);
            worldPos.y += METERS_TO_PIXELS(3.0f);
            player->body.Teleport(worldPos);
        }
    }

    ImGui::NewLine();

    const size_t snapshotCount = netClient.worldHistory.Count();
    if (snapshotCount) {
        DLB_ASSERT(snapshotCount <= CL_WORLD_HISTORY);

        float times[CL_WORLD_HISTORY]{};
        for (size_t i = 0; i < snapshotCount; i++) {
            times[i] = (float)(renderAt - netClient.worldHistory.At(i).clock);
        }
        ImGui::Text("Times:");
        ImGui::PlotHistogram("times", times, (int)snapshotCount, 0, 0, -2.0f, 2.0f, ImVec2(300.0f, 50.0f));

        ImGui::NewLine();

        if (ImGui::CollapsingHeader("Snapshots")) {
            for (int i = 0; i < (int)snapshotCount; i++) {
                const WorldSnapshot &snap = netClient.worldHistory.At(i);
                ImGui::PushID(i);
                if (ImGui::TreeNode("SnapshotNode", "[%zu] Tick #%u", i, snap.tick)) {
                    ImGui::Text("Players (%u)", snap.playerCount);
                    ImGui::Text("NPCs    (%u)", snap.npcCount);
                    ImGui::Text("Items   (%u)", snap.itemCount);
                    ImGui::TreePop();
                    ImGui::Separator();
                }
                ImGui::PopID();
            }
        }
    }

    ImGui::PopStyleColor(styleCols);
    ImGui::End();
}

int UI::ItemCompareWithSortSpecs(const void *lhs, const void *rhs)
{
    const ItemType *aType = (ItemType *)lhs;
    const ItemType *bType = (ItemType *)rhs;
    const ItemProto &a = g_item_catalog.FindProto(*aType);
    const ItemProto &b = g_item_catalog.FindProto(*bType);
    for (int n = 0; n < itemSortSpecs->SpecsCount; n++) {
        // Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
        // We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
        const ImGuiTableColumnSortSpecs *sort_spec = &itemSortSpecs->Specs[n];
        int delta = 0;
        switch (sort_spec->ColumnUserID) {
            case ItemColumn_ID:         delta = (a.itemType   > b.itemType);   break;
            case ItemColumn_StackLimit: delta = (a.stackLimit > b.stackLimit); break;
            case ItemColumn_Name:       delta = strcmp(a.nameSingular, b.nameSingular); break;
            case ItemColumn_NamePlural: delta = strcmp(a.namePlural,   b.namePlural  ); break;
            case ItemColumn_Class:      {
                const char *aClass = ItemClassToString(a.itemClass);
                const char *bClass = ItemClassToString(b.itemClass);
                delta = strcmp(aClass, bClass);
                break;
            }
            case ItemColumn_Value: {
                float aValue = a.FindAffixProto(ItemAffix_Value).minRange.min;
                float bValue = b.FindAffixProto(ItemAffix_Value).minRange.min;
                delta = (aValue > bValue);
                break;
            }
            case ItemColumn_DamageFlatMin: {
                float aDamage = a.FindAffixProto(ItemAffix_DamageFlat).minRange.min;
                float bDamage = b.FindAffixProto(ItemAffix_DamageFlat).minRange.min;
                delta = (aDamage > bDamage);
                break;
            }
            case ItemColumn_DamageFlatMax: {
                float aDamage = a.FindAffixProto(ItemAffix_DamageFlat).minRange.max;
                float bDamage = b.FindAffixProto(ItemAffix_DamageFlat).minRange.max;
                delta = (aDamage > bDamage);
                break;
            }
            default:
                E_WARN("Unknown column id", 0);
        }
        if (delta > 0)
            return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? +1 : -1;
        if (delta < 0)
            return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? -1 : +1;
    }
    return (a.itemType > b.itemType);
}

void UI::ItemProtoEditor(World &world)
{
    UNUSED(world);

    if (!showItemProtoEditor) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(380, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(360, 100), ImGuiCond_FirstUseEver);
    ImGui::Begin("Item Proto Editor", &showItemProtoEditor,
        0
        //ImGuiWindowFlags_NoTitleBar
        //ImGuiWindowFlags_NoMove |
        //ImGuiWindowFlags_NoResize |
        //ImGuiWindowFlags_NoCollapse
    );

    static ImGuiTableFlags flags =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
        | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_NoSavedSettings;

    static std::vector<ItemType> virtualItems{};
    if (!virtualItems.size()) {
        virtualItems.reserve(ItemType_Count);
        for (ItemType typ = 0; typ < ItemType_Count; typ++) {
            virtualItems.push_back(typ);
        }
    }

    const int columnCount = 9;
    if (ImGui::BeginTable("items", columnCount, flags)) {
        ImGui::TableSetupColumn("Actions",       ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
        ImGui::TableSetupColumn("ID",            ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort, 40.0f, ItemColumn_ID);
        ImGui::TableSetupColumn("Class",         ImGuiTableColumnFlags_WidthFixed, 0.0f, ItemColumn_Class);
        ImGui::TableSetupColumn("Name",          ImGuiTableColumnFlags_WidthFixed, 0.0f, ItemColumn_Name);
        ImGui::TableSetupColumn("Name Plural",   ImGuiTableColumnFlags_WidthFixed, 0.0f, ItemColumn_NamePlural);
        ImGui::TableSetupColumn("Stack Limit",   ImGuiTableColumnFlags_WidthFixed, 0.0f, ItemColumn_StackLimit);
        ImGui::TableSetupColumn("Value",         ImGuiTableColumnFlags_WidthFixed, 0.0f, ItemColumn_Value);
        ImGui::TableSetupColumn("DamageFlatMin", ImGuiTableColumnFlags_WidthFixed, 0.0f, ItemColumn_DamageFlatMin);
        ImGui::TableSetupColumn("DamageFlatMax", ImGuiTableColumnFlags_WidthFixed, 0.0f, ItemColumn_DamageFlatMax);
        ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
        ImGui::TableHeadersRow();

        if (ImGuiTableSortSpecs *sorts_specs = ImGui::TableGetSortSpecs()) {
            if (sorts_specs->SpecsDirty) {
                itemSortSpecs = sorts_specs; // Store in variable accessible by the sort function.
                qsort((void *)virtualItems.data(), virtualItems.size(), sizeof(virtualItems[0]), ItemCompareWithSortSpecs);
                itemSortSpecs = NULL;
                sorts_specs->SpecsDirty = false;
            }
        }

        for (ItemType typ : virtualItems) {
            ItemProto &proto = g_item_catalog.FindProto(typ);
            ImGui::PushID(typ);
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            if (ImGui::Button("Spwn")) {
                // TODO: Send command to server to spawn (via chat?)
                //ItemUID uid = g_item_db.SV_Spawn(proto.itemType);
                //world.itemSystem.SpawnItem(world.LocalPlayer()->body.WorldPosition(), uid, 1);
            } else if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted("Spawn this item into the world");
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            ImGui::TableNextColumn();
            ImGui::Text("%d", proto.itemType);

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::BeginCombo("##item_class", ItemClassToString(proto.itemClass))) {
                for (ItemClass cls = 0; cls < ItemClass_Count; cls++) {
                    ImGui::PushID(cls);
                    if (ImGui::Selectable(ItemClassToString(cls), cls == proto.itemClass)) {
                        proto.itemClass = cls;
                    }
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(320.0f);
            ImGui::InputText("##name", CSTR0(proto.nameSingular));

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(320.0f);
            ImGui::InputText("##name_plural", CSTR0(proto.namePlural));

            int stackLimit = proto.stackLimit;
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(120.0f);
            ImGui::InputInt("##stack_limit", &stackLimit, 1, 5);
            proto.stackLimit = CLAMP(stackLimit, 1, UINT8_MAX);

            float value = proto.FindAffixProto(ItemAffix_Value).minRange.min;
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(120.0f);
            ImGui::InputFloat("##value", &value, 1.0f, 5.0f, "%.0f");
            value = MAX(0, value);
            proto.SetAffixProto(ItemAffix_Value, { value, value });

            ItemAffixProto damageFlatAffix = proto.FindAffixProto(ItemAffix_DamageFlat);
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(120.0f);
            ImGui::InputFloat("##damage_flat_min", &damageFlatAffix.minRange.min, 1.0f, 5.0f, "%.0f");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(120.0f);
            ImGui::InputFloat("##damage_flat_max", &damageFlatAffix.minRange.max, 1.0f, 5.0f, "%.0f");
            damageFlatAffix.minRange.min = CLAMP(damageFlatAffix.minRange.min, 0, 999);
            damageFlatAffix.minRange.max = CLAMP(damageFlatAffix.minRange.max, 0, 999);
            proto.SetAffixProto(ItemAffix_DamageFlat, damageFlatAffix.minRange);

            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

void UI::HUD(const Font &font, World &world, const DebugStats &debugStats)
{
    DLB_ASSERT(spycam);

    Player *player = world.FindPlayer(world.playerId);
    if (!player) {
        return;
    }

    const char *text = 0;
    float hudCursorY = 0;

#define PUSH_TEXT(text, color) \
    DrawTextFont(font, text, margin + pad, hudCursorY, 0, 0, font.baseSize, color); \
    hudCursorY += font.baseSize + pad;

    int linesOfText = 9;
    if (debugStats.tick) {
        linesOfText += 3;
    }
    if (debugStats.rtt) {
        linesOfText += 7;
    }
    const float margin = 6.0f;   // margin
    const float pad = 4.0f;      // pad
    const float hudWidth = 280.0f;
    const float hudHeight = (float)linesOfText * ((float)font.baseSize + pad) + pad;

    hudCursorY += margin;

    const Color darkerGray{ 40, 40, 40, 255 };
    UNUSED(darkerGray);
    DrawRectangle((int)margin, (int)hudCursorY, (int)hudWidth, (int)hudHeight, Fade(BLACK, 0.6f));
    DrawRectangleLines((int)margin, (int)hudCursorY, (int)hudWidth, (int)hudHeight, BLACK);

    hudCursorY += pad;

    text = SafeTextFormat("%2i fps %.03f ms %u ticks", GetFPS(), debugStats.frameDt * 1000.0f, debugStats.tick);
    PUSH_TEXT(text, LIGHTGRAY);
    text = SafeTextFormat("Kilometers walked %.2f", player->stats.kmWalked);
    PUSH_TEXT(text, WHITE);
    text = SafeTextFormat("Damage dealt      %.2f", player->stats.damageDealt);
    PUSH_TEXT(text, RED);
    text = SafeTextFormat("Times fist swung  %u", player->stats.timesFistSwung);
    PUSH_TEXT(text, RED);
    text = SafeTextFormat("Times sword swung %u", player->stats.timesSwordSwung);
    PUSH_TEXT(text, RED);
    text = SafeTextFormat("Slimes slain      %u", player->stats.npcsSlain[NPC::Type_Slime]);
    PUSH_TEXT(text, GREEN);

    const Vector2 playerBC = player->body.GroundPosition();
    text = SafeTextFormat("World: %0.2f, %0.2f", playerBC.x, playerBC.y);
    PUSH_TEXT(text, LIGHTGRAY);
    const int16_t playerChunkX = world.map.CalcChunk(playerBC.x);
    const int16_t playerChunkY = world.map.CalcChunk(playerBC.y);
    const int16_t playerTileX = world.map.CalcChunkTile(playerBC.x);
    const int16_t playerTileY = world.map.CalcChunkTile(playerBC.y);
    text = SafeTextFormat("Chunk: %d, %d", playerChunkX, playerChunkY);
    PUSH_TEXT(text, LIGHTGRAY);
    text = SafeTextFormat("Tile:  %d, %d", playerTileX, playerTileY);
    PUSH_TEXT(text, LIGHTGRAY);

    if (debugStats.tick) {
        text = SafeTextFormat("client.now   %.03f", g_clock.now);
        PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("server.now   %.03f", g_clock.serverNow);
        PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("timeOfDay     %s (%.03f)", g_clock.timeOfDayStr(), g_clock.timeOfDay);
        PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("daylight      %.03f", g_clock.daylightPerc);
        PUSH_TEXT(text, GRAY);
        //text = SafeTextFormat("Camera speed  %.03f", debugStats.cameraSpeed);
        //PUSH_TEXT(text, GRAY);
        //text = SafeTextFormat("Zoom          %.03f", spycam->GetZoom());
        //PUSH_TEXT(text, GRAY);
        //text = SafeTextFormat("Zoom inverse  %.03f", spycam->GetInvZoom());
        //PUSH_TEXT(text, GRAY);
        //text = SafeTextFormat("Zoom mip      %d", spycam->GetZoomMipLevel());
        //PUSH_TEXT(text, GRAY);
        //text = SafeTextFormat("Tiles visible %zu", debugStats.tilesDrawn);
        //PUSH_TEXT(text, GRAY);
        //text = SafeTextFormat("Particle FX   %zu", debugStats.effectsActive);
        //PUSH_TEXT(text, GRAY);
        //text = SafeTextFormat("Particles     %zu", debugStats.particlesActive);
        //PUSH_TEXT(text, GRAY);
    }

    thread_local static uint64_t bytesSentStart = 0;
    thread_local static uint64_t bytesRecvStart = 0;
    thread_local static double bandwidthTimerStartedAt = 0;
    thread_local static float kbpsOut = 0.0f;
    thread_local static float kbpsIn = 0.0f;

    if (debugStats.rtt) {
        if (!bandwidthTimerStartedAt) {
            bandwidthTimerStartedAt = GetTime();
        }
        if (GetTime() - bandwidthTimerStartedAt >= 1.0) {
            kbpsOut = (float)(debugStats.bytes_sent - bytesSentStart) / 1024.0f;
            kbpsIn = (float)(debugStats.bytes_recv - bytesRecvStart) / 1024.0f;
            bandwidthTimerStartedAt = GetTime();
            bytesSentStart = debugStats.bytes_sent;
            bytesRecvStart = debugStats.bytes_recv;
        }
        text = SafeTextFormat("Ping          %u", debugStats.rtt);
        PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("Packets sent  %llu", debugStats.packets_sent);
        PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("Packets lost  %u", debugStats.packets_lost);
        PUSH_TEXT(text, GRAY);
        //text = SafeTextFormat("Bytes sent    %llu", debugStats.bytes_sent);
        //PUSH_TEXT(text, GRAY);
        //text = SafeTextFormat("Bytes recv    %llu", debugStats.bytes_recv);
        //PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("Up         %8.03f kbps", kbpsOut);
        PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("Down       %8.03f kbps", kbpsIn);
        PUSH_TEXT(text, GRAY);
        //text = SafeTextFormat("CL seq        %u", debugStats.cl_input_seq);
        //PUSH_TEXT(text, GRAY);
        //text = SafeTextFormat("SV seq ack    %u", debugStats.sv_input_ack);
        //PUSH_TEXT(text, GRAY);
        text = SafeTextFormat("input bufd    %u", debugStats.input_buf_size);
        PUSH_TEXT(text, GRAY);
    } else {
        bytesSentStart = 0;
        bytesRecvStart = 0;
        bandwidthTimerStartedAt = 0;
        kbpsOut = 0.0f;
        kbpsIn = 0.0f;
    }

#if 0
    // TODO: Keep track of list of other clients in GameClient
    // TODO: Keep track of who is the host in NetServer (is host==127.0.0.1 sufficient?)
    // Render connected clients
    if (args.server) {
        int linesOfText = 1 + (int)gameServer.clients.size();

        const float margin = 6.0f;   // margin
        const float pad = 4.0f;      // pad
        const float hudWidth = 200.0f;
        const float hudHeight = linesOfText * (fontHeight + pad) + pad;

        hudCursorY += margin;

        const Color darkerGray{ 40, 40, 40, 255 };
        UNUSED(darkerGray);
        DrawRectangle((int)margin, (int)hudCursorY, (int)hudWidth, (int)hudHeight, DARKBLUE);
        DrawRectangleLines((int)margin, (int)hudCursorY, (int)hudWidth, (int)hudHeight, BLACK);

        hudCursorY += pad;

        PUSH_TEXT("Connected clients:", WHITE);

        for (auto &kv : gameServer.clients) {
            text = SafeTextFormatIP(kv.second.address);
            PUSH_TEXT(text, WHITE);
        }
    }
#endif
#undef PUSH_TEXT

    rlDrawRenderBatchActive();

    const float xpBarWindowPad = 4.0f;
    const float levelDiamondRadius = 18.0f;
    ImVec2 xpBarWindowSize{ 512, levelDiamondRadius * 2.0f + xpBarWindowPad * 2.0f };
    ImVec2 xpBarWindowCenter{ screenSize.x / 2.0f, 2.0f };
    ImGui::SetNextWindowPos(xpBarWindowCenter, 0, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(xpBarWindowSize);

    int styleVars = 0;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(xpBarWindowPad, xpBarWindowPad)); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f); styleVars++;
    int styleCols = 0;
    //ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32_BLACK); styleCols++;
    ImGui::PushStyleColor(ImGuiCol_Border, ImColor(0, 0, 0, 255).Value); styleCols++;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImColor(128, 128, 0, 255).Value); styleCols++;
    ImGui::PushFont(g_fonts.imFontHack16);

    ImGui::Begin("##hud_xp_bar", 0,
        //ImGuiWindowFlags_NoTitleBar |
        //ImGuiWindowFlags_NoResize |
        //ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoSavedSettings
    );

    ImDrawList *drawList = ImGui::GetWindowDrawList();

    const ImVec2 topLeft {
        ImGui::GetItemRectMin().x + ImGui::GetWindowContentRegionMin().x,
        ImGui::GetItemRectMin().y + ImGui::GetWindowContentRegionMin().y
    };
    const ImVec2 bottomRight {
        ImGui::GetItemRectMin().x + ImGui::GetWindowContentRegionMax().x,
        ImGui::GetItemRectMin().y + ImGui::GetWindowContentRegionMax().y
    };
    ImVec2 levelDiamondPos{
        topLeft.x + 8.0f + levelDiamondRadius * 0.5f,
        topLeft.y + 8.0f + levelDiamondRadius * 0.5f
    };
    const float xpBarBgHeight = 16.0f;
    ImVec2 xpBarBgTopLeft {
        levelDiamondPos.x - 8.0f + levelDiamondRadius,
        levelDiamondPos.y + 8.0f
    };
    ImVec2 xpBarBgBottomRight {
        bottomRight.x,
        xpBarBgTopLeft.y + xpBarBgHeight * 0.5f
    };

    // XP bar gray background
    const float xpBarRadius = 4.0f;
    drawList->AddRectFilled(xpBarBgTopLeft, xpBarBgBottomRight, ImColor(0.7f, 0.7f, 0.7f, 1.0f), xpBarRadius);

    // XP bar green bar
    uint32_t xpNextLevel = player->combat.level * 20u;
    float xpProgress = (float)player->xp / xpNextLevel;
    //xpProgress = sinf((float)g_clock.now) / 2.0f + 0.5f;
    if (xpProgress) {
        const float xpEarnedWidth = MAX(6.0f, xpProgress * (xpBarBgBottomRight.x - xpBarBgTopLeft.x));
        //const ImColor xpBarColor = ImColor(140, 230, 30, 255);
        const ImColor xpBarColor = ImColor(184, 132, 30, 255);
        drawList->AddRectFilled(xpBarBgTopLeft, ImVec2(xpBarBgTopLeft.x + xpEarnedWidth, xpBarBgBottomRight.y), xpBarColor, xpBarRadius);
    }

    // XP bar black outline
    drawList->AddRect(xpBarBgTopLeft, xpBarBgBottomRight, IM_COL32_BLACK, xpBarRadius, 0, 2.0f);

    // XP bar level diamond
    drawList->AddNgonFilled(levelDiamondPos, levelDiamondRadius, IM_COL32_BLACK, 4);

    // XP bar level diamond outline
    drawList->AddNgon(levelDiamondPos, levelDiamondRadius, ImColor(184, 132, 30, 255), 4, 2.0f);

    // XP bar level diamond text
    char levelDiamondTextBuf[32]{};
    uint8_t level = player->combat.level;
    //level = (uint8_t)((sin(g_clock.now) / 2.0f + 0.5f) * UINT8_MAX);
    snprintf(CSTR0(levelDiamondTextBuf), "%u", level);
    ImVec2 levelDiamondTextPos{
        topLeft.x - 1.0f + levelDiamondRadius * 0.5f,
        topLeft.y + 1.0f + levelDiamondRadius * 0.5f
    };
    if (level < 10) {
        levelDiamondTextPos.x += 4.0f;
    } else if (level > 99) {
        levelDiamondTextPos.x -= 4.0f;
    }
    drawList->AddText(levelDiamondTextPos, IM_COL32_WHITE, levelDiamondTextBuf);

    //ImGui::LabelText("##hud_xp_text", "%u / %u", player->xp, xpNextLevel);

    //char buf[32]{};
    //snprintf(buf, sizeof(buf), "%u / %u", player->xp, xpNextLevel);
    //ImGui::ProgressBar(xpProgress, ImVec2(-FLT_MIN, 0), 0); //buf);

    ImGui::End();
    ImGui::PopFont();
    ImGui::PopStyleColor(styleCols);
    ImGui::PopStyleVar(styleVars);
}

void UI::QuickHUD(const Font &font, const Player &player, const Tilemap &tilemap)
{
    const char *text = 0;

    //int linesOfText = 1;
    const float margin = 6.0f;   // margin
    const float pad = 4.0f;      // pad
    //const float hudWidth = 240.0f;
    //const float hudHeight = linesOfText * (font.baseSize + pad) + pad;

    float hudCursorY = margin;
    hudCursorY += pad;

    const Spritesheet &coinSpritesheet = Catalog::g_spritesheets.FindById(Catalog::SpritesheetID::Item_Coins);
    const SpriteDef *coinGildedSpriteDef = coinSpritesheet.FindSprite("coin_gilded");
    Rectangle frameRect{};
    frameRect.x      = (float)coinGildedSpriteDef->spritesheet->frames[3].x;
    frameRect.y      = (float)coinGildedSpriteDef->spritesheet->frames[3].y;
    frameRect.width  = (float)coinGildedSpriteDef->spritesheet->frames[3].width;
    frameRect.height = (float)coinGildedSpriteDef->spritesheet->frames[3].height;
    DrawTextureRec(coinGildedSpriteDef->spritesheet->texture, frameRect, { margin + pad, hudCursorY }, WHITE);

    text = SafeTextFormat("%d", player.inventory.slots[PlayerInventory::SlotId_Coin_Copper].stack.count);
    DrawTextFont(font, text, margin + pad + frameRect.width + pad, hudCursorY, 0, 0, font.baseSize, WHITE);
    hudCursorY += font.baseSize + pad;

    const Vector2 playerBC = player.body.GroundPosition();
    const int16_t playerChunkX = tilemap.CalcChunk(playerBC.x);
    const int16_t playerChunkY = tilemap.CalcChunk(playerBC.y);
    const int16_t playerTileX = tilemap.CalcChunkTile(playerBC.x);
    const int16_t playerTileY = tilemap.CalcChunkTile(playerBC.y);

    text = SafeTextFormat("world: %0.2f %0.2f", playerBC.x, playerBC.y);
    DrawTextFont(font, text, margin + pad + frameRect.width + pad, hudCursorY, 0, 0, font.baseSize, WHITE);
    hudCursorY += font.baseSize + pad;

    text = SafeTextFormat("chunk: %7d %7d", playerChunkX, playerChunkY);
    DrawTextFont(font, text, margin + pad + frameRect.width + pad, hudCursorY, 0, 0, font.baseSize, WHITE);
    hudCursorY += font.baseSize + pad;

    text = SafeTextFormat(" tile: %7d %7d", playerTileX, playerTileY);
    DrawTextFont(font, text, margin + pad + frameRect.width + pad, hudCursorY, 0, 0, font.baseSize, WHITE);
    hudCursorY += font.baseSize + pad;
}

// NOTE: ImGui trolls us by auto-selecting all text (select_all) when a textbox is focused
// via code (SetKeyboardFocusHere), so this callback will detect a newly opened chatbox
// opened via '/' and deselect the slash + ensure cursor is to the right of it.
static int ChatInputFixSlash(ImGuiInputTextCallbackData *data) {
    bool *skipSlashSelectAll = (bool *)data->UserData;
    if (data->BufTextLen == 1 && skipSlashSelectAll && *skipSlashSelectAll) {
        data->ClearSelection();
        data->CursorPos = 1;
        *skipSlashSelectAll = false;
    }
    return 0;
}

void UI::Chat(const Font &font, int fontSize, World &world, NetClient &netClient, bool &escape)
{
    const char *chatPopupId = "ChatInput";
    const bool chatActive = ImGui::IsPopupOpen(chatPopupId);

    // Render chat history
    const float margin = 6.0f;   // left/bottom margin
    const float pad = 4.0f;      // left/bottom pad
    const float inputBoxHeight = fontSize + pad * 2.0f;
    const float chatWidth = 800.0f;
    const float chatBottom = screenSize.y - margin - inputBoxHeight;
    world.chatHistory.Render(font, fontSize, world, margin, chatBottom, chatWidth, chatActive);
    rlDrawRenderBatchActive();

    // Render chat input box, if open
    thread_local static char chatInputText[CHATMSG_LENGTH_MAX]{};

    ImVec2 chatInputPos{
        margin,
        screenSize.y - margin - inputBoxHeight
    };
    ImGui::SetNextWindowPos(chatInputPos);

    thread_local static bool fixSlash = false;
    if (!ImGui::IsPopupOpen(chatPopupId) && (input->openChatTextbox || input->openChatTextboxSlash)) {
        ImGui::OpenPopup(chatPopupId);
        if (input->openChatTextboxSlash) {
            chatInputText[0] = '/';
            fixSlash = true;
        }
    }

    int styleVars = 0;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); styleVars++;
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0, 0, 0, 0));
    bool chatOpen = ImGui::BeginPopupModal(chatPopupId, 0,
        //ImGuiWindowFlags_NoTitleBar |
        //ImGuiWindowFlags_NoResize |
        //ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoSavedSettings
    );
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(styleVars);

    if (chatOpen) {
        if (!ImGui::IsAnyItemFocused()) {
            ImGui::SetKeyboardFocusHere(0);
        }
        ImGui::PushItemWidth(chatWidth);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, CHAT_BG_ALPHA));
        const char *chatInputId = "##ui_chat_input";
        if (ImGui::InputText(chatInputId, chatInputText, sizeof(chatInputText), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways, ChatInputFixSlash, &fixSlash)) {
            if (chatInputText[0]) {
                size_t chatInputTextLen = strnlen(CSTR0(chatInputText));
                ErrorType sendResult = netClient.SendChatMessage(chatInputText, chatInputTextLen);
                if (sendResult == ErrorType::NotConnected) {
                    //world.chatHistory.PushSam(CSTR("You're not connected to a server. Nobody is listening. :("));
                    world.chatHistory.PushPlayer(world.playerId, chatInputText, chatInputTextLen);
                }
                memset(chatInputText, 0, sizeof(chatInputText));
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::PopStyleColor();

        if (escape) {
            ImGui::CloseCurrentPopup();
            memset(chatInputText, 0, sizeof(chatInputText));
            escape = false;
        }

        ImGui::EndPopup();
    }
}

void UI::TileHoverTip(const Font &font, Tilemap &map)
{
    const Tile *mouseTile = map.TileAtWorld(mouseWorld.x, mouseWorld.y);
    if (!mouseTile) {
        return;
    }

    const float tooltipOffsetX = 10.0f;
    const float tooltipOffsetY = 10.0f;
    const float tooltipPad = 4.0f;

    float tooltipX = mouseScreen.x + tooltipOffsetX;
    float tooltipY = mouseScreen.y + tooltipOffsetY;
    const float tooltipW = 220.0f + tooltipPad * 2.0f;
    const float tooltipH = 78.0f + tooltipPad * 2.0f;

    if (tooltipX + tooltipW > screenSize.x) tooltipX = screenSize.x - tooltipW;
    if (tooltipY + tooltipH > screenSize.y) tooltipY = screenSize.y - tooltipH;

    Rectangle tooltipRect{ tooltipX, tooltipY, tooltipW, tooltipH };
    DrawRectangleRec(tooltipRect, Fade(DARKGRAY, 0.8f));
    DrawRectangleLinesEx(tooltipRect, 1, Fade(BLACK, 0.8f));

    const int mouseChunkX = map.CalcChunk(mouseWorld.x);
    const int mouseChunkY = map.CalcChunk(mouseWorld.y);
    const int mouseTileX = map.CalcChunkTile(mouseWorld.x);
    const int mouseTileY = map.CalcChunkTile(mouseWorld.y);

    int lineOffset = 0;
    DrawTextFont(font, SafeTextFormat("chunk: %d, %d", mouseChunkX, mouseChunkY),
        tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, 0, 0, font.baseSize, WHITE);
    lineOffset += font.baseSize;
    DrawTextFont(font, SafeTextFormat("tile : %d, %d", mouseTileX, mouseTileY),
        tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, 0, 0, font.baseSize, WHITE);
    lineOffset += font.baseSize;
    DrawTextFont(font, SafeTextFormat("type : %d", mouseTile->type),
        tooltipX + tooltipPad, tooltipY + tooltipPad + lineOffset, 0, 0, font.baseSize, WHITE);
    lineOffset += font.baseSize;
}

int UI::OldRaylibMenu(const Font &font, const char **items, size_t itemCount)
{
    // TODO: Better animations/audio
    // Animations: https://www.youtube.com/watch?v=WjyP7cvZ1Ns
    // Audio: https://www.youtube.com/watch?v=5ewvDbt5U3c

    int itemPressed = -1;

    Vector2 menuPos{};
    Vector2 menuSize{};
    Vector2 menuPad{ 60.0f, 40.0f };
    const float menuSpacingY = 20.0f;
    const float hitboxPadY = 4.0f;

    struct MenuItem {
        const char *text;
        Vector2 size;
        Vector2 offset;  // offset itemX from center line and itemY from xpBarTop
    } menuItems[UI_MENU_ITEMS_MAX]{};
    DLB_ASSERT(itemCount < ARRAY_SIZE(menuItems));

    Vector2 cursor{};
    cursor.y = menuPad.y;
    menuSize.y += menuPad.y;
    for (int i = 0; i < (int)itemCount; i++) {
        menuItems[i].text = items[i];
        menuItems[i].size = MeasureTextEx(font, menuItems[i].text, (float)font.baseSize, font.baseSize/10.0f);
        menuItems[i].offset.x = -menuItems[i].size.x / 2.0f;
        menuItems[i].offset.y = cursor.y;
        cursor.y += menuItems[i].size.y + menuSpacingY;
        menuSize.x = MAX(menuSize.x, menuItems[i].size.x + menuPad.x * 2);
    }
    cursor.y -= menuSpacingY;
    cursor.y += menuPad.y;
    menuSize.y = cursor.y;

    menuPos.x = (screenSize.x - menuSize.x) / 2.0f;
    menuPos.y = (screenSize.y - menuSize.y) / 2.0f;

    Rectangle menuRect{ menuPos.x, menuPos.y, menuSize.x, menuSize.y };
    DrawRectangleRec(menuRect, { 130, 170, 240, 255 }); // Fade(BLACK, 0.7f));
    DrawRectangleLinesEx(menuRect, 2.0f, BLACK);
    //const float menuCenterX = menuPos.x + menuSize.x / 2.0f;
    for (int i = 0; i < (int)itemCount; i++) {
        Vector2 itemPos{};
        //itemPos.x = menuCenterX + menuItems[i].offset.x;
        itemPos.x = menuPos.x + menuPad.x;
        itemPos.y = menuPos.y + menuItems[i].offset.y;
        Rectangle hitbox{ menuPos.x + menuPad.x, itemPos.y - hitboxPadY, menuSize.x - menuPad.x * 2, menuItems[i].size.y + hitboxPadY * 2 };
        bool hovered = PointInRect(mouseScreen, hitbox);
        bool pressed = hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
        if (pressed) {
            itemPressed = i;
        }
        int size = font.baseSize;
        float offsetX = 3.0f;
        float offsetY = 3.0f;
        const float ox = 3.0f;
        const float oy = 5.0f;
        offsetX += CLAMP((mouseScreen.x - (hitbox.x + hitbox.width / 2.0f)) * ox / hitbox.width, -ox, ox);
        offsetY += CLAMP((mouseScreen.y - (hitbox.y + hitbox.height / 2.0f)) * oy / hitbox.height, -oy, oy);

        if (pressed || hovered) {
            itemPos.x += 6.0f;
            offsetX = 3.0f;
            offsetY = 3.0f;
        }
        //DrawRectangleRec(hitbox, Fade(RED, 0.3f + 0.3f * i));
        //DrawRectangleRec({ itemPos.x, itemPos.y, menuItems[i].size.x, menuItems[i].size.y }, Fade(GREEN, 0.3f + 0.3f * i));
        DrawTextFont(font, menuItems[i].text, itemPos.x, itemPos.y, offsetX, offsetY, size, hovered ? pressed ? RED : YELLOW : WHITE);
    }

    return itemPressed;
}

bool UI::BreadcrumbButton(const char *label)
{
    bool pressed = ImGui::Button(label);
    if (ImGui::IsItemHovered()) {
        UI::hoverLabel = label;
    }
    //pressed = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    return pressed;
}

void UI::Breadcrumbs(void)
{
    MenuID breadcrumbClicked = Menu_Null;
    for (int i = 0; i < menuStack.Count() - 1; i++) {
        const Menu &menu = menuStack.Get(i);

        if (UI::BreadcrumbButton(menu.name)) {
            breadcrumbClicked = menu.id;
        }

        ImGui::SameLine();
        ImGui::Text(">");
        ImGui::SameLine();
    }

    ImGui::BeginDisabled(true);
    UI::BreadcrumbButton(menuStack.Last().name);
    ImGui::EndDisabled();

    if (breadcrumbClicked) {
        menuStack.BackTo(breadcrumbClicked);
    }
}

bool UI::MenuButton(const char *label, const ImVec2 &size)
{
    bool pressed = ImGui::Button(label, size);
    if (ImGui::IsItemHovered()) {
        UI::hoverLabel = label;
    }
    //pressed = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    return pressed;
}

void UI::MenuMultiplayer(GameClient &game) {
    thread_local static const char *message = 0;
    thread_local static bool msgIsError = false;
    thread_local static bool connecting = false;
    thread_local static size_t connectingIdx = 0;
    thread_local static double connectingIdxLastUpdate = 0;
    thread_local static bool reset = false;

    if (game.netClient.IsDisconnected()) {
        if (connecting) {
            message = "DandyNet is offline. :(";
            msgIsError = true;
        }

        if (message) {
            if (msgIsError) ImGui::PushStyleColor(ImGuiCol_Text, ImColor(220, 0, 0, 255).Value);
            //CenterNextItem(message);
            ImGui::Text(message);
            if (msgIsError) ImGui::PopStyleColor(1);
            if (ImGui::Button("Okay")) {
                reset = true;
            }
        } else {
            const DB::ServerDB *serverDB = game.netClient.server_db.flat;
            auto servers = serverDB->servers();

            // Display user'npc saved servers
            ErrorType err = ErrorType::Success;
            int deleteServerIdx = -1;
            ImGui::PushFont(g_fonts.imFontHack48);
            for (int serverIdx = 0; servers && serverIdx < (int)servers->size(); serverIdx++) {
                const DB::Server *server = servers->Get(serverIdx);
                if (UI::MenuButton((const char *)server->desc()->data())) {
                    uint16_t port = server->port() ? server->port() : SV_DEFAULT_PORT;
                    E_INFO("Attempting to connect to server: %s\n%s@%s:%hu",
                        server->desc()->data(),
                        server->user()->data(),
                        server->host()->data(), port
                    );
                    err = game.netClient.Connect(
                        (const char *)server->host()->data(), port,
                        (const char *)server->user()->data(),
                        (const char *)server->pass()->data()
                    );
                }
                ImGui::SameLine();
                if (UI::MenuButton(">", { 0, 0 })) {
                    editingServerIdx = serverIdx;
                    menuStack.Push(Menu_Multiplayer_EditServer, "Edit Server", "Edit Server");
                }
                ImGui::SameLine();
                if (UI::MenuButton("x", { 0, 0 })) {
                    deleteServerIdx = serverIdx;
                }
            }
            ImGui::PopFont();

            if (deleteServerIdx >= 0) {
                game.netClient.server_db.Delete(deleteServerIdx);
                game.netClient.server_db.Save();
            }

            if (err != ErrorType::Success) {
                TraceLog(LOG_ERROR, "Failed to connect to server");
            }

            if (UI::MenuButton("[+] Add Server")) {
                editingServerIdx = -1;
                menuStack.Push(Menu_Multiplayer_EditServer, "Add Server", "Add Server");
            }
        }

        if (menuStack.Changed()) {
            reset = true;
        }
    } else if (game.netClient.IsConnecting()) {
        const char *text[]{
            "Attempting to connect   ",
            "Attempting to connect.  ",
            "Attempting to connect.. ",
            "Attempting to connect...",
        };
        message = text[connectingIdx];
        connecting = true;
        if (g_clock.now - connectingIdxLastUpdate > 0.25) {
            connectingIdx = (connectingIdx + 1) % ARRAY_SIZE(text);
            connectingIdxLastUpdate = g_clock.now;
        }

        if (message) {
            if (msgIsError) ImGui::PushStyleColor(ImGuiCol_Text, ImColor(220, 0, 0, 255).Value);
            //CenterNextItem(message);
            ImGui::Text(message);
            if (msgIsError) ImGui::PopStyleColor(1);
        } else {
            ImGui::Text("");
        }

        const bool cancel = ImGui::Button("Cancel");
        if (cancel || menuStack.Changed()) {
            disconnectRequested = true;
            reset = true;
        }
    } else if (game.netClient.IsConnected()) {
        reset = true;
    }

    if (reset) {
        message = 0;
        msgIsError = false;
        connecting = false;
        connectingIdx = 0;
        reset = false;
    }
}

void UI::MenuMultiplayerNew(NetClient &netClient) {
    DLB_ASSERT(editingServerIdx != SERVER_IDX_NONE);

    thread_local static const char *message = 0;
    thread_local static bool showPassword = false;
    bool formValid = true;

    thread_local static struct {
        char desc[SERV_DESC_LENGTH_MAX];
        char host[HOSTNAME_LENGTH_MAX];
        uint16_t port;
        char user[USERNAME_LENGTH_MAX];
        char pass[PASSWORD_LENGTH_MAX];
    } form{};

    if (!form.port) {
        if (editingServerIdx == SERVER_IDX_NEW) {
            form.port = SV_DEFAULT_PORT;
        } else if(editingServerIdx < (int)netClient.server_db.Size()) {
            const DB::Server &server = *netClient.server_db.flat->servers()->Get(editingServerIdx);
            strncpy(form.desc, server.desc()->c_str(), sizeof(form.desc));
            strncpy(form.host, server.host()->c_str(), sizeof(form.host));
            form.port = server.port();
            strncpy(form.user, server.user()->c_str(), sizeof(form.user));
            strncpy(form.pass, server.pass()->c_str(), sizeof(form.pass));
        } else {
            E_ERROR(ErrorType::OutOfBounds, "Invalid server list index", 0);
        }
    }

    int styleVars = 0;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 4 }); styleVars++;

    ImGui::Text("Description ");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputText("##multiplayer_new_desc", CSTR0(form.desc));

    ImGui::Text("Host / IP   ");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (menuStack.ChangedSinceLastFrame()) {
        ImGui::SetKeyboardFocusHere();
    }
    ImGui::InputText("##multiplayer_new_host", CSTR0(form.host)); //, ImGuiInputTextFlags_Password | ImGuiInputTextFlags_ReadOnly);

    ImGui::Text("Port #      ");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    int portInt = form.port;
    ImGui::InputInt("##multiplayer_new_port", &portInt, 1, 100); //, ImGuiInputTextFlags_ReadOnly);
    form.port = CLAMP(portInt, 1, USHRT_MAX);

    ImGui::Text("Username    ");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputText("##multiplayer_new_user", CSTR0(form.user));

    ImGui::Text("Password    ");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 85);
    ImGui::InputText("##multiplayer_new_pass", CSTR0(form.pass), showPassword ? 0 : ImGuiInputTextFlags_Password);
    ImGui::SameLine();
    if (ImGui::Button(showPassword ? "***" : "ABC", { 85, 0 })) {
        showPassword = !showPassword;
    }

    ImGui::PopStyleVar(styleVars);

    message = 0;
    thread_local static char buf[64]{};
    size_t hostnameLen = strnlen(CSTR0(form.host));
    size_t usernameLen = strnlen(CSTR0(form.user));
    size_t passwordLen = strnlen(CSTR0(form.pass));
    if (hostnameLen < HOSTNAME_LENGTH_MIN) {
        formValid = false;
        snprintf(buf, sizeof(buf), "Host/IP must be at least %d characters", HOSTNAME_LENGTH_MIN);
        message = buf;
    } else if (usernameLen < USERNAME_LENGTH_MIN) {
        formValid = false;
        snprintf(buf, sizeof(buf), "Username must be at least %d characters", USERNAME_LENGTH_MIN);
        message = buf;
    } else if (passwordLen < PASSWORD_LENGTH_MIN) {
        formValid = false;
        snprintf(buf, sizeof(buf), "Password must be at least %d characters", PASSWORD_LENGTH_MIN);
        message = buf;
    }

    if (message) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImColor(220, 0, 0, 255).Value);
        //CenterNextItem(message);
        ImGui::Text(message);
        ImGui::PopStyleColor(1);
    } else {
        ImGui::Text("");
    }


    int stylesPushed = 0;
    ImGui::PushStyleColor(ImGuiCol_Button, formValid ? 0xFFBF8346 : 0xFF666666); stylesPushed++;
    if (!formValid) {
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF666666); stylesPushed++;
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF666666); stylesPushed++;
    }
    ImGui::BeginDisabled(message);
    bool save = ImGui::Button("Save##login_window:save");
    ImGui::EndDisabled();
    ImGui::PopStyleColor(stylesPushed);

    bool accept = false;
    ImGuiIO &io = ImGui::GetIO();
    if (formValid && (save ||
        IsKeyPressed(io.KeyMap[ImGuiKey_Enter]) ||
        IsKeyPressed(io.KeyMap[ImGuiKey_KeyPadEnter]))) {
        accept = true;
    }

    ImGui::SameLine();
    // TODO(hack): Figure out how to fix margin
    ImGui::Text(" ");
    ImGui::SameLine();
    //ImGui::PushStyleColor(ImGuiCol_Button, 0xFF999999);
    bool cancel = ImGui::Button("Cancel##login_window:cancel");
    //ImGui::PopStyleColor();

    if (accept) {
        DB::ServerT server{};
        server.desc = std::string(form.desc);
        server.host = std::string(form.host);
        server.port = form.port;
        server.user = std::string(form.user);
        server.pass = std::string(form.pass);
        if (editingServerIdx >= 0) {
            netClient.server_db.Replace(editingServerIdx, server);
        } else {
            netClient.server_db.Add(server);
        }
        netClient.server_db.Save();
    }

    if (accept || cancel) {
        menuStack.Pop();
    }

    if (menuStack.Changed()) {
        showPassword = false;
        editingServerIdx = SERVER_IDX_NONE;
        memset(&form, 0, sizeof(form));
    }

    return;
}

void UI::MainMenu(bool &escape, GameClient &game)
{
    Menu curMenu = menuStack.Last();
    menuStack.Begin();
    if (escape && menuStack.Pop()) {
        escape = false;
    }

    ImVec2 menuCenter{
        screenSize.x / 2.0f,
        screenSize.y / 2.0f
    };
    ImGui::SetNextWindowPos(menuCenter, 0, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(screenSize.x, screenSize.y));

    ImGui::PushFont(g_fonts.imFontHack32);

    int styleVars = 0;
    int colorVars = 0;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(300.0f, 200.0f)); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f)); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16.0f, 8.0f)); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f)); styleVars++;
    ImGui::PushStyleColor(ImGuiCol_Button, {}); colorVars++;
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0, 0.5f, 0, 1 }); colorVars++;
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0, 0.3f, 0, 1 }); colorVars++;

    ImGui::Begin("MainMenu", 0,
        //ImGuiWindowFlags_NoTitleBar |
        //ImGuiWindowFlags_NoResize |
        //ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        //ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoSavedSettings
    );

    // Breadcrumbs
    ImGui::PushFont(g_fonts.imFontHack16);
    if (menuStack.Count() > 1) {
        UI::Breadcrumbs();
    } else {
        // HACK(dlb): This just takes up the same amount of space as a BreadcrumbButton
        // would, without showing the button, to keep menu titles vertically aligned.
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
        ImGui::Button("##breadcrumb_placeholder");
        ImGui::PopStyleColor(3);
    }
    ImGui::PopFont();

    UI::MenuTitle(curMenu.title);

    switch (curMenu.id) {
        case Menu_Main: {
            ImGui::PushFont(g_fonts.imFontHack48);

            const UI::Menu menus[] = {
                { Menu_Singleplayer, "Single Player", "Choose a world"  },
                { Menu_Multiplayer , "Multiplayer"  , "Choose a server" },
                { Menu_Audio       , "Audio"        , "Audio Settings"  },
                { Menu_Quit        , "Quit"         , "Quit"            },
            };

            for (int i = 0; i < (int)ARRAY_SIZE(menus); i++) {
                if (UI::MenuButton(menus[i].name)) {
                    if (menus[i].id == Menu_Quit) {
                        quitRequested = true;
                    } else {
                        menuStack.Push(menus[i].id, menus[i].name, menus[i].title);
                    }
                }
            }

            ImGui::PopFont();
            break;
        } case Menu_Singleplayer: {
            thread_local static const char *message = 0;
            thread_local static size_t loadingIdx = 0;
            thread_local static double loadingIdxLastUpdate = 0;

            if (game.netClient.IsDisconnected()) {
                ImGui::PushFont(g_fonts.imFontHack48);
                if (UI::MenuButton("Dandyland")) {
                    game.localServer = new GameServer(game.args);
                    if (game.netClient.Connect(game.args->host, game.args->port, game.args->user, game.args->pass) != ErrorType::Success) {
                        TraceLog(LOG_ERROR, "Failed to connect to local server");
                    }
                }
                ImGui::PopFont();
            } else if (game.netClient.IsConnecting()) {
                const char *text[]{
                    "Loading   ",
                    "Loading.  ",
                    "Loading.. ",
                    "Loading...",
                };
                message = text[loadingIdx];
                if (g_clock.now - loadingIdxLastUpdate > 0.25) {
                    loadingIdx = (loadingIdx + 1) % ARRAY_SIZE(text);
                    loadingIdxLastUpdate = g_clock.now;
                }

                if (message) {
                    //CenterNextItem(message);
                    ImGui::Text(message);
                } else {
                    ImGui::Text("");
                }

                const bool cancel = ImGui::Button("Cancel");
                if (cancel) {
                    disconnectRequested = true;
                }
            }

            if (game.netClient.IsConnected() || disconnectRequested) {
                message = 0;
                loadingIdx = 0;
            }

            break;
        } case Menu_Multiplayer: {
            UI::MenuMultiplayer(game);
            break;
        } case Menu_Multiplayer_EditServer: {
            UI::MenuMultiplayerNew(game.netClient);
            break;
        } case Menu_Audio: {
            ImGui::PushFont(g_fonts.imFontHack32);
            SliderFloatLeft("##Master", &Catalog::g_mixer.masterVolume, 0.0f, 1.0f);
            SliderFloatLeft("##Music ", &Catalog::g_mixer.musicVolume, 0.0f, 1.0f);
            SliderFloatLeft("##Sfx   ", &Catalog::g_mixer.sfxVolume, 0.0f, 1.0f);
            ImGui::PopFont();

            ImGui::PushFont(g_fonts.imFontHack16);
            if (ImGui::TreeNode("Advanced")) {
                if (ImGui::TreeNode("Tracks")) {
                    for (size_t i = 1; i < (size_t)Catalog::TrackID::Count; i++) {
                        if (ImGui::TreeNode(Catalog::TrackIDString((Catalog::TrackID)i))) {
                            SliderFloatLeft("##vLimit  ", &Catalog::g_tracks.mixer.volumeLimit[i], 0.0f, 1.0f);
                            SliderFloatLeft("##vVolume ", &Catalog::g_tracks.mixer.volume[i], 0.0f, 1.0f);
                            SliderFloatLeft("##vSpeed  ", &Catalog::g_tracks.mixer.volumeSpeed[i], 0.0f, 2.0f);
                            SliderFloatLeft("##vTarget ", &Catalog::g_tracks.mixer.volumeTarget[i], 0.0f, 1.0f);
                            ImGui::TreePop();
                        }
                    }
                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Sound FX")) {
                    for (size_t i = 1; i < (size_t)Catalog::SoundID::Count; i++) {
                        if (ImGui::TreeNode(Catalog::SoundIDString((Catalog::SoundID)i))) {
                            SliderFloatLeft("##vLimit  ", &Catalog::g_sounds.mixer.volumeLimit[i], 0.0f, 1.0f);
                            ImGui::TreePop();
                        }
                    }
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
            ImGui::PopFont();
            break;
        }
    }

    // HACK(dlb): Prevent playing duplicate sounds when coincidentally hovering something immediately after changing menus
    thread_local static bool hoverChangedLastFrame = false;
    if (menuStack.Changed()) {
        disconnectRequested = true;

        if (menuStack.WentBack()) {
            Catalog::g_sounds.Play(Catalog::SoundID::Click1, 1.0f + dlb_rand32f_variance(0.01f), true);
        } else if(menuStack.WentForward()) {
            Catalog::g_sounds.Play(Catalog::SoundID::Click1, 1.2f + dlb_rand32f_variance(0.01f), true);
        }
    } else if (UI::hoverLabel != UI::prevHoverLabel) {
        UI::prevHoverLabel = UI::hoverLabel;
        if (UI::hoverLabel && !hoverChangedLastFrame) {
            Catalog::g_sounds.Play(Catalog::SoundID::Click1, 2.0f + dlb_rand32f_variance(0.01f), true);
        }
    }
    hoverChangedLastFrame = menuStack.Changed();

    ImGui::End();
    ImGui::PopStyleVar(styleVars);
    ImGui::PopStyleColor(colorVars);
    ImGui::PopFont();
}

void UI::InGameMenu(bool &escape, bool connectedToServer)
{
    const char *menuId = "EscMenu";
    if (!ImGui::IsPopupOpen(menuId) && escape) {
        ImGui::OpenPopup(menuId);
        escape = false;
    }

    ImVec2 menuCenter {
        screenSize.x / 2.0f,
        screenSize.y / 2.0f
    };
    ImGui::SetNextWindowPos(menuCenter, 0, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 400));

    thread_local static MenuID currentMenu = Menu_Main;

    bool menuOpen = ImGui::BeginPopupModal(menuId, 0,
        //ImGuiWindowFlags_NoTitleBar |
        //ImGuiWindowFlags_NoResize |
        //ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        //ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoSavedSettings
    );

    if (menuOpen) {
        switch (currentMenu) {
            case Menu_Main: {
                if (menuOpen && escape) {
                    ImGui::CloseCurrentPopup();
                    escape = false;
                    break;
                }

                ImGui::PushFont(g_fonts.imFontHack64);
                ImGui::Text("What's up?");
                ImGui::PopFont();

                ImGui::PushFont(g_fonts.imFontHack48);
                ImGui::MenuItem("Resume");
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::MenuItem("Audio");
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                    currentMenu = Menu_Audio;
                }
                if (connectedToServer) {
                    ImGui::MenuItem("Log Off");
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                        disconnectRequested = true;
                        ImGui::CloseCurrentPopup();
                    }
                } else {
                    ImGui::MenuItem("Quit");
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                        quitRequested = true;
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::PopFont();
                break;
            } case Menu_Audio: {
                if ((menuOpen && escape) || ImGui::Button("< Back")) {
                    currentMenu = Menu_Main;
                    escape = false;
                    break;
                }

                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - 100);
                thread_local static bool audioAdvanced = false;
                if (ImGui::Button(audioAdvanced ? "Show Less" : "Show More", ImVec2(100, 0))) {
                    audioAdvanced = !audioAdvanced;
                }
                ImGui::SameLine();
                ImGui::SetCursorPosY(5);  // idk how to calculate this properly
                ImGui::PushFont(g_fonts.imFontHack64);
                ImGui::Text("Audio");
                ImGui::PopFont();

                ImGui::PushFont(g_fonts.imFontHack48);
                SliderFloatLeft("##Master", &Catalog::g_mixer.masterVolume, 0.0f, 1.0f);
                SliderFloatLeft("##Music ", &Catalog::g_mixer.musicVolume, 0.0f, 1.0f);
                SliderFloatLeft("##Sfx   ", &Catalog::g_mixer.sfxVolume, 0.0f, 1.0f);
                if (audioAdvanced) {
                    if (ImGui::TreeNode("Tracks")) {
                        for (size_t i = 1; i < (size_t)Catalog::TrackID::Count; i++) {
                            if (ImGui::TreeNode(Catalog::TrackIDString((Catalog::TrackID)i))) {
                                SliderFloatLeft("##vLimit  ", &Catalog::g_tracks.mixer.volumeLimit[i], 0.0f, 1.0f);
                                SliderFloatLeft("##vVolume ", &Catalog::g_tracks.mixer.volume[i], 0.0f, 1.0f);
                                SliderFloatLeft("##vSpeed  ", &Catalog::g_tracks.mixer.volumeSpeed[i], 0.0f, 2.0f);
                                SliderFloatLeft("##vTarget ", &Catalog::g_tracks.mixer.volumeTarget[i], 0.0f, 1.0f);
                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Sound FX")) {
                        for (size_t i = 1; i < (size_t)Catalog::SoundID::Count; i++) {
                            if (ImGui::TreeNode(Catalog::SoundIDString((Catalog::SoundID)i))) {
                                SliderFloatLeft("##vLimit  ", &Catalog::g_sounds.mixer.volumeLimit[i], 0.0f, 1.0f);
                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                }
                ImGui::PopFont();
                break;
            }
        }
        ImGui::EndPopup();
    }
}

void UI::InventoryItemTooltip(ItemStack &invStack, int slot, Player &player, NetClient &netClient)
{
    player.inventory.skipUpdate = true;
    ItemStack &cursorStack = player.inventory.CursorSlot().stack;

    // Terraria:
    //   on mouse down
    //   always bottom right of cursor
    //   left mouse down = take/swap/combine/drop stack
    //   right mouse down = if stack is same (or cursor empty), take 1 item immediately, then repeat with acceleration
    //   sin wave scale bobbing
    //   can use held item as if equipped
    // D2R:
    //   Always centers item on cursor
    //   empty cursor:
    //     left mouse down = take/swap/drop stack (or combine for rare cases like keys)
    // Minecraft:
    //   empty cursor:
    //     left mouse down = pick up stack
    //     right mouse down = pick up bigger half of stack [(int)ceilf(count / 2.0f)]
    //   filled cursor:
    //     left mouse down + drag split stack evenly
    //     right mouse down + drag 1 item piles
    //     mouse up = keep remainder on cursor
    //   Always centers item on cursor
    //   squish (less width, more height) animation on pick-up

    const double minimumVacuumDelay = 0.02f;
    const double defaultVacumDelay = 0.3f;
    thread_local static double lastVacuum = 0;
    thread_local static double vacuumDelay = defaultVacumDelay;

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        vacuumDelay = defaultVacumDelay;
    }

    if (ImGui::IsItemHovered()) {
        const int scrollY = (int)GetMouseWheelMove();
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            const bool doubleClick = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
            bool success = player.inventory.SlotClick(slot, doubleClick);
            if (success) {
                netClient.SendSlotClick(slot, doubleClick);
            }
            Catalog::g_sounds.Play(Catalog::SoundID::Click1, (success ? 1.0f : 0.77f) + dlb_rand32f_variance(0.01f), true);
        } else if (scrollY) {
            // TODO(dlb): This will break if the window has any scrolling controls
            bool success = player.inventory.SlotScroll(slot, scrollY);
            if (success) {
                netClient.SendSlotScroll(slot, scrollY);
            }
            Catalog::g_sounds.Play(Catalog::SoundID::Click1, (success ? 1.0f : 0.77f) + dlb_rand32f_variance(0.01f), true);
        // TODO(dlb): Refactor out this input handling into controller sample code
        } else if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            // Vaccuum up items from slot to cursor
            if (invStack.count) {
                DLB_ASSERT(invStack.uid);
                uint32_t vacuumCount = IsKeyDown(KEY_LEFT_SHIFT) ? invStack.count : 1;
                if (vacuumCount) {
                    const double sinceLastVacuum = g_clock.now - lastVacuum;

                    // Been awhile since last vaccuum, reset acceleration
                    if (sinceLastVacuum > defaultVacumDelay * 2.0f) {
                        vacuumDelay = defaultVacumDelay;
                    }

                    // Time to vaccuum again
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || sinceLastVacuum > vacuumDelay) {
                        bool success = player.inventory.SlotScroll(slot, -(int)vacuumCount);
                        if (success) {
                            netClient.SendSlotScroll(slot, -(int)vacuumCount);
                        }
                        vacuumDelay = MAX(minimumVacuumDelay, vacuumDelay - 0.05f);
                        lastVacuum = g_clock.now;
                        Catalog::g_sounds.Play(Catalog::SoundID::Click1, (success ? 1.0f : 0.77f) + dlb_rand32f_variance(0.01f), true);
                    }
                }
            }
            //// Drop item(npc) from cursor stack into hovered slot or onto ground
            //if (cursorStack.count) {
            //    DLB_ASSERT(cursorStack.uid);
            //    uint32_t dropCount = IsKeyDown(KEY_LEFT_CONTROL) ? cursorStack.count : 1;
            //    if (dropCount) {
            //        netClient.SendSlotScroll(slot, dropCount);
            //    }
            //}
        }

        if (!cursorStack.count && invStack.count && invStack.uid) {
            const Item item = g_item_db.Find(invStack.uid);
            const ItemProto proto = item.Proto();

            ItemAffix afxDamageFlat    = item.FindAffix(ItemAffix_DamageFlat);
            ItemAffix afxKnockbackFlat = item.FindAffix(ItemAffix_KnockbackFlat);
            ItemAffix afxMoveSpeedFlat = item.FindAffix(ItemAffix_MoveSpeedFlat);
            ItemAffix afxValue         = item.FindAffix(ItemAffix_Value);

            bool hasDamageFlat    = afxDamageFlat.value.min || afxDamageFlat.value.max;
            bool hasKnockbackFlat = afxKnockbackFlat.value.min || afxKnockbackFlat.value.max;
            bool hasMoveSpeedFlat = afxMoveSpeedFlat.value.min || afxMoveSpeedFlat.value.max;
            bool hasValue         = afxValue.value.min || afxValue.value.max;

            const char *invName = item.Name();
            char invNameWithCountBuf[64]{};
            if (invStack.count > 1) {
                snprintf(CSTR0(invNameWithCountBuf), "%s (%u)", invName, invStack.count);
                invName = invNameWithCountBuf;
            }
            const char *itemClass = ItemClassToString(proto.itemClass);
            Color itemClassColor = ItemClassToColor(proto.itemClass);
            ImVec4 itemClassImColor = RayToImColor(itemClassColor);
            const char *damageFlatText = "000.0 - 000.0 damage";
            const char *knockbackFlatText = "00.0 - 00.0 knockback";
            const char *moveSpeedFlatText = "00.0 movement speed";
            const char *valueText = "sell @ 00.00";
#if CL_DEBUG_ADVANCED_ITEM_TOOLTIPS
            const char *itemUidText = "uid: 000000";
            const char *itemTypeText = "itemType: 0000";
            const char *itemSeedText = "seed: 00000000000000000000";
#endif

            float maxWidth = 0.0f;
            maxWidth = MAX(maxWidth, ImGui::CalcTextSize(invName).x);
            maxWidth = MAX(maxWidth, ImGui::CalcTextSize(itemClass).x);
            maxWidth = MAX(maxWidth, ImGui::CalcTextSize(damageFlatText).x * hasDamageFlat);
            maxWidth = MAX(maxWidth, ImGui::CalcTextSize(knockbackFlatText).x * hasKnockbackFlat);
            maxWidth = MAX(maxWidth, ImGui::CalcTextSize(moveSpeedFlatText).x * hasMoveSpeedFlat);
            maxWidth = MAX(maxWidth, ImGui::CalcTextSize(valueText).x * hasValue);
#if CL_DEBUG_ADVANCED_ITEM_TOOLTIPS
            maxWidth = MAX(maxWidth, ImGui::CalcTextSize(itemUidText).x);
            maxWidth = MAX(maxWidth, ImGui::CalcTextSize(itemTypeText).x);
            maxWidth = MAX(maxWidth, ImGui::CalcTextSize(itemSeedText).x);
#endif
            maxWidth += 8.0f;

            ImGui::SetNextWindowSize({ maxWidth, 0 });

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 2 });
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 4 });
            ImGui::BeginTooltip();
            //ImGui::SetTooltip("%u %npc\n%npc\nDamage: %.2f\nValue: %.2f", invStack.count, invName, itemClass, item.damage, item.value);

            // Name (count)
            CenterNextItem(invName);
            //ImGui::TextColored(itemClassImColor, invName);
            ImGui::Text(invName);

            // Type
            CenterNextItem(itemClass);
            ImGui::TextColored(itemClassImColor, itemClass);

            // Flat damage
            if (hasDamageFlat) {
                CenterNextItem(damageFlatText);
                //ImGui::TextColored(RayToImColor(WHITE), "Deals ");
                //ImGui::SameLine();
                ImGui::TextColored(RayToImColor(MAROON), "%.1f", afxDamageFlat.value.min);
                ImGui::SameLine();
                ImGui::TextColored(RayToImColor(WHITE), " - ");
                ImGui::SameLine();
                ImGui::TextColored(RayToImColor(MAROON), "%-.1f", afxDamageFlat.value.max);
                ImGui::SameLine();
                ImGui::TextColored(RayToImColor(WHITE), " damage");
            }

            // Flat knockback
            if (hasKnockbackFlat) {
                CenterNextItem(knockbackFlatText);
                ImGui::TextColored(RayToImColor(MAROON), "%.1f", afxKnockbackFlat.value.min);
                ImGui::SameLine();
                ImGui::TextColored(RayToImColor(WHITE), " - ");
                ImGui::SameLine();
                ImGui::TextColored(RayToImColor(MAROON), "%-.1f", afxKnockbackFlat.value.max);
                ImGui::SameLine();
                ImGui::TextColored(RayToImColor(WHITE), " knockback");
            }

            // Flat movement speed
            if (hasMoveSpeedFlat) {
                CenterNextItem(moveSpeedFlatText);
                ImGui::TextColored(RayToImColor(MAROON), "%.1f", afxMoveSpeedFlat.value.min);
                ImGui::SameLine();
                ImGui::TextColored(RayToImColor(WHITE), " movement speed");
            }

            // Value
            if (hasValue) {
                CenterNextItem(valueText);
                ImGui::TextColored(RayToImColor(WHITE), "sell @ ");
                ImGui::SameLine();
                ImGui::TextColored(RayToImColor(YELLOW), "%-.2f", afxValue.value.min);
            }

#if CL_DEBUG_ADVANCED_ITEM_TOOLTIPS
            CenterNextItem(itemUidText);
            ImVec4 uidColor = item.uid == invStack.uid ? RayToImColor(DARKPURPLE) : RayToImColor(RED);
            ImGui::TextColored(uidColor, "uid: %06u", invStack.uid);
            CenterNextItem(itemTypeText);
            ImGui::TextColored(RayToImColor(DARKPURPLE), "itemType: %04u", item.type);
            CenterNextItem(itemSeedText);
            ImGui::TextColored(RayToImColor(DARKPURPLE), "seed: %020llu", item.seed);
#endif

            ImGui::EndTooltip();
            ImGui::PopStyleVar(2);
        }
    }
    player.inventory.skipUpdate = false;
}

void UI::InventorySlot(bool inventoryActive, int slot, const Texture &invItems, Player &player, NetClient &netClient)
{
    ItemStack &invStack = player.inventory.GetInvSlot(slot).stack;

    if (invStack.count) {
        Vector2 uv0{};
        Vector2 uv1{};

        const Item &item = g_item_db.Find(invStack.uid);
        player.inventory.TexRect(invItems, item.type, uv0, uv1);
        ImGui::ImageButton("##invSlotButton", (ImTextureID)(size_t)invItems.id, ImVec2{ ITEM_W, ITEM_H }, ImVec2{ uv0.x, uv0.y }, ImVec2{ uv1.x, uv1.y });

        if (item.Proto().stackLimit > 1) {
            const ImVec2 topLeft = ImGui::GetItemRectMin();
            const float bottom = ImGui::GetItemRectMax().y;
            const float fontHeight = ImGui::GetFontSize();
            ImDrawList *drawList = ImGui::GetWindowDrawList();

            char countBuf[16]{};
            snprintf(countBuf, sizeof(countBuf), "%d", invStack.count);
            //snprintf(countBuf, sizeof(countBuf), "%d", newInvStack.count);
            drawList->AddText(
#if CL_CURSOR_ITEM_TEXT_BOTTOM_LEFT
                { topLeft.x + 4, bottom - fontHeight },
#else
                { topLeft.x + 4, topLeft.y + 2 },
#endif
                IM_COL32_WHITE,
                countBuf
            );
        }
    } else {
        ImGui::Button("##inv_slot", ImVec2(48, 48));

        // TODO: Add inv placeholder icon (was grayed out sword in items.png)
        //ImGui::ImageButton((ImTextureID)(size_t)invItems.type, size, uv0, uv1, -1,
        //    ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 0.5f));
    }

    if (inventoryActive) {
        InventoryItemTooltip(invStack, slot, player, netClient);
    }
}

void UI::Inventory(const Texture &invItems, Player& player, NetClient &netClient, bool &escape, bool &inventoryActive)
{
    if (inventoryActive && escape) {
        inventoryActive = false;
        escape = false;
    }

    int styleVars = 0;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4)); styleVars++;
    int styleCols = 0;
    auto bgWindow = BLACK;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(bgWindow.r, bgWindow.g, bgWindow.b, 0.6f * 255.0f)); styleCols++;
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32_BLACK); styleCols++;

    ImVec2 invCenter{ screenSize.x / 2.0f, 48.0f };
    ImGui::SetNextWindowPos(invCenter, 0, ImVec2(0.5f, 0.0f));

    ImGui::Begin("##inventory", 0,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_AlwaysAutoResize |
        //ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoSavedSettings
    );

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2)); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f)); styleVars++;
    auto bgColor = DARKBLUE;
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(bgColor.r, bgColor.g, bgColor.b, 0.4f * 255.0f)); styleCols++;
    //ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 255, 0, 255)); styleCols++;
    //ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 0, 255, 255)); styleCols++;

    int hoveredSlot = -1;

    ImGui::PushID("inv_hotbar");
    for (int hotbarSlot = PlayerInventory::SlotId_Hotbar_0; hotbarSlot <= PlayerInventory::SlotId_Hotbar_9; hotbarSlot++) {
        ImGui::PushID(hotbarSlot);
        if (hotbarSlot == player.inventory.selectedSlot) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImColor(170, 170, 0, 255).Value);
        }
        UI::InventorySlot(inventoryActive, hotbarSlot, invItems, player, netClient);
        if (hotbarSlot == player.inventory.selectedSlot) {
            ImGui::PopStyleColor();
        }
        if (hotbarSlot < PlayerInventory::SlotId_Hotbar_9) {
            ImGui::SameLine();
        }
        ImGui::PopID();

        if (ImGui::IsItemHovered()) {
            hoveredSlot = hotbarSlot;
        }
    }
    ImGui::PopID();

    if (inventoryActive) {
        thread_local static bool ignoreEmpty = false;
        if (ImGui::Button("Sort")) player.inventory.Sort(ignoreEmpty);
        ImGui::SameLine();
        if (ImGui::Button("Sort & Combine")) player.inventory.SortAndCombine(ignoreEmpty);
#if _DEBUG
        auto dbgColor = PINK;
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(dbgColor.r, dbgColor.g, dbgColor.b, 0.7f * 255.0f));
        ImGui::SameLine();
        if (ImGui::Button("Combine")) player.inventory.Combine(ignoreEmpty);
        ImGui::SameLine();
        ImGui::Checkbox("Ignore empty", &ignoreEmpty);
        ImGui::PopStyleColor();
#endif

#if 0
        ImGui::PushID("inv_coinage");
        for (int coinSlot = PlayerInvSlot_Coin_Copper; coinSlot <= PlayerInvSlot_Coin_Gilded; coinSlot++) {
            ImGui::PushID(coinSlot);

            UI::InventorySlot(inventoryActive, coinSlot, invItems, player, netClient);

            if (coinSlot < PlayerInvSlot_Coin_Gilded) {
                ImGui::SameLine();
            }
            ImGui::PopID();
        }
        ImGui::PopID();
#endif
        ImGui::PushID("inv_slots");
        for (int row = 0; row < PLAYER_INV_ROWS; row++) {
            ImGui::PushID(row);
            for (int col = 0; col < PLAYER_INV_COLS; col++) {
                ImGui::PushID(col);

                int slot = row * PLAYER_INV_COLS + col;
                UI::InventorySlot(inventoryActive, slot, invItems, player, netClient);

                if (col < PLAYER_INV_COLS - 1) {
                    ImGui::SameLine();
                }
                ImGui::PopID();

                if (ImGui::IsItemHovered()) {
                    hoveredSlot = slot;
                }
            }
            ImGui::PopID();
        }
        ImGui::PopID();
    }

    ImGui::PopStyleColor(styleCols);
    ImGui::PopStyleVar(styleVars);

    ItemStack &cursorStack = player.inventory.CursorSlot().stack;
    if (cursorStack.count) {
        // Drop item(npc) from cursor stack onto ground
        if (hoveredSlot < 0 && IsKeyPressed(KEY_Q)) {
            DLB_ASSERT(cursorStack.uid);
            uint32_t dropCount = IsKeyDown(KEY_LEFT_SHIFT) ? cursorStack.count : 1;
            if (dropCount) {
                if (hoveredSlot >= 0) {
                    netClient.SendSlotScroll(hoveredSlot, dropCount);
                } else {
                    netClient.SendSlotDrop(PlayerInventory::SlotId_Cursor, dropCount);
                }
            }
        }

        const Item &cursorItem = g_item_db.Find(cursorStack.uid);
#if CL_CURSOR_ITEM_HIDES_POINTER
        HideCursor();
#endif

        ImDrawList *drawList = ImGui::GetForegroundDrawList();
        //ImDrawList *drawList = ImGui::GetWindowDrawList();
        //drawList->PushClipRect({0, 0}, {screenSize.x, screenSize.y});

        Vector2 uv0{};
        Vector2 uv1{};
        player.inventory.TexRect(invItems, cursorItem.type, uv0, uv1);

        Vector2 cursorOffset{};
#if CL_CURSOR_ITEM_RELATIVE_TERRARIA || 1
        cursorOffset.x = 0; //8; //-(ITEM_W);
        cursorOffset.y = 0; //8; //-(ITEM_H);
#else
        cursorOffset.x = -(ITEM_W / 2);
        cursorOffset.y = -(ITEM_H / 2);
#endif

#if 1
        const Rectangle dstRect{
           mouseScreen.x + cursorOffset.x,
           mouseScreen.y + cursorOffset.y,
           32.0f,
           32.0f
        };
#else
        // NOTE: This is because there'npc a fair time gap between EndDrawing() (which calls glfwPollInput
        // internally) and this code running (e.g. networking happens) during which the mouse can still be
        // moving. Perhaps it makes sense to always just re-query mouse position rather than using the
        // Raylib CORE.input cache for any UI stuff?
        GLFWwindow *glfwWindow = glfwGetCurrentContext();
        double freshX, freshY;
        glfwGetCursorPos(glfwWindow, &freshX, &freshY);
        const Rectangle dstRect{
           (float)freshX + cursorOffset.x,
           (float)freshY + cursorOffset.y,
           32.0f,
           32.0f
        };
#endif

#if 1
        if (uv0.x >= 0.0f && uv0.x < 1.0f &&
            uv0.y >= 0.0f && uv0.y < 1.0f &&
            uv1.x > 0.0f && uv1.x <= 1.0f &&
            uv1.y > 0.0f && uv1.y <= 1.0f)
        {
            drawList->AddImage(
                (ImTextureID)(size_t)invItems.id,
                ImVec2(dstRect.x, dstRect.y),
                ImVec2(dstRect.x + dstRect.width, dstRect.y + dstRect.height),
                ImVec2{ uv0.x, uv0.y }, ImVec2{ uv1.x, uv1.y }
            );
        } else {
            drawList->AddRectFilled(
                ImVec2(dstRect.x - (ITEM_W / 2), dstRect.y - (ITEM_H / 2)),
                ImVec2(dstRect.x + (ITEM_W / 2), dstRect.y + (ITEM_H / 2)),
                ImColor(PINK.r, PINK.g, PINK.b, PINK.a)
            );
        }
#else
        drawList->AddImage(
            (ImTextureID)(size_t)invItems.type,
            ImVec2(dstRect.x, dstRect.y),
            ImVec2(dstRect.x + dstRect.width, dstRect.y + dstRect.height),
            ImVec2{ uv0.x, uv0.y }, ImVec2{ uv1.x, uv1.y }
        );
#endif

        if (cursorItem.Proto().stackLimit > 1) {
            char countBuf[16]{};
            snprintf(countBuf, sizeof(countBuf), "%d", cursorStack.count);
#if CL_CURSOR_ITEM_TEXT_BOTTOM_LEFT
            drawList->AddText({ dstRect.x - 4, dstRect.y + dstRect.height - 6 }, IM_COL32_WHITE, countBuf);
#else
            drawList->AddText({ dstRect.x - 4, dstRect.y - 6 }, IM_COL32_WHITE, countBuf);
#endif
        }

        //drawList->PopClipRect();
    } else {
#if CL_CURSOR_ITEM_HIDES_POINTER
        ShowCursor();
#endif
    }

    ImGui::End();
}

void UI::Dialog(World &world)
{
    Player *player = world.LocalPlayer();
    if (!player) {
        return;
    }

    Vector2 topCenter = player->WorldTopCenter2D();

    //DrawCircleLines(topCenter.x, topCenter.y - topCenter.z, 2.0f, RED);
    //HealthBar::Dialog({ topCenter.x, topCenter.y - 10.0f },
    //    "Damn it, I wish you people would just\n"
    //    "leave me alone! I...\n"
    //    "\n"
    //    "Oh, you're new here, aren't you?\n"
    //    "\n"
    //    "I am Alkor, the Alchemist. I dabble in\n"
    //    "potions and salves, andI can sell you\n"
    //    "some if you really need them.\n"
    //    "\n"
    //    "But don't make a habit of coming here. I\n"
    //    "don't like to be disturbed while I'm\n"
    //    "studying!"
    //);

    Vector2 menuCenter = spycam->WorldToScreen(topCenter);
    menuCenter.y -= 10.0f;
    ImGui::SetNextWindowPos({ menuCenter.x, menuCenter.y }, 0, ImVec2(0.5f, 1.0f));
    ImGui::SetNextWindowSize({ 0.0f, 240.0f });

    ImGui::PushFont(g_fonts.imFontHack16);

    thread_local static float spacing = 5.0f;
    int styleVars = 0;
    int colVars = 0;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f                 ); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(16.0f, 8.0f)  ); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,      ImVec2(0.0f, spacing)); styleVars++;
    ImGui::PushStyleColor(ImGuiCol_WindowBg,      ImVec4(0.05f, 0.05f, 0.05f, 0.7f)); colVars++;
    ImGui::PushStyleColor(ImGuiCol_Border,        ImVec4(0.49f, 0.42f, 0.25f, 0.7f)); colVars++;
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f , 0.3f , 0.3f , 0.7f)); colVars++;
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.25f, 0.25f, 0.25f, 0.7f)); colVars++;

    ImGui::Begin("Dialog", 0,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        //ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_AlwaysVerticalScrollbar |
        //ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        //ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoSavedSettings
    );

    //ImGui::SliderFloat("spacing", &spacing, 0.0f, 10.0f);

    ImGui::Text("Damn it, I wish you people would just");
    ImGui::Text("leave me alone! I...");
    ImGui::Text("");
    ImGui::Text("Oh, you're new here, aren't you?");
    ImGui::Text("I am ");
    ImGui::SameLine();
    ImGui::TextColored({ 0.8f, 0.1f, 0.1f, 1.0f }, "Alkor, the Alchemist");
    thread_local static bool alkor = false;
    if (ImGui::IsItemClicked()) {
        alkor = !alkor;
    }
    ImGui::SameLine();
    ImGui::Text(". I dabble in");
    if (alkor) {
        ImGui::Separator();
        ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1.0f }, "Alkor the Alchemist is a wonderful man");
        ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1.0f }, "with a wonderful plan who loves to eat");
        ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1.0f }, "cabbage on Tuesdays.");
        ImGui::Separator();
    }
    ImGui::Text("potions and salves, and I can sell you");
    ImGui::Text("some if you really need them.");
    ImGui::Text("");
    ImGui::Text("But don't make a habit of coming here. I");
    ImGui::Text("don't like to be disturbed while I'm");
    ImGui::Text("studying!");
    ImGui::Text("");

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
    ImGui::Separator();
    ImGui::Selectable("##unique_id");
    ImGui::SameLine();
    ImGui::TextColored({ 0.1f, 0.7f, 0.1f, 1.0f }, "Nice to meet you!");
    ImGui::Separator();
    ImGui::Selectable("##unique_id");
    ImGui::SameLine();
    ImGui::TextColored({ 0.7f, 0.7f, 0.7f, 1.0f }, "Cool... I have to run.");
    ImGui::Separator();
    ImGui::Selectable("##unique_id");
    ImGui::SameLine();
    ImGui::TextColored({ 0.7f, 0.1f, 0.1f, 1.0f }, "You're annoying, old man!");
    ImGui::Separator();
    ImGui::PopStyleVar();

    ImGui::End();
    ImGui::PopStyleVar(styleVars);
    ImGui::PopStyleColor(colVars);
    ImGui::PopFont();
}

void UI::ParticleText(Vector2 pos, const char *text)
{
#if 1
    Font font = g_fonts.fontBig;
    int fontSize = 24;
    Vector2 size = MeasureTextEx(font, text, (float)fontSize, 1.0f);
    DrawTextFont(font, text, pos.x - size.x * 0.5f, pos.y - size.y, 0, 0, fontSize, RED);
#else
    Vector2 posScreen = spycam->WorldToScreen(pos);
    //menuCenter.y -= 10.0f;
    ImGui::SetNextWindowPos({ posScreen.x, posScreen.y }, 0, ImVec2(0.5f, 1.0f));
    //ImGui::SetNextWindowSize({ 0.0f, 240.0f });

    ImGui::PushFont(g_fonts.imFontHack16);

    thread_local static float spacing = 5.0f;
    int styleVars = 0;
    int colVars = 0;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 8.0f)); styleVars++;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, spacing)); styleVars++;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 0.7f)); colVars++;
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.49f, 0.42f, 0.25f, 0.7f)); colVars++;
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.7f)); colVars++;
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.25f, 0.25f, 0.25f, 0.7f)); colVars++;

    ImGui::Begin("##ParticleText", 0,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        //ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_AlwaysVerticalScrollbar |
        //ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        //ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoSavedSettings
    );

    ImGui::Text(text);

    ImGui::End();
    ImGui::PopStyleVar(styleVars);
    ImGui::PopStyleColor(colVars);
    ImGui::PopFont();
#endif
}
