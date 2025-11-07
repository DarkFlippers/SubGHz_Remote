# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SubGHz Remote is a Flipper Zero external application (FAP) that allows users to combine up to 5 .sub files into one remote control interface. Users can trigger Sub-GHz transmissions using the device's directional pad (D-pad) buttons.

**Key Concepts:**
- **Map Files**: Text files (.txt) in FlipperFormat that store mappings between buttons (Up/Down/Left/Right/Ok) and .sub file paths, with custom labels
- **Sub Files**: Sub-GHz signal files (.sub) containing transmission data (Static, Dynamic, RAW, BinRAW protocols)
- **Button Mapping**: Each of the 5 D-pad buttons can be assigned to a different .sub file

## Architecture

### Core Application Structure

The app follows Flipper Zero's scene-based architecture pattern:

```
SubGhzRemoteApp (main app context)
├── View Dispatcher (manages view transitions)
├── Scene Manager (handles scene flow)
├── SubGhzTxRx (transmission/reception subsystem from official SubGHz app)
├── SubRemMapPreset (contains 5 SubRemSubFilePreset slots for buttons)
└── Views (Remote view, Edit Menu view, standard Flipper views)
```

**Main App Structure** (`subghz_remote_app_i.h`):
- `SubGhzRemoteApp`: Primary application context containing all subsystems
- `SubRemMapPreset`: Container for 5 button presets (Up/Down/Left/Right/Ok)
- `SubRemSubFilePreset`: Individual button configuration (file path, label, frequency, protocol, button code)

### Scene Flow

Defined in `scenes/subrem_scene_config.h` using the `ADD_SCENE` macro pattern:

1. **Start** → Main menu entry point
2. **OpenMapFile** → File browser for selecting map files
3. **Remote** → Main remote control interface (transmit signals)
4. **EditMenu** → Map file editor navigation
5. **EditSubMenu** → Button slot selection for editing
6. **EditLabel** → Edit custom label for button
7. **OpenSubFile** → File browser for selecting .sub files
8. **EditPreview** → Preview and save map file
9. **EnterNewName** → Name input for new map files
10. **FwWarning** → Warning screen for Official FW compatibility

### Key Subsystems

**TxRx Module** (`helpers/txrx/`):
- Adapted from official Flipper SubGHz app with Unleashed firmware enhancements
- Handles signal transmission, frequency/preset configuration, protocol loading
- Manages custom button support (non-Official firmware feature)

**View Components** (`views/`):
- `remote.c/h`: Main D-pad interface for triggering transmissions
- `edit_menu.c/h`: Slot editor for configuring map files

**Preset Management** (`helpers/subrem_presets.c/h`):
- Loads/validates .sub files
- Parses frequency, modulation, and protocol data
- Manages transmission state for each button slot

## File Format Details

### Map File Structure (FlipperFormat .txt)

```
Filetype: Flipper SubRem Map file
Version: 1
UP: /ext/subghz/file1.sub
DOWN: /ext/subghz/file2.sub
LEFT: /ext/subghz/file3.sub
RIGHT: /ext/subghz/file4.sub
OK: /ext/subghz/file5.sub
ULABEL: Garage Door
DLABEL: Gate
LLABEL: Light
RLABEL: Fan
OKLABEL: Alarm
UBUTTON: 01
DBUTTON: 02
```

**Key-Value Pairs:**
- Button paths: `UP`, `DOWN`, `LEFT`, `RIGHT`, `OK`
- Button labels: `ULABEL`, `DLABEL`, `LLABEL`, `RLABEL`, `OKLABEL`
- Button codes (optional): `UBUTTON`, `DBUTTON`, etc. (hex values for custom button support)

### Storage Locations

- **App folder**: `/ext/subghz_remote/` (defined as `SUBREM_APP_FOLDER`)
- **Legacy migration**: Old data from `/ext/unirf/` is automatically migrated
- **Sub files**: Typically stored in `/ext/subghz/` but can be anywhere on SD card

## Firmware Compatibility

The app supports both Official and Custom (Unleashed/RogueMaster) firmware:

**Conditional Compilation:**
- `#ifdef FW_ORIGIN_Official`: Official firmware-specific code
- `#ifndef FW_ORIGIN_Official`: Custom firmware features (custom buttons, dynamic protocols)

**Custom Button Feature**:
- Available only in custom firmwares
- Allows dynamic modification of button codes in transmission data
- Requires `subghz_custom_btn_*` functions from Unleashed firmware

## Common Development Tasks

### Building the Application

This is a Flipper Application Package (FAP). Build using the Flipper Build Tool (fbt) from the main firmware repository:

```bash
# From the firmware root, with this app in applications_user/subghz_remote/:
./fbt fap_subghz_remote_ofw

# Or build all external apps:
./fbt faps
```

The `application.fam` file defines the app metadata, dependencies, and build configuration.

### Testing Changes

**Manual Testing on Device:**
1. Copy built `.fap` file to `/ext/apps/Sub-GHz/` on SD card
2. Launch from Apps → Sub-GHz → Sub-GHz Remote
3. Test with existing map files or create new ones

**Test Scenarios:**
- Load map file with all 5 slots filled
- Load map file with partial slots (some empty)
- Transmit signals using D-pad buttons
- Edit map file and save changes
- Create new map file from scratch
- Test with different protocol types (Static, Dynamic, RAW, BinRAW)

### Adding New Features

**Adding a new scene:**
1. Add entry in `scenes/subrem_scene_config.h` using `ADD_SCENE` macro
2. Create `scenes/subrem_scene_<name>.c` with on_enter/on_event/on_exit handlers
3. Update scene flow logic in related scenes

**Modifying transmission behavior:**
- Edit `subrem_tx_start_sub()` and `subrem_tx_stop_sub()` in `subghz_remote_app_i.c`
- TxRx module is in `helpers/txrx/subghz_txrx.c`

**Adding map file fields:**
- Update `map_file_labels` array in `subghz_remote_app_i.c`
- Modify load/save functions: `subrem_map_preset_load()`, `subrem_save_map_to_file()`
- Update `SubRemSubFilePreset` structure in `helpers/subrem_presets.h`

## Important Constants and Limits

- `SUBREM_MAX_LEN_NAME`: 64 (maximum name length for files)
- `SubRemSubKeyNameMaxCount`: 5 (number of buttons/slots)
- Stack size: 2KB (defined in `application.fam`)
- Target hardware: F7 (Flipper Zero)

## Code Patterns to Follow

**Error Handling:**
- Use `SubRemLoadMapState` enum for map file loading results
- Use `SubRemLoadSubState` enum for individual .sub file loading results
- Always check return values from storage and format operations

**Memory Management:**
- All subsystems use explicit alloc/free patterns
- `SubRemSubFilePreset` objects must be allocated/freed properly
- FuriString requires `furi_string_alloc()` and `furi_string_free()`

**Logging:**
- Use `FURI_LOG_I/W/E()` macros with `TAG` constant
- Debug logs wrapped in `#ifdef FURI_DEBUG`

## Known Issues and Limitations

- Custom modulations not yet supported
- RAW file transmission improved in v1.7 (see Changelog.md)
- Official firmware may have incompatible .sub file formats (warning shown)
- Button press on remote screen: short press back stops transmission or exits

### Fixed Issues (v1.8.6 - FINAL STABLE)

**Critical Bug #1 - Freeze on Transmission (FIXED)**:
- **Symptom**: App would freeze/hang when pressing a button to transmit, requiring hard reboot
- **Root Cause**: NULL pointer dereference in `subghz_txrx_tx_stop()` at line 361
  - During transmission, `decoder_result` could be NULL or uninitialized
  - Code tried to access `decoder_result->protocol->type` without NULL checks
  - This caused segmentation fault, freezing the entire device

**Critical Bug #2 - BusFault on Transmission (FIXED)**:
- **Symptom**: BusFault crash when pressing transmission button
- **Root Cause**: Use-after-free in `subrem_tx_stop_sub()` at line 277
  - Code attempted to use `transmitter` pointer AFTER it was freed
  - `subghz_transmitter_deserialize(app->txrx->transmitter, ...)` accessed freed memory
  - This caused BusFault exception, crashing the entire Flipper

**Critical Bug #3 - NULL Pointer on Startup (FIXED)**:
- **Symptom**: App crashes immediately on launch with NULL pointer dereference
- **Root Cause**: Uninitialized variable `chosen_sub` in `subghz_remote_app_alloc()`
  - `chosen_sub` contained random garbage memory value
  - Functions accessing `app->map_preset->subs_preset[app->chosen_sub]` used invalid index
  - Could access out-of-bounds memory or NULL pointer

**Critical Bug #4 - Crash After Selecting Remote (FIXED)**:
- **Symptom**: App crashes when user selects a remote/map file from file browser
- **Root Cause**: Use-after-close in `subrem_map_file_load()` at line 164
  - File was closed at line 161 with `flipper_format_file_close()`
  - Then accessed again at line 164 in `subrem_map_preset_check()`
  - Accessing closed file handle caused crash

**Critical Bug #5 - Interference with Other SubGHz Apps (FIXED)**:
- **Symptom**: Other SubGHz apps stop working after using this app
- **Root Cause**: Global device deinitialization in `subghz_txrx_free()` at line 88
  - `subghz_devices_deinit()` is a GLOBAL call affecting all SubGHz apps
  - When this app closes, it kills device access for ALL apps
  - Other apps then crash when trying to use SubGHz hardware

**Critical Bug #6 - Scene Manager Crash (FIXED)**:
- **Symptom**: Crash in `applications/services/gui/scene_manager.c` when selecting remote file
- **Root Cause**: Invalid scene transition in `subrem_scene_open_map_file_on_enter()` at line 17
  - Scene transitions (`scene_manager_next_scene()`) called INSIDE `on_enter()` callback
  - This violates scene manager lifecycle and causes internal state corruption
  - Scene manager cannot handle nested transitions - causes crash in GUI service

**Critical Bug #7 - NULL Pointer Radio Device Crash (FIXED)**:
- **Symptom**: Flipper crashes and reboots with NULL pointer dereference error during transmission or initialization
- **Root Cause**: `radio_device` can be NULL if device initialization fails in `subghz_txrx_alloc()` at line 77-82
  - When `subghz_devices_get_by_name(SUBGHZ_DEVICE_CC1101_INT_NAME)` fails, `radio_device` is set to NULL
  - Code throughout `helpers/txrx/subghz_txrx.c` uses `radio_device` without NULL checks
  - Any call to functions like `subghz_txrx_begin()`, `subghz_txrx_tx()`, `subghz_txrx_rx()`, etc. will crash
  - This occurs in certain firmware variants or device states where radio initialization fails

**Critical Bug #8 - Bus Fault Crashes (FIXED)**:
- **Symptom**: Flipper crashes with Bus fault error during app exit or protocol operations
- **Root Causes**: Multiple NULL pointer dereferences without checks
  1. **In `subghz_txrx_free()` at line 93**: Calls `subghz_devices_end(instance->radio_device)` without checking if `radio_device` is NULL
     - When app exits with NULL radio_device, this causes immediate Bus fault
  2. **In `subghz_txrx_protocol_is_serializable()` at line 711**: Accesses `instance->decoder_result->protocol` without NULL check
     - If `decoder_result` is NULL, dereferencing causes Bus fault
  3. **In `subghz_txrx_protocol_is_transmittable()` at line 717**: Same issue accessing `decoder_result->protocol`
     - Causes Bus fault when checking protocol capabilities with NULL decoder

**All Fixes Applied (v1.8.10)**:
1. **NULL checks for decoder_result** (`helpers/txrx/subghz_txrx.c:446-451, 711-714, 725-728`)
2. **Stack size increased** from 2KB to 4KB (`application.fam:10`)
3. **Preset validation** in `subrem_tx_start_sub()` (`subghz_remote_app_i.c:274-277`)
4. **Safe transmitter cleanup** - NULL check before free, set to NULL after (`helpers/txrx/subghz_txrx.c:439-442`)
5. **Transmitter initialization** to NULL on alloc (`helpers/txrx/subghz_txrx.c:46`)
6. **Removed use-after-free** code in custom button handling (`subghz_remote_app_i.c:319-326`)
7. **Initialized chosen_sub** to 0 in app alloc (`subghz_remote_app.c`)
8. **Bounds checking** for chosen_sub in all access points (`subghz_remote_app_i.c:237-240, 311-315; subrem_scene_remote.c:96-110`)
9. **Assert → Safe checks** for all state transitions (`subghz_txrx.c:425-428`)
10. **Notification NULL checks** before calling notification_message (`subrem_scene_remote.c:106-108, 122-124, 128-130, 137-139, 146-148, 171-173`)
11. **Destruction flag** to prevent callback execution during cleanup (`subghz_remote_app_i.h:69; subghz_remote_app.c; subrem_scene_remote.c:13-15, 25-27, 71-74`)
12. **Fixed file-after-close** bug in map loading (`subghz_remote_app_i.c:166-177, 188`)
13. **Removed global device deinit** to allow coexistence with other SubGHz apps (`subghz_txrx.c:100-102`)
14. **Fixed scene transitions** - moved from on_enter to on_event using custom events (`subrem_scene_open_map_file.c`)
15. **NULL checks for radio_device** - Added comprehensive NULL pointer checks before all `radio_device` usage (`helpers/txrx/subghz_txrx.c:181-184, 198-201, 213-216, 233-236, 251-254, 270-273, 431-434, 445-448, 533-536, 550-561, 567-570, 588-591, 712-715, 724-727, 736-739, 749-752`)
16. **Bus fault fix in free()** - NULL check before calling `subghz_devices_end()` (`helpers/txrx/subghz_txrx.c:95-97`)
17. **Bus fault fix in protocol functions** - NULL checks for `decoder_result` and `protocol` before dereferencing (`helpers/txrx/subghz_txrx.c:711-714, 725-728`)

**Files Modified**:
- `helpers/txrx/subghz_txrx.c` - Multiple safety improvements, removed global deinit, comprehensive NULL checks for radio_device and decoder_result
- `application.fam` - Increased stack size from 2KB to 4KB
- `subghz_remote_app_i.c` - Fixed use-after-free, bounds checks, file-after-close
- `subghz_remote_app.c` - Added initialization and destruction flag
- `scenes/subrem_scene_remote.c` - Added notification checks and destruction handling
- `scenes/subrem_scene_open_map_file.c` - Fixed scene transition lifecycle bug
- `CLAUDE.md` - Documentation of all bugs and fixes

## Related Documentation

- Main README: `catalog/docs/Readme.md`
- Changelog: `catalog/docs/Changelog.md`
- TxRx subsystem notes: `helpers/txrx/Readme.md`
- Flipper Format specification: Part of Flipper firmware documentation
