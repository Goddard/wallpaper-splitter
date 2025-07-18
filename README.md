# Wallpaper Splitter

A C++ application for splitting wallpapers across multiple monitors with native desktop environment support. Currently supports KDE Plasma with a modular architecture designed for easy extension to other desktop environments.

## Features

- **Multi-Monitor Support**: Automatically detects connected monitors and their layouts
- **Smart Image Splitting**: Splits images based on monitor geometry and positioning
- **Native Desktop Integration**: Uses native wallpaper application methods for each desktop environment
- **Modular Architecture**: Core functionality separated from UI for easy extension
- **KDE Plasma Interface**: Native Qt/KDE GUI with modern design
- **Command Line Interface**: Full CLI support for automation and scripting

## Architecture

The project follows a modular architecture with clear separation of concerns:

```
wallpaper-splitter/
├── include/core/           # Core library headers
│   ├── monitor_info.h      # Monitor data structures
│   ├── monitor_detector.h  # Monitor detection interface
│   ├── image_splitter.h    # Image splitting interface
│   └── wallpaper_applier.h # Wallpaper application interface
├── src/
│   ├── core/               # Core library implementation
│   │   ├── monitor_detector.cpp    # X11-based monitor detection
│   │   ├── image_splitter.cpp      # ImageMagick-based image splitting
│   │   └── wallpaper_applier.cpp   # KDE Plasma wallpaper application
│   ├── kde/                # KDE Plasma GUI
│   │   ├── main.cpp        # Application entry point
│   │   ├── mainwindow.cpp  # Main window implementation
│   │   ├── monitorwidget.cpp # Individual monitor display
│   │   └── imagepreview.cpp # Image preview widget
│   └── cli/                # Command line interface
│       └── main.cpp        # CLI implementation
└── kde/                    # KDE-specific resources
    ├── wallpaper-splitter.desktop # Desktop integration
    └── icons/              # Application icons
```

### Core Components

1. **MonitorDetector**: Detects connected monitors using X11/RandR
2. **ImageSplitter**: Splits images using ImageMagick based on monitor layout
3. **WallpaperApplier**: Applies wallpapers using desktop-specific methods

### Interface Layer

- **KDE Plasma GUI**: Native Qt/KDE interface with monitor preview and image selection
- **Command Line Interface**: Full-featured CLI for automation
- **Future**: GNOME, XFCE, and other desktop interfaces can be easily added

## Dependencies

### Required
- **Qt6**: Core Qt libraries (Core, Widgets)
- **KF6**: KDE Frameworks (CoreAddons, WidgetsAddons, I18n)
- **ImageMagick++**: Image processing library
- **X11**: X11 development libraries
- **CMake**: Build system

### Installation on Manjaro/Arch
```bash
sudo pacman -S qt6-base qt6-tools kf6-kcoreaddons kf6-kwidgetsaddons kf6-ki18n imagemagick libx11 cmake
```

### Installation on Ubuntu/Debian
```bash
sudo apt install qt6-base-dev qt6-tools-dev libkf6coreaddons-dev libkf6widgetsaddons-dev libkf6i18n-dev libmagick++-dev libx11-dev cmake
```

## Building

1. **Clone the repository**:
   ```bash
   git clone https://github.com/yourusername/wallpaper-splitter.git
   cd wallpaper-splitter
   ```

2. **Create build directory**:
   ```bash
   mkdir build && cd build
   ```

3. **Configure and build**:
   ```bash
   cmake ..
   make -j$(nproc)
   ```

4. **Install** (optional):
   ```bash
   sudo make install
   ```

## Usage

### Graphical Interface (KDE Plasma)

1. Launch the application:
   ```bash
   wallpaper-splitter-kde
   ```

2. Click "Select Image" to choose your wallpaper image
3. Review detected monitors in the list
4. Click "Apply Wallpapers" to split and apply the wallpaper

### Command Line Interface

**List detected monitors**:
```bash
wallpaper-splitter-cli -l
```

**Split image without applying**:
```bash
wallpaper-splitter-cli -i /path/to/image.jpg -o /output/directory
```

**Split and apply wallpapers**:
```bash
wallpaper-splitter-cli -i /path/to/image.jpg -a
```

**Full options**:
```bash
wallpaper-splitter-cli -i /path/to/image.jpg -o /output/directory -a
```

## How It Works

1. **Monitor Detection**: Uses X11 RandR extension to detect connected monitors, their resolutions, and positions
2. **Image Splitting**: Calculates the optimal crop regions for each monitor based on their virtual desktop positions
3. **Wallpaper Application**: Uses desktop-specific commands (e.g., `plasma-apply-wallpaperimage` for KDE) to apply wallpapers

### Image Splitting Algorithm

The application calculates the virtual desktop size encompassing all monitors and scales the input image to match. Each monitor's wallpaper is then cropped from the appropriate region of the scaled image.

## Extending for Other Desktop Environments

To add support for other desktop environments (e.g., GNOME, XFCE):

1. **Create a new wallpaper applier**:
   - Inherit from `WallpaperApplier`
   - Implement desktop-specific wallpaper application logic
   - Add detection for the desktop environment

2. **Create a new GUI interface**:
   - Create a new directory under `src/` (e.g., `src/gnome/`)
   - Implement native UI components for that desktop
   - Use the same core library components

3. **Update CMakeLists.txt**:
   - Add new targets for the interface
   - Include appropriate dependencies

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## License

This project is licensed under the GPL v3 License - see the LICENSE file for details.

## Troubleshooting

### Common Issues

**No monitors detected**:
- Ensure X11 is running
- Check that RandR extension is available
- Verify monitor connections

**Image splitting fails**:
- Ensure ImageMagick is properly installed
- Check image file permissions
- Verify image format is supported

**Wallpaper application fails**:
- Ensure you're running on a supported desktop environment
- Check that desktop-specific wallpaper tools are installed
- Verify monitor names match detected outputs

### Debug Mode

Run with debug output:
```bash
QT_LOGGING_RULES="*=true" wallpaper-splitter-kde
```

## Roadmap

- [ ] GNOME Shell support
- [ ] XFCE support
- [ ] Wayland support
- [ ] Automatic wallpaper rotation
- [ ] Preset configurations
- [ ] Batch processing
- [ ] Plugin system for custom wallpaper sources 