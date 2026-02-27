#include "store.hpp"
#include "icons.h"
#include <span>

ResourceStore::ResourceStore() {
    icons.insert({"battery_charging_on", std::span<const unsigned char>(battery_charging_on, battery_charging_on_len)});
    icons.insert({"battery_charging_off", std::span<const unsigned char>(battery_charging_off, battery_charging_off_len)});
    icons.insert({"battery_battery_low", std::span<const unsigned char>(battery_battery_low, battery_battery_low_len)});
    icons.insert({"notifications_dnd_off", std::span<const unsigned char>(notifications_dnd_off, notifications_dnd_off_len)});
    icons.insert({"notifications_dnd_on", std::span<const unsigned char>(notifications_dnd_on, notifications_dnd_on_len)});
    icons.insert({"audio_mic_mute", std::span<const unsigned char>(audio_mic_mute, audio_mic_mute_len)});
    icons.insert({"audio_speaker_unmute", std::span<const unsigned char>(audio_speaker_unmute, audio_speaker_unmute_len)});
    icons.insert({"audio_mic_unmute", std::span<const unsigned char>(audio_mic_unmute, audio_mic_unmute_len)});
    icons.insert({"audio_speaker_mute", std::span<const unsigned char>(audio_speaker_mute, audio_speaker_mute_len)});
    icons.insert({"bluetooth_connected", std::span<const unsigned char>(bluetooth_connected, bluetooth_connected_len)});
    icons.insert({"bluetooth_media_on", std::span<const unsigned char>(bluetooth_media_on, bluetooth_media_on_len)});
    icons.insert({"bluetooth_headphones_battery", std::span<const unsigned char>(bluetooth_headphones_battery, bluetooth_headphones_battery_len)});
    icons.insert({"bluetooth_base", std::span<const unsigned char>(bluetooth_base, bluetooth_base_len)});
    icons.insert({"bluetooth_headset_mic", std::span<const unsigned char>(bluetooth_headset_mic, bluetooth_headset_mic_len)});
    icons.insert({"bluetooth_searching", std::span<const unsigned char>(bluetooth_searching, bluetooth_searching_len)});
    icons.insert({"bluetooth_disabled", std::span<const unsigned char>(bluetooth_disabled, bluetooth_disabled_len)});
    icons.insert({"brightness_base", std::span<const unsigned char>(brightness_base, brightness_base_len)});
}