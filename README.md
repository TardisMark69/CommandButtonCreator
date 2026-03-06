# Command Button Creator - Qt Version

A Qt C++ application for creating customizable buttons that execute bash commands on Linux.

## Features

1. **Add Buttons** - Click "Add Button" to create new command buttons
2. **Drag to Move** - Left-click and drag to reposition buttons
3. **Resize** - Drag from bottom-right corner (resize handle) to change size
4. **Edit** - Right-click any button to edit label and command
5. **Execute** - Left-click (release without dragging) to run the command
6. **Save/Load** - Persist button layouts to buttons.json

## Building with Qt Creator

### Method 1: Open in Qt Creator (Recommended)
1. Open Qt Creator
2. File → Open File or Project
3. Select `CommandButtonCreator.pro`
4. Click Configure Project
5. Press Ctrl+R to build and run

### Method 2: Command Line
```bash
qmake CommandButtonCreator.pro
make
./CommandButtonCreator
```

## Usage

### Basic Operations
- **Add Button**: Click "Add Button" in toolbar
- **Move Button**: Left-click and drag anywhere on button
- **Resize Button**: Drag the resize handle (bottom-right corner)
- **Edit Button**: Right-click button → edit label and command
- **Execute Command**: Left-click button (click, don't drag)
- **Save Layout**: Click "Save" button
- **Load Layout**: Click "Load" button (automatic on startup)

### Command Examples

**Single Commands:**
```bash
ls -la
date
echo "Hello World"
```

**Multiple Commands (separate lines or use &&):**
```bash
cd /tmp
ls
pwd
```

Or using &&:
```bash
cd /tmp && ls && pwd
```

**More Complex:**
```bash
for i in {1..5}; do echo "Count: $i"; done
find . -name "*.txt" | head -10
ps aux | grep firefox
```

## Technical Details

- **Framework**: Qt 5/6 with Widgets
- **Language**: C++17
- **Command Execution**: bash shell with 5-second timeout
- **Data Format**: JSON (buttons.json)
- **Platform**: Linux

## File Structure

```
CommandButtonCreator.pro    - Qt project file
main.cpp                    - Application entry point
mainwindow.h/cpp            - Main window and logic
commandbutton.h/cpp         - Custom draggable/resizable button
editdialog.h/cpp            - Edit dialog for button properties
buttons.json                - Saved button layout (auto-generated)
```

## Requirements

- Qt 5.12+ or Qt 6.x
- C++17 compiler (g++ or clang++)
- Linux OS with bash shell

## Notes

- Commands execute in bash with 5-second timeout
- Command output/errors shown in message box
- Buttons auto-load on application start if buttons.json exists
- Right-click to edit, left-click to execute
- Drag from corner to resize, drag from center to move
