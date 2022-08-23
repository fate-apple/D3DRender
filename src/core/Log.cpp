#include "pch.h"
#include "Log.h"
#include "imgui.h"
#include "Memory.h"
#include "fontawesome/IconsFontAwesome5.h"

struct LogMessage
{
    const char* data;
    MessageType type;
    float lifetime;
    const char* file;
    const char* function;
    uint32 line;
};

static MemoryArena arena;
static std::vector<LogMessage> messages;

void InitializeMessageLog()
{
    arena.Initialize(0, GB(1));
}

void LogMessageImpl(MessageType type, const char* file, const char* function, uint32 line, const char* format, ...)
{
    arena.EnsureFreeSize(MAX_SINGLE_LOG_LENGTH);
    char* buffer = static_cast<char*>(arena.GetCurrent());

    va_list args;
    //Initializes args to retrieve the additional arguments after parameter format.
    va_start(args, format);
    int bytesWritten = vsnprintf(buffer, MAX_SINGLE_LOG_LENGTH, format, args);
    va_end(args);

    messages.push_back({buffer, type, 5.f, file, function, line});
    arena.SetCurrentTo(buffer + bytesWritten + 1);
}

static const ImVec4 colorPerType[] = 
{
    ImVec4(1.f, 1.f, 1.f, 1.f),     // info => white
    ImVec4(1.f, 1.f, 0.f, 1.f),     //  warning => yellow
    ImVec4(1.f, 0.f, 0.f, 1.f),     //  error => red
};
bool logWindowOpen = false;
void UpdateMessage(float dt)
{
    // for debug
    dt = min(dt, 1.f);

    uint32 count = static_cast<uint32>(messages.size());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.1f));
    ImGui::SetNextWindowSize(ImVec2(0.f, 0.f)); // Auto-resize to content.
    bool windowOpen = ImGui::Begin("##MessageLog", nullptr,
                                   ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar
                                   | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs);
    ImGui::PopStyleVar(2);
    
    uint32 startIndex;
    
    for (uint32 i = count - 1; i != UINT32_MAX; --i)
    {
        auto& msg = messages[i];
    
        if (msg.lifetime <= 0.f)
        {
            startIndex = i + 1;
            break;
        }
        msg.lifetime -= dt;
    }
    uint32 numMessagesToShow = count - startIndex;
    numMessagesToShow = min(numMessagesToShow, 8u);
    startIndex = count - numMessagesToShow;
    
    if (windowOpen)
    {
        for (uint32 i = startIndex; i < count; ++i)
        {
            auto& msg = messages[i];
            ImGui::TextColored(colorPerType[msg.type], msg.data);
        }
    }
    ImGui::End();
    if (logWindowOpen)
    {
        if (ImGui::Begin(ICON_FA_CLIPBOARD_LIST "  Message log", &logWindowOpen))
        {
            for (uint32 i = 0; i < count; ++i)
            {
                auto& msg = messages[i];
                ImGui::TextColored(colorPerType[msg.type], "%s (%s [%u])", msg.data, msg.function, msg.line);
            }
        }
        ImGui::End();
    }
}


void updateMessageLog(float dt)
{
    dt = min(dt, 1.f); // If the app hangs, we don't want all the messages to go missing.

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.1f));
    ImGui::SetNextWindowSize(ImVec2(0.f, 0.f)); // Auto-resize to content.
    bool windowOpen = ImGui::Begin("##MessageLog", 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs);
    ImGui::PopStyleVar(2);

    uint32 count = (uint32)messages.size();
    uint32 startIndex = 0;

    for (uint32 i = count - 1; i != UINT32_MAX; --i)
    {
        auto& msg = messages[i];

        if (msg.lifetime <= 0.f)
        {
            startIndex = i + 1;
            break;
        }
        msg.lifetime -= dt;
    }

    uint32 numMessagesToShow = count - startIndex;
    numMessagesToShow = min(numMessagesToShow, 8u);
    startIndex = count - numMessagesToShow;

    if (windowOpen)
    {
        for (uint32 i = startIndex; i < count; ++i)
        {
            auto& msg = messages[i];
            ImGui::TextColored(colorPerType[msg.type], msg.data);
        }
    }
    ImGui::End();


    if (logWindowOpen)
    {
        if (ImGui::Begin(ICON_FA_CLIPBOARD_LIST "  Message log", &logWindowOpen))
        {
            for (uint32 i = 0; i < count; ++i)
            {
                auto& msg = messages[i];
                ImGui::TextColored(colorPerType[msg.type], "%s (%s [%u])", msg.data, msg.function, msg.line);
            }
        }
        ImGui::End();
    }
}