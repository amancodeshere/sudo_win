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
import re
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


def donation_marker(output: str) -> tuple[int, int, int, int] | None:
    markers = [line for line in output.splitlines() if line.startswith("INDICATOR SUDO_WIN_DONATION")]
    if len(markers) != 1:
        return None
    found = re.fullmatch(r"INDICATOR SUDO_WIN_DONATION ([01]) ([0-9]+) ([0-9]+) ([2-6])", markers[0])
    return tuple(map(int, found.groups())) if found else None


def action_metrics(output: bytes) -> tuple[dict[str, int], str | None]:
    actions = [line for line in output.decode(errors="replace").splitlines()
               if line.startswith(("MOVE ", "SPLIT "))]
    if not actions and (donation := donation_marker(output.decode(errors="replace"))):
        return {"donate": 1, "predicted_queen_food": donation[3]}, None
    if len(actions) != 1:
        return {}, f"expected one final action, received {len(actions)}"
    action = actions[0]
    if re.fullmatch(r"MOVE [NESW]+", action):
        steps = len(action.split()[1])
        return {"move" if steps == 1 else "sprint": 1, "movement_steps": steps}, None
    if re.fullmatch(r"SPLIT [0-9]+", action):
        return {"split": 1, "split_segments": int(action.split()[1])}, None
    return {}, f"malformed action: {action}"


def visible_action_check(stdin: str, stdout: str) -> dict:
    """Independent protocol check of visible collisions and sprint payments.

    Unknown portal destinations/body ranks remain uncertain. This does not
    predict future enemy actions or claim a safe move is strategically good.
    """
    lines = stdin.splitlines()
    identity = int(next(line.split()[1] for line in lines if line.startswith("ID ")))
    width, height = map(int, next(line.split()[1:] for line in lines if line.startswith("MAP ")))
    length = int(next(line.split()[1] for line in lines if line.startswith("LENGTH ")))
    team = next(line.split()[1] for line in lines if line.startswith("TEAM "))
    unit_count = int(next(line.split()[1] for line in lines if line.startswith("UNIT_COUNT ")))
    start = next(i for i, line in enumerate(lines) if re.fullmatch(r"-?\d+ -?\d+ [01] -?\d+", line))
    tiles = [tuple(map(int, line.split())) for line in lines[start:start + 49]]
    pearl_tiles = {(x, y) for x, y, pearl, _ in tiles if pearl}
    body_start = start + 49
    count = int(lines[body_start].split()[1])
    parts = [line.split() for line in lines[body_start + 1:body_start + 1 + count]]
    occupants = {(int(p[2]), int(p[3])): int(p[1]) for p in parts}
    own = {(int(p[2]), int(p[3])): p[4] for p in parts if int(p[1]) == identity}
    head = next((int(p[2]), int(p[3])) for p in parts if int(p[1]) == identity and p[5] == "1")
    base_x, base_y = tiles[0][:2]
    edge_start = body_start + 1 + count
    edges, portals = {}, {}
    for orientation, rows, columns, offset in (("H", 8, 7, 0), ("V", 7, 8, 8)):
        for row in range(rows):
            for column, token in enumerate(lines[edge_start + offset + row].split()[:columns]):
                key = (orientation, (base_x + column) % width, (base_y + row) % height)
                edges[key] = token
                if token.isdigit():
                    portals.setdefault(token, set()).add(key)
    offsets = {"N": (0, -1), "E": (1, 0), "S": (0, 1), "W": (-1, 0)}

    def destination(position, direction):
        x, y = position
        boundary = (("H", x, y) if direction == "N" else
                    ("H", x, (y + 1) % height) if direction == "S" else
                    ("V", x, y) if direction == "W" else ("V", (x + 1) % width, y))
        token = edges.get(boundary)
        if token == "w":
            return None, "wall"
        if token is None:
            return None, "unknown"
        dx, dy = offsets[direction]
        if token == ".":
            return ((x + dx) % width, (y + dy) % height), "known"
        ends = portals.get(token, set())
        if len(ends) != 2:
            return None, "unknown"
        partner = next(edge for edge in ends if edge != boundary)
        if partner[0] != boundary[0]:
            return None, "unknown"
        _, x, y = partner
        return ((x - (direction == "W")) % width, (y - (direction == "N")) % height), "known"

    reasons = {}
    visible_positions = {(x, y) for x, y, _, _ in tiles}
    for direction in offsets:
        target, kind = destination(head, direction)
        reasons[direction] = kind if target is None else "unknown" if target not in visible_positions else (
            "occupied" if target in occupants else "safe")
    safe = [direction for direction, reason in reasons.items() if reason == "safe"]
    action = next((line for line in stdout.splitlines() if line.startswith("MOVE ")), None)
    result = {"safe_directions": safe, "first_step_reasons": reasons, "avoidable_collision": False}
    ranked = [head]
    while len(ranked) < length:
        previous = ranked[-1]
        segment = next((p for p, direction in own.items() if p not in ranked
                        and destination(p, direction)[0] == previous), None)
        if segment is None:
            break
        ranked.append(segment)
    if action is None:
        donation = donation_marker(stdout)
        if donation is None:
            return result
        queen_id, target_x, target_y, expected = donation
        result["verified_donation"] = False
        round_num = int(next(line.split()[1] for line in lines if line.startswith("ROUND ")))
        queens = [p for p in parts if int(p[1]) == queen_id and p[0] == team]
        queen_head = next(((int(p[2]), int(p[3])) for p in queens if p[5] == "1"), None)
        if identity <= 1 or unit_count < 3 or not 4 <= length <= 12 or round_num < 120 \
                or len(ranked) != length or len(queens) < 2 or queen_head is None:
            return result
        drops = set(ranked[::2]) - pearl_tiles
        first = (target_x, target_y)
        if first not in drops or expected * 2 < length:
            return result
        blockers = set(occupants) - set(own)
        blockers.discard(queen_head)
        queue = [(queen_head, [], set())]
        for current, path, collected in queue:
            if len(path) >= 3:
                continue
            for direction in offsets:
                target, kind = destination(current, direction)
                if kind != "known" or target not in visible_positions or target in blockers \
                        or target == queen_head or target in path:
                    continue
                route = path + [target]
                income = collected | ({target} if target in drops else set())
                first_pickup = next((p for p in route if p in drops), None)
                if len(income) >= expected and first_pickup == first:
                    result["verified_donation"] = True
                    result["predicted_queen_food"] = expected
                    return result
                queue.append((target, route, income))
        return result
    unranked = set(own) - set(ranked)
    ranked += [None] * (length - len(ranked))
    eaten = set()
    free_steps = (length + 3) // 4
    for step, direction in enumerate(action.split()[1]):
        paid = step >= free_steps
        if paid and len(ranked) <= 2:
            result.update(fatal_step=step + 1, fatal_reason="unaffordable sprint", avoidable_collision=bool(safe))
            break
        target, kind = destination(ranked[0], direction)
        if target is None:
            if kind == "wall":
                result.update(fatal_step=step + 1, fatal_reason=kind, avoidable_collision=bool(safe))
            break
        if target in ranked or (target in unranked and step == 0) or (
                target in occupants and occupants[target] != identity):
            result.update(fatal_step=step + 1, fatal_reason="occupied", avoidable_collision=bool(safe))
            victim = next((p for p in parts if (int(p[2]), int(p[3])) == target), None)
            if victim is not None and victim[0] != team and victim[5] == '1' and identity > 1 and length <= 3 and unit_count > 1:
                observed_length = sum(p[1] == victim[1] for p in parts)
                if int(victim[1]) <= 1 or observed_length >= length + 2:
                    result['favourable_head_trade'] = {'enemy_id': int(victim[1]),
                                                     'enemy_visible_length': observed_length,
                                                     'our_start_length': length,
                                                     'enemy_queen': int(victim[1]) <= 1}
            break
        if target in unranked or target not in visible_positions:
            break
        ranked.insert(0, target)
        if target in pearl_tiles and target not in eaten:
            eaten.add(target)
        else:
            ranked.pop()
        if paid:
            ranked.pop()
    return result


def match(engine, map_path, bots, seed, sandbox, replay_path, candidate_team="A", allow_favourable_trades=False):
    from unswbc.bot import Bot, Pool
    from unswbc.engine import DEBUG_ALL, DEBUG_LIMITS
    from unswbc.sandbox import SandboxBot, WasmPool
    live, teams, pools = {}, {}, {}
    points = {"A": [], "B": []}
    errors, deaths, notices = [], [], []
    peaks = {"A": 0, "B": 0}
    turns = {"A": 0, "B": 0}
    actions = {"A": Counter(), "B": Counter()}
    initial_blocks, last_turn = {}, {}
    bot_type, pool_type = (SandboxBot, WasmPool) if sandbox else (Bot, Pool)
    try:
        for team, bot in zip(("A", "B"), bots):
            pools[team] = pool_type([str(bot)], cwd=str(bot.parent), key=f"{seed:016x}-{team.lower()}")

        def spawn(dragon_id, init):
            team = next(line.split()[1] for line in init.decode().splitlines() if line.startswith("TEAM "))
            teams[dragon_id] = team
            initial_blocks[dragon_id] = init.decode()
            live[dragon_id] = bot_type(pools[team], init=init, name=str(dragon_id))

        def reply(dragon_id, block):
            team, worker = teams[dragon_id], live[dragon_id]
            for line in block.decode().splitlines():
                if line.startswith("LENGTH "):
                    peaks[team] = max(peaks[team], int(line.split()[1]))
            turns[team] += 1
            output = worker.ask(block)
            last_turn[dragon_id] = {"stdin": initial_blocks[dragon_id] + block.decode(),
                                   "stdout": output.decode(errors="replace")}
            metrics, action_error = action_metrics(output)
            actions[team].update(metrics)
            if metrics.get("donate") and not visible_action_check(
                    last_turn[dragon_id]["stdin"], last_turn[dragon_id]["stdout"]).get("verified_donation"):
                errors.append({"id": dragon_id, "team": team, "error": "unverified queen donation"})
            if action_error:
                errors.append({"id": dragon_id, "team": team, "error": action_error})
            if worker.error:
                errors.append({"id": dragon_id, "team": team, "error": worker.error})
            metrics = getattr(worker, "live", None)
            if metrics and metrics[0]:
                points[team].append(metrics[0])
            return output

        def death(dragon_id, round_num, reason):
            diagnostic = replay_path.with_suffix(f".death-{dragon_id}-{round_num}.json")
            trace = last_turn.get(dragon_id, {})
            if trace:
                trace["visible_check"] = visible_action_check(trace["stdin"], trace["stdout"])
                expected_trade = allow_favourable_trades and reason == 'H' and bool(
                    trace['visible_check'].get('favourable_head_trade'))
                if trace["visible_check"]["avoidable_collision"] and not expected_trade:
                    errors.append({"id": dragon_id, "team": teams[dragon_id],
                                   "error": "avoidable visible collision", "diagnostic": str(diagnostic)})
            diagnostic.write_text(json.dumps(trace, indent=2) + "\n")
            deaths.append({"id": dragon_id, "team": teams[dragon_id], "round": round_num, "reason": reason,
                           "diagnostic": str(diagnostic), "verified_head_trade": bool(
                               trace.get('visible_check', {}).get('favourable_head_trade')) and reason == 'H',
                           "verified_donation": bool(trace.get('visible_check', {}).get('verified_donation'))})
            worker = live.pop(dragon_id, None)
            if worker:
                worker.stop()

        result = engine.run(map_path.read_bytes(), reply, death, spawn, notices.append,
                            DEBUG_ALL | (DEBUG_LIMITS if sandbox else 0), seed)
        replay = engine.replay("candidate" if candidate_team == "A" else "opponent",
                               "opponent" if candidate_team == "A" else "candidate")
        replay_path.write_bytes(replay)
        return {
            **asdict(result), "seed": seed, "map": str(map_path),
            "map_sha256": hashlib.sha256(map_path.read_bytes()).hexdigest(),
            "mode": "sandbox" if sandbox else "native-unmetered",
            "replay": str(replay_path), "replay_sha256": hashlib.sha256(replay).hexdigest(),
            "errors": errors, "deaths": deaths, "notices": notices,
            "peak_observed_length": peaks, "turns": turns,
            "actions": {team: dict(counts) for team, counts in actions.items()},
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
    parser.add_argument("--allow-favourable-trades", action="store_true",
                        help="Allow independently verified small-helper head trades; other collision gates remain active")
    parser.add_argument("--max-points", type=int, default=90_000_000)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    args = parser.parse_args()
    if args.repeat < 1 or any(not 0 <= seed < 2**64 for seed in args.seeds):
        parser.error("repeat must be positive and seeds must be unsigned 64-bit integers")
    version = importlib.metadata.version("unswbc")
    if version != "1.2.5":
        parser.error(f"requires unswbc==1.2.5, found {version}")
    from unswbc import clangtool
    from unswbc.engine import EngineModule
    from unswbc.project import Project
    import unswbc
    maps = args.maps or sorted((pathlib.Path(unswbc.__file__).parent / "templates/maps").glob("*.map"))
    if not maps or any(not path.is_file() for path in maps):
        parser.error("each benchmark map must be an existing map file")
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
        from verify_rules import verify_engine_rules
        rules = verify_engine_rules(engine)
        for index, map_path in enumerate(maps):
            for seed in args.seeds:
                for reverse in range(2 if args.both_colours else 1):
                    hashes = []
                    candidate_team = "B" if reverse else "A"
                    for repeat in range(args.repeat):
                        replay = args.output / f"{index}-{map_path.stem}-{seed}-{reverse}-{repeat}.replay"
                        record = match(engine, map_path, built[::-1] if reverse else built,
                                       seed, not args.native, replay, candidate_team, args.allow_favourable_trades)
                        record.update(toolkit=version, live_rules=rules, candidate_team=candidate_team,
                                      benchmark_sha256=hashlib.sha256(pathlib.Path(__file__).read_bytes()).hexdigest(),
                                      candidate_sha256=staged[0][1], opponent_sha256=staged[1][1])
                        hashes.append(record["replay_sha256"])
                        records.append(record)
                        with (args.output / "matches.jsonl").open("a") as stream:
                            stream.write(json.dumps(record, sort_keys=True) + "\n")
                        outcome = "draw" if record["winner"] is None else (
                            "win" if record["winner"] == candidate_team else "loss")
                        if repeat == 0:
                            outcomes[outcome] += 1
                        # An older opponent can contain known rule bugs. Record
                        # both teams' failures, but promotion gates our candidate.
                        failed |= any(e.get('team') == candidate_team for e in record['errors'])
                        failed |= any(d['team'] == candidate_team and d['reason'] == 'A' for d in record['deaths'])
                        failed |= record['points'][candidate_team]['max'] > args.max_points
                        print(f"{map_path.name} seed={seed} colour={candidate_team} {outcome} "
                              f"lengths={record['a_length']}:{record['b_length']} "
                              f"points={record['points'][candidate_team]['max']}", flush=True)
                    failed |= len(set(hashes)) != 1
    summary = {"outcomes": dict(outcomes), "matches": len(records), "passed": not failed,
               "mode": "native-unmetered" if args.native else "sandbox",
               "max_points": max((p["max"] for r in records for p in r["points"].values()), default=0),
               "candidate_max_points": max((r['points'][r['candidate_team']]['max'] for r in records), default=0),
               "candidate_errors": sum(e.get('team') == r['candidate_team'] for r in records for e in r['errors']),
               "opponent_errors": sum(e.get('team') != r['candidate_team'] for r in records for e in r['errors']),
               "median_rounds": statistics.median(r["rounds"] + 1 for r in records)}
    candidate_actions = Counter()
    for record in records:
        candidate_actions.update(record["actions"][record["candidate_team"]])
    summary["candidate_actions"] = dict(candidate_actions)
    summary['verified_donations'] = sum(d.get('verified_donation', False) for r in records for d in r['deaths'])
    summary['verified_head_trades'] = sum(d['verified_head_trade'] for r in records for d in r['deaths'])
    (args.output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
