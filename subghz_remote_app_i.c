#include "subghz_remote_app_i.h"
#include "helpers/txrx/subghz_txrx_i.h"
#include <lib/toolbox/path.h>
#include <toolbox/strint.h>
#include <flipper_format/flipper_format_i.h>

#include "helpers/txrx/subghz_txrx.h"
#ifndef FW_ORIGIN_Official
#include <lib/subghz/blocks/custom_btn.h>
#endif

#define TAG "SubGhzRemote"

static const char* map_file_labels[SubRemSubKeyNameMaxCount][3] = {
    [SubRemSubKeyNameUp] = {"UP", "ULABEL", "UBUTTON"},
    [SubRemSubKeyNameDown] = {"DOWN", "DLABEL", "DBUTTON"},
    [SubRemSubKeyNameLeft] = {"LEFT", "LLABEL", "LBUTTON"},
    [SubRemSubKeyNameRight] = {"RIGHT", "RLABEL", "RBUTTON"},
    [SubRemSubKeyNameOk] = {"OK", "OKLABEL", "OKBUTTON"},
};

void subrem_map_preset_reset(SubRemMapPreset* map_preset) {
    furi_assert(map_preset);

    for(uint8_t i = 0; i < SubRemSubKeyNameMaxCount; i++) {
        // CRITICAL: Check if preset is valid before resetting
        if(map_preset->subs_preset[i]) {
            subrem_sub_file_preset_reset(map_preset->subs_preset[i]);
        } else {
            FURI_LOG_E(TAG, "NULL preset at index %d during reset", i);
        }
    }
}

static SubRemLoadMapState subrem_map_preset_check(
    SubRemMapPreset* map_preset,
    SubGhzTxRx* txrx,
    FlipperFormat* fff_data_file) {
    furi_assert(map_preset);
    // TxRx is optional - if NULL, skip protocol validation (lazy loading)

    bool all_loaded = true;
    SubRemLoadMapState ret = SubRemLoadMapStateErrorBrokenFile;

    SubRemLoadSubState sub_loading_state;
    SubRemSubFilePreset* sub_preset;

    for(uint8_t i = 0; i < SubRemSubKeyNameMaxCount; i++) {
        sub_preset = map_preset->subs_preset[i];

        // CRITICAL: Check if preset is valid before accessing
        if(!sub_preset) {
            FURI_LOG_E(TAG, "NULL preset at index %d during map check", i);
            all_loaded = false;
            continue;
        }

        sub_loading_state = SubRemLoadSubStateErrorNoFile;

        if(furi_string_empty(sub_preset->file_path)) {
            // FURI_LOG_I(TAG, "Empty file path");
        } else if(!flipper_format_file_open_existing(
                      fff_data_file, furi_string_get_cstr(sub_preset->file_path))) {
            sub_preset->load_state = SubRemLoadSubStateErrorNoFile;
            FURI_LOG_E(
                TAG,
                "Cannot open .sub file: %s\n"
                "Make sure the file path in your .map file is correct.\n"
                "The file should exist at the exact path specified.",
                furi_string_get_cstr(sub_preset->file_path));
        } else {
            sub_loading_state = subrem_sub_preset_load(sub_preset, txrx, fff_data_file);
        }

        if(sub_loading_state != SubRemLoadSubStateOK) {
            all_loaded = false;
        } else {
            ret = SubRemLoadMapStateNotAllOK;
        }

        if(ret != SubRemLoadMapStateErrorBrokenFile && all_loaded) {
            ret = SubRemLoadMapStateOK;
        }

        flipper_format_file_close(fff_data_file);
    }

    return ret;
}

static bool subrem_map_preset_load(SubRemMapPreset* map_preset, FlipperFormat* fff_data_file) {
    furi_assert(map_preset);
    bool ret = false;
    SubRemSubFilePreset* sub_preset;
    for(uint8_t i = 0; i < SubRemSubKeyNameMaxCount; i++) {
        sub_preset = map_preset->subs_preset[i];

        // CRITICAL: Check if preset is valid before accessing
        if(!sub_preset) {
            FURI_LOG_E(TAG, "NULL preset at index %d during map load", i);
            continue;
        }

        if(!flipper_format_read_string(
               fff_data_file, map_file_labels[i][0], sub_preset->file_path)) {
#ifdef FURI_DEBUG
            FURI_LOG_W(TAG, "No file patch for %s", map_file_labels[i][0]);
#endif
            sub_preset->type = SubGhzProtocolTypeUnknown;
        } else if(!path_contains_only_ascii(furi_string_get_cstr(sub_preset->file_path))) {
            FURI_LOG_E(TAG, "Incorrect characters in [%s] file path", map_file_labels[i][0]);
            sub_preset->type = SubGhzProtocolTypeUnknown;
        } else if(!flipper_format_rewind(fff_data_file)) {
            // Rewind error
        } else if(!flipper_format_read_string(
                      fff_data_file, map_file_labels[i][1], sub_preset->label)) {
#ifdef FURI_DEBUG
            FURI_LOG_W(TAG, "No Label for %s", map_file_labels[i][0]);
#endif
            ret = true;
        } else {
            ret = true;
        }

        FuriString* button_str = furi_string_alloc();
        uint16_t button_code = 0;

        // Block does not change ret, value is optional.
        if(!flipper_format_rewind(fff_data_file)) {
            // Rewind error
        } else if(!flipper_format_read_string(fff_data_file, map_file_labels[i][2], button_str)) {
#ifdef FURI_DEBUG
            FURI_LOG_W(TAG, "No Button for %s", map_file_labels[i][0]);
#endif
        } else if(
            strint_to_uint16(furi_string_get_cstr(button_str), NULL, &button_code, 16) !=
            StrintParseNoError) {
#ifdef FURI_DEBUG
            FURI_LOG_W(TAG, "Invalid Button for %s: %s", map_file_labels[i][0], button_str);
#endif
        } else {
            sub_preset->button = (uint8_t)button_code;
        }

        if(ret) {
            // Preload seccesful
            FURI_LOG_I(
                TAG,
                "%-5s: %s %s - Btn %s",
                map_file_labels[i][0],
                furi_string_get_cstr(sub_preset->label),
                furi_string_get_cstr(sub_preset->file_path),
                furi_string_get_cstr(button_str));
            sub_preset->load_state = SubRemLoadSubStatePreloaded;
        }

        furi_string_free(button_str);

        flipper_format_rewind(fff_data_file);
    }
    return ret;
}

SubRemLoadMapState subrem_map_file_load(SubGhzRemoteApp* app, const char* file_path) {
    furi_assert(app);
    furi_assert(file_path);
#ifdef FURI_DEBUG
    FURI_LOG_I(TAG, "Load Map File Start");
#endif
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* fff_data_file = flipper_format_file_alloc(storage);
    SubRemLoadMapState ret = SubRemLoadMapStateErrorOpenError;
#ifdef FURI_DEBUG
    FURI_LOG_I(TAG, "Open Map File..");
#endif
    subrem_map_preset_reset(app->map_preset);

    if(!flipper_format_file_open_existing(fff_data_file, file_path)) {
        FURI_LOG_E(TAG, "Could not open MAP file %s", file_path);
        ret = SubRemLoadMapStateErrorOpenError;
    } else {
        if(!subrem_map_preset_load(app->map_preset, fff_data_file)) {
            FURI_LOG_E(TAG, "Could no Sub file path in MAP file");
            ret = SubRemLoadMapStateErrorBrokenFile;
        } else if(app->txrx) {
            // Check presets BEFORE closing the file
            // Only if TxRx is initialized - otherwise lazy load
            ret = subrem_map_preset_check(app->map_preset, app->txrx, fff_data_file);
        } else {
            // Lazy mode: TxRx not initialized yet, just preload file paths
            // Full validation will happen on first transmission
            FURI_LOG_I(TAG, "Lazy loading map file (TxRx not initialized yet)");
            ret = SubRemLoadMapStateOK;
        }
        // Close file after all operations
        flipper_format_file_close(fff_data_file);
    }

    if(ret == SubRemLoadMapStateOK) {
        FURI_LOG_I(TAG, "Load Map File Successful");
    } else if(ret == SubRemLoadMapStateNotAllOK) {
        FURI_LOG_I(TAG, "Load Map File Successful [Not all files]");
    } else {
        FURI_LOG_E(TAG, "Broken Map File");
    }

    flipper_format_file_close(fff_data_file);
    flipper_format_free(fff_data_file);

    furi_record_close(RECORD_STORAGE);
    return ret;
}

bool subrem_save_protocol_to_file(FlipperFormat* flipper_format, const char* sub_file_name) {
    furi_assert(flipper_format);
    furi_assert(sub_file_name);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    Stream* flipper_format_stream = flipper_format_get_raw_stream(flipper_format);

    bool saved = false;
    uint32_t repeat = 200;
    FuriString* file_dir = furi_string_alloc();

    path_extract_dirname(sub_file_name, file_dir);
    do {
        // removing additional fields
        flipper_format_delete_key(flipper_format, "Repeat");
        // flipper_format_delete_key(flipper_format, "Manufacture");

        if(!storage_simply_remove(storage, sub_file_name)) {
            break;
        }

        //ToDo check Write
        stream_seek(flipper_format_stream, 0, StreamOffsetFromStart);
        stream_save_to_file(flipper_format_stream, storage, sub_file_name, FSOM_CREATE_ALWAYS);

        if(!flipper_format_insert_or_update_uint32(flipper_format, "Repeat", &repeat, 1)) {
            FURI_LOG_E(TAG, "Unable Repeat");
            break;
        }

        saved = true;
    } while(0);

    furi_string_free(file_dir);
    furi_record_close(RECORD_STORAGE);
    return saved;
}

void subrem_save_active_sub(void* context) {
    furi_assert(context);
    SubGhzRemoteApp* app = context;

    if(app->chosen_sub >= SubRemSubKeyNameMaxCount) {
        FURI_LOG_E(TAG, "Invalid chosen_sub index: %d", app->chosen_sub);
        return;
    }

    SubRemSubFilePreset* sub_preset = app->map_preset->subs_preset[app->chosen_sub];

    // CRITICAL: Check if preset is valid before accessing
    if(!sub_preset) {
        FURI_LOG_E(TAG, "NULL preset at chosen_sub %d in save", app->chosen_sub);
        return;
    }

    if(!sub_preset->fff_data || furi_string_empty(sub_preset->file_path)) {
        FURI_LOG_E(TAG, "Invalid preset data at chosen_sub %d", app->chosen_sub);
        return;
    }

    subrem_save_protocol_to_file(
        sub_preset->fff_data, furi_string_get_cstr(sub_preset->file_path));
}

// Lazy initialization of TxRx - only allocate when first needed
static void subrem_ensure_txrx(SubGhzRemoteApp* app) {
    if(app->txrx == NULL) {
        FURI_LOG_I(TAG, "Lazy initializing TxRx subsystem");
        app->txrx = subghz_txrx_alloc();
        if(app->txrx) {
            subghz_txrx_set_need_save_callback(app->txrx, subrem_save_active_sub, app);
        } else {
            FURI_LOG_E(TAG, "CRITICAL: Failed to allocate TxRx!");
        }
    }
}

bool subrem_tx_start_sub(SubGhzRemoteApp* app, SubRemSubFilePreset* sub_preset) {
    furi_assert(app);
    furi_assert(sub_preset);
    bool ret = false;

    // Lazy init TxRx on first transmission
    subrem_ensure_txrx(app);
    if(!app->txrx) {
        FURI_LOG_E(TAG, "Cannot start TX: TxRx not available");
        return false;
    }

    subrem_tx_stop_sub(app, true);

    if(sub_preset->type == SubGhzProtocolTypeUnknown || sub_preset->fff_data == NULL) {
        FURI_LOG_E(TAG, "Invalid sub_preset: type=%d, fff_data=%p", sub_preset->type, sub_preset->fff_data);
        ret = false;
    } else {
        FURI_LOG_I(TAG, "Send %s", furi_string_get_cstr(sub_preset->label));

        subghz_txrx_load_decoder_by_name_protocol(
            app->txrx, furi_string_get_cstr(sub_preset->protocol_name));

        subghz_txrx_set_preset(
            app->txrx,
            furi_string_get_cstr(sub_preset->freq_preset.name),
            sub_preset->freq_preset.frequency,
            NULL,
            0);
#ifndef FW_ORIGIN_Official
        subghz_custom_btns_reset();
        subghz_txrx_custom_button_reset(app->txrx);

        subghz_txrx_custom_button_set(app->txrx, sub_preset->button);
#endif
        if(subghz_txrx_tx_start(app->txrx, sub_preset->fff_data) == SubGhzTxRxStartTxStateOk) {
            ret = true;
        }
    }

    return ret;
}

bool subrem_tx_stop_sub(SubGhzRemoteApp* app, bool forced) {
    furi_assert(app);

    // If TxRx was never initialized, nothing to stop
    if(!app->txrx) {
        return true;
    }

    if(app->chosen_sub >= SubRemSubKeyNameMaxCount) {
        FURI_LOG_E(TAG, "Invalid chosen_sub index in stop: %d", app->chosen_sub);
        subghz_txrx_stop(app->txrx);
        return true;
    }

    SubRemSubFilePreset* sub_preset = app->map_preset->subs_preset[app->chosen_sub];

    // CRITICAL: Check if preset is valid before accessing
    if(!sub_preset) {
        FURI_LOG_E(TAG, "NULL preset at chosen_sub %d in stop", app->chosen_sub);
        subghz_txrx_stop(app->txrx);
        return true;
    }

    if(forced || (sub_preset->type != SubGhzProtocolTypeRAW)) {
#ifndef FW_ORIGIN_Official
        // Reset custom button before stopping transmission
        if(subghz_custom_btn_get() != SUBGHZ_CUSTOM_BTN_OK) {
            subghz_custom_btn_set(SUBGHZ_CUSTOM_BTN_OK);
            subghz_txrx_custom_button_reset(app->txrx);
            subghz_custom_btns_reset();
        }
#endif
        subghz_txrx_stop(app->txrx);
#ifndef FW_ORIGIN_Official
        if(sub_preset->type == SubGhzProtocolTypeDynamic) {
            subghz_txrx_reset_dynamic_and_custom_btns(app->txrx);
        }
        subghz_custom_btns_reset();
#endif
        return true;
    }

    return false;
}

SubRemLoadMapState subrem_load_from_file(SubGhzRemoteApp* app) {
    furi_assert(app);

    FuriString* file_path = furi_string_alloc();
    SubRemLoadMapState ret = SubRemLoadMapStateBack;

    DialogsFileBrowserOptions browser_options;
    dialog_file_browser_set_basic_options(&browser_options, SUBREM_APP_EXTENSION, &I_subrem_10px);
    browser_options.base_path = SUBREM_APP_FOLDER;

    // Input events and views are managed by file_select
    if(!dialog_file_browser_show(app->dialogs, app->file_path, app->file_path, &browser_options)) {
    } else {
        ret = subrem_map_file_load(app, furi_string_get_cstr(app->file_path));
    }

    furi_string_free(file_path);

    return ret;
}

bool subrem_save_map_to_file(SubGhzRemoteApp* app) {
    furi_assert(app);

    const char* file_name = furi_string_get_cstr(app->file_path);
    bool saved = false;
    FlipperFormat* fff_data = flipper_format_string_alloc();

    SubRemSubFilePreset* sub_preset;

    flipper_format_write_header_cstr(
        fff_data, SUBREM_APP_APP_FILE_TYPE, SUBREM_APP_APP_FILE_VERSION);
    for(uint8_t i = 0; i < SubRemSubKeyNameMaxCount; i++) {
        sub_preset = app->map_preset->subs_preset[i];
        // CRITICAL: Check if preset is valid before accessing
        if(!sub_preset) {
            FURI_LOG_E(TAG, "NULL preset at index %d during save (file_path)", i);
            continue;
        }
        if(!furi_string_empty(sub_preset->file_path)) {
            flipper_format_write_string(fff_data, map_file_labels[i][0], sub_preset->file_path);
        }
    }
    for(uint8_t i = 0; i < SubRemSubKeyNameMaxCount; i++) {
        sub_preset = app->map_preset->subs_preset[i];
        // CRITICAL: Check if preset is valid before accessing
        if(!sub_preset) {
            FURI_LOG_E(TAG, "NULL preset at index %d during save (label)", i);
            continue;
        }
        if(!furi_string_empty(sub_preset->label)) {
            flipper_format_write_string(fff_data, map_file_labels[i][1], sub_preset->label);
        }
    }

    for(uint8_t i = 0; i < SubRemSubKeyNameMaxCount; i++) {
        sub_preset = app->map_preset->subs_preset[i];
        // CRITICAL: Check if preset is valid before accessing
        if(!sub_preset) {
            FURI_LOG_E(TAG, "NULL preset at index %d during save (button)", i);
            continue;
        }
        if(sub_preset->button != 0) {
            flipper_format_write_hex(fff_data, map_file_labels[i][2], &sub_preset->button, 1);
        }
    }

    Storage* storage = furi_record_open(RECORD_STORAGE);
    Stream* flipper_format_stream = flipper_format_get_raw_stream(fff_data);

    do {
        if(!storage_simply_remove(storage, file_name)) {
            break;
        }
        //ToDo check Write
        stream_seek(flipper_format_stream, 0, StreamOffsetFromStart);
        stream_save_to_file(flipper_format_stream, storage, file_name, FSOM_CREATE_ALWAYS);

        saved = true;
    } while(0);

    furi_record_close(RECORD_STORAGE);
    flipper_format_free(fff_data);

    return saved;
}
