import sys
import json
from PyQt5.QtWidgets import (
    QApplication, QWidget, QTabWidget, QVBoxLayout, QHBoxLayout, QLabel, 
    QLineEdit, QTextEdit, QPushButton, QFileDialog, QComboBox, QMessageBox, 
    QListWidget, QListWidgetItem, QFormLayout, QMainWindow, QAction, QMenu,
    QUndoStack, QUndoCommand
)
from PyQt5.QtCore import Qt, QSettings
import os

class GameJsonCreator(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Pioza Game Manifest Creator")
        self.setMinimumSize(1100, 700)
        
        # Initialize data
        self.recent_files = []
        self.load_recent_files()
        self.undo_stack = QUndoStack(self)
        self.current_file = None
        
        # Create central widget and its layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QHBoxLayout(central_widget)
        
        # Create menu bar
        self.create_menu_bar()
        
        # Left side (tabs)
        left_widget = QWidget()
        left_layout = QVBoxLayout(left_widget)
        self.tabs = QTabWidget()
        left_layout.addWidget(self.tabs)
        
        # Right side (JSON preview)
        right_widget = QWidget()
        right_layout = QVBoxLayout(right_widget)
        self.preview = QTextEdit()
        self.preview.setReadOnly(True)
        self.preview.setMinimumWidth(400)
        right_layout.addWidget(self.preview)
        
        # Validation label
        self.validation_label = QLabel()
        self.validation_label.setWordWrap(True)
        right_layout.addWidget(self.validation_label)
        
        # Add both sides to main layout
        main_layout.addWidget(left_widget, 2)  # 2/3 of width
        main_layout.addWidget(right_widget, 1)  # 1/3 of width
        
        self.init_tabs()
        self.update_json_preview()

    def init_tabs(self):
        self.general_tab = QWidget()
        self.lang_tab = QWidget()
        self.links_tab = QWidget()
        self.tabs.addTab(self.general_tab, "General")
        self.tabs.addTab(self.lang_tab, "Languages")
        self.tabs.addTab(self.links_tab, "Links & Meta")
        self.init_general_tab()
        self.init_lang_tab()
        self.init_links_tab()

    def init_general_tab(self):
        layout = QFormLayout()
        self.name_edit = QLineEdit()
        self.name_edit.setPlaceholderText("e.g. PixelRunner")
        self.gameid_edit = QLineEdit()
        self.gameid_edit.setPlaceholderText("e.g. PixelRunner")
        layout.addRow("Game Name", self.name_edit)
        layout.addRow("Game ID", self.gameid_edit)
        self.general_tab.setLayout(layout)
        self.name_edit.textChanged.connect(self.update_json_preview)
        self.gameid_edit.textChanged.connect(self.update_json_preview)

    def init_lang_tab(self):
        layout = QVBoxLayout()
        # Language management
        lang_ctrl_layout = QHBoxLayout()
        self.lang_combo = QComboBox()
        self.lang_combo.setEditable(True)
        self.lang_combo.addItem("en")
        self.add_lang_btn = QPushButton("Add Language")
        self.add_lang_btn.clicked.connect(self.add_language)
        self.remove_lang_btn = QPushButton("Remove Language")
        self.remove_lang_btn.clicked.connect(self.remove_language)
        lang_ctrl_layout.addWidget(QLabel("Languages:"))
        lang_ctrl_layout.addWidget(self.lang_combo)
        lang_ctrl_layout.addWidget(self.add_lang_btn)
        lang_ctrl_layout.addWidget(self.remove_lang_btn)
        layout.addLayout(lang_ctrl_layout)
        # Zakładki językowe
        self.lang_tabs = QTabWidget()
        self.lang_fields = {}
        self.langs = ["en"]
        self.add_language_tab("en")
        layout.addWidget(self.lang_tabs)
        self.lang_tab.setLayout(layout)

    def add_language_tab(self, lang):
        fields = {}
        tab = QWidget()
        form = QFormLayout()
        fields['title'] = QLineEdit()
        fields['title'].setPlaceholderText("np. PixelRunner: Oldschool Edition")
        fields['dev'] = QLineEdit()
        fields['dev'].setPlaceholderText("np. DashoGames")
        fields['desc'] = QTextEdit()
        fields['desc'].setPlaceholderText("Opis gry w tym języku...")
        fields['changelog'] = QTextEdit()
        fields['changelog'].setPlaceholderText("Lista zmian w tym języku...")
        fields['reqs'] = QTextEdit()
        fields['reqs'].setPlaceholderText("Minimalne wymagania sprzętowe w tym języku...")
        form.addRow(f"Tytuł ({lang})", fields['title'])
        form.addRow(f"Dev ({lang})", fields['dev'])
        form.addRow(f"Opis ({lang})", fields['desc'])
        form.addRow(f"Changelog ({lang})", fields['changelog'])
        form.addRow(f"Minimalne wymagania ({lang})", fields['reqs'])
        fields['title'].textChanged.connect(self.update_json_preview)
        fields['dev'].textChanged.connect(self.update_json_preview)
        fields['desc'].textChanged.connect(self.update_json_preview)
        fields['changelog'].textChanged.connect(self.update_json_preview)
        fields['reqs'].textChanged.connect(self.update_json_preview)
        tab.setLayout(form)
        self.lang_fields[lang] = fields
        self.lang_tabs.addTab(tab, lang)

    def add_language(self):
        lang = self.lang_combo.currentText().strip()
        if lang and lang not in self.langs:
            self.langs.append(lang)
            self.add_language_tab(lang)
            self.update_json_preview()

    def remove_language(self):
        lang = self.lang_combo.currentText().strip()
        if lang and lang in self.langs and lang != "en":
            idx = self.langs.index(lang)
            self.langs.remove(lang)
            self.lang_tabs.removeTab(idx)
            self.lang_fields.pop(lang)
            self.update_json_preview()

    # create_lang_fields is no longer needed
    def load_data(self, data):
        # Ogólne
        self.name_edit.setText(data.get("name", ""))
        self.gameid_edit.setText(data.get("gameID", ""))
        
        # Języki
        for lang, title in data.get("gameTitle", {}).items():
            if lang not in self.langs:
                self.lang_combo.setCurrentText(lang)
                self.add_language()
            f = self.lang_fields[lang]
            f['title'].setText(title)
            f['dev'].setText(data.get("devName", {}).get(lang, ""))
            f['desc'].setPlainText(data.get("description", {}).get(lang, ""))
            f['changelog'].setPlainText(data.get("changelog", {}).get(lang, ""))
            f['reqs'].setPlainText(data.get("minimumReqs", {}).get(lang, ""))

        # Video
        vids = data.get("videoURLs", [])
        self.video_url.setText(vids[0] if vids else "")

        # Custom Meta
        self.meta_list.clear()
        self.custom_meta = {}
        for key, value in data.get("customMeta", {}).items():
            self.custom_meta[key] = value
            self.meta_list.addItem(f"{key}: {value}")
            
        # Platformy
        # Usuń istniejące zakładki
        while self.platform_tabs.count() > 0:
            self.platform_tabs.removeTab(0)
        self.platform_fields.clear()
        
        # Dodaj platformy z pliku
        platforms = data.get("supportedPlatforms", [])
        executableNames = data.get("executableNames", {})
        exeNamesForTracking = data.get("exeNamesForTracking", {})
        gameURLs = data.get("gameURLs", {})
        
        for platform in platforms:
            self.platform_combo.setCurrentText(platform)
            self.add_platform()
            fields = self.platform_fields[platform]
            fields['exe'].setText(executableNames.get(platform, ""))
            fields['track'].setText(exeNamesForTracking.get(platform, ""))
            fields['url'].setText(gameURLs.get(platform, ""))
        for platform in platforms:
            if platform in self.platform_fields:
                fields = self.platform_fields[platform]
                fields['exe'].setText(executableNames.get(platform, ""))
                fields['track'].setText(exeNamesForTracking.get(platform, ""))
                fields['url'].setText(gameURLs.get(platform, ""))

    def load_json(self):
        if self.maybe_save():
            file_path, _ = QFileDialog.getOpenFileName(
                self, "Open game.json", "", "JSON Files (*.json)")
            if file_path:
                self.load_json_file(file_path)
                
    def load_json_file(self, file_path):
        try:
            with open(file_path, "r", encoding="utf-8") as f:
                text = f.read()
                self.preview.setPlainText(text)
                arr = json.loads(text)
            if not arr or not isinstance(arr, list):
                raise ValueError("Invalid file format (not a list)")
            data = arr[0]
            self.load_data(data)
            self.current_file = file_path
            self.add_recent_file(file_path)
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to load file:\n{e}")
            return

    def remove_language_tab(self, lang):
        if lang in self.langs and lang != "en":
            idx = self.langs.index(lang)
            self.langs.remove(lang)
            self.lang_tabs.removeTab(idx)
            self.lang_fields.pop(lang)

    def add_custom_meta(self):
        key = self.meta_key.text().strip()
        value = self.meta_value.text().strip()
        if key and value:
            self.custom_meta[key] = value
            self.meta_list.addItem(f"{key}: {value}")
            self.meta_key.clear()
            self.meta_value.clear()
            self.update_json_preview()
    
    def remove_custom_meta(self, item):
        key = item.text().split(":")[0].strip()
        if key in self.custom_meta:
            del self.custom_meta[key]
            self.meta_list.takeItem(self.meta_list.row(item))
            self.update_json_preview()

    def add_platform_tab(self, platform):
        tab = QWidget()
        form = QFormLayout()
        fields = {}
        
        fields['exe'] = QLineEdit()
        fields['exe'].setPlaceholderText(f"Nazwa pliku wykonywalnego dla {platform}")
        fields['exe'].textChanged.connect(self.update_json_preview)
        
        fields['track'] = QLineEdit()
        fields['track'].setPlaceholderText(f"Nazwa pliku do trackowania dla {platform}")
        fields['track'].textChanged.connect(self.update_json_preview)
        
        fields['url'] = QLineEdit()
        fields['url'].setPlaceholderText(f"URL do gry dla {platform}")
        fields['url'].textChanged.connect(self.update_json_preview)
        
        form.addRow("Executable", fields['exe'])
        form.addRow("Exe for tracking", fields['track'])
        form.addRow("Game URL", fields['url'])
        
        tab.setLayout(form)
        self.platform_fields[platform] = fields
        self.platform_tabs.addTab(tab, platform)

    def init_links_tab(self):
        self.links_layout = QVBoxLayout()
        self.platform_fields = {}
        
        # Platform management
        plat_ctrl_layout = QHBoxLayout()
        self.platform_combo = QComboBox()
        self.platform_combo.setEditable(True)
        self.platform_combo.addItems(["Windows", "Linux"])
        self.add_platform_btn = QPushButton("Add Platform")
        self.add_platform_btn.clicked.connect(self.add_platform)
        self.remove_platform_btn = QPushButton("Remove Platform")
        self.remove_platform_btn.clicked.connect(self.remove_platform)
        plat_ctrl_layout.addWidget(QLabel("Platforms:"))
        plat_ctrl_layout.addWidget(self.platform_combo)
        plat_ctrl_layout.addWidget(self.add_platform_btn)
        plat_ctrl_layout.addWidget(self.remove_platform_btn)
        self.links_layout.addLayout(plat_ctrl_layout)
        
        # Zakładki platform
        self.platform_tabs = QTabWidget()
        self.links_layout.addWidget(self.platform_tabs)
        
        # Video URL
        video_layout = QHBoxLayout()
        self.video_url = QLineEdit()
        self.video_url.setPlaceholderText("np. https://youtube.com/embed/xyz")
        video_layout.addWidget(QLabel("Video URL:"))
        video_layout.addWidget(self.video_url)
        self.links_layout.addLayout(video_layout)
        
        # Custom Meta section
        meta_group = QWidget()
        meta_layout = QVBoxLayout()
        meta_group.setLayout(meta_layout)
        
        # Inputs for new meta entry
        meta_input_layout = QHBoxLayout()
        self.meta_key = QLineEdit()
        self.meta_key.setPlaceholderText("Key")
        self.meta_value = QLineEdit()
        self.meta_value.setPlaceholderText("Value")
        add_meta_btn = QPushButton("Add")
        add_meta_btn.clicked.connect(self.add_custom_meta)
        meta_input_layout.addWidget(self.meta_key)
        meta_input_layout.addWidget(self.meta_value)
        meta_input_layout.addWidget(add_meta_btn)
        meta_layout.addLayout(meta_input_layout)
        
        # List of meta entries
        self.meta_list = QListWidget()
        self.meta_list.itemDoubleClicked.connect(self.remove_custom_meta)
        meta_layout.addWidget(QLabel("Custom Meta (double-click to remove)"))
        meta_layout.addWidget(self.meta_list)
        
        self.links_layout.addWidget(meta_group)
        self.links_tab.setLayout(self.links_layout)
        
        self.video_url.textChanged.connect(self.update_json_preview)
        self.custom_meta = {}

    def add_platform(self):
        platform = self.platform_combo.currentText().strip()
        if platform and platform not in self.platform_fields:
            self.add_platform_tab(platform)
            self.update_json_preview()
            
    def remove_platform(self):
        platform = self.platform_combo.currentText().strip()
        if platform in self.platform_fields:
            idx = self.platform_tabs.currentIndex()
            self.platform_tabs.removeTab(idx)
            del self.platform_fields[platform]
            self.update_json_preview()

    def get_supported_platforms(self):
        return list(self.platform_fields.keys())

    def create_menu_bar(self):
        menubar = self.menuBar()
        
        # File Menu
        file_menu = menubar.addMenu('&File')
        
        new_action = QAction('&New', self)
        new_action.setShortcut('Ctrl+N')
        new_action.triggered.connect(self.new_file)
        file_menu.addAction(new_action)
        
        open_action = QAction('&Open...', self)
        open_action.setShortcut('Ctrl+O')
        open_action.triggered.connect(self.load_json)
        file_menu.addAction(open_action)
        
        save_action = QAction('&Save', self)
        save_action.setShortcut('Ctrl+S')
        save_action.triggered.connect(self.save_json)
        file_menu.addAction(save_action)
        
        save_as_action = QAction('Save &As...', self)
        save_as_action.setShortcut('Ctrl+Shift+S')
        save_as_action.triggered.connect(self.save_json_as)
        file_menu.addAction(save_as_action)
        
        file_menu.addSeparator()
        
        # Recent files submenu
        self.recent_menu = file_menu.addMenu('Recent Files')
        self.update_recent_files_menu()
        
        file_menu.addSeparator()
        
        exit_action = QAction('E&xit', self)
        exit_action.setShortcut('Alt+F4')
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)
        
        # Edit Menu
        edit_menu = menubar.addMenu('&Edit')
        
        undo_action = self.undo_stack.createUndoAction(self, '&Undo')
        undo_action.setShortcut('Ctrl+Z')
        edit_menu.addAction(undo_action)
        
        redo_action = self.undo_stack.createRedoAction(self, '&Redo')
        redo_action.setShortcut('Ctrl+Y')
        edit_menu.addAction(redo_action)
        
        edit_menu.addSeparator()
        
        replace_action = QAction('&Replace...', self)
        replace_action.setShortcut('Ctrl+H')
        replace_action.triggered.connect(self.show_replace_dialog)
        edit_menu.addAction(replace_action)
        
        # Help Menu
        help_menu = menubar.addMenu('&Help')
        
        about_action = QAction('&About Pioza Game Manifest Creator', self)
        about_action.triggered.connect(self.show_about_dialog)
        help_menu.addAction(about_action)
        
    def new_file(self):
        if self.maybe_save():
            self.reset_form()
            self.current_file = None
            self.update_json_preview()
            
    def reset_form(self):
        # Reset general fields
        self.name_edit.clear()
        self.gameid_edit.clear()
        self.video_url.clear()
        self.meta_list.clear()
        self.custom_meta.clear()
        
        # Reset languages
        # First clear existing English fields
        if "en" in self.lang_fields:
            fields = self.lang_fields["en"]
            fields["title"].clear()
            fields["dev"].clear()
            fields["desc"].clear()
            fields["changelog"].clear()
            fields["reqs"].clear()
        
        # Remove all non-English tabs
        while self.lang_tabs.count() > 1:  # Keep 'en'
            self.lang_tabs.removeTab(1)
        # Reset language data structures
        self.lang_fields = {}
        self.langs = ["en"]
        self.add_language_tab("en")
        
        # Reset platforms
        while self.platform_tabs.count() > 0:
            self.platform_tabs.removeTab(0)
        self.platform_fields.clear()
        
        # Reset validation label
        self.validation_label.setText("")
        
        # Update JSON preview
        self.update_json_preview()
            
    def maybe_save(self):
        if self.is_modified():
            reply = QMessageBox.question(self, 'Game JSON Creator',
                'Do you want to save your changes?',
                QMessageBox.Save | QMessageBox.Discard | QMessageBox.Cancel,
                QMessageBox.Save)
            
            if reply == QMessageBox.Save:
                return self.save_json()
            elif reply == QMessageBox.Cancel:
                return False
        return True
        
    def is_modified(self):
        # TODO: Implement actual modification tracking
        return True
        
    def save_json_as(self):
        file_path, _ = QFileDialog.getSaveFileName(
            self, "Save game.json As", "", "JSON Files (*.json)"
        )
        if file_path:
            self.current_file = file_path
            self.save_json()
            self.add_recent_file(file_path)
            
    def load_recent_files(self):
        settings = QSettings('GameJsonCreator', 'RecentFiles')
        self.recent_files = settings.value('recentFiles', [], str)
        
    def save_recent_files(self):
        settings = QSettings('GameJsonCreator', 'RecentFiles')
        settings.setValue('recentFiles', self.recent_files[:10])  # Keep last 10 files
        
    def add_recent_file(self, file_path):
        if file_path in self.recent_files:
            self.recent_files.remove(file_path)
        self.recent_files.insert(0, file_path)
        self.recent_files = self.recent_files[:10]  # Keep last 10 files
        self.save_recent_files()
        self.update_recent_files_menu()
        
    def update_recent_files_menu(self):
        self.recent_menu.clear()
        for file in self.recent_files:
            if os.path.exists(file):
                action = QAction(os.path.basename(file), self)
                action.setData(file)
                action.triggered.connect(lambda checked, f=file: self.open_recent_file(f))
                self.recent_menu.addAction(action)
                
    def open_recent_file(self, file_path):
        if self.maybe_save():
            self.load_json_file(file_path)
            
    def show_replace_dialog(self):
        # TODO: Implement replace dialog
        QMessageBox.information(self, "Not implemented", 
                              "Replace functionality will be implemented in a future version.")
                              
    def show_about_dialog(self):
        about_text = f"""<h2>Pioza Game Manifest Creator</h2>
<p>Version 0.1</p>

<p>A dedicated tool for creating and managing game manifest files for the Pioza Launcher. 
This application simplifies the process of preparing games for distribution through the Pioza platform 
by providing an intuitive interface for manifest file creation.</p>

<p>Features:</p>
<ul>
    <li>Multi-language game description support</li>
    <li>Cross-platform configuration</li>
    <li>Real-time manifest validation</li>
    <li>Custom metadata management</li>
    <li>Recent files tracking</li>
</ul>

<p><b>Author:</b> Shieldziak (DashoGames)</p>

<p><b>License:</b> MIT</p>
<p>Copyright © 2025 Shieldziak</p>

<p>Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:</p>

<p>The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.</p>

<p>THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.</p>"""

        QMessageBox.about(self, "About Pioza Game Manifest Creator", about_text)

    def save_json(self):
        data = self.collect_data()
        valid, msg = self.validate_json(data)
        if not valid:
            QMessageBox.critical(self, "Validation Error", msg)
            return False
            
        if not hasattr(self, 'current_file') or not self.current_file:
            return self.save_json_as()
            
        try:
            with open(self.current_file, "w", encoding="utf-8") as f:
                json.dump([data], f, ensure_ascii=False, indent=4)
            return True
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to save file:\n{e}")
            return False

    def validate_json(self, data):
        # Podstawowe pola
        required_str = ["name", "gameID"]
        for key in required_str:
            if not isinstance(data.get(key), str) or not data.get(key).strip():
                return False, f"Pole '{key}' jest wymagane i nie może być puste."

        # Platformy
        if not data.get("supportedPlatforms") or not isinstance(data["supportedPlatforms"], list):
            return False, "Pole 'supportedPlatforms' musi być listą i nie może być puste."
        
        # Pola zależne od platform
        platforms = data["supportedPlatforms"]
        platform_fields = {
            "executableNames": "executable",
            "exeNamesForTracking": "exe for tracking",
            "gameURLs": "game URL"
        }
        for field_name, display_name in platform_fields.items():
            field_data = data.get(field_name, {})
            for platform in platforms:
                if platform not in field_data:
                    return False, f"Brak pola {display_name} dla platformy {platform}"
                if not field_data[platform].strip():
                    return False, f"Pole {display_name} dla platformy {platform} nie może być puste"

        # Pola dla każdego języka
        required_lang_fields = {
            "gameTitle": "tytuł",
            "devName": "deweloper",
            "description": "opis"
        }
        
        for lang_field, display_name in required_lang_fields.items():
            field_data = data.get(lang_field, {})
            # Sprawdź czy pole istnieje dla angielskiego
            if "en" not in field_data:
                return False, f"Brak pola {display_name} dla języka angielskiego"
            # Sprawdź czy pole nie jest puste dla angielskiego
            if not field_data["en"].strip():
                return False, f"Pole {display_name} dla języka angielskiego nie może być puste"
            # Sprawdź inne języki jeśli istnieją
            for lang in field_data:
                if not field_data[lang].strip():
                    return False, f"Pole {display_name} ({lang}) nie może być puste"

        # Custom Meta
        if "customMeta" not in data:
            return False, "Pole customMeta jest wymagane"
        if not isinstance(data["customMeta"], dict):
            return False, "Custom Meta musi być słownikiem"

        return True, ""

    def collect_data(self):
        # Zbiera dane z GUI do dict
        gameTitle = {}
        devName = {}
        description = {}
        changelog = {}
        minimumReqs = {}
        for lang in self.langs:
            f = self.lang_fields[lang]
            gameTitle[lang] = f['title'].text()
            devName[lang] = f['dev'].text()
            description[lang] = f['desc'].toPlainText()
            changelog[lang] = f['changelog'].toPlainText()
            minimumReqs[lang] = f['reqs'].toPlainText()
        platforms = self.get_supported_platforms()
        executableNames = {}
        exeNamesForTracking = {}
        gameURLs = {}
        for plat in platforms:
            fields = self.platform_fields[plat]
            executableNames[plat] = fields['exe'].text()
            exeNamesForTracking[plat] = fields['track'].text()
            gameURLs[plat] = fields['url'].text()
        data = {
            "name": self.name_edit.text(),
            "gameID": self.gameid_edit.text(),
            "executableNames": executableNames,
            "exeNamesForTracking": exeNamesForTracking,
            "gameURLs": gameURLs,
            "gameTitle": gameTitle,
            "devName": devName,
            "description": description,
            "changelog": changelog,
            "supportedPlatforms": platforms,
            "minimumReqs": minimumReqs,
            "videoURLs": [self.video_url.text()] if self.video_url.text() else [],
            "customMeta": self.custom_meta
        }
        return data

    def update_json_preview(self):
        data = self.collect_data()
        try:
            text = json.dumps([data], ensure_ascii=False, indent=4)
            self.preview.setPlainText(text)
            
            # Validate the data
            valid, msg = self.validate_json(data)
            if valid:
                self.validation_label.setText("✓ JSON jest poprawny")
                self.validation_label.setStyleSheet("QLabel { color: green; }")
            else:
                self.validation_label.setText(f"❌ Błąd walidacji: {msg}")
                self.validation_label.setStyleSheet("QLabel { color: red; }")
                
        except Exception as e:
            text = f"Błąd generowania JSON: {e}"
            self.preview.setPlainText(text)
            self.validation_label.setText(f"❌ Błąd składni JSON: {str(e)}")
            self.validation_label.setStyleSheet("QLabel { color: red; }")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = GameJsonCreator()
    window.show()
    sys.exit(app.exec_())
