#
# Copyright (c) 2026 Nightwind
#

import pathlib
import urllib.request
import zipfile
import shutil

SCRIPT_DIR = pathlib.Path(__file__).parent
TARGET_PATH = SCRIPT_DIR / "files" / "PXLdaemon"
BREEZY_ZIP_PATH = SCRIPT_DIR / "Breezy.zip"
BREEZY_APP_PATH = SCRIPT_DIR / "Breezy.app"

BASE_URL = "https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/pxl/Breezy-20070827.zip"

def main() -> None:
    if TARGET_PATH.exists():
        print(f"[!] Dependencies have already been fetched!")
        return

    print(f"[+] Downloading Breezy.zip from Google Code Archive...")

    try:
        urllib.request.urlretrieve(BASE_URL, BREEZY_ZIP_PATH)
        print(f"[+] Successfully downloaded Breezy")
    except Exception as e:
        print(f"[-] Failed to download Breezy: {e}")
        return

    print(f"[+] Extracting {BREEZY_ZIP_PATH}...")

    try:
        with zipfile.ZipFile(BREEZY_ZIP_PATH, 'r') as zip_ref:
            zip_ref.extractall(SCRIPT_DIR)

        BREEZY_ZIP_PATH.unlink()
        print(f"[+] Successfully fetched Breezy.app!")
    except Exception as e:
        print(f"[-] Failed to extract {BREEZY_APP_PATH}: {e}")
        return

    print(f"[+] Copying PXLdaemon to {TARGET_PATH}")

    shutil.copyfile(BREEZY_APP_PATH / "Contents" / "Resources" / "PXLdaemon", TARGET_PATH)
    shutil.rmtree(BREEZY_APP_PATH)

    print(f"[+] Done!")

if __name__ == "__main__":
    main()
