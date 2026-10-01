#!/usr/bin/env python3
"""Reproducible matches through the pinned toolkit's engine and judge sandbox."""
from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import asdict
import hashlib
import importlib.metadata
import json
import pathlib
import shutil
import statistics
import tempfile


def stage(bot: pathlib.Path, output: pathlib.Path) -> tuple[pathlib.Path, str]:
    from unswbc.project import Project
    project = Project.from_dir(bot)
    project.collect_sources()
    digest = hashlib.sha256()
    for name in project.sources:
        digest.update(name.encode() + b"\0" + (bot / name).read_bytes())
    fingerprint = digest.hexdigest()
    target = output / fingerprint
    target.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(bot / "bot.toml", target / "bot.toml")
    for name in project.sources:
        path = target / name
        path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(bot / name, path)
    return target, fingerprint


def percentile(values: list[int], percent: int) -> int:
    if not values:
        return 0
    ordered = sorted(values)
    return ordered[max(0, (len(ordered) * percent + 99) // 100 - 1)]


def match(engine, map_path, bots, seed, sandbox, replay_path):
    from unswbc.bot import Bot, Pool
    from unswbc.engine import DEBUG_ALL, DEBUG_LIMITS
    from unswbc.sandbox import SandboxBot, WasmPool
    live, teams, pools = {}, {}, {}
    points = {"A": [], "B": []}
    errors, deaths, notices = [], [], []
    peaks = {"A": 0, "B": 0}
    turns = {"A": 0, "B": 0}
    bot_type, pool_type = (SandboxBot, WasmPool) if sandbox else (Bot, Pool)
    try:
        for team, bot in zip(("A", "B"), bots):
            pools[team] = pool_type([str(bot)], cwd=str(bot.parent), key=f"{seed:016x}-{team.lower()}")

        def spawn(dragon_id, init):
            team = next(line.split()[1] for line in init.decode().splitlines() if line.startswith("TEAM "))
            teams[dragon_id] = team
            live[dragon_id] = bot_type(pools[team], init=init, name=str(dragon_id))

        def reply(dragon_id, block):
            team, worker = teams[dragon_id], live[dragon_id]
            for line in block.decode().splitlines():
                if line.startswith("LENGTH "):
                    peaks[team] = max(peaks[team], int(line.split()[1]))
            turns[team] += 1
            output = worker.ask(block)
            if worker.error:
                errors.append({"id": dragon_id, "team": team, "error": worker.error})
            metrics = getattr(worker, "live", None)
            if metrics and metrics[0]:
                points[team].append(metrics[0])
            return output

        def death(dragon_id, round_num, reason):
            deaths.append({"id": dragon_id, "team": teams[dragon_id], "round": round_num, "reason": reason})
            worker = live.pop(dragon_id, None)
            if worker:
                worker.stop()

        result = engine.run(map_path.read_bytes(), reply, death, spawn, notices.append,
                            DEBUG_ALL | (DEBUG_LIMITS if sandbox else 0), seed)
        replay = engine.replay("candidate", "opponent")
        replay_path.write_bytes(replay)
        return {
            **asdict(result), "seed": seed, "map": str(map_path),
            "map_sha256": hashlib.sha256(map_path.read_bytes()).hexdigest(),
            "mode": "sandbox" if sandbox else "native-unmetered",
            "replay": str(replay_path), "replay_sha256": hashlib.sha256(replay).hexdigest(),
            "errors": errors, "deaths": deaths, "notices": notices,
            "peak_observed_length": peaks, "turns": turns,
            "points": {team: {"p50": percentile(v, 50), "p95": percentile(v, 95),
                               "max": max(v, default=0)} for team, v in points.items()},
        }
    finally:
        for worker in live.values():
            worker.stop()
        for pool in pools.values():
            pool.close()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("candidate", type=pathlib.Path)
    parser.add_argument("opponent", type=pathlib.Path)
    parser.add_argument("--maps", nargs="+", type=pathlib.Path)
    parser.add_argument("--seeds", nargs="+", type=lambda s: int(s, 0), default=[1, 2, 3])
    parser.add_argument("--both-colours", action="store_true")
    parser.add_argument("--repeat", type=int, default=1, help="repeat each matchup and require identical replays")
    parser.add_argument("--native", action="store_true", help="diagnostics only; no judge CPU verification")
    parser.add_argument("--max-points", type=int, default=90_000_000)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    args = parser.parse_args()
    if args.repeat < 1 or any(not 0 <= seed < 2**64 for seed in args.seeds):
        parser.error("repeat must be positive and seeds must be unsigned 64-bit integers")
    version = importlib.metadata.version("unswbc")
    if version != "1.2.2":
        parser.error(f"requires unswbc==1.2.2, found {version}")
    from unswbc import clangtool
    from unswbc.engine import EngineModule
    from unswbc.project import Project
    import unswbc
    maps = args.maps or sorted((pathlib.Path(unswbc.__file__).parent / "templates/maps").glob("*.map"))
    args.output.mkdir(parents=True, exist_ok=True)
    if (args.output / "matches.jsonl").exists():
        parser.error("choose a fresh output directory to preserve earlier results")
    failed, records, outcomes = False, [], Counter()
    with tempfile.TemporaryDirectory(prefix="sudo-win-benchmark-") as work:
        staged = [stage(bot.resolve(), pathlib.Path(work)) for bot in (args.candidate, args.opponent)]
        if args.native:
            from unswbc.toolchain import BOT_NAME
            built = [Project.from_dir(path).compile() / BOT_NAME for path, _ in staged]
        else:
            built = [clangtool.build(path) for path, _ in staged]
        engine = EngineModule()
        for index, map_path in enumerate(maps):
            for seed in args.seeds:
                for reverse in range(2 if args.both_colours else 1):
                    hashes = []
                    candidate_team = "B" if reverse else "A"
                    for repeat in range(args.repeat):
                        replay = args.output / f"{index}-{map_path.stem}-{seed}-{reverse}-{repeat}.replay"
                        record = match(engine, map_path, built[::-1] if reverse else built,
                                       seed, not args.native, replay)
                        record.update(toolkit=version, candidate_team=candidate_team,
                                      candidate_sha256=staged[0][1], opponent_sha256=staged[1][1])
                        hashes.append(record["replay_sha256"])
                        records.append(record)
                        with (args.output / "matches.jsonl").open("a") as stream:
                            stream.write(json.dumps(record, sort_keys=True) + "\n")
                        outcome = "draw" if record["winner"] is None else (
                            "win" if record["winner"] == candidate_team else "loss")
                        if repeat == 0:
                            outcomes[outcome] += 1
                        failed |= bool(record["errors"]) or any(d["reason"] == "A" for d in record["deaths"])
                        failed |= any(p["max"] > args.max_points for p in record["points"].values())
                        print(f"{map_path.name} seed={seed} colour={candidate_team} {outcome} "
                              f"lengths={record['a_length']}:{record['b_length']} "
                              f"points={record['points'][candidate_team]['max']}", flush=True)
                    failed |= len(set(hashes)) != 1
    summary = {"outcomes": dict(outcomes), "matches": len(records), "passed": not failed,
               "mode": "native-unmetered" if args.native else "sandbox",
               "max_points": max((p["max"] for r in records for p in r["points"].values()), default=0),
               "median_rounds": statistics.median(r["rounds"] + 1 for r in records)}
    (args.output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
