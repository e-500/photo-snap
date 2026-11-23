# Photo Snap

**Your photo archive, on your terms.**

Photo Snap is a completely free, open-source, cross-platform desktop application for Windows, macOS, and GNU/Linux, built for managing a domestic picture archive. Bring photos from many folders into one handy, date-based library while the originals stay in their folders. Browse a fast local thumbnail catalog, find a moment by year and month, and copy the photos you want into an export folder.

No subscription. No cloud library. No reshuffling your archive.

**Platform setup guides and ready-to-install packages are coming soon.** For now, build Photo Snap from source using the instructions below.

## One library for all your folders

Photo Snap brings together folders from internal drives, external disks, and removable media in one timeline. It records each photo’s location and keeps a local thumbnail and metadata cache, so there’s no need to move or duplicate your originals just to organize them.

Browse by **year → month**, ordered by either **shooting date** or **file creation date**. When a source drive is disconnected, its cached thumbnails and catalog entries remain available; reconnect it to open or export the originals again.

## Made for everyday photo sorting

- **See your whole archive together.** Scan multiple folders and browse them in a single library.
- **Find photos by date.** Explore collapsed year and month sections using shooting or creation dates.
- **Get a quick preview.** A local SQLite cache stores metadata and small thumbnails; the library loads thumbnail batches as you scroll.
- **Correct a shooting timestamp.** Edit the EXIF shooting date and time, and Photo Snap updates its catalog entry. This operation writes the metadata to the original image file.
- **Rotate while viewing.** Turn the current view to inspect an image in the right orientation.
- **Export with copy and paste.** Copy the current photo or selected library photos, then paste them into an export folder. File copies retain their original filenames; image content can also be pasted into compatible applications.
- **Keep your archive in place.** Scanning and organizing photos does not move or rename source files.

## Get started

Open a photo from the command line, or launch Photo Snap and press **Ctrl+O**:

```sh
photo-snap /path/to/photo.jpg
```

Use **Open library** in the image title bar or press **Ctrl+Shift+B** to browse the local catalog. Choose **Scan folder** in the library title bar to add photos from a folder and its subfolders. Scanning skips photos already in the cache and builds thumbnails for new readable images.

Useful shortcuts:

| Shortcut | Action |
| --- | --- |
| `Ctrl+O` | Open a photo |
| `Ctrl+Shift+B` | Open or close the library |
| `←` / `→` | Previous or next photo in the current folder |
| `Ctrl+C` | Copy selected library photos, or the current photo |
| `Ctrl+R` | Rotate the current view |
| `F` | Fit photo to window |
| `1` | Show at actual size |
| `F11` | Toggle full screen |

## Your files stay yours

Photo Snap keeps its cache in a per-user SQLite database named `image-cache.sqlite` under the operating system’s application data directory. It stores paths, metadata, and 128 × 128 PNG thumbnails, not full copies of the photos. A small local service shares the cache between Photo Snap processes.

Removing or disconnecting a source does not erase its cached catalog entries. The cached preview remains available; the original file must be accessible to open or paste a file copy. Editing a shooting timestamp is the exception to the “originals stay untouched” workflow: it writes the corrected EXIF timestamp into that image’s metadata.

## Build

Requirements: CMake 3.16 or newer, a C++17 toolchain, and Qt 5 or Qt 6 with the Widgets, SQL, and Network modules.

Configure and build with CMake:

```sh
cmake -S . -B output/build
cmake --build output/build --config Release
```

To create a deployable application, use the script for your platform:

```powershell
.\tools\02-make-executable\windows.ps1
```

```sh
bash tools/02-make-executable/linux.sh
bash tools/02-make-executable/macos.sh
```

The Windows and Linux executables are placed in `output/bin/`; the macOS app bundle is placed in `output/`. CMake build files are kept in `output/build/`.

To check build dependencies and verify that the project can compile:

```powershell
.\tools\01-check-build-tools\windows.ps1
```

```sh
bash tools/01-check-build-tools/linux.sh
bash tools/01-check-build-tools/macos.sh
```

The scripts honor `CMAKE_PREFIX_PATH` and `Qt6_DIR` / `Qt5_DIR` when set, and check common local Qt installer locations otherwise.

## License

Photo Snap is free and open-source software released under the **GNU General Public License, version 3 or (at your option) any later version**. See [LICENSE.md](LICENSE.md) for the license text.
