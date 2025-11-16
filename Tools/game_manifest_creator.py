import sys
import json
import re
from PyQt5.QtWidgets import (
    QApplication, QWidget, QTabWidget, QVBoxLayout, QHBoxLayout, QLabel,
    QLineEdit, QTextEdit, QPushButton, QFileDialog, QComboBox, QMessageBox,
    QListWidget, QListWidgetItem, QFormLayout, QMainWindow, QAction, QMenu,
    QUndoStack, QUndoCommand, QDialog, QCheckBox, QGridLayout
)
from PyQt5.QtCore import Qt, QSettings
import os

class ReplaceDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Replace Text")
        self.setModal(True)

        layout = QGridLayout()

        self.find_edit = QLineEdit()
        self.replace_edit = QLineEdit()
        layout.addWidget(QLabel("Find:"), 0, 0)
        layout.addWidget(self.find_edit, 0, 1)
        layout.addWidget(QLabel("Replace with:"), 1, 0)
        layout.addWidget(self.replace_edit, 1, 1)

        self.case_sensitive = QCheckBox("Case sensitive")
        self.whole_word = QCheckBox("Whole word")
        layout.addWidget(self.case_sensitive, 2, 0)
        layout.addWidget(self.whole_word, 2, 1)

        btn_layout = QHBoxLayout()
        replace_btn = QPushButton("Replace All")
        replace_btn.clicked.connect(self.accept)
        cancel_btn = QPushButton("Cancel")
        cancel_btn.clicked.connect(self.reject)
        btn_layout.addWidget(replace_btn)
        btn_layout.addWidget(cancel_btn)
        layout.addLayout(btn_layout, 3, 0, 1, 2)

        self.setLayout(layout)

class GameJsonCreator(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Pioza Game Manifest Creator")
        self.setMinimumSize(1100, 700)

        self.recent_files = []
        self.load_recent_files()
        self.undo_stack = QUndoStack(self)
        self.current_file = None

        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QHBoxLayout(central_widget)

        self.create_menu_bar()

        left_widget = QWidget()
        left_layout = QVBoxLayout(left_widget)
        self.tabs = QTabWidget()
        left_layout.addWidget(self.tabs)

        right_widget = QWidget()
        right_layout = QVBoxLayout(right_widget)
        self.preview = QTextEdit()
        self.preview.setReadOnly(True)
        self.preview.setMinimumWidth(400)
        right_layout.addWidget(self.preview)

        self.validation_label = QLabel()
        self.validation_label.setWordWrap(True)
        right_layout.addWidget(self.validation_label)

        main_layout.addWidget(left_widget, 2)
        main_layout.addWidget(right_widget, 1)

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
        layout.addRow("Name", self.name_edit)
        layout.addRow("GameID", self.gameid_edit)
        self.general_tab.setLayout(layout)
        self.name_edit.textChanged.connect(self.update_json_preview)
        self.gameid_edit.textChanged.connect(self.update_json_preview)

    def init_lang_tab(self):
        layout = QVBoxLayout()
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
        fields['title'].setPlaceholderText("e.g. PixelRunner: Oldschool Edition")
        fields['dev'] = QLineEdit()
        fields['dev'].setPlaceholderText("e.g. DashoGames")
        fields['desc'] = QTextEdit()
        fields['desc'].setPlaceholderText("Game description in this language...")
        fields['changelog'] = QTextEdit()
        fields['changelog'].setPlaceholderText("Changelog in this language...")
        fields['reqs'] = QTextEdit()
        fields['reqs'].setPlaceholderText("Minimum system requirements in this language...")
        form.addRow(f"GameTitle ({lang})", fields['title'])
        form.addRow(f"DevName ({lang})", fields['dev'])
        form.addRow(f"Description ({lang})", fields['desc'])
        form.addRow(f"Changelog ({lang})", fields['changelog'])
        form.addRow(f"MinimumReqs ({lang})", fields['reqs'])
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
        if lang and lang in self.langs and len(self.langs) > 1:
            idx = self.langs.index(lang)
            self.langs.remove(lang)
            self.lang_tabs.removeTab(idx)
            self.lang_fields.pop(lang)
            self.update_json_preview()

    def load_data(self, data):
        self.name_edit.setText(data.get("Name", ""))
        self.gameid_edit.setText(data.get("GameID", ""))

        for lang, title in data.get("GameTitle", {}).items():
            if lang not in self.langs:
                self.lang_combo.setCurrentText(lang)
                self.add_language()
            f = self.lang_fields[lang]
            f['title'].setText(title)
            f['dev'].setText(data.get("DevName", {}).get(lang, ""))
            f['desc'].setPlainText(data.get("Description", {}).get(lang, ""))
            f['changelog'].setPlainText(data.get("Changelog", {}).get(lang, ""))
            f['reqs'].setPlainText(data.get("minimumReqs", {}).get(lang, ""))

        vids = data.get("VideoURLs", [])
        self.video_url.setText(vids[0] if vids else "")

        self.meta_list.clear()
        self.custom_meta = {}
        for key, value in data.get("CustomMeta", {}).items():
            self.custom_meta[key] = value
            self.meta_list.addItem(f"{key}: {value}")

        while self.platform_tabs.count() > 0:
            self.platform_tabs.removeTab(0)
        self.platform_fields.clear()

        platforms = data.get("SupportedPlatforms", [])
        executableNames = data.get("ExecutableNames", {})
        exeNamesForTracking = data.get("ExeNamesForTracking", {})
        gameURLs = data.get("GameURLs", {})

        for platform in platforms:
            self.platform_combo.setCurrentText(platform)
            self.add_platform()
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
        fields['exe'].setPlaceholderText(f"Executable name for {platform}")
        fields['exe'].textChanged.connect(self.update_json_preview)

        fields['track'] = QLineEdit()
        fields['track'].setPlaceholderText(f"Playtime Tracking Executable name for {platform}")
        fields['track'].textChanged.connect(self.update_json_preview)

        fields['url'] = QLineEdit()
        fields['url'].setPlaceholderText(f"URL to game .zip for {platform}")
        fields['url'].textChanged.connect(self.update_json_preview)

        form.addRow("ExecutableNames", fields['exe'])
        form.addRow("ExeNamesForTracking", fields['track'])
        form.addRow("GameURLs", fields['url'])

        tab.setLayout(form)
        self.platform_fields[platform] = fields
        self.platform_tabs.addTab(tab, platform)

    def init_links_tab(self):
        self.links_layout = QVBoxLayout()
        self.platform_fields = {}

        plat_ctrl_layout = QHBoxLayout()
        self.platform_combo = QComboBox()
        self.platform_combo.setEditable(True)
        self.platform_combo.addItems(["Windows", "Linux", "Android"])
        self.add_platform_btn = QPushButton("Add Platform")
        self.add_platform_btn.clicked.connect(self.add_platform)
        self.remove_platform_btn = QPushButton("Remove Platform")
        self.remove_platform_btn.clicked.connect(self.remove_platform)
        plat_ctrl_layout.addWidget(QLabel("Platforms:"))
        plat_ctrl_layout.addWidget(self.platform_combo)
        plat_ctrl_layout.addWidget(self.add_platform_btn)
        plat_ctrl_layout.addWidget(self.remove_platform_btn)
        self.links_layout.addLayout(plat_ctrl_layout)

        self.platform_tabs = QTabWidget()
        self.links_layout.addWidget(self.platform_tabs)

        video_layout = QHBoxLayout()
        self.video_url = QLineEdit()
        self.video_url.setPlaceholderText("e.g. https://youtube.com/embed/xyz (or leave empty)")
        video_layout.addWidget(QLabel("VideoURLs:"))
        video_layout.addWidget(self.video_url)
        self.links_layout.addLayout(video_layout)

        meta_group = QWidget()
        meta_layout = QVBoxLayout()
        meta_group.setLayout(meta_layout)

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

        self.meta_list = QListWidget()
        self.meta_list.itemDoubleClicked.connect(self.remove_custom_meta)
        meta_layout.addWidget(QLabel("CustomMeta (double-click to remove)"))
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

        self.recent_menu = file_menu.addMenu('Recent Files')
        self.update_recent_files_menu()

        file_menu.addSeparator()

        exit_action = QAction('E&xit', self)
        exit_action.setShortcut('Alt+F4')
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)

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
        self.name_edit.clear()
        self.gameid_edit.clear()
        self.video_url.clear()
        self.meta_list.clear()
        self.custom_meta = {}

        for lang in list(self.lang_fields.keys()):
            fields = self.lang_fields[lang]
            fields["title"].clear()
            fields["dev"].clear()
            fields["desc"].clear()
            fields["changelog"].clear()
            fields["reqs"].clear()

        while self.lang_tabs.count() > 0:
            self.lang_tabs.removeTab(0)
        self.lang_fields = {}
        self.langs = ["en"]
        self.add_language_tab("en")

        while self.platform_tabs.count() > 0:
            self.platform_tabs.removeTab(0)
        self.platform_fields.clear()

        self.validation_label.setText("")
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
        settings.setValue('recentFiles', self.recent_files[:10])

    def add_recent_file(self, file_path):
        if file_path in self.recent_files:
            self.recent_files.remove(file_path)
        self.recent_files.insert(0, file_path)
        self.recent_files = self.recent_files[:10]
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
        dialog = ReplaceDialog(self)
        if dialog.exec_() == QDialog.Accepted:
            find_text = dialog.find_edit.text()
            replace_text = dialog.replace_edit.text()
            case_sensitive = dialog.case_sensitive.isChecked()
            whole_word = dialog.whole_word.isChecked()
            count = self.replace_all_text(find_text, replace_text, case_sensitive, whole_word)
            QMessageBox.information(self, "Replace Complete",
                                  f"Replaced {count} occurrence(s)")

    def replace_all_text(self, find_text, replace_text, case_sensitive, whole_word):
        if not find_text:
            return 0

        count = 0

        def replace_in_text(text):
            nonlocal count
            if not case_sensitive:
                pattern = re.escape(find_text)
                if whole_word:
                    pattern = f"\\b{pattern}\\b"
                matches = re.finditer(pattern, text, re.IGNORECASE)
                positions = [(m.start(), m.end()) for m in matches]
                for start, end in reversed(positions):
                    text = text[:start] + replace_text + text[end:]
                    count += 1
            else:
                if whole_word:
                    text_parts = text.split()
                    new_parts = []
                    for part in text_parts:
                        if part == find_text:
                            new_parts.append(replace_text)
                            count += 1
                        else:
                            new_parts.append(part)
                    text = " ".join(new_parts)
                else:
                    text = text.replace(find_text, replace_text)
                    count += len(text.split(find_text)) - 1
            return text

        self.name_edit.setText(replace_in_text(self.name_edit.text()))
        self.gameid_edit.setText(replace_in_text(self.gameid_edit.text()))
        self.video_url.setText(replace_in_text(self.video_url.text()))

        for lang in self.langs:
            fields = self.lang_fields[lang]
            fields['title'].setText(replace_in_text(fields['title'].text()))
            fields['dev'].setText(replace_in_text(fields['dev'].text()))
            fields['desc'].setPlainText(replace_in_text(fields['desc'].toPlainText()))
            fields['changelog'].setPlainText(replace_in_text(fields['changelog'].toPlainText()))
            fields['reqs'].setPlainText(replace_in_text(fields['reqs'].toPlainText()))

        for platform in self.platform_fields:
            fields = self.platform_fields[platform]
            fields['exe'].setText(replace_in_text(fields['exe'].text()))
            fields['track'].setText(replace_in_text(fields['track'].text()))
            fields['url'].setText(replace_in_text(fields['url'].text()))

        new_meta = {}
        for key, value in self.custom_meta.items():
            new_key = replace_in_text(key)
            new_value = replace_in_text(value)
            new_meta[new_key] = new_value
        self.custom_meta = new_meta

        self.meta_list.clear()
        for key, value in self.custom_meta.items():
            self.meta_list.addItem(f"{key}: {value}")

        self.update_json_preview()

        return count

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
        required_str = ["Name", "GameID"]
        for key in required_str:
            if not isinstance(data.get(key), str) or not data.get(key).strip():
                return False, f"Field '{key}' is required and cannot be empty."

        if not data.get("SupportedPlatforms") or not isinstance(data["SupportedPlatforms"], list):
            return False, "Field 'SupportedPlatforms' must be a non-empty list."

        platforms = data["SupportedPlatforms"]
        platform_fields = {
            "ExecutableNames": "executable",
            "ExeNamesForTracking": "exe for tracking",
            "GameURLs": "game URL"
        }
        for field_name, display_name in platform_fields.items():
            field_data = data.get(field_name, {})
            for platform in platforms:
                if platform not in field_data:
                    return False, f"Missing {display_name} for platform {platform}"
                if not field_data[platform].strip():
                    return False, f"Field {display_name} for platform {platform} cannot be empty"
                if field_name == "GameURLs" and field_data[platform].strip():
                    if not self.is_valid_url(field_data[platform].strip()):
                        return False, f"Field {display_name} for platform {platform} is not a valid URL"

        required_lang_fields = {
            "GameTitle": "title",
            "DevName": "developer",
            "Description": "description",
            "Changelog": "changelog",
            "minimumReqs": "minimum requirements"
        }

        for lang_field, display_name in required_lang_fields.items():
            field_data = data.get(lang_field, {})
            if not field_data:
                return False, f"Field {lang_field} must have at least one language"
            for lang in field_data:
                if not field_data[lang].strip():
                    return False, f"Field {display_name} ({lang}) cannot be empty"

        if "VideoURLs" not in data:
            return False, "Field VideoURLs is required"
        if not isinstance(data["VideoURLs"], list):
            return False, "VideoURLs must be a list"
        for url in data.get("VideoURLs", []):
            if url and not self.is_valid_url(url):
                return False, "Video URL is not a valid URL"

        if "CustomMeta" not in data:
            return False, "Field CustomMeta is required"
        if not isinstance(data["CustomMeta"], dict):
            return False, "CustomMeta must be a dictionary"

        return True, ""

    def is_valid_url(self, url):
        regex = re.compile(
            r'^(?:http|ftp)s?://'
            r'(?:\S+(?::\S*)?@)?'
            r'(?:'
            r'(?P<private_ip>'
            r'(?:(?:10|127)\.\d{1,3}\.\d{1,3}\.\d{1,3})|'
            r'(?:(?:169\.254|192\.168)\.\d{1,3}\.\d{1,3})|'
            r'(?:172\.(?:1[6-9]|2\d|3[0-1])\.\d{1,3}\.\d{1,3})'
            r')|'
            r'(?P<public_ip>'
            r'(?:[1-9]\d?|1\d\d|2[01]\d|22[0-3])'
            r'(?:\.(?:1?\d{1,2}|2[0-4]\d|25[0-5])){3}'
            r')|'
            r'(?P<domain>'
            r'(?:[a-z\u00a1-\uffff0-9]-*)*'
            r'(?:[a-z\u00a1-\uffff0-9]+)'
            r'(?:\.(?:[a-z\u00a1-\uffff0-9]-*)*'
            r'(?:[a-z\u00a1-\uffff0-9]+))*'
            r'(?:\.(?:[a-z\u00a1-\uffff]{2,}))'
            r')'
            r')'
            r'(?::\d{2,5})?'
            r'(?:/\S*)?$', re.IGNORECASE)
        return re.match(regex, url) is not None

    def collect_data(self):
        GameTitle = {}
        DevName = {}
        Description =Description = {}
        Changelog = {}
        minimumReqs = {}
        for lang in self.langs:
            f = self.lang_fields[lang]
            GameTitle[lang] = f['title'].text()
            DevName[lang] = f['dev'].text()
            Description[lang] = f['desc'].toPlainText()
            Changelog[lang] = f['changelog'].toPlainText()
            minimumReqs[lang] = f['reqs'].toPlainText()
        platforms = self.get_supported_platforms()
        ExecutableNames = {}
        ExeNamesForTracking = {}
        GameURLs = {}
        for plat in platforms:
            fields = self.platform_fields[plat]
            ExecutableNames[plat] = fields['exe'].text()
            ExeNamesForTracking[plat] = fields['track'].text()
            GameURLs[plat] = fields['url'].text()
        data = {
            "Name": self.name_edit.text(),
            "GameID": self.gameid_edit.text(),
            "ExecutableNames": ExecutableNames,
            "ExeNamesForTracking": ExeNamesForTracking,
            "GameURLs": GameURLs,
            "GameTitle": GameTitle,
            "DevName": DevName,
            "Description": Description,
            "Changelog": Changelog,
            "SupportedPlatforms": platforms,
            "minimumReqs": minimumReqs,
            "VideoURLs": [self.video_url.text()] if self.video_url.text() else [""],
            "CustomMeta": self.custom_meta if self.custom_meta else {}
        }
        return data

    def update_json_preview(self):
        data = self.collect_data()
        try:
            text = json.dumps([data], ensure_ascii=False, indent=4)
            self.preview.setPlainText(text)

            valid, msg = self.validate_json(data)
            if valid:
                self.validation_label.setText("✓ JSON is valid")
                self.validation_label.setStyleSheet("QLabel { color: green; }")
            else:
                self.validation_label.setText(f"❌ Validation error: {msg}")
                self.validation_label.setStyleSheet("QLabel { color: red; }")

        except Exception as e:
            text = f"JSON generation error: {e}"
            self.preview.setPlainText(text)
            self.validation_label.setText(f"❌ JSON syntax error: {str(e)}")
            self.validation_label.setStyleSheet("QLabel { color: red; }")

    def show_about_dialog(self):
        about_text = f"""<h2>Pioza Game Manifest Creator</h2>
<p>Version 0.3</p>

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

if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setStyle('Fusion')

    from PyQt5.QtGui import QPalette, QColor
    palette = QPalette()
    palette.setColor(QPalette.Window, QColor(53, 53, 53))
    palette.setColor(QPalette.WindowText, QColor(255, 255, 255))
    palette.setColor(QPalette.Base, QColor(25, 25, 25))
    palette.setColor(QPalette.AlternateBase, QColor(53, 53, 53))
    palette.setColor(QPalette.ToolTipBase, QColor(255, 255, 255))
    palette.setColor(QPalette.ToolTipText, QColor(255, 255, 255))
    palette.setColor(QPalette.Text, QColor(255, 255, 255))
    palette.setColor(QPalette.Button, QColor(53, 53, 53))
    palette.setColor(QPalette.ButtonText, QColor(255, 255, 255))
    palette.setColor(QPalette.BrightText, QColor(255, 0, 0))
    palette.setColor(QPalette.Link, QColor(42, 130, 218))
    palette.setColor(QPalette.Highlight, QColor(42, 130, 218))
    palette.setColor(QPalette.HighlightedText, QColor(255, 255, 255))
    app.setPalette(palette)

    window = GameJsonCreator()
    window.show()
    sys.exit(app.exec_())