#!/usr/bin/env python3
"""Package only manifest-selected sources, with explicit strategy profiles."""
from __future__ import annotations

import argparse
import fnmatch
import hashlib
import json
import pathlib
import re
import tomllib
import zipfile

PROFILES = {"stable": {
    "enable_sprinting": True, "enable_splitting": False, "enable_growth_splitting": True,
    "enable_long_sprint_threats": False, "enable_pocket_priority": False,
    "enable_funded_sprint_priority": False, "enable_favourable_trades": False,
    "enable_helper_portals": True, "enable_portal_escape": True, "enable_queen_hunting": True,
    "enable_sonar": True, "enable_indicators": False,
}}
PROFILES["no-sprint"] = {**PROFILES["stable"], "enable_sprinting": False}
PROFILES["no-sonar"] = {**PROFILES["stable"], "enable_sonar": False}
PROFILES["no-growth"] = {**PROFILES["stable"], "enable_growth_splitting": False}
PROFILES["no-helper-portals"] = {**PROFILES["stable"], "enable_helper_portals": False}
PROFILES["no-hunting"] = {**PROFILES["stable"], "enable_queen_hunting": False}
PROFILES["no-corridors"] = {**PROFILES["stable"], "enable_queen_corridors": False}
PROFILES["no-territory"] = {**PROFILES["stable"], "enable_territorial_growth": False}
PROFILES["no-farms"] = {**PROFILES["stable"], "enable_champion_farms": False}
PROFILES["no-portal-routes"] = {**PROFILES["stable"], "enable_portal_routing": False}
PROFILES["no-economics"] = {**PROFILES["stable"], "enable_movement_economics": False}
PROFILES["growth"] = {**PROFILES["stable"]}  # retained CLI alias
PROFILES["combat"] = {**PROFILES["stable"], "enable_favourable_trades": True}
PROFILES["experimental"] = {**PROFILES["stable"], "enable_splitting": True,
    "enable_long_sprint_threats": True, "enable_pocket_priority": True,
    "enable_funded_sprint_priority": True, "enable_favourable_trades": True}


def prepare(bot: pathlib.Path, output: pathlib.Path, profile: str) -> dict:
    bot, output = bot.resolve(), output.resolve()
    if output == bot or bot in output.parents:
        raise ValueError("submission output must be outside the bot source directory")
    if output.exists():
        raise ValueError("choose a fresh output directory; existing artifacts are preserved")
    archive = output.with_suffix(".zip")
    if archive.exists():
        raise ValueError(f"archive already exists: {archive}")
    manifest = tomllib.loads((bot / "bot.toml").read_text())
    patterns = manifest["project"]["include"]
    files = {"bot.toml": (bot / "bot.toml").read_bytes()}
    for path in sorted(bot.rglob("*")):
        if not path.is_file() or path.is_symlink():
            continue
        name = path.relative_to(bot).as_posix()
        if any(fnmatch.fnmatchcase(name, pattern) or fnmatch.fnmatchcase(path.name, pattern)
               for pattern in patterns):
            files[name] = path.read_bytes()
    config_name = "include/sudo_win/config/config.h"
    config = files[config_name].decode()
    for flag, enabled in PROFILES[profile].items():
        config, count = re.subn(rf"(inline constexpr auto {flag} = )(?:true|false)(;)",
                               rf"\g<1>{str(enabled).lower()}\2", config)
        if count != 1:
            raise ValueError(f"expected one definition of {flag}, found {count}")
    files[config_name] = config.encode()
    output.mkdir(parents=True)
    for name, content in files.items():
        target = output / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(content)
    with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED) as stream:
        for name, content in sorted(files.items()):
            info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            stream.writestr(info, content)
    digest = hashlib.sha256()
    for name, content in sorted(files.items()):
        digest.update(name.encode() + b"\0" + content)
    flags = dict(re.findall(r"inline constexpr auto (enable_\w+) = (true|false);", config))
    metadata = {"profile": profile, "files": sorted(files), "source_sha256": digest.hexdigest(),
                "effective_flags": flags, "archive": str(archive),
                "archive_sha256": hashlib.sha256(archive.read_bytes()).hexdigest()}
    output.with_suffix(".json").write_text(json.dumps(metadata, indent=2) + "\n")
    return metadata


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bot", type=pathlib.Path)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    parser.add_argument("--profile", choices=PROFILES, default="stable")
    args = parser.parse_args()
    try:
        result = prepare(args.bot, args.output, args.profile)
    except (ValueError, KeyError, OSError) as error:
        parser.error(str(error))
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
