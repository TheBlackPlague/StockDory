#!/usr/bin/env python3

from __future__ import annotations

import os
import shutil
from pathlib import Path


APP_DIRECTORY = Path("/app")
DEFAULT_CONFIG = APP_DIRECTORY / "config.yml.default"
PERSISTENT_CONFIG = Path("/config/config.yml")
APP_CONFIG = APP_DIRECTORY / "config.yml"


def initialize_config() -> None:
    PERSISTENT_CONFIG.parent.mkdir(parents=True, exist_ok=True)

    if not PERSISTENT_CONFIG.exists():
        shutil.copyfile(DEFAULT_CONFIG, PERSISTENT_CONFIG)
        print(
            f"Created BotLi configuration at {PERSISTENT_CONFIG}. "
            "Edit this file and restart the container.",
            flush=True,
        )

    if APP_CONFIG.is_symlink() or APP_CONFIG.exists(): APP_CONFIG.unlink()

    APP_CONFIG.symlink_to(PERSISTENT_CONFIG)


def main() -> None:
    initialize_config()

    # BotLi stores certain runtime files relative to its working directory.
    os.chdir(PERSISTENT_CONFIG.parent)

    os.execvp(
        "python3",
        [
            "python3",
            "/app/user_interface.py",
            "--config",
            "/app/config.yml",
        ],
    )


if __name__ == "__main__":
    main()