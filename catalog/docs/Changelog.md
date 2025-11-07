## v1.8.11
- **CRITICAL FIX: Fixed NULL pointer dereference when opening map files (Bug #12)**
  - Added comprehensive NULL safety checks throughout map loading and preset access code
  - Fixed crashes in: `subrem_map_preset_check()`, `subrem_map_preset_load()`, `subrem_save_active_sub()`
  - Fixed crashes in: `subrem_tx_stop_sub()`, `subrem_save_map_to_file()`, `subrem_map_preset_reset()`
  - Scene fixes: `edit_label`, `open_sub_file` now validate presets before access
  - Added allocation verification for preset objects
  - Added proper NULL checks before freeing presets

## v1.8.10
- **CRITICAL FIX: Fixed view_dispatcher crash on app exit (Bug #11)**
  - Fixed improper shutdown order that caused crashes in `applications/services/gui/view_dispatcher.c`
  - Scene manager is now freed BEFORE views are removed, preventing use-after-free
  - Added safety checks to scene on_exit handlers to prevent access to freed components
  - Fixed: Remote scene, Edit Menu scene, and Start scene on_exit handlers now check `is_destroying` flag

## v1.8.9
- Fixed critical stability issues (Bugs #7-#10)
  - NULL pointer dereferences in transmission code
  - Use-after-free in custom button handling
  - Uninitialized variables causing crashes
  - Scene transition lifecycle bugs
  - Stack size increased to 4KB

## v1.7
- Fixes for RAW files sending by WillyJL

## v1.6
- Support for custom buttons in UL FW by MrLego8-9

## v1.5
- Fixes for new API

## v1.4
- Various fixes

## v1.3
- **New UI design**
- New remote design by @Svaarich
- Fix defines for new API
- Fix typo

## v1.2
- **Official FirmWare Support**
- Add warning screen on CustomFW
    - The .sub file format may differ from the official one and may be broken

## v1.1
- **Was combined with a configuration plugin**
    - Editing/Creating map file
- Support for starting arguments

## v1.0

**Initial implementation:**
- Transmission
- GUI
- All .sub files for which transfer is available are supported
- Signal types:
    - Static
    - Dynamic
    - RAW
    - BinRAW

*Custom modulations are not supported yet*

**Map File Format** - FlipperFormat .txt file