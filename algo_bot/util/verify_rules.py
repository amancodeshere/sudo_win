#!/usr/bin/env python3
"""Check the pinned official judge against live queen and sprint rules."""
import hashlib
import importlib.metadata
from unswbc.engine import EngineModule, WASM_PATH

TOOLKIT_VERSION = "1.2.5"
ENGINE_SHA256 = "26e68680e45eb0f221db702aead9eefde776c2ad2ba066f4ddf8c12500c6a546"


def fixture(lengths, reverse_teams=False, pearl=False):
    lines = ["MAP 16 8", "MAP_NAME Live rules conformance", "UNIT_LIMIT 64", "TILE_COUNT 128"]
    for y in range(8):
        for x in range(16):
            spawn = int(pearl and (x, y) == (10, 0))
            lines.append(f"TILE {x} {y} {spawn} {spawn}")
    lines += ["EDGE_COUNT 0", "DRAGON_COUNT 4"]
    for identity, length in enumerate(lengths):
        team = identity % 2 ^ reverse_teams
        body = " ".join(f"{9-i} {identity*2}" for i in range(length))
        lines.append(f"DRAGON {team} {length} {body}")
    return ("\n".join(lines) + "\n").encode()


def verify_engine_rules(engine=None):
    version = importlib.metadata.version("unswbc")
    digest = hashlib.sha256(WASM_PATH.read_bytes()).hexdigest()
    if version != TOOLKIT_VERSION or digest != ENGINE_SHA256:
        raise RuntimeError(f"Expected official unswbc {TOOLKIT_VERSION} engine {ENGINE_SHA256}; found {version} {digest}")
    engine = engine or EngineModule()
    for reverse in (False, True):
        result = engine.run(fixture([2, 3, 8, 2], reverse), lambda *_: b"MOVE E\nENDTURN\n", debug=0)
        expected = "A" if reverse else "B"
        if result.winner != expected:
            raise RuntimeError("Judge does not prioritize fixed queen length over the longest dragon")
    observed = {}

    def reply(identity, block):
        header = dict(line.split(maxsplit=1) for line in block.decode().splitlines()[:4])
        rnd, length = int(header["ROUND"]), int(header["LENGTH"])
        if identity == 0:
            observed[rnd] = length
        return b"MOVE EE\nENDTURN\n" if identity == 0 and rnd == 0 else b"MOVE E\nENDTURN\n"

    engine.run(fixture([5, 3, 8, 2]), reply, debug=0)
    if observed.get(1) != 5:
        raise RuntimeError("Judge does not grant two free steps to an action starting at length five")
    observed.clear()
    engine.run(fixture([4, 3, 8, 2], pearl=True), reply, debug=0)
    if observed.get(1) != 4:
        raise RuntimeError("Judge recalculates free steps after pearl growth instead of freezing starting length")

    result = engine.run(fixture([2, 3, 8, 2]),
                        lambda identity, _: b"MOVE W\nENDTURN\n" if identity == 0 else b"MOVE E\nENDTURN\n",
                        debug=0)
    if result.winner != "B" or result.a_queen != 0:
        raise RuntimeError("Judge incorrectly replaces a dead queen with another living dragon")
    return {"toolkit": version, "engine_sha256": digest, "queen_scoring": True,
            "free_sprint_steps": True, "fixed_start_length": True, "dead_queen_zero": True}


if __name__ == "__main__":
    import json
    print(json.dumps(verify_engine_rules(), indent=2))
