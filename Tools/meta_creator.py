import sys
import os
import shutil
from pathlib import Path
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QLineEdit, QAction, QMessageBox,
    QListWidget, QPushButton, QGroupBox, QProgressBar,
    QFileDialog, QScrollArea, QFrame
)
from PyQt5.QtCore import Qt, QSettings, QThread, pyqtSignal
from PyQt5.QtGui import QPixmap, QPalette, QColor
from PIL import Image, ImageOps, ImageFilter, ImageEnhance


class MediaProcessor(QThread):
    progress_updated = pyqtSignal(int)
    status_updated = pyqtSignal(str)
    finished_processing = pyqtSignal(bool, str)

    def __init__(self, output_dir, icon_file, background_file, screen_files, effect_file, theme_file):
        super().__init__()
        self.output_dir = output_dir
        self.icon_file = icon_file
        self.background_file = background_file
        self.screen_files = list(screen_files) if screen_files else []
        self.effect_file = effect_file
        self.theme_file = theme_file

    def run(self):
        try:
            total_tasks = 0
            if self.icon_file: total_tasks += 1
            if self.background_file: total_tasks += 1
            if self.screen_files: total_tasks += len(self.screen_files)
            if self.effect_file: total_tasks += 1
            if self.theme_file: total_tasks += 1

            # Avoid division by zero
            if total_tasks == 0:
                self.finished_processing.emit(False, "No media to process.")
                return

            current_task = 0

            meta_dir = Path(self.output_dir) / "meta"
            screens_dir = meta_dir / "screens"
            meta_dir.mkdir(parents=True, exist_ok=True)
            screens_dir.mkdir(parents=True, exist_ok=True)

            if self.icon_file:
                self.status_updated.emit("Processing icon...")
                self.process_image(self.icon_file, meta_dir / "icon.jpg", (512, 512))
                current_task += 1
                self.progress_updated.emit(int((current_task / total_tasks) * 100))

            if self.background_file:
                self.status_updated.emit("Processing background...")
                self.process_image(self.background_file, meta_dir / "background.jpg", (1920, 1080))
                current_task += 1
                self.progress_updated.emit(int((current_task / total_tasks) * 100))

            for i, screen_file in enumerate(self.screen_files):
                self.status_updated.emit(f"Processing screenshot {i+1}...")
                self.process_image(screen_file, screens_dir / f"screen{i}.jpg", (1920, 1080))
                current_task += 1
                self.progress_updated.emit(int((current_task / total_tasks) * 100))

            if self.effect_file:
                self.status_updated.emit("Processing effect audio...")
                self.process_audio(self.effect_file, meta_dir / "effect.mp3")
                current_task += 1
                self.progress_updated.emit(int((current_task / total_tasks) * 100))

            if self.theme_file:
                self.status_updated.emit("Processing theme audio...")
                self.process_audio(self.theme_file, meta_dir / "theme.mp3")
                current_task += 1
                self.progress_updated.emit(int((current_task / total_tasks) * 100))

            self.status_updated.emit("Processing completed!")
            self.finished_processing.emit(True, "Media processing completed successfully!")

        except Exception as e:
            self.finished_processing.emit(False, f"Error during processing: {str(e)}")

    def process_image(self, input_path, output_path, target_size):
        """Process and resize image to target size with smart background handling"""
        with Image.open(input_path) as img:
            # Convert to RGB if necessary
            if img.mode in ('RGBA', 'LA', 'P'):
                background = Image.new('RGB', img.size, (255, 255, 255))
                if img.mode == 'P':
                    img = img.convert('RGBA')
                background.paste(img, mask=img.split()[-1] if img.mode in ('RGBA', 'LA') else None)
                img = background

            # Calculate aspect ratios
            img_ratio = img.width / img.height
            target_ratio = target_size[0] / target_size[1]

            if abs(img_ratio - target_ratio) < 0.1:
                # Aspect ratios are similar, use regular fit
                img = ImageOps.fit(img, target_size, Image.Resampling.LANCZOS)
            else:
                # Different aspect ratios - create smart background
                background = img.copy()

                # Scale background to fill the entire target size
                if img_ratio > target_ratio:
                    # Image is wider - scale by height
                    new_height = target_size[1]
                    new_width = int(new_height * img_ratio)
                else:
                    # Image is taller - scale by width
                    new_width = target_size[0]
                    new_height = int(new_width / img_ratio)

                background = background.resize((new_width, new_height), Image.Resampling.LANCZOS)

                # Crop from center to get target size
                left = (new_width - target_size[0]) // 2
                top = (new_height - target_size[1]) // 2
                background = background.crop((left, top, left + target_size[0], top + target_size[1]))

                # Apply blur to background
                background = background.filter(ImageFilter.GaussianBlur(radius=15))

                # Darken the background
                enhancer = ImageEnhance.Brightness(background)
                background = enhancer.enhance(0.3)  # Make it darker

                # Now scale the original image to fit within target size while maintaining aspect ratio
                img.thumbnail(target_size, Image.Resampling.LANCZOS)

                # Center the original image on the blurred background
                paste_x = (target_size[0] - img.width) // 2
                paste_y = (target_size[1] - img.height) // 2

                # Create a subtle shadow effect
                shadow = Image.new('RGBA', (img.width + 20, img.height + 20), (0, 0, 0, 0))
                shadow_img = Image.new('RGBA', shadow.size, (0, 0, 0, 128))
                shadow.paste(shadow_img, (10, 10))
                shadow = shadow.filter(ImageFilter.GaussianBlur(radius=5))

                # Paste shadow first, then the image
                if paste_x >= 10 and paste_y >= 10:
                    background.paste(shadow, (paste_x - 10, paste_y - 10), shadow)
                # If background is RGB, ensure pasted image is RGB
                if img.mode != 'RGBA':
                    background.paste(img, (paste_x, paste_y))
                else:
                    background.paste(img, (paste_x, paste_y), img)

                img = background

            # Save as JPEG with high quality
            img.save(output_path, 'JPEG', quality=95, optimize=True)

    def process_audio(self, input_path, output_path):
        """Copy audio file to destination (basic implementation)"""
        try:
            src = Path(input_path).resolve()
            dst = Path(output_path).resolve()

            if src == dst:
                # Skip copy if source and destination are the same
                return

            shutil.copy2(src, dst)
        except Exception as e:
            raise RuntimeError(f"Failed to process audio {input_path}: {e}")



class PiozaMetaCreator(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Pioza Meta Creator")
        self.setMinimumSize(900, 700)

        # Initialize data
        self.output_directory = ""
        self.icon_file = ""
        self.background_file = ""
        self.screen_files = []
        self.effect_file = ""
        self.theme_file = ""

        # Create central widget and layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QHBoxLayout(central_widget)

        # Create menu bar
        self.create_menu_bar()

        # Left side - Form
        left_widget = QWidget()
        left_layout = QVBoxLayout(left_widget)

        # Right side - Preview
        preview_widget = QWidget()
        preview_layout = QVBoxLayout(preview_widget)
        preview_layout.addWidget(QLabel("<h3>Media Preview</h3>"))

        # Preview scroll area
        self.preview_scroll = QScrollArea()
        self.preview_scroll.setWidgetResizable(True)
        self.preview_scroll.setMinimumWidth(350)
        self.preview_content = QWidget()
        self.preview_layout = QVBoxLayout(self.preview_content)
        self.preview_scroll.setWidget(self.preview_content)
        preview_layout.addWidget(self.preview_scroll)

        main_layout.addWidget(left_widget, 2)
        main_layout.addWidget(preview_widget, 1)

        # Output directory section
        output_group = QGroupBox("Project Directory")
        output_layout = QVBoxLayout(output_group)

        dir_layout = QHBoxLayout()
        self.output_edit = QLineEdit()
        self.output_edit.setPlaceholderText("Select project directory...")
        self.output_edit.setReadOnly(True)

        output_browse_btn = QPushButton("Browse...")
        output_browse_btn.clicked.connect(self.select_output_directory)

        dir_layout.addWidget(self.output_edit)
        dir_layout.addWidget(output_browse_btn)
        output_layout.addLayout(dir_layout)

        # Project status
        self.project_status = QLabel("No project loaded")
        self.project_status.setStyleSheet("color: gray; font-style: italic;")
        output_layout.addWidget(self.project_status)

        left_layout.addWidget(output_group)

        # Media files section
        media_layout = QHBoxLayout()

        # Left column - Required files
        required_group = QGroupBox("Required Files")
        required_layout = QVBoxLayout(required_group)

        # Icon
        icon_layout = QHBoxLayout()
        self.icon_edit = QLineEdit()
        self.icon_edit.setPlaceholderText("No icon selected")
        self.icon_edit.setReadOnly(True)
        icon_btn = QPushButton("Select Icon")
        icon_btn.clicked.connect(self.select_icon)
        icon_layout.addWidget(QLabel("Icon (512x512):"))
        icon_layout.addWidget(self.icon_edit)
        icon_layout.addWidget(icon_btn)
        required_layout.addLayout(icon_layout)

        # Background
        bg_layout = QHBoxLayout()
        self.background_edit = QLineEdit()
        self.background_edit.setPlaceholderText("No background selected")
        self.background_edit.setReadOnly(True)
        bg_btn = QPushButton("Select Background")
        bg_btn.clicked.connect(self.select_background)
        bg_layout.addWidget(QLabel("Background (1920x1080):"))
        bg_layout.addWidget(self.background_edit)
        bg_layout.addWidget(bg_btn)
        required_layout.addLayout(bg_layout)

        media_layout.addWidget(required_group)

        # Right column - Optional files
        optional_group = QGroupBox("Optional Files")
        optional_layout = QVBoxLayout(optional_group)

        # Effect audio
        effect_layout = QHBoxLayout()
        self.effect_edit = QLineEdit()
        self.effect_edit.setPlaceholderText("No effect audio selected")
        self.effect_edit.setReadOnly(True)
        effect_btn = QPushButton("Select Effect")
        effect_btn.clicked.connect(self.select_effect)
        effect_layout.addWidget(QLabel("Effect Audio:"))
        effect_layout.addWidget(self.effect_edit)
        effect_layout.addWidget(effect_btn)
        optional_layout.addLayout(effect_layout)

        # Theme audio
        theme_layout = QHBoxLayout()
        self.theme_edit = QLineEdit()
        self.theme_edit.setPlaceholderText("No theme audio selected")
        self.theme_edit.setReadOnly(True)
        theme_btn = QPushButton("Select Theme")
        theme_btn.clicked.connect(self.select_theme)
        theme_layout.addWidget(QLabel("Theme Audio:"))
        theme_layout.addWidget(self.theme_edit)
        theme_layout.addWidget(theme_btn)
        optional_layout.addLayout(theme_layout)

        media_layout.addWidget(optional_group)
        left_layout.addLayout(media_layout)

        # Screenshots section
        screens_group = QGroupBox("Screenshots")
        screens_layout = QVBoxLayout(screens_group)

        screens_controls = QHBoxLayout()
        add_screens_btn = QPushButton("Add Screenshots")
        add_screens_btn.clicked.connect(self.add_screenshots)
        clear_screens_btn = QPushButton("Clear All")
        clear_screens_btn.clicked.connect(self.clear_screenshots)
        screens_controls.addWidget(add_screens_btn)
        screens_controls.addWidget(clear_screens_btn)
        screens_controls.addStretch()
        screens_layout.addLayout(screens_controls)

        self.screens_list = QListWidget()
        self.screens_list.setMaximumHeight(150)
        screens_layout.addWidget(self.screens_list)

        left_layout.addWidget(screens_group)

        # Progress section
        progress_group = QGroupBox("Progress")
        progress_layout = QVBoxLayout(progress_group)

        self.progress_bar = QProgressBar()
        self.progress_bar.setVisible(False)

        self.status_label = QLabel("Ready to process media files")

        progress_layout.addWidget(self.progress_bar)
        progress_layout.addWidget(self.status_label)

        left_layout.addWidget(progress_group)

        # Control buttons
        controls_layout = QHBoxLayout()

        self.process_btn = QPushButton("Process Media")
        self.process_btn.clicked.connect(self.process_media)
        self.process_btn.setMinimumHeight(40)

        reset_btn = QPushButton("Reset All")
        reset_btn.clicked.connect(self.reset_all)

        controls_layout.addWidget(reset_btn)
        controls_layout.addStretch()
        controls_layout.addWidget(self.process_btn)

        left_layout.addLayout(controls_layout)

        # Load settings
        self.load_settings()
        self.update_preview()

    def create_menu_bar(self):
        menubar = self.menuBar()

        # File Menu
        file_menu = menubar.addMenu('&File')

        new_action = QAction('&New Project', self)
        new_action.setShortcut('Ctrl+N')
        new_action.triggered.connect(self.new_project)
        file_menu.addAction(new_action)

        open_folder_action = QAction('&Open Game Folder...', self)
        open_folder_action.setShortcut('Ctrl+O')
        open_folder_action.triggered.connect(self.load_game_folder)
        file_menu.addAction(open_folder_action)

        file_menu.addSeparator()

        reset_action = QAction('&Reset All', self)
        reset_action.setShortcut('Ctrl+R')
        reset_action.triggered.connect(self.reset_all)
        file_menu.addAction(reset_action)

        file_menu.addSeparator()

        exit_action = QAction('E&xit', self)
        exit_action.setShortcut('Alt+F4')
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)

        # Help Menu
        help_menu = menubar.addMenu('&Help')

        about_action = QAction('&About Pioza Meta Creator', self)
        about_action.triggered.connect(self.show_about_dialog)
        help_menu.addAction(about_action)

    def show_about_dialog(self):
        about_text = f"""<h2>Pioza Meta Creator</h2>
    <p>Version 0.1</p>
    <p>A comprehensive tool for organizing and processing media files for the Pioza game launcher.
    This application streamlines the preparation of games for the Pioza platform by providing
    an intuitive interface for media asset management and file organization.</p>
    <p>Features:</p>
    <ul>
        <li>Automatic image conversion and resizing</li>
        <li>Directory structure organization</li>
        <li>Media asset optimization</li>
        <li>Batch file processing</li>
        <li>Format validation and conversion</li>
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

        QMessageBox.about(self, "About Pioza Meta Creator", about_text)

    def select_output_directory(self):
        directory = QFileDialog.getExistingDirectory(self, "Select Output Directory", self.output_directory or "")
        if directory:
            self.output_directory = directory
            self.output_edit.setText(directory)
            self.save_settings()
            self.project_status.setText("Project directory selected")

    def select_icon(self):
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Select Icon Image", "",
            "Image Files (*.png *.jpg *.jpeg *.bmp *.gif *.tiff)")
        if file_path:
            self.icon_file = file_path
            self.icon_edit.setText(os.path.basename(file_path))
            self.update_preview()

    def select_background(self):
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Select Background Image", "",
            "Image Files (*.png *.jpg *.jpeg *.bmp *.gif *.tiff)")
        if file_path:
            self.background_file = file_path
            self.background_edit.setText(os.path.basename(file_path))
            self.update_preview()

    def select_effect(self):
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Select Effect Audio", "",
            "Audio Files (*.mp3 *.wav *.ogg *.flac)")
        if file_path:
            self.effect_file = file_path
            self.effect_edit.setText(os.path.basename(file_path))
            self.update_preview()

    def select_theme(self):
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Select Theme Audio", "",
            "Audio Files (*.mp3 *.wav *.ogg *.flac)")
        if file_path:
            self.theme_file = file_path
            self.theme_edit.setText(os.path.basename(file_path))
            self.update_preview()

    def add_screenshots(self):
        file_paths, _ = QFileDialog.getOpenFileNames(
            self, "Select Screenshot Images", "",
            "Image Files (*.png *.jpg *.jpeg *.bmp *.gif *.tiff)")

        for file_path in file_paths:
            if file_path not in self.screen_files:
                self.screen_files.append(file_path)
                self.screens_list.addItem(os.path.basename(file_path))

        self.update_preview()

    def clear_screenshots(self):
        self.screen_files.clear()
        self.screens_list.clear()
        self.update_preview()

    def update_preview(self):
        """Update the media preview panel"""
        # Clear existing preview
        for i in reversed(range(self.preview_layout.count())):
            item = self.preview_layout.takeAt(i)
            if item:
                w = item.widget()
                if w:
                    w.deleteLater()

        # Icon preview
        if self.icon_file:
            icon_frame = self.create_image_preview("Icon (512x512)", self.icon_file, (100, 100))
            self.preview_layout.addWidget(icon_frame)

        # Background preview
        if self.background_file:
            bg_frame = self.create_image_preview("Background (1920x1080)", self.background_file, (150, 84))
            self.preview_layout.addWidget(bg_frame)

        # Screenshots preview
        if self.screen_files:
            screens_frame = QFrame()
            screens_frame.setFrameStyle(QFrame.Box)
            screens_layout = QVBoxLayout(screens_frame)

            title_label = QLabel(f"<b>Screenshots ({len(self.screen_files)})</b>")
            screens_layout.addWidget(title_label)

            for i, screen_file in enumerate(self.screen_files):
                screen_preview = self.create_image_preview(f"screen{i}.jpg", screen_file, (120, 67))
                screens_layout.addWidget(screen_preview)

            self.preview_layout.addWidget(screens_frame)

        # Audio files preview
        audio_files = []
        if self.effect_file:
            audio_files.append(("Effect Audio", self.effect_file))
        if self.theme_file:
            audio_files.append(("Theme Audio", self.theme_file))

        if audio_files:
            audio_frame = QFrame()
            audio_frame.setFrameStyle(QFrame.Box)
            audio_layout = QVBoxLayout(audio_frame)

            title_label = QLabel("<b>Audio Files</b>")
            audio_layout.addWidget(title_label)

            for title, file_path in audio_files:
                audio_info = QLabel(f"🎵 {title}\n{os.path.basename(file_path)}")
                audio_info.setWordWrap(True)
                audio_info.setStyleSheet("padding: 5px; background-color: rgba(100, 100, 100, 50);")
                audio_layout.addWidget(audio_info)

            self.preview_layout.addWidget(audio_frame)

        # Add stretch to push content to top
        self.preview_layout.addStretch()

    def create_image_preview(self, title, image_path, size):
        """Create a preview widget for an image"""
        frame = QFrame()
        frame.setFrameStyle(QFrame.Box)
        layout = QVBoxLayout(frame)

        # Title
        title_label = QLabel(f"<b>{title}</b>")
        layout.addWidget(title_label)

        # Image
        try:
            pixmap = QPixmap(image_path)
            if not pixmap.isNull():
                # Scale image to fit preview size while maintaining aspect ratio
                scaled_pixmap = pixmap.scaled(size[0], size[1], Qt.KeepAspectRatio, Qt.SmoothTransformation)

                image_label = QLabel()
                image_label.setPixmap(scaled_pixmap)
                image_label.setAlignment(Qt.AlignCenter)
                image_label.setMinimumSize(size[0], size[1])
                image_label.setStyleSheet("border: 1px solid gray;")
                layout.addWidget(image_label)
            else:
                error_label = QLabel("❌ Invalid image")
                error_label.setAlignment(Qt.AlignCenter)
                layout.addWidget(error_label)
        except Exception:
            error_label = QLabel(f"❌ Error loading image")
            error_label.setAlignment(Qt.AlignCenter)
            layout.addWidget(error_label)

        # Filename
        filename_label = QLabel(os.path.basename(image_path))
        filename_label.setWordWrap(True)
        filename_label.setStyleSheet("color: gray; font-size: 10px;")
        layout.addWidget(filename_label)

        return frame

    def process_media(self):
        # Validate inputs
        if not self.output_directory:
            QMessageBox.warning(self, "Missing Output Directory",
                                "Please select an output directory.")
            return

        if not self.icon_file:
            QMessageBox.warning(self, "Missing Icon",
                                "Please select an icon image.")
            return

        if not self.background_file:
            QMessageBox.warning(self, "Missing Background",
                                "Please select a background image.")
            return

        # Start processing
        self.process_btn.setEnabled(False)
        self.progress_bar.setVisible(True)
        self.progress_bar.setValue(0)

        self.processor = MediaProcessor(
            self.output_directory,
            self.icon_file,
            self.background_file,
            self.screen_files,
            self.effect_file,
            self.theme_file
        )

        self.processor.progress_updated.connect(self.progress_bar.setValue)
        self.processor.status_updated.connect(self.status_label.setText)
        self.processor.finished_processing.connect(self.on_processing_finished)

        self.processor.start()

    def on_processing_finished(self, success, message):
        self.process_btn.setEnabled(True)
        self.progress_bar.setVisible(False)

        if success:
            QMessageBox.information(self, "Success", message)
            self.status_label.setText("Processing completed successfully!")
        else:
            QMessageBox.critical(self, "Error", message)
            self.status_label.setText("Processing failed!")

    def new_project(self):
        """Create a new project"""
        reply = QMessageBox.question(self, 'New Project',
                                     'This will clear all current data in the GUI. Continue?',
                                     QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.Yes:
            self.reset_all()

    def load_game_folder(self):
        """Load an existing game folder with meta directory"""
        folder_path = QFileDialog.getExistingDirectory(
            self, "Select Game Folder (containing meta directory)", "")

        if not folder_path:
            return

        meta_path = Path(folder_path) / "meta"

        if not meta_path.exists():
            reply = QMessageBox.question(self, 'No Meta Folder Found',
                                         f'The selected folder does not contain a "meta" directory.\n'
                                         f'Would you like to use this folder as output directory for a new project?',
                                         QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

            if reply == QMessageBox.Yes:
                self.output_directory = folder_path
                self.output_edit.setText(folder_path)
                self.project_status.setText("New project directory selected")
                self.save_settings()
            return

        try:
            # Set output directory (keep it)
            self.output_directory = folder_path
            self.output_edit.setText(folder_path)
            self.project_status.setText(f"Loaded existing game project")

            # Clear current media (BUT keep output_directory)
            self.icon_file = ""
            self.background_file = ""
            self.screen_files = []
            self.effect_file = ""
            self.theme_file = ""
            self.icon_edit.clear()
            self.background_edit.clear()
            self.screens_list.clear()
            self.effect_edit.clear()
            self.theme_edit.clear()
            self.update_preview()

            # Load icon
            icon_path = meta_path / "icon.jpg"
            if icon_path.exists():
                self.icon_file = str(icon_path)
                self.icon_edit.setText("icon.jpg (loaded from meta)")

            # Load background
            bg_path = meta_path / "background.jpg"
            if bg_path.exists():
                self.background_file = str(bg_path)
                self.background_edit.setText("background.jpg (loaded from meta)")

            # Load screenshots
            screens_path = meta_path / "screens"
            if screens_path.exists():
                screen_files = []
                for i in range(100):  # Check for screen0.jpg to screen99.jpg
                    screen_file = screens_path / f"screen{i}.jpg"
                    if screen_file.exists():
                        screen_files.append(str(screen_file))
                        self.screens_list.addItem(f"screen{i}.jpg (loaded from meta)")
                    else:
                        break  # No more sequential screenshots
                self.screen_files = screen_files

            # Load audio files
            effect_path = meta_path / "effect.mp3"
            if effect_path.exists():
                self.effect_file = str(effect_path)
                self.effect_edit.setText("effect.mp3 (loaded from meta)")

            theme_path = meta_path / "theme.mp3"
            if theme_path.exists():
                self.theme_file = str(theme_path)
                self.theme_edit.setText("theme.mp3 (loaded from meta)")

            self.update_preview()
            self.save_settings()

            loaded_files = []
            if self.icon_file: loaded_files.append("icon")
            if self.background_file: loaded_files.append("background")
            if self.screen_files: loaded_files.append(f"{len(self.screen_files)} screenshots")
            if self.effect_file: loaded_files.append("effect audio")
            if self.theme_file: loaded_files.append("theme audio")

            if loaded_files:
                QMessageBox.information(self, 'Game Folder Loaded',
                                        f'Successfully loaded game folder!\n\n'
                                        f'Found: {", ".join(loaded_files)}')
            else:
                QMessageBox.information(self, 'Empty Meta Folder',
                                        'Game folder loaded, but no media files were found in the meta directory.')

        except Exception as e:
            QMessageBox.critical(self, 'Error Loading Game Folder',
                                 f'Failed to load game folder:\n{str(e)}')

    def reset_all(self):
        """Reset all GUI data (do NOT clear output_directory or saved settings)"""
        # Clear internal media references
        self.icon_file = ""
        self.background_file = ""
        self.screen_files.clear()
        self.effect_file = ""
        self.theme_file = ""

        # Clear GUI fields
        self.icon_edit.clear()
        self.background_edit.clear()
        self.screens_list.clear()
        self.effect_edit.clear()
        self.theme_edit.clear()

        # Reset progress/status
        self.status_label.setText("Ready to process media files")
        self.progress_bar.setVisible(False)
        self.progress_bar.setValue(0)

        # Update project status (GUI only)
        self.project_status.setText("Project cleared")
        self.project_status.setStyleSheet("color: gray; font-style: italic;")

        # Clear preview panel
        for i in reversed(range(self.preview_layout.count())):
            item = self.preview_layout.takeAt(i)
            if item:
                w = item.widget()
                if w:
                    w.deleteLater()
        self.preview_layout.addStretch()

    def load_settings(self):
        settings = QSettings('PiozaMetaCreator', 'Settings')
        self.output_directory = settings.value('outputDirectory', '')
        if self.output_directory:
            self.output_edit.setText(self.output_directory)
            self.project_status.setText("Project directory loaded")

    def save_settings(self):
        settings = QSettings('PiozaMetaCreator', 'Settings')
        settings.setValue('outputDirectory', self.output_directory)

    def closeEvent(self, event):
        self.save_settings()
        event.accept()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setStyle('Fusion')

    # Dark theme palette
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

    window = PiozaMetaCreator()
    window.show()
    sys.exit(app.exec_())
