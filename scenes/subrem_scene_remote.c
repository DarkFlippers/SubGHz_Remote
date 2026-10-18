#include "../subghz_remote_app_i.h"
#include "../views/remote.h"

#include <lib/subghz/protocols/raw.h>
#ifndef FW_ORIGIN_Official
#include <lib/subghz/blocks/custom_btn.h>
#endif

#define TAG "SubRemScenRemote"

void subrem_scene_remote_callback(SubRemCustomEvent event, void* context) {
    furi_assert(context);
    SubGhzRemoteApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, event);
}

static uint8_t subrem_scene_remote_event_to_index(SubRemCustomEvent event_id) {
    uint8_t ret = 0;

    if(event_id == SubRemCustomEventViewRemoteStartUP) {
        ret = SubRemSubKeyNameUp;
    } else if(event_id == SubRemCustomEventViewRemoteStartDOWN) {
        ret = SubRemSubKeyNameDown;
    } else if(event_id == SubRemCustomEventViewRemoteStartLEFT) {
        ret = SubRemSubKeyNameLeft;
    } else if(event_id == SubRemCustomEventViewRemoteStartRIGHT) {
        ret = SubRemSubKeyNameRight;
    } else if(event_id == SubRemCustomEventViewRemoteStartOK) {
        ret = SubRemSubKeyNameOk;
    }

    return ret;
}

static bool subrem_scene_remote_is_separator(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// Find "<key> <value>" in protocol text, key has to start a word: "Cnt:" must not match "mCnt:"
static bool subrem_scene_remote_find_value(const char* text, const char* key, FuriString* value) {
    size_t key_len = strlen(key);
    for(const char* p = strstr(text, key); p; p = strstr(p + 1, key)) {
        if(p != text && !subrem_scene_remote_is_separator(p[-1])) continue;

        p += key_len;
        while(*p == ' ')
            p++;
        if(strncmp(p, "0x", 2) == 0) p += 2;

        size_t len = 0;
        while(p[len] != '\0' && !subrem_scene_remote_is_separator(p[len]))
            len++;
        if(len == 0) return false;

        furi_string_set_strn(value, p, len);
        return true;
    }
    return false;
}

// Bit count from protocol text first line: "Princeton 24bit" -> 24, 0 if not found
static uint16_t subrem_scene_remote_get_bit_count(const char* text) {
    for(const char* p = strstr(text, "bit"); p; p = strstr(p + 1, "bit")) {
        const char* start = p;
        while(start > text && start[-1] >= '0' && start[-1] <= '9')
            start--;
        if(start == p) continue;

        uint16_t bits = 0;
        for(; start < p; start++)
            bits = bits * 10 + (*start - '0');
        return bits;
    }
    return 0;
}

static void subrem_scene_remote_update_info(SubGhzRemoteApp* app) {
    SubRemSubFilePreset* sub_preset = app->map_preset->subs_preset[app->chosen_sub];

    if(sub_preset->type == SubGhzProtocolTypeUnknown) {
        subrem_view_remote_set_info(app->subrem_remote_view, "", "", "", "", "", "");
        return;
    }

    FuriString* text = furi_string_alloc();
    FuriString* value = furi_string_alloc();
    FuriString* frequency = furi_string_alloc();
    FuriString* key = furi_string_alloc();
    FuriString* serial = furi_string_alloc();
    FuriString* counter = furi_string_alloc();
    FuriString* manufacture = furi_string_alloc();

    uint32_t freq = sub_preset->freq_preset.frequency;
    // Only first two letters of modulation fit after frequency: AM650 -> AM, FM238 -> FM
    furi_string_printf(
        frequency,
        "%lu.%02lu %.2s",
        freq / 1000000,
        (freq % 1000000) / 10000,
        furi_string_get_cstr(sub_preset->freq_preset.name));

    // Decoder is loaded by TX start, fff_data already holds the key that was just sent
    SubGhzProtocolDecoderBase* decoder = subghz_txrx_get_decoder(app->txrx);
    if(decoder && subghz_protocol_decoder_base_deserialize(decoder, sub_preset->fff_data) ==
                      SubGhzProtocolStatusOk) {
        subghz_protocol_decoder_base_get_string(decoder, text);
        const char* text_cstr = furi_string_get_cstr(text);

        // Short keys fit on one line: 32bit is 8 hex digits at most
        uint16_t bits = subrem_scene_remote_get_bit_count(text_cstr);
        if(bits > 0 && bits <= 32 && subrem_scene_remote_find_value(text_cstr, "Key:", value)) {
            furi_string_printf(key, "Key:%s", furi_string_get_cstr(value));
        }

        if(subrem_scene_remote_find_value(text_cstr, "Sn:", value) ||
           subrem_scene_remote_find_value(text_cstr, "Serial:", value)) {
            furi_string_printf(serial, "Sn:%s", furi_string_get_cstr(value));
        } else if(subrem_scene_remote_find_value(text_cstr, "Fix:", value)) {
            // KeeLoq-like protocols show Fix part (button + serial) instead of serial
            furi_string_printf(serial, "Fix:%s", furi_string_get_cstr(value));
        }
        if(subrem_scene_remote_find_value(text_cstr, "Cnt:", value)) {
            furi_string_printf(counter, "Cnt:%s", furi_string_get_cstr(value));
        }
        if(subrem_scene_remote_find_value(text_cstr, "MF:", value)) {
            furi_string_printf(manufacture, "MF:%s", furi_string_get_cstr(value));
        }
    }

    subrem_view_remote_set_info(
        app->subrem_remote_view,
        furi_string_get_cstr(sub_preset->protocol_name),
        furi_string_get_cstr(frequency),
        furi_string_get_cstr(key),
        furi_string_get_cstr(serial),
        furi_string_get_cstr(counter),
        furi_string_get_cstr(manufacture));

    furi_string_free(manufacture);
    furi_string_free(counter);
    furi_string_free(serial);
    furi_string_free(key);
    furi_string_free(frequency);
    furi_string_free(value);
    furi_string_free(text);
}

static void subrem_scene_remote_tx_stop(SubGhzRemoteApp* app) {
    subrem_tx_stop_sub(app);
    app->tx_running = false;
    app->tx_held = false;
    subrem_view_remote_set_state(app->subrem_remote_view, SubRemViewRemoteStateIdle, 0);
    notification_message(app->notifications, &sequence_blink_stop);
}

void subrem_scene_remote_on_enter(void* context) {
    SubGhzRemoteApp* app = context;

    app->tx_running = false;
    app->tx_held = false;
#ifndef FW_ORIGIN_Official
    subghz_custom_btns_reset();
#endif

    subrem_view_remote_update_data_labels(app->subrem_remote_view, app->map_preset->subs_preset);
    subrem_view_remote_set_radio(
        app->subrem_remote_view,
        subghz_txrx_radio_device_get(app->txrx) != SubGhzRadioDeviceTypeInternal);
    subrem_view_remote_set_info(app->subrem_remote_view, "", "", "", "", "", "");

    subrem_view_remote_set_callback(app->subrem_remote_view, subrem_scene_remote_callback, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, SubRemViewIDRemote);
}

bool subrem_scene_remote_on_event(void* context, SceneManagerEvent event) {
    // Same concept as Sub-GHz transmitter: one press always sends the full signal.
    // Protocols TX endlessly while the button is held and send their own repeats after release.
    // RAW is sent to the end of file, and starts again if the button is still held by then.
    // Back stops any TX right away.
    SubGhzRemoteApp* app = context;
    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == SubRemCustomEventViewRemoteBack) {
            if(!scene_manager_previous_scene(app->scene_manager)) {
                scene_manager_stop(app->scene_manager);
                view_dispatcher_stop(app->view_dispatcher);
            }
            return true;
        } else if(
            event.event == SubRemCustomEventViewRemoteStartUP ||
            event.event == SubRemCustomEventViewRemoteStartDOWN ||
            event.event == SubRemCustomEventViewRemoteStartLEFT ||
            event.event == SubRemCustomEventViewRemoteStartRIGHT ||
            event.event == SubRemCustomEventViewRemoteStartOK) {
            // Stop the previous sub before chosen_sub changes, it restores that sub's button
            subrem_tx_stop_sub(app);

            uint8_t chosen_sub = subrem_scene_remote_event_to_index(event.event);
            app->chosen_sub = chosen_sub;
            app->tx_held = true;

            subrem_view_remote_set_state(
                app->subrem_remote_view, SubRemViewRemoteStateLoading, chosen_sub);

            app->tx_running = subrem_tx_start_sub(app, app->map_preset->subs_preset[chosen_sub]);
            subrem_scene_remote_update_info(app);

            if(app->tx_running) {
                subrem_view_remote_set_state(
                    app->subrem_remote_view, SubRemViewRemoteStateSending, chosen_sub);
                notification_message(app->notifications, &sequence_blink_start_magenta);
            } else {
                app->tx_held = false;
                subrem_view_remote_set_state(
                    app->subrem_remote_view, SubRemViewRemoteStateIdle, 0);
                notification_message(app->notifications, &sequence_blink_red_100);
            }
            return true;
        } else if(event.event == SubRemCustomEventViewRemoteForcedStop) {
            subrem_scene_remote_tx_stop(app);
            return true;
        } else if(event.event == SubRemCustomEventViewRemoteStop) {
            // Button released: TX goes on until the signal is fully sent, tick catches the end
            app->tx_held = false;
            if(app->tx_running) {
                subrem_tx_release_sub(app);
            }
            return true;
        }
    } else if(event.type == SceneManagerEventTypeTick) {
        if(app->tx_running && subrem_tx_is_complete(app)) {
            SubRemSubFilePreset* sub_preset = app->map_preset->subs_preset[app->chosen_sub];
            if(app->tx_held && sub_preset->type == SubGhzProtocolTypeRAW) {
                // RAW is over but button is still held - send it again
                app->tx_running = subrem_tx_start_sub(app, sub_preset);
                if(!app->tx_running) {
                    subrem_scene_remote_tx_stop(app);
                }
            } else {
                subrem_scene_remote_tx_stop(app);
            }
        }
        return true;
    }
    return false;
}

void subrem_scene_remote_on_exit(void* context) {
    SubGhzRemoteApp* app = context;

    subrem_tx_stop_sub(app);
    app->tx_running = false;
    app->tx_held = false;

    subrem_view_remote_set_state(app->subrem_remote_view, SubRemViewRemoteStateIdle, 0);

    notification_message(app->notifications, &sequence_blink_stop);

#ifndef FW_ORIGIN_Official
    subghz_txrx_reset_dynamic_and_custom_btns(app->txrx);
#endif
}
