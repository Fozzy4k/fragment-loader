#pragma once

#include <string>

// Owns the whole "get me into game" sequence: licence verification, keeping
// fragment.exe up to date, launching it, and reporting progress to the menu.
namespace loader
{
    // Call once during menu setup. Restores the remembered session if it is
    // still valid, so the user is not asked for a key again.
    void initialize();

    // Called every frame from the main loop; handles the silent-mode close.
    void poll();

    // Licence
    void sign_in_async(const std::string& key);
    bool sign_in_busy();
    bool restoring_session();
    bool signed_in();
    std::string auth_error();

    // Display
    std::string plan_text();
    std::string remaining_text();
    std::string last_updated_text();
    std::string fragment_version();
    std::string hwid_text();

    // Environment checks
    bool roblox_running();
    bool fragment_present();

    // Load
    void begin(int game_index, bool debug);
    void tick(int& game_index);
}
