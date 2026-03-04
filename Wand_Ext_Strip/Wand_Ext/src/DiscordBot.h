// DiscordBot.h
#pragma once

// FOR DEVELOPERS:
// 1. Install DPP: .\vcpkg.exe install dpp:x86-windows
// 2. Enable VS integration: .\vcpkg.exe integrate install
// 3. Required DLLs: dpp.dll, libssl-*.dll, libcrypto-*.dll, zlib1.dll

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include <string>
#include <vector>
#include <deque>
#include <atomic>
#include <thread>
#include <functional>
#include <mutex>
#include <unordered_map>

#pragma warning(push)
#pragma warning(disable: 4251)
#include <dpp/dpp.h>
#pragma warning(pop)

// Chat entry for monitoring
struct DiscordChatEntry {
    int32_t index = 0;
    int32_t type = 0;
    int32_t channelId = 0;
    uint32_t color = 0;
    std::string content;
};

enum class DiscordStatus {
    Disconnected,
    Connecting,
    Connected
};

class DiscordBot {
public:
    using CommandHandler = std::function<void(const dpp::slashcommand_t&)>;
    using ButtonHandler = std::function<void(const dpp::button_click_t&)>;
    using FormHandler = std::function<void(const dpp::form_submit_t&)>;
    using MessageHandler = std::function<void(const dpp::message_create_t&)>;

    static DiscordBot& Instance() {
        static DiscordBot instance;
        return instance;
    }

    // Lifecycle
    void Start(const std::string& token, const std::string& channelId, const std::string& guildId);
    void Stop();
    bool IsRunning() const { return running_.load(); }
    
    // Status
    DiscordStatus GetStatus() const { return status_.load(); }
    std::string GetStatusText() const;
    std::string GetLastError() const;
    
    // Channel & Guild
    void SetChannelId(const std::string& channelId);
    std::string GetChannelId() const;
    void SetGuildId(const std::string& guildId);
    std::string GetGuildId() const;
    
    // Messaging
    void SendMessage(const std::string& message);
    void SendEmbed(const std::string& title, const std::string& description, uint32_t color = 0x00FF00);
    void AddReaction(dpp::snowflake messageId, dpp::snowflake channelId, const std::string& emoji);
    
    // Command & Button registration
    void RegisterCommand(const std::string& name, const std::string& description, CommandHandler handler);
    void RegisterCommand(const std::string& name, const std::string& description,
                         const std::vector<dpp::command_option>& options, CommandHandler handler);
    void RegisterButton(const std::string& buttonId, ButtonHandler handler);
    void RegisterFormHandler(const std::string& customId, FormHandler handler);

    // Message handler (for non-command messages in channel)
    void SetMessageHandler(MessageHandler handler);
    
    // Chat log monitoring
    void StartChatLogMonitor(std::function<std::vector<DiscordChatEntry>()> getChatLogs);
    void StopChatLogMonitor();
    bool IsChatLogMonitoring() const { return chatLogRunning_.load(); }
    
    // Panel management
    void SetPanelInfo(dpp::snowflake channelId, dpp::snowflake messageId);
    void DeletePanel();
    void SendPanel();  // Send new panel to channel
    void RefreshPanel();  // Delete old panel and send new one
    
    // Callbacks
    std::function<void(const std::string&)> onLog;
    std::function<void(dpp::message&)> panelBuilder;  // Callback to build panel message

private:
    DiscordBot() = default;
    ~DiscordBot();
    
    DiscordBot(const DiscordBot&) = delete;
    DiscordBot& operator=(const DiscordBot&) = delete;
    
    void BotThread(std::string token);
    void Log(const std::string& msg);
    
    // Bot core
    std::unique_ptr<dpp::cluster> bot_;
    std::thread botThread_;
    
    std::atomic<bool> running_{false};
    std::atomic<bool> stopRequested_{false};
    std::atomic<DiscordStatus> status_{DiscordStatus::Disconnected};
    
    mutable std::mutex mutex_;
    std::string channelId_;
    std::string guildId_;
    std::string lastError_;
    std::string botName_;

    // Command & Button registry
    std::unordered_map<std::string, CommandHandler> commandHandlers_;
    std::unordered_map<std::string, std::string> pendingCommands_;
    std::unordered_map<std::string, std::vector<dpp::command_option>> pendingCommandOptions_;
    std::unordered_map<std::string, ButtonHandler> buttonHandlers_;
    std::unordered_map<std::string, FormHandler> formHandlers_;
    MessageHandler messageHandler_;

    // Chat log monitoring
    std::thread chatLogThread_;
    std::atomic<bool> chatLogRunning_{false};
    std::mutex chatLogMutex_;
    // Track the last N entries we've seen (by type|content, no index)
    // to detect which entries in the new snapshot are actually new
    std::deque<std::string> lastSeenEntries_;

    // Panel tracking
    dpp::snowflake panelChannelId_{0};
    dpp::snowflake panelMessageId_{0};

    // Delayed panel send thread
    std::thread panelThread_;
};