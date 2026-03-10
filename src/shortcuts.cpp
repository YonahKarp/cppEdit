#include "shortcuts.h"
#include <fstream>
#include <sys/stat.h>

static const char* SHORTCUTS_FILENAME = "Shortcuts.txt";

static const char* SHORTCUTS_CONTENT = R"(Welcome to JustType!

A distraction-free text editor for focused writing.

==========================

                  SHORTCUTS
==========================

GENERAL
___________________________________________________
  Cmd                         Open/close file menu
  Ctrl+T                       Toggle dark/light theme
  Ctrl+Z                       Undo last action
  Ctrl+C                      Copy selected text
  Ctrl+X                      Cut selected text
  Ctrl+V                      Paste text
  Ctrl+Shift+Alt+Esc  Quit application

NAVIGATION
___________________________________________________
  Ctrl+Up              Jump to previous paragraph
  Ctrl+Down        Jump to next paragraph
  Alt+Up               Jump to top of document
  Alt+Down         Jump to bottom of document

SELECTION
___________________________________________________
  Shift+Up                   Extend selection up one line
  Shift+Down              Extend selection down one line
  Shift+Left                 Extend selection left one character
  Shift+Right               Extend selection right one character
  Ctrl+A                      Select all text
  Ctrl+Shift+Up         Extend selection to previous paragraph
  Ctrl+Shift+Down    Extend selection to next paragraph

EDITING
___________________________________________________
  Ctrl+Backspace      Delete previous word

FONT SIZE
___________________________________________________
  Ctrl++              Increase font size
  Ctrl+-               Decrease font size

SEARCH
___________________________________________________
  Ctrl+F              Open search (in editor)
                      Open file search (in file menu)
  Enter                   Next match
  Shift+Enter         Previous match
  Escape                 Close search

FILE MENU
___________________________________________________
  Up/Down                      Navigate files
  Enter                             Open selected file
  Left                                Create new file
  Right                              Rename selected file
  Delete/Backspace       Delete selected file
  Escape                          Close file menu

=====================================================

Note: Deleting this file will restore it on next startup.

)";

void ensure_shortcuts_file(const std::string& user_files_dir) {
    std::string shortcuts_path = user_files_dir + "/" + SHORTCUTS_FILENAME;
    
    struct stat st;
    if (stat(shortcuts_path.c_str(), &st) != 0) {
        std::ofstream file(shortcuts_path);
        if (file) {
            file << SHORTCUTS_CONTENT;
        }
    }
}
