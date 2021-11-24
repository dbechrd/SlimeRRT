#include "error.h"
#include "net_client.h"
#include "imgui/imgui.h"
#include "GLFW/glfw3.h"

static void CenteredText(const char *text, const char *textToMeasure = 0)
{
    if (!textToMeasure) textToMeasure = text;
    auto windowWidth = ImGui::GetWindowSize().x;
    auto textWidth = ImGui::CalcTextSize(textToMeasure).x;
    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::Text(text);
}

void UILoginForm(NetClient &netClient, ImGuiIO& io, bool &escape)
{
    if (netClient.IsConnected()) {
        ImGui::CloseCurrentPopup();
    } else {
        ImGui::SetNextWindowSize(ImVec2(240, 0));
        ImGui::SetNextWindowPos(ImVec2(6, 340));
        auto rayDarkBlue = DARKBLUE;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(rayDarkBlue.r, rayDarkBlue.g, rayDarkBlue.b, rayDarkBlue.a));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32_BLACK);
        ImGui::Begin("##mini_menu", 0,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse
        );
        ImGui::PopStyleColor(2);
        ImGui::Text("Play with friends!");
        if (ImGui::Button("Connect to DandyNet", ImVec2(150, 0))) {
            ImGui::OpenPopup("Connect to Server##login_window");
        }

        if (ImGui::BeginPopupModal("Connect to Server##login_window", 0, ImGuiWindowFlags_AlwaysAutoResize)) {
            static char host[SV_HOSTNAME_LENGTH_MAX]{ "slime.theprogrammingjunkie.com" };
            static int  port = SV_DEFAULT_PORT;
            static char username[USERNAME_LENGTH_MAX]{ "guest" };
            static char password[PASSWORD_LENGTH_MAX]{ "pizza" };

            ImGui::Text("    Host:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(226);
            ImGui::InputText("##host", host, sizeof(host), ImGuiInputTextFlags_Password | ImGuiInputTextFlags_ReadOnly);

            ImGui::Text("    Port:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(89);
            ImGui::InputInt("##port", &port, 1, 100, ImGuiInputTextFlags_ReadOnly);
            port = CLAMP(port, 0, USHRT_MAX);

            ImGui::Text("Username:");
            ImGui::SameLine();
            // https://github.com/ocornut/imgui/issues/455#issuecomment-167440172
            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
                ImGui::SetKeyboardFocusHere();
            }
            ImGui::SetNextItemWidth(226);
            ImGui::InputText("##username", username, sizeof(username));

            ImGui::Text("Password:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(226);
            ImGui::InputText("##password", password, sizeof(password), ImGuiInputTextFlags_Password);

            static double connectingIdxLastUpdate = 0;
            static int connectingIdx = 0;
            static bool triedConnecting = false;
            static double failedToConnectShownAt = 0;
            if (netClient.IsConnecting()) {
                const char *text[3]{
                    "Attempting to connect.",
                    "Attempting to connect..",
                    "Attempting to connect...",
                };
                CenteredText(text[connectingIdx], text[2]);
                triedConnecting = true;
                if (glfwGetTime() - connectingIdxLastUpdate > 0.2) {
                    connectingIdx = (connectingIdx + 1) % ARRAY_SIZE(text);
                    connectingIdxLastUpdate = glfwGetTime();
                }
            } else {
                if (triedConnecting) {
                    CenteredText("DandyNet is offline. :(");
                    if (!failedToConnectShownAt) {
                        failedToConnectShownAt = glfwGetTime();
                    } else if (glfwGetTime() - failedToConnectShownAt > 5.0) {
                        triedConnecting = false;
                        failedToConnectShownAt = 0;
                    }
                } else {
                    ImGui::Text("");
                }
                connectingIdx = 0;
            }

            bool closePopup = escape;

            ImGui::BeginDisabled(netClient.IsConnecting());

            ImGui::SetCursorPosX(177.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, 0xFFBF8346);
            bool login = ImGui::Button("Connect##login_window:connect", ImVec2(60, 0));
            ImGui::PopStyleColor();
            if (login ||
                IsKeyPressed(io.KeyMap[ImGuiKey_Enter]) ||
                IsKeyPressed(io.KeyMap[ImGuiKey_KeyPadEnter]))                 {
                netClient.Connect(host, (unsigned short)port, username, password);
            }

            ImGui::SameLine();
            //ImGui::PushStyleColor(ImGuiCol_Button, 0xFF999999);
            bool cancel = ImGui::Button("Cancel##login_window:cancel", ImVec2(60, 0));
            //ImGui::PopStyleColor();
            if (cancel || IsKeyPressed(io.KeyMap[ImGuiKey_Escape])) {
                closePopup = true;
            }

            ImGui::EndDisabled();

            if (closePopup) {
                ImGui::CloseCurrentPopup();
                memset(username, 0, sizeof(username));
                memset(password, 0, sizeof(password));
                escape = false;
            }

            ImGui::EndPopup();
        }
        ImGui::End();
    }
}