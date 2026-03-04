// DiscordBot.cpp
#include "DiscordBot.h"
#include <chrono>
#include <future>

DiscordBot::~DiscordBot() {
    Stop();
}

void DiscordBot::RegisterCommand(const std::string& name, const std::string& description, CommandHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    commandHandlers_[name] = std::move(handler);
    pendingCommands_[name] = description;
}

void DiscordBot::RegisterCommand(const std::string& name, const std::string& description,
                                 const std::vector<dpp::command_option>& options, CommandHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    commandHandlers_[name] = std::move(handler);
    pendingCommands_[name] = description;
    pendingCommandOptions_[name] = options;
}

void DiscordBot::RegisterButton(const std::string& buttonId, ButtonHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    buttonHandlers_[buttonId] = std::move(handler);
}

void DiscordBot::RegisterFormHandler(const std::string& customId, FormHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    formHandlers_[customId] = std::move(handler);
}

void DiscordBot::SetMessageHandler(MessageHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    messageHandler_ = std::move(handler);
}

void DiscordBot::Start(const std::string& token, const std::string& channelId, const std::string& guildId) {
    if (running_.load())
        return;
    
    if (token.empty() || token == "token") {
        std::lock_guard<std::mutex> lock(mutex_);
        lastError_ = "Invalid token";
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        channelId_ = channelId;
        guildId_ = guildId;
        lastError_.clear();
    }
    
    stopRequested_ = false;
    running_ = true;
    status_ = DiscordStatus::Connecting;
    
    botThread_ = std::thread(&DiscordBot::BotThread, this, token);
}

void DiscordBot::Stop() {
    if (!running_.load())
        return;

    StopChatLogMonitor();
    stopRequested_ = true;

    // Join panel thread before shutting down bot
    if (panelThread_.joinable())
        panelThread_.join();

    if (bot_) {
        bot_->shutdown();
    }

    if (botThread_.joinable()) {
        auto fut = std::async(std::launch::async, [this]() { botThread_.join(); });
        if (fut.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
            Log("Bot thread did not stop in time, detaching.");
            botThread_.detach();
        }
    }

    bot_.reset();
    running_ = false;
    status_ = DiscordStatus::Disconnected;

    Log("Bot stopped");
}

std::string DiscordBot::GetStatusText() const {
    switch (status_.load()) {
        case DiscordStatus::Disconnected: return "Disconnected";
        case DiscordStatus::Connecting:   return "Connecting...";
        case DiscordStatus::Connected:    return "Connected";
        default: return "Unknown";
    }
}

std::string DiscordBot::GetLastError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return lastError_;
}

void DiscordBot::SetChannelId(const std::string& channelId) {
    std::lock_guard<std::mutex> lock(mutex_);
    channelId_ = channelId;
}

std::string DiscordBot::GetChannelId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return channelId_;
}

void DiscordBot::SetGuildId(const std::string& guildId) {
    std::lock_guard<std::mutex> lock(mutex_);
    guildId_ = guildId;
}

std::string DiscordBot::GetGuildId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return guildId_;
}

void DiscordBot::SendMessage(const std::string& message) {
    if (!bot_ || status_.load() != DiscordStatus::Connected)
        return;
    
    std::string chId;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        chId = channelId_;
    }
    
    if (chId.empty() || chId == "channel_id")
        return;
    
    try {
        dpp::snowflake channel = std::stoull(chId);
        bot_->message_create(dpp::message(channel, message));
    } catch (...) {
        Log("Failed to send message: invalid channel ID");
    }
}

void DiscordBot::SendEmbed(const std::string& title, const std::string& description, uint32_t color) {
    if (!bot_ || status_.load() != DiscordStatus::Connected)
        return;
    
    std::string chId;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        chId = channelId_;
    }
    
    if (chId.empty() || chId == "channel_id")
        return;
    
    try {
        dpp::snowflake channel = std::stoull(chId);
        dpp::embed embed;
        embed.set_title(title);
        embed.set_description(description);
        embed.set_color(color);
        embed.set_timestamp(time(nullptr));
        
        bot_->message_create(dpp::message(channel, embed));
    } catch (...) {
        Log("Failed to send embed: invalid channel ID");
    }
}

void DiscordBot::AddReaction(dpp::snowflake messageId, dpp::snowflake channelId, const std::string& emoji) {
    if (!bot_ || status_.load() != DiscordStatus::Connected)
        return;
    bot_->message_add_reaction(messageId, channelId, emoji);
}

void DiscordBot::Log(const std::string& msg) {
    if (onLog)
        onLog(msg);
}

void DiscordBot::SetPanelInfo(dpp::snowflake channelId, dpp::snowflake messageId) {
    std::lock_guard<std::mutex> lock(mutex_);
    panelChannelId_ = channelId;
    panelMessageId_ = messageId;
}

void DiscordBot::DeletePanel() {
    dpp::snowflake channelId, messageId;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        channelId = panelChannelId_;
        messageId = panelMessageId_;
        panelChannelId_ = 0;
        panelMessageId_ = 0;
    }
    
    if (bot_ && channelId != 0 && messageId != 0) {
        bot_->message_delete(messageId, channelId);
    }
}

void DiscordBot::SendPanel() {
    if (!bot_ || status_.load() != DiscordStatus::Connected)
        return;
    
    std::string chId;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        chId = channelId_;
    }
    
    if (chId.empty() || chId == "channel_id")
        return;
    
    if (!panelBuilder) {
        Log("Warning: panelBuilder not set");
        return;
    }
    
    try {
        dpp::snowflake channel = std::stoull(chId);
        
        dpp::message msg;
        msg.channel_id = channel;
        panelBuilder(msg);
        
        bot_->message_create(msg, [this](const dpp::confirmation_callback_t& callback) {
            if (!callback.is_error()) {
                auto m = callback.get<dpp::message>();
                SetPanelInfo(m.channel_id, m.id);
                Log("Panel sent");
            } else {
                Log("Failed to send panel: " + callback.get_error().message);
            }
        });
    } catch (...) {
        Log("Failed to send panel: invalid channel ID");
    }
}

void DiscordBot::RefreshPanel() {
    dpp::snowflake channelId, messageId;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        channelId = panelChannelId_;
        messageId = panelMessageId_;
        panelChannelId_ = 0;
        panelMessageId_ = 0;
    }

    if (bot_ && channelId != 0 && messageId != 0) {
        bot_->message_delete(messageId, channelId, [this](const dpp::confirmation_callback_t&) {
            SendPanel();
        });
    } else {
        SendPanel();
    }
}

void DiscordBot::BotThread(std::string token) {
    try {
        bot_ = std::make_unique<dpp::cluster>(token, dpp::i_default_intents | dpp::i_message_content);
        
        // Ready event
        bot_->on_ready([this](const dpp::ready_t& event) {
            status_ = DiscordStatus::Connected;
            
            {
                std::lock_guard<std::mutex> lock(mutex_);
                botName_ = bot_->me.username;
            }
            
            Log("Bot online as: " + bot_->me.username);

            // Send greeting message
            std::string chId;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                chId = channelId_;
            }

            if (!chId.empty() && chId != "channel_id") {
                try {
                    dpp::snowflake channel = std::stoull(chId);
                    
                    dpp::embed embed;
                    embed.set_title("🐸 Bot Online");
                    embed.set_description("Hello! I'm now connected and ready.");
                    embed.set_color(0x00FF00);
                    embed.set_timestamp(time(nullptr));
                    bot_->message_create(dpp::message(channel, embed));
                } catch (...) {}
                
                // Send panel after greeting (small delay)
                if (panelThread_.joinable())
                    panelThread_.join();
                panelThread_ = std::thread([this]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    if (!stopRequested_.load())
                        SendPanel();
                });
            }


            // Register commands
            std::string guildIdCopy;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                guildIdCopy = guildId_;
            }
            
            if (guildIdCopy.empty()) {
                Log("Warning: Guild ID not set, skipping command registration");
                return;
            }

            dpp::snowflake guild_id = std::stoull(guildIdCopy);
            
            std::vector<dpp::slashcommand> commands;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                for (const auto& [name, description] : pendingCommands_) {
                    dpp::slashcommand cmd(name, description, bot_->me.id);
                    auto optIt = pendingCommandOptions_.find(name);
                    if (optIt != pendingCommandOptions_.end()) {
                        for (const auto& opt : optIt->second)
                            cmd.add_option(opt);
                    }
                    commands.push_back(cmd);
                }
            }

            bot_->guild_bulk_command_create(commands, guild_id, [this](const dpp::confirmation_callback_t& callback) {
                if (callback.is_error()) {
                    Log("Failed to register commands: " + callback.get_error().message);
                } else {
                    auto cmds = std::get<dpp::slashcommand_map>(callback.value);
                    Log("Registered " + std::to_string(cmds.size()) + " commands");
                }
            });
        });
        
        // Slash command handler
        bot_->on_slashcommand([this](const dpp::slashcommand_t& event) {
            std::string cmd = event.command.get_command_name();
            std::string user = event.command.get_issuing_user().username;
            
            Log("Command /" + cmd + " from " + user);
            
            CommandHandler handler;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = commandHandlers_.find(cmd);
                if (it != commandHandlers_.end()) {
                    handler = it->second;
                }
            }
            
            if (handler) {
                handler(event);
            } else {
                event.reply("Unknown command: /" + cmd);
            }
        });

        // Button click handler
        bot_->on_button_click([this](const dpp::button_click_t& event) {
            std::string buttonId = event.custom_id;
            std::string user = event.command.get_issuing_user().username;
            
            Log("Button " + buttonId + " clicked by " + user);
            
            ButtonHandler handler;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = buttonHandlers_.find(buttonId);
                if (it != buttonHandlers_.end()) {
                    handler = it->second;
                }
            }
            
            if (handler) {
                handler(event);
            } else {
                event.reply("Unknown button: " + buttonId);
            }
        });
        
        // Form submit handler (for modals)
        bot_->on_form_submit([this](const dpp::form_submit_t& event) {
            std::string formId = event.custom_id;
            Log("Form submitted: " + formId);

            FormHandler handler;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = formHandlers_.find(formId);
                if (it != formHandlers_.end())
                    handler = it->second;
            }

            if (handler) {
                handler(event);
            } else {
                event.reply("Unknown form: " + formId);
            }
        });

        // Message handler (for non-command messages, e.g. chat redirect)
        bot_->on_message_create([this](const dpp::message_create_t& event) {
            // Ignore bot's own messages
            if (event.msg.author.id == bot_->me.id)
                return;
            // Ignore messages that look like commands
            if (event.msg.content.empty() || event.msg.content[0] == '/')
                return;

            // Check if message is in our channel
            std::string chId;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                chId = channelId_;
            }
            if (chId.empty())
                return;
            try {
                if (event.msg.channel_id != dpp::snowflake(std::stoull(chId)))
                    return;
            } catch (...) { return; }

            // Dispatch to message handler
            MessageHandler handler;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                handler = messageHandler_;
            }
            if (handler)
                handler(event);
        });

        // Error handler
        bot_->on_log([this](const dpp::log_t& event) {
            if (event.severity >= dpp::ll_error) {
                std::lock_guard<std::mutex> lock(mutex_);
                lastError_ = event.message;
                Log("Error: " + event.message);
            }
        });
        
        Log("Starting bot...");
        bot_->start(dpp::st_wait);
        
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(mutex_);
        lastError_ = e.what();
        Log("Exception: " + std::string(e.what()));
    }
    
    status_ = DiscordStatus::Disconnected;
    running_ = false;
}

void DiscordBot::StartChatLogMonitor(std::function<std::vector<DiscordChatEntry>()> getChatLogs) {
    if (chatLogRunning_.load())
        return;

    chatLogRunning_ = true;

    // Snapshot existing chat entries so we don't re-send old history.
    // Key = "type|content" (index is NOT used because the game's circular
    // buffer shifts indices every time a new message arrives).
    {
        std::lock_guard<std::mutex> lock(chatLogMutex_);
        lastSeenEntries_.clear();

        std::vector<DiscordChatEntry> existingLogs = getChatLogs();
        for (const auto& entry : existingLogs) {
            std::string key = std::to_string(entry.channelId) + "|" + std::to_string(entry.type) + "|" + entry.content;
            lastSeenEntries_.push_back(key);
        }
    }

    chatLogThread_ = std::thread([this, getChatLogs]() {
        constexpr size_t MAX_HISTORY = 100;

        std::this_thread::sleep_for(std::chrono::seconds(1));

        while (chatLogRunning_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            if (!chatLogRunning_.load() || status_.load() != DiscordStatus::Connected || !bot_)
                continue;

            std::vector<DiscordChatEntry> logs = getChatLogs();
            if (logs.empty())
                continue;

            std::string chId;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                chId = channelId_;
            }

            if (chId.empty() || chId == "channel_id")
                continue;

            dpp::snowflake channel;
            try {
                channel = std::stoull(chId);
            } catch (...) {
                continue;
            }

            // Build keys for the current snapshot
            std::vector<std::string> currentKeys;
            currentKeys.reserve(logs.size());
            for (const auto& entry : logs) {
                currentKeys.push_back(std::to_string(entry.channelId) + "|" + std::to_string(entry.type) + "|" + entry.content);
            }

            // Find where old snapshot ends in the new snapshot.
            // We try to find the longest suffix of lastSeenEntries_ that appears
            // as a contiguous subsequence ending at some position in currentKeys.
            // We scan from the END of currentKeys (most recent) to prefer the
            // latest match — this avoids anchoring on a stale duplicate further back.
            size_t newStart = currentKeys.size(); // default: nothing new
            {
                std::lock_guard<std::mutex> lock(chatLogMutex_);

                if (!lastSeenEntries_.empty()) {
                    bool found = false;

                    // For each candidate end-position in currentKeys (newest first)
                    for (int ci = (int)currentKeys.size() - 1; ci >= 0; ci--) {
                        // Try to match the tail of lastSeenEntries_ ending here
                        int si = (int)lastSeenEntries_.size() - 1;
                        int vi = ci;
                        int matched = 0;
                        while (si >= 0 && vi >= 0 && lastSeenEntries_[si] == currentKeys[vi]) {
                            si--;
                            vi--;
                            matched++;
                        }

                        if (matched == 0)
                            continue;

                        // Require 2+ matches for confidence, OR 1 match if that's all we had
                        if (matched >= 2 || (int)lastSeenEntries_.size() == 1) {
                            newStart = ci + 1;
                            found = true;
                            break; // newest match wins
                        }
                    }

                    if (!found) {
                        // Old tail not found — map change / buffer reset.
                        // Skip everything to avoid spam; next scan picks up new ones.
                        newStart = currentKeys.size();
                    }
                }

                // Update lastSeenEntries_ to the current snapshot
                lastSeenEntries_.clear();
                for (const auto& key : currentKeys)
                    lastSeenEntries_.push_back(key);
                while (lastSeenEntries_.size() > MAX_HISTORY)
                    lastSeenEntries_.pop_front();
            }

            // Send only the new entries
            for (size_t i = newStart; i < logs.size(); i++) {
                if (!chatLogRunning_.load())
                    break;

                const auto& entry = logs[i];
                uint32_t color = entry.color & 0x00FFFFFF;

                const char* typeName;
                switch (entry.type) {
                    case 0: typeName = "All";      break;
                    case 1: typeName = "Whisper";   break;
                    case 2: typeName = "Party";     break;
                    case 3: typeName = "Buddy";     break;
                    case 4: typeName = "Guild";     break;
                    case 5: typeName = "Alliance";  break;
                    case 6: typeName = "Spouse";    break;
                    case 7: typeName = "System";    break;
                    default: typeName = "Other";    break;
                }

                char prefix[48];
                if (entry.channelId == 0)
                    snprintf(prefix, sizeof(prefix), "[%s][Here]", typeName);
                else
                    snprintf(prefix, sizeof(prefix), "[%s][CH: %d]", typeName, entry.channelId);

                dpp::embed embed;
                embed.set_description(std::string(prefix) + " " + entry.content);
                embed.set_color(color);

                bot_->message_create(dpp::message(channel, embed));

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    });
}

void DiscordBot::StopChatLogMonitor() {
    chatLogRunning_ = false;
    if (chatLogThread_.joinable()) {
        chatLogThread_.join();
    }

    std::lock_guard<std::mutex> lock(chatLogMutex_);
    lastSeenEntries_.clear();
}