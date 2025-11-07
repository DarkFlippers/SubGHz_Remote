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

### Fixed Issues (v1.8.11 - CURRENT STABLE)

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

**Critical Bug #11 - View Dispatcher Crash on App Exit (FIXED v1.8.10)**:
- **Symptom**: App crashes in `applications/services/gui/view_dispatcher.c` when exiting
- **Root Cause**: Improper shutdown order in `subghz_remote_app_free()` at line 186-187
  - Scene manager was freed AFTER views were removed and freed
  - When `scene_manager_free()` called active scenes' `on_exit` handlers, views were already freed
  - Scene `on_exit` handlers tried to access freed views (e.g., `subrem_view_remote_set_state()`)
  - This caused use-after-free, crashing the view_dispatcher system

**Critical Bug #12 - NULL Pointer Dereference When Opening Map Files (FIXED v1.8.11)**:
- **Symptom**: App crashes in `applications/services/gui/view_dispatcher.c` when exiting
- **Root Cause**: Improper shutdown order in `subghz_remote_app_free()` at line 186-187
  - Scene manager was freed AFTER views were removed and freed
  - When `scene_manager_free()` called active scenes' `on_exit` handlers, views were already freed
  - Scene `on_exit` handlers tried to access freed views (e.g., `subrem_view_remote_set_state()`)
  - This caused use-after-free, crashing the view_dispatcher system

- **Symptom**: App crashes with NULL pointer dereference when loading or accessing map files
- **Root Cause**: Missing NULL checks throughout preset access code
  - Multiple functions accessed `map_preset->subs_preset[i]` without verifying pointer validity
  - Preset allocation could fail silently, leaving NULL pointers in the array
  - Functions like `subrem_map_preset_check()`, `subrem_map_preset_load()`, and scene handlers crashed when accessing NULL presets
  - No validation before freeing presets, causing double-free or NULL-free attempts

**All Fixes Applied (v1.8.11)**:
1. **NULL checks for decoder_result** (`helpers/txrx/subghz_txrx.c:365-366`)
2. **Stack size increased** from 2KB to 4KB (`application.fam:10`)
3. **Preset validation** in `subrem_tx_start_sub()` (`subghz_remote_app_i.c:237-238`)
4. **Safe transmitter cleanup** - NULL check before free, set to NULL after (`helpers/txrx/subghz_txrx.c:358-362`)
5. **Transmitter initialization** to NULL on alloc (`helpers/txrx/subghz_txrx.c:45-46`)
6. **Removed use-after-free** code in custom button handling (`subghz_remote_app_i.c:273-277`)
7. **Initialized chosen_sub** to 0 in app alloc (`subghz_remote_app.c:119`)
8. **Bounds checking** for chosen_sub in all access points (`subghz_remote_app_i.c:225-228, 274-278`)
9. **Assert → Safe checks** for all state transitions (`subghz_txrx.c:360-364, 210-214, 196-200, 234-238`)
10. **Notification NULL checks** before calling notification_message (`subrem_scene_remote.c:82,88,97,106,125`)
11. **Destruction flag** to prevent callback execution during cleanup (`subghz_remote_app.c:128, subrem_scene_remote.c:55`)
12. **Fixed file-after-close** bug in map loading (`subghz_remote_app_i.c:161-166`)
13. **Removed global device deinit** to allow coexistence with other SubGHz apps (`subghz_txrx.c:88-90`)
14. **Fixed scene transitions** - moved from on_enter to on_event using custom events (`subrem_scene_open_map_file.c:8-52`)

15. **Corrected shutdown order** - Scene manager freed BEFORE views are removed (`subghz_remote_app.c:155-191`)
16. **Scene on_exit safety checks** - Added is_destroying checks to prevent access to freed components:
    - `scenes/subrem_scene_remote.c:161-165, 178-180`
    - `scenes/subrem_scene_edit_menu.c:138-141`
    - `scenes/subrem_scene_start.c:105-108`
17. **NULL preset checks in map operations** - Added comprehensive NULL validation:
    - `subghz_remote_app_i.c:26-31` - Reset function
    - `subghz_remote_app_i.c:46-51` - Check function
    - `subghz_remote_app_i.c:93-97` - Load function
    - `subghz_remote_app_i.c:258-267` - Save active sub
    - `subghz_remote_app_i.c:345-350` - Stop transmission
    - `subghz_remote_app_i.c:408-438` - Save map file (3 loops)
18. **Allocation verification** - Check preset allocation success (`subghz_remote_app.c:131-134`)
19. **Free NULL checks** - Only free presets if allocated (`subghz_remote_app.c:204-208`)
20. **Scene preset validation** - NULL checks in scene handlers:
    - `scenes/subrem_scene_open_sub_file.c:12-24, 63-66`
    - `scenes/subrem_scene_edit_label.c:32-44, 91-102`

**Files Modified**:
- `helpers/txrx/subghz_txrx.c` - Multiple safety improvements, removed global deinit
- `application.fam` - Increased stack size from 2KB to 4KB, version bumped to 1.8.11
- `subghz_remote_app_i.c` - Fixed use-after-free, bounds checks, file-after-close, NULL preset checks
- `subghz_remote_app.c` - Added initialization and destruction flag, corrected shutdown order, allocation verification
- `scenes/subrem_scene_remote.c` - Added notification checks, destruction handling, and on_exit safety
- `scenes/subrem_scene_edit_menu.c` - Added on_exit safety checks
- `scenes/subrem_scene_start.c` - Added on_exit safety checks
- `scenes/subrem_scene_open_map_file.c` - Fixed scene transition lifecycle bug
- `scenes/subrem_scene_open_sub_file.c` - Added NULL preset validation
- `scenes/subrem_scene_edit_label.c` - Added NULL preset validation
- `catalog/docs/Changelog.md` - Updated with v1.8.11 changes

## Related Documentation

- Main README: `catalog/docs/Readme.md`
- Changelog: `catalog/docs/Changelog.md`
- TxRx subsystem notes: `helpers/txrx/Readme.md`
- Flipper Format specification: Part of Flipper firmware documentation
