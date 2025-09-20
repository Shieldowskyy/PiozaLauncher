import sys
import json
import os
from urllib.parse import urlparse
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, 
    QLabel, QLineEdit, QTextEdit, QAction, QMessageBox,
    QListWidget, QListWidgetItem, QPushButton, QFormLayout,
    QFileDialog
)
from PyQt5.QtCore import Qt, QSettings

class RepoManifestCreator(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Pioza Repo Manifest Creator")
        self.setMinimumSize(1000, 600)
        
        # Initialize data
        self.recent_files = []
        self.load_recent_files()
        self.current_file = None
        self.games_urls = {}
        self.custom_meta = {"": ""}
        
        # Create central widget and its layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QHBoxLayout(central_widget)
        
        # Create menu bar
        self.create_menu_bar()
        
        # Left side (form)
        left_widget = QWidget()
        left_layout = QFormLayout(left_widget)
        
        # Basic fields
        self.name_edit = QLineEdit()
        self.name_edit.setPlaceholderText("e.g. community")
        self.name_edit.textChanged.connect(self.update_json_preview)
        
        self.manifest_name_edit = QLineEdit()
        self.manifest_name_edit.setPlaceholderText("e.g. Pioza Community Manifest")
        self.manifest_name_edit.textChanged.connect(self.update_json_preview)
        
        self.manifest_desc_edit = QTextEdit()
        self.manifest_desc_edit.setPlaceholderText("Enter manifest description...")
        self.manifest_desc_edit.textChanged.connect(self.update_json_preview)
        self.manifest_desc_edit.setMaximumHeight(100)
        
        self.version_edit = QLineEdit()
        self.version_edit.setText("2")
        self.version_edit.textChanged.connect(self.update_json_preview)
        
        self.min_version_edit = QLineEdit()
        self.min_version_edit.setText("70")
        self.min_version_edit.textChanged.connect(self.update_json_preview)
        
        left_layout.addRow("Repository Name", self.name_edit)
        left_layout.addRow("Manifest Name", self.manifest_name_edit)
        left_layout.addRow("Description", self.manifest_desc_edit)
        left_layout.addRow("Manifest Version", self.version_edit)
        left_layout.addRow("Min Supported Version", self.min_version_edit)
        
        # Games URLs section
        games_widget = QWidget()
        games_layout = QVBoxLayout(games_widget)
        
        games_input_layout = QHBoxLayout()
        self.game_id_edit = QLineEdit()
        self.game_id_edit.setPlaceholderText("Game ID")
        self.game_url_edit = QLineEdit()
        self.game_url_edit.setPlaceholderText("URL to game.json")
        add_game_btn = QPushButton("Add Game")
        add_game_btn.clicked.connect(self.add_game)
        games_input_layout.addWidget(self.game_id_edit)
        games_input_layout.addWidget(self.game_url_edit)
        games_input_layout.addWidget(add_game_btn)
        games_layout.addLayout(games_input_layout)
        
        self.games_list = QListWidget()
        self.games_list.itemDoubleClicked.connect(self.remove_game)
        games_layout.addWidget(QLabel("Games (double-click to remove)"))
        games_layout.addWidget(self.games_list)
        
        left_layout.addRow("Games", games_widget)
        
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
        
        self.update_json_preview()

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
        
        # Help Menu
        help_menu = menubar.addMenu('&Help')
        
        about_action = QAction('&About Pioza Repo Manifest Creator', self)
        about_action.triggered.connect(self.show_about_dialog)
        help_menu.addAction(about_action)

    def show_about_dialog(self):
        about_text = f"""<h2>Pioza Repo Manifest Creator</h2>
<p>Version 0.1</p>

<p>A companion tool for creating repository manifest files for the Pioza Launcher. 
This application helps in managing game repositories by providing an easy way to 
create and edit repository manifest files.</p>

<p>Features:</p>
<ul>
    <li>Simple repository configuration</li>
    <li>Game URL management</li>
    <li>Real-time JSON preview</li>
    <li>Manifest validation</li>
</ul>

<p><b>Author:</b> Shieldziak (DashoGames)</p>

<p><b>License:</b> MIT</p>
<p>Copyright © 2025 Shieldziak</p>"""

        QMessageBox.about(self, "About Pioza Repo Manifest Creator", about_text)

    def add_game(self):
        game_id = self.game_id_edit.text().strip()
        url = self.game_url_edit.text().strip()
        if game_id and url:
            # Validate URL
            try:
                result = urlparse(url)
                if not all([result.scheme in ['http', 'https'], result.netloc]):
                    QMessageBox.warning(self, "Invalid URL", 
                        "Please enter a valid HTTP or HTTPS URL")
                    return
            except Exception:
                QMessageBox.warning(self, "Invalid URL", 
                    "Please enter a valid URL")
                return
            
            self.games_urls[game_id] = url
            self.games_list.addItem(f"{game_id}: {url}")
            self.game_id_edit.clear()
            self.game_url_edit.clear()
            self.update_json_preview()

    def remove_game(self, item):
        game_id = item.text().split(":")[0].strip()
        if game_id in self.games_urls:
            del self.games_urls[game_id]
            self.games_list.takeItem(self.games_list.row(item))
            self.update_json_preview()

    def validate_json(self, data):
        required_str = ["name", "manifestName", "manifestDesc"]
        for key in required_str:
            if not isinstance(data.get(key), str) or not data.get(key).strip():
                return False, f"Field '{key}' is required and cannot be empty."
                
        try:
            version = int(data.get("manifestVersion", 0))
            if version != 2:
                return False, "Manifest version must be 2"
        except ValueError:
            return False, "Manifest version must be a number"
            
        try:
            min_version = int(data.get("minSupportedVersion", 0))
            if min_version < 1:
                return False, "Minimum supported version must be positive"
        except ValueError:
            return False, "Minimum supported version must be a number"
            
        if not data.get("gamesURLs") or not isinstance(data["gamesURLs"], dict):
            return False, "At least one game URL must be specified"
            
        return True, ""

    def collect_data(self):
        data = {
            "name": self.name_edit.text(),
            "manifestName": self.manifest_name_edit.text(),
            "manifestDesc": self.manifest_desc_edit.toPlainText(),
            "manifestVersion": int(self.version_edit.text() or 2),
            "minSupportedVersion": int(self.min_version_edit.text() or 70),
            "gamesURLs": self.games_urls,
            "customMeta": self.custom_meta
        }
        return data

    def update_json_preview(self):
        try:
            data = self.collect_data()
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
            text = f"Error generating JSON: {e}"
            self.preview.setPlainText(text)
            self.validation_label.setText(f"❌ JSON syntax error: {str(e)}")
            self.validation_label.setStyleSheet("QLabel { color: red; }")

    def reset_form(self):
        self.name_edit.clear()
        self.manifest_name_edit.clear()
        self.manifest_desc_edit.clear()
        self.version_edit.setText("2")
        self.min_version_edit.setText("70")
        self.games_urls.clear()
        self.games_list.clear()
        self.validation_label.setText("")
        self.update_json_preview()

    def load_recent_files(self):
        settings = QSettings('PiozaRepoManifestCreator', 'RecentFiles')
        self.recent_files = settings.value('recentFiles', [], str)

    def save_recent_files(self):
        settings = QSettings('PiozaRepoManifestCreator', 'RecentFiles')
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

    def new_file(self):
        if self.maybe_save():
            self.reset_form()
            self.current_file = None

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

    def load_data(self, data):
        self.name_edit.setText(data.get("name", ""))
        self.manifest_name_edit.setText(data.get("manifestName", ""))
        self.manifest_desc_edit.setText(data.get("manifestDesc", ""))
        self.version_edit.setText(str(data.get("manifestVersion", 2)))
        self.min_version_edit.setText(str(data.get("minSupportedVersion", 70)))
        
        # Load games
        self.games_urls.clear()
        self.games_list.clear()
        for game_id, url in data.get("gamesURLs", {}).items():
            self.games_urls[game_id] = url
            self.games_list.addItem(f"{game_id}: {url}")
        
        # Load custom meta
        self.custom_meta = data.get("customMeta", {"": ""})
        
        self.update_json_preview()

    def load_json(self):
        if self.maybe_save():
            file_path, _ = QFileDialog.getOpenFileName(
                self, "Open repo.json", "", "JSON Files (*.json)")
            if file_path:
                self.load_json_file(file_path)

    def save_json_as(self):
        file_path, _ = QFileDialog.getSaveFileName(
            self, "Save repo.json As", "", "JSON Files (*.json)"
        )
        if file_path:
            self.current_file = file_path
            self.save_json()
            self.add_recent_file(file_path)

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

    def maybe_save(self):
        if self.is_modified():
            reply = QMessageBox.question(self, 'Pioza Repo Manifest Creator',
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

if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setStyle('Fusion')  # Use Fusion theme for a modern look
    
    # Optional: You can also customize the palette for a dark theme
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
    
    window = RepoManifestCreator()
    window.show()
    sys.exit(app.exec_())