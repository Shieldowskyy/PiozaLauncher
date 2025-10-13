---
name: Pre-Build Checklist
about: Template for testing and verifying builds before release
title: Pre-Build Checklist - [BUILD VERSION]
labels: testing
assignees: ''

---

# Pre-Build Checklist for [BUILD VERSION]

## 1. Launcher
- [ ] Launches on Windows (various versions, e.g., 10/11)
- [ ] Launches on Linux x64 (different distros)
- [ ] Launches on Linux ARM64 (GUI and terminal)
- [ ] Metadata correctly formatted
    - [ ] Videos play correctly
- [ ] UI scales properly
- [ ] Sound works
    - [ ] Menu, effects, videos
- [ ] Console works
    - [ ] `refresh` command works
    - [ ] `getrepo` and `setrepo default/community` work
    - [ ] Errors display correctly

## 2. Game Installation
- [ ] Installs to default directory
- [ ] Installs to custom directory
- [ ] Handles missing permissions correctly
- [ ] Handles paths with spaces and special characters
- [ ] Adds shortcut to Start Menu
- [ ] Adds shortcut to Desktop

## 3. Game Launch
- [ ] Native Windows game works
    - [ ] Playtime tracking works correctly
    - [ ] 1-hour runtime test (check for memory leaks)
- [ ] Native Linux game works
    - [ ] Playtime tracking works correctly
    - [ ] 1-hour runtime test (check for memory leaks)
- [ ] Windows game via Proton works
    - [ ] Playtime tracking works correctly
    - [ ] 1-hour runtime test (check for memory leaks)

## 4. Launcher Features
- [ ] Browse works
- [ ] Move Game works
- [ ] Backup works
- [ ] Restore works
- [ ] Uninstall works
    - [ ] Removes game files
    - [ ] Removes shortcuts from Desktop and Start Menu
- [ ] Theme change works
- [ ] Offline mode works
- [ ] About screen works

## 5. Performance
- [ ] No stuttering while switching games
- [ ] Launcher reaches minimum 60FPS on reference machine (T440s)
- [ ] Startup time checked
- [ ] No memory leaks during 1-hour runtime test

## 6. Extras
- [ ] Localization / multiple languages work correctly

---

## NOTES:
*(Any additional notes from testing)*

## KNOWN BUGS:
*(List known issues and bugs here)*

## RESULT:
*(Pass / Fail / Partial - summary of the build test)*
