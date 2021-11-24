#pragma once

struct NetClient;
struct ImGuiIO;

void UILoginForm(NetClient &netClient, ImGuiIO& io, bool &escape);