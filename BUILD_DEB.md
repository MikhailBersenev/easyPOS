# Building easyPOS deb package

## Quick build (recommended)

Use the provided script for automated building:

```bash
cd /home/mikhail/Документы/easyPOS/easyPOS
./build_deb.sh
```

The script automatically:
- Checks and installs required dependencies
- Cleans previous builds (on request)
- Builds the deb package
- Shows information about the created package
- Offers to install the package

## Manual build

### Requirements

To build the deb package, you need to install the following packages:

```bash
sudo apt-get update
sudo apt-get install build-essential debhelper cmake qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools pkg-config
```

### Building the package

1. Navigate to the project root directory:
```bash
cd /home/mikhail/Документы/easyPOS/easyPOS
```

2. Build the deb package:
```bash
dpkg-buildpackage -b -us -uc
```

Or for signed build (if configured):
```bash
dpkg-buildpackage -b
```

3. After successful build, the deb package will be in the parent directory:
```bash
ls ../easypos_26.02-1_*.deb
```

## Installing the package

To install the built package:
```bash
sudo dpkg -i ../easypos_26.02-1_*.deb
```

If there are dependency issues:
```bash
sudo apt-get install -f
```

## Removing the package

To remove the installed package:
```bash
sudo dpkg -r easypos
```

## Debian package structure

- `debian/control` - package description and dependencies
- `debian/rules` - build rules
- `debian/changelog` - version history
- `debian/copyright` - license information
- `debian/easypos.install` - list of files to install
- `debian/easypos.desktop` - desktop file for application menu
- `debian/compat` - debhelper compatibility level
- `debian/source/format` - source format

## Notes

- Package version is specified in `debian/changelog` (26.02-1)
- Application is installed to `/usr/bin/easyPOS`
- Translations are installed to `/usr/share/easyPOS/i18n/`
- Styles are installed to `/usr/share/easyPOS/styles/`
- Desktop file is installed to `/usr/share/applications/`
