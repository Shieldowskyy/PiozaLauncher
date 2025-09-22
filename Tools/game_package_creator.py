import sys
import os
import zipfile
import shutil
from pathlib import Path
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QLineEdit, QPushButton, QTextEdit, QMessageBox,
    QFileDialog, QFormLayout, QGroupBox, QCheckBox, QSpinBox,
    QProgressBar, QAction
)
from PyQt5.QtCore import Qt, QThread, pyqtSignal, QSettings
from PyQt5.QtGui import QPalette, QColor


class PackingThread(QThread):
    progress = pyqtSignal(int)
    status = pyqtSignal(str)
    finished_signal = pyqtSignal(bool, str)

    def __init__(self, source_folder, output_path, platform, version, compression_level):
        super().__init__()
        self.source_folder = source_folder
        self.output_path = output_path
        self.platform = platform
        self.version = str(version)
        self.compression_level = compression_level

    def run(self):
        try:
            self.status.emit("Preparing files...")
            
            # Create version.txt file in source folder
            version_file = os.path.join(self.source_folder, "version.txt")
            with open(version_file, 'w', encoding='utf-8') as f:
                f.write(self.version)
            
            # Collect all files
            all_files = []
            total_size = 0
            for root, dirs, files in os.walk(self.source_folder):
                for file in files:
                    file_path = os.path.join(root, file)
                    all_files.append(file_path)
                    if os.path.exists(file_path):
                        total_size += os.path.getsize(file_path)
            
            total_files = len(all_files)
            self.status.emit(f"Packing {total_files} files...")
            
            # Create ZIP archive
            with zipfile.ZipFile(self.output_path, 'w', zipfile.ZIP_DEFLATED, compresslevel=self.compression_level) as zipf:
                for i, file_path in enumerate(all_files):
                    # Relative path from source folder
                    rel_path = os.path.relpath(file_path, self.source_folder)
                    zipf.write(file_path, rel_path)
                    
                    # Update progress
                    progress = int((i + 1) / total_files * 100)
                    self.progress.emit(progress)
            
            # Get final ZIP size and calculate compression ratio
            zip_size = os.path.getsize(self.output_path)
            compression_ratio = (1 - zip_size / total_size) * 100 if total_size > 0 else 0
            
            # Format sizes
            def format_size(size_bytes):
                if size_bytes == 0:
                    return "0 B"
                import math
                size_names = ["B", "KB", "MB", "GB"]
                i = int(math.floor(math.log(size_bytes, 1024)))
                p = math.pow(1024, i)
                s = round(size_bytes / p, 2)
                return f"{s} {size_names[i]}"
            
            original_size_str = format_size(total_size)
            zip_size_str = format_size(zip_size)
            
            self.status.emit("Packing completed!")
            result_message = f"Game packaged to: {self.output_path}\nOriginal size: {original_size_str}\nCompressed size: {zip_size_str}\nCompression ratio: {compression_ratio:.1f}%"
            self.finished_signal.emit(True, result_message)
            
        except Exception as e:
            self.finished_signal.emit(False, f"Error during packing: {str(e)}")


class PiozaGamePackageCreator(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Pioza Game Package Creator")
        self.setMinimumSize(500, 650)
        self.resize(700, 850)
        
        # Variables
        self.game_folder = ""
        self.current_version = 0
        self.packing_thread = None
        self.output_folder = ""
        
        self.create_menu_bar()
        self.setup_ui()
        self.apply_dark_theme()
        
    def create_menu_bar(self):
        """Create menu bar"""
        menubar = self.menuBar()
        
        # File Menu
        file_menu = menubar.addMenu('&File')
        
        exit_action = QAction('E&xit', self)
        exit_action.setShortcut('Alt+F4')
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)
        
        # Help Menu
        help_menu = menubar.addMenu('&Help')
        
        about_action = QAction('&About Pioza Game Package Creator', self)
        about_action.triggered.connect(self.show_about_dialog)
        help_menu.addAction(about_action)

    def show_about_dialog(self):
        """Show about dialog"""
        about_text = f"""<h2>Pioza Game Package Creator</h2>
<p>Version 1.0</p>

<p>A tool for packaging games into ZIP archives for distribution. 
This application helps in creating game packages by providing an easy way to 
compress game files with proper version management.</p>

<p>Features:</p>
<ul>
    <li>Automatic game executable detection</li>
    <li>Version management with version.txt</li>
    <li>Configurable compression settings</li>
    <li>Multi-threaded packing option</li>
    <li>Folder size analysis</li>
</ul>

<p><b>Author:</b> Shieldziak (DashoGames)</p>

<p><b>License:</b> MIT</p>
<p>Copyright © 2025 Shieldziak</p>"""

        QMessageBox.about(self, "About Pioza Game Package Creator", about_text)
        
    def setup_ui(self):
        """Setup user interface"""
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)
        main_layout.setSpacing(15)
        main_layout.setContentsMargins(20, 20, 20, 20)
        
        # Create scroll area for better scaling
        from PyQt5.QtWidgets import QScrollArea
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        
        scroll_widget = QWidget()
        scroll_layout = QVBoxLayout(scroll_widget)
        scroll_layout.setSpacing(15)
        scroll_layout.setContentsMargins(10, 10, 10, 10)
        
        # # Title
        # title_label = QLabel("Pioza Game Package Creator")
        # title_label.setAlignment(Qt.AlignCenter)
        # title_label.setStyleSheet("font-size: 18px; font-weight: bold; margin-bottom: 10px; color: white;")
        # scroll_layout.addWidget(title_label)
        
        # Game folder selection
        folder_group = QGroupBox("Game Folder")
        folder_layout = QVBoxLayout(folder_group)
        
        folder_input_layout = QHBoxLayout()
        self.folder_path_edit = QLineEdit()
        self.folder_path_edit.setPlaceholderText("Select game folder...")
        self.folder_path_edit.setReadOnly(True)
        
        self.browse_btn = QPushButton("Browse...")
        self.browse_btn.clicked.connect(self.browse_folder)
        
        folder_input_layout.addWidget(self.folder_path_edit)
        folder_input_layout.addWidget(self.browse_btn)
        folder_layout.addLayout(folder_input_layout)
        
        # Folder info labels
        self.folder_size_label = QLabel("")
        self.exe_found_label = QLabel("")
        folder_layout.addWidget(self.folder_size_label)
        folder_layout.addWidget(self.exe_found_label)
        
        scroll_layout.addWidget(folder_group)
        
        # Version section
        version_group = QGroupBox("Version")
        version_layout = QFormLayout(version_group)
        
        version_input_layout = QHBoxLayout()
        self.version_edit = QLineEdit()
        self.version_edit.setPlaceholderText("e.g. 1")
        self.version_edit.textChanged.connect(self.validate_version)
        
        self.bump_version_btn = QPushButton("Bump Version")
        self.bump_version_btn.clicked.connect(self.bump_version)
        self.bump_version_btn.setEnabled(False)
        
        version_input_layout.addWidget(self.version_edit)
        version_input_layout.addWidget(self.bump_version_btn)
        
        version_layout.addRow("Version (integer):", version_input_layout)
        
        self.version_info_label = QLabel("")
        self.version_info_label.setStyleSheet("color: #888; font-size: 10px;")
        version_layout.addRow("", self.version_info_label)
        
        scroll_layout.addWidget(version_group)
        
        # Platform section
        platform_group = QGroupBox("Platform")
        platform_layout = QFormLayout(platform_group)
        
        self.platform_edit = QLineEdit()
        self.platform_edit.setPlaceholderText("e.g. Windows, Linux, Android...")
        self.platform_edit.textChanged.connect(self.update_ui_state)
        platform_layout.addRow("Target Platform:", self.platform_edit)
        
        scroll_layout.addWidget(platform_group)
        
        # Output folder section
        output_group = QGroupBox("Output Settings")
        output_layout = QVBoxLayout(output_group)
        
        output_input_layout = QHBoxLayout()
        self.output_folder_edit = QLineEdit()
        self.output_folder_edit.setPlaceholderText("Select output folder (optional)...")
        self.output_folder_edit.setReadOnly(True)
        
        self.browse_output_btn = QPushButton("Browse Output...")
        self.browse_output_btn.clicked.connect(self.browse_output_folder)
        
        self.clear_output_btn = QPushButton("Clear")
        self.clear_output_btn.clicked.connect(self.clear_output_folder)
        
        output_input_layout.addWidget(self.output_folder_edit)
        output_input_layout.addWidget(self.browse_output_btn)
        output_input_layout.addWidget(self.clear_output_btn)
        output_layout.addLayout(output_input_layout)
        
        output_info_label = QLabel("If not set, you'll be prompted to choose location when packing")
        output_info_label.setStyleSheet("color: #888; font-size: 10px;")
        output_layout.addWidget(output_info_label)
        
        self.output_error_label = QLabel("")
        self.output_error_label.setStyleSheet("color: #f44336; font-size: 10px;")
        output_layout.addWidget(self.output_error_label)
        
        scroll_layout.addWidget(output_group)
        
        # Compression settings
        compression_group = QGroupBox("Compression Settings")
        compression_layout = QFormLayout(compression_group)
        
        self.multithreaded_checkbox = QCheckBox("Use multithreaded compression")
        self.multithreaded_checkbox.setChecked(True)
        compression_layout.addRow("", self.multithreaded_checkbox)
        
        self.compression_spinbox = QSpinBox()
        self.compression_spinbox.setRange(1, 9)
        self.compression_spinbox.setValue(1)
        self.compression_spinbox.setToolTip("1 = Fast (recommended for testing), 6 = Balanced, 9 = Best compression (slow)")
        compression_layout.addRow("Compression Level:", self.compression_spinbox)
        
        compression_note = QLabel("Recommended: Level 1 for testing, Level 6 for release")
        compression_note.setStyleSheet("color: #888; font-size: 10px;")
        compression_layout.addRow("", compression_note)
        
        scroll_layout.addWidget(compression_group)
        
        # Packing section
        pack_group = QGroupBox("Packing")
        pack_layout = QVBoxLayout(pack_group)
        
        self.pack_btn = QPushButton("Create Package")
        self.pack_btn.clicked.connect(self.pack_game)
        self.pack_btn.setEnabled(False)
        pack_layout.addWidget(self.pack_btn)
        
        self.progress_bar = QProgressBar()
        self.progress_bar.setVisible(False)
        pack_layout.addWidget(self.progress_bar)
        
        self.status_label = QLabel("")
        self.status_label.setAlignment(Qt.AlignCenter)
        pack_layout.addWidget(self.status_label)
        
        scroll_layout.addWidget(pack_group)
        
        # Log
        log_group = QGroupBox("Log")
        log_layout = QVBoxLayout(log_group)
        
        self.log_text = QTextEdit()
        self.log_text.setMaximumHeight(120)
        self.log_text.setReadOnly(True)
        log_layout.addWidget(self.log_text)
        
        scroll_layout.addWidget(log_group)
        
        scroll_layout.addStretch()
        
        # Set scroll widget and add to main layout
        scroll.setWidget(scroll_widget)
        main_layout.addWidget(scroll)
    
    def apply_dark_theme(self):
        """Apply dark theme"""
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
        self.setPalette(palette)
    
    def log_message(self, message):
        """Add message to log"""
        from datetime import datetime
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log_text.append(f"[{timestamp}] {message}")
        # Auto-scroll to bottom
        self.log_text.verticalScrollBar().setValue(
            self.log_text.verticalScrollBar().maximum()
        )
    
    def get_folder_size(self, folder_path):
        """Calculate folder size in bytes"""
        total_size = 0
        try:
            for dirpath, dirnames, filenames in os.walk(folder_path):
                for filename in filenames:
                    file_path = os.path.join(dirpath, filename)
                    if os.path.exists(file_path):
                        total_size += os.path.getsize(file_path)
        except Exception as e:
            self.log_message(f"Error calculating folder size: {str(e)}")
        return total_size
    
    def format_size(self, size_bytes):
        """Format size in human readable format"""
        if size_bytes == 0:
            return "0 B"
        
        size_names = ["B", "KB", "MB", "GB"]
        import math
        i = int(math.floor(math.log(size_bytes, 1024)))
        p = math.pow(1024, i)
        s = round(size_bytes / p, 2)
        return f"{s} {size_names[i]}"
    
    def find_exe_files(self, folder_path):
        """Find executable files in folder"""
        exe_files = []
        try:
            # Check root folder for executables
            for file in os.listdir(folder_path):
                file_path = os.path.join(folder_path, file)
                if os.path.isfile(file_path):
                    # Windows executables
                    if file.lower().endswith(('.exe', '.msi')):
                        exe_files.append(file)
                    # Mac executables
                    elif file.lower().endswith(('.app', '.dmg')):
                        exe_files.append(file)
                    # Linux packages
                    elif file.lower().endswith(('.deb', '.rpm', '.appimage')):
                        exe_files.append(file)
                    # Check if file is executable (Linux/Unix style)
                    elif os.access(file_path, os.X_OK) and not file.startswith('.'):
                        # Additional check to avoid false positives
                        if not any(file.lower().endswith(ext) for ext in ['.txt', '.md', '.log', '.cfg', '.ini', '.json', '.xml']):
                            exe_files.append(f"{file} (executable)")
        except Exception as e:
            self.log_message(f"Error searching for executables: {str(e)}")
        return exe_files
    
    def browse_folder(self):
        """Open folder selection dialog"""
        folder = QFileDialog.getExistingDirectory(
            self, "Select Game Folder", ""
        )
        if folder:
            self.game_folder = folder
            self.folder_path_edit.setText(folder)
            self.log_message(f"Selected folder: {folder}")
            
            # Analyze folder
            self.analyze_folder()
            self.check_version_file()
            self.update_ui_state()
    
    def browse_output_folder(self):
        """Browse for output folder"""
        folder = QFileDialog.getExistingDirectory(
            self, "Select Output Folder", ""
        )
        if folder:
            self.output_folder = folder
            self.output_folder_edit.setText(folder)
            self.log_message(f"Output folder set to: {folder}")
    
    def clear_output_folder(self):
        """Clear output folder setting"""
        self.output_folder = ""
        self.output_folder_edit.clear()
        self.log_message("Output folder cleared")
    def analyze_folder(self):
        """Analyze selected folder"""
        if not self.game_folder:
            return
        
        # Calculate folder size
        folder_size = self.get_folder_size(self.game_folder)
        size_text = self.format_size(folder_size)
        self.folder_size_label.setText(f"Folder size: {size_text}")
        self.log_message(f"Folder size: {size_text}")
        
        # Find executable files
        exe_files = self.find_exe_files(self.game_folder)
        if exe_files:
            exe_text = f"Found executables: {', '.join(exe_files)}"
            self.exe_found_label.setText(exe_text)
            self.exe_found_label.setStyleSheet("color: #4CAF50;")
            self.log_message(exe_text)
        else:
            exe_text = "No executable files found"
            self.exe_found_label.setText(exe_text)
            self.exe_found_label.setStyleSheet("color: #FF9800;")
            self.log_message(exe_text)
    
    def check_version_file(self):
        """Check if version.txt exists and read current version"""
        version_file = os.path.join(self.game_folder, "version.txt")
        if os.path.exists(version_file):
            try:
                with open(version_file, 'r', encoding='utf-8') as f:
                    version_text = f.read().strip()
                
                # Try to parse as integer
                try:
                    self.current_version = int(version_text)
                    self.version_edit.setText(str(self.current_version))
                    self.version_info_label.setText(f"Found version.txt with version: {self.current_version}")
                    self.bump_version_btn.setEnabled(True)
                    self.log_message(f"Found version.txt with version: {self.current_version}")
                except ValueError:
                    self.version_info_label.setText(f"Warning: version.txt contains non-integer value: {version_text}")
                    self.bump_version_btn.setEnabled(False)
                    self.log_message(f"Warning: version.txt contains non-integer value: {version_text}")
                    
            except Exception as e:
                self.log_message(f"Error reading version.txt: {str(e)}")
                self.version_info_label.setText("Error reading version.txt")
        else:
            self.version_info_label.setText("No version.txt found - will be created")
            self.bump_version_btn.setEnabled(False)
            self.current_version = 0
            self.log_message("No version.txt found in game folder")
    
    def validate_version(self):
        """Validate version input in real-time"""
        version_text = self.version_edit.text().strip()
        if not version_text:
            self.version_info_label.setText("Version cannot be empty")
            self.version_info_label.setStyleSheet("color: #f44336;")
            self.update_ui_state()
            return
        
        try:
            version = int(version_text)
            if version < 0:
                self.version_info_label.setText("Version must be non-negative")
                self.version_info_label.setStyleSheet("color: #f44336;")
            else:
                self.version_info_label.setText("Version is valid")
                self.version_info_label.setStyleSheet("color: #4CAF50;")
        except ValueError:
            self.version_info_label.setText("Version must be an integer")
            self.version_info_label.setStyleSheet("color: #f44336;")
        
        self.update_ui_state()
    
    def bump_version(self):
        """Increase version by 1"""
        try:
            current_text = self.version_edit.text().strip()
            if current_text:
                current_version = int(current_text)
            else:
                current_version = self.current_version
            
            reply = QMessageBox.question(
                self, "Bump Version",
                f"Current version: {current_version}\n\nDo you want to increase the version?",
                QMessageBox.Yes | QMessageBox.No
            )
            
            if reply == QMessageBox.Yes:
                new_version = current_version + 1
                self.version_edit.setText(str(new_version))
                self.log_message(f"Version bumped from {current_version} to {new_version}")
        except ValueError:
            QMessageBox.warning(self, "Error", "Invalid version number")
        except Exception as e:
            self.log_message(f"Error bumping version: {str(e)}")
    
    def validate_version_input(self):
        """Validate that version is a valid integer"""
        version_text = self.version_edit.text().strip()
        if not version_text:
            return False, "Version cannot be empty"
        
        try:
            version = int(version_text)
            if version < 0:
                return False, "Version must be non-negative"
            return True, version
        except ValueError:
            return False, "Version must be an integer"
    
    def update_ui_state(self):
        """Update UI buttons based on current state"""
        has_folder = bool(self.game_folder)
        valid_version, _ = self.validate_version_input()
        has_platform = bool(self.platform_edit.text().strip())
        
        self.pack_btn.setEnabled(has_folder and valid_version and has_platform)
    
    def pack_game(self):
        """Start game packing process"""
        # Clear any previous error messages
        self.output_error_label.setText("")
        
        if not self.game_folder:
            QMessageBox.warning(self, "Error", "Please select a game folder!")
            return
        
        valid, version = self.validate_version_input()
        if not valid:
            QMessageBox.warning(self, "Error", f"Invalid version: {version}")
            return
        
        platform = self.platform_edit.text().strip()
        if not platform:
            QMessageBox.warning(self, "Error", "Please enter a platform name!")
            return
        
        # Check if output folder is set
        if not self.output_folder:
            self.output_error_label.setText("⚠️ Please set an output folder above")
            QMessageBox.warning(self, "Output Folder Required", "Please select an output folder before creating the package.")
            return
        
        # Create output directory structure
        game_name = os.path.basename(self.game_folder)
        output_dir = os.path.join(self.output_folder, f"{game_name}_{platform}")
        
        try:
            os.makedirs(output_dir, exist_ok=True)
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Could not create output directory: {str(e)}")
            return
        
        # Create paths for ZIP and version.txt
        zip_path = os.path.join(output_dir, f"{platform}.zip")
        version_txt_path = os.path.join(output_dir, "version.txt")
        
        # Create version.txt in output directory
        try:
            with open(version_txt_path, 'w', encoding='utf-8') as f:
                f.write(str(version))
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Could not create version.txt: {str(e)}")
            return
        
        # Start packing
        compression_level = self.compression_spinbox.value()
        use_threading = self.multithreaded_checkbox.isChecked()
        
        if use_threading:
            self.start_threaded_packing(zip_path, platform, version, compression_level)
        else:
            self.start_direct_packing(zip_path, platform, version, compression_level)
    
    def start_threaded_packing(self, output_path, platform, version, compression_level):
        """Start packing in separate thread"""
        self.pack_btn.setEnabled(False)
        self.progress_bar.setVisible(True)
        self.progress_bar.setValue(0)
        
        self.packing_thread = PackingThread(
            self.game_folder, output_path, platform, version, compression_level
        )
        self.packing_thread.progress.connect(self.progress_bar.setValue)
        self.packing_thread.status.connect(self.status_label.setText)
        self.packing_thread.finished_signal.connect(self.packing_finished)
        self.packing_thread.start()
        
        self.log_message(f"Started threaded packing for platform {platform} with version {version}")
    
    def start_direct_packing(self, output_path, platform, version, compression_level):
        """Start packing directly (blocking)"""
        self.pack_btn.setEnabled(False)
        self.progress_bar.setVisible(True)
        self.status_label.setText("Packing (single-threaded)...")
        
        try:
            # Create version.txt file in source folder
            version_file = os.path.join(self.game_folder, "version.txt")
            with open(version_file, 'w', encoding='utf-8') as f:
                f.write(str(version))
            
            # Collect all files and calculate total size
            all_files = []
            total_size = 0
            for root, dirs, files in os.walk(self.game_folder):
                for file in files:
                    file_path = os.path.join(root, file)
                    all_files.append(file_path)
                    if os.path.exists(file_path):
                        total_size += os.path.getsize(file_path)
            
            # Create ZIP archive
            with zipfile.ZipFile(output_path, 'w', zipfile.ZIP_DEFLATED, compresslevel=compression_level) as zipf:
                for i, file_path in enumerate(all_files):
                    rel_path = os.path.relpath(file_path, self.game_folder)
                    zipf.write(file_path, rel_path)
                    progress = int((i + 1) / len(all_files) * 100)
                    self.progress_bar.setValue(progress)
                    QApplication.processEvents()  # Keep UI responsive
            
            # Calculate compression stats
            zip_size = os.path.getsize(output_path)
            compression_ratio = (1 - zip_size / total_size) * 100 if total_size > 0 else 0
            
            # Format sizes
            original_size_str = self.format_size(total_size)
            zip_size_str = self.format_size(zip_size)
            
            result_message = f"Game packaged to: {output_path}\nOriginal size: {original_size_str}\nCompressed size: {zip_size_str}\nCompression ratio: {compression_ratio:.1f}%"
            self.packing_finished(True, result_message)
            
        except Exception as e:
            self.packing_finished(False, f"Error during packing: {str(e)}")
    
    def packing_finished(self, success, message):
        """Handle packing completion"""
        self.pack_btn.setEnabled(True)
        self.progress_bar.setVisible(False)
        self.status_label.setText("")
        
        if success:
            self.log_message("Packing completed successfully!")
            QMessageBox.information(self, "Success", message)
        else:
            self.log_message(f"Packing failed: {message}")
            QMessageBox.critical(self, "Error", message)
        
        self.packing_thread = None
        self.update_ui_state()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    
    window = PiozaGamePackageCreator()
    window.show()
    
    sys.exit(app.exec_())