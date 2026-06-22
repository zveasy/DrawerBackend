#!/usr/bin/env python3
"""
Generate EOL/hardening evidence and update checklist status files.
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import re
import sys
from pathlib import Path
from typing import Dict, List, Tuple


EOL_ITEMS = [
    "Self-check",
    "Home shutter",
    "Dispense coins",
    "Scale verify",
    "Present/close",
    "Telemetry seen",
]

HARDENING_ITEMS = [
    "Service runs as `registermvp` user",
    "SSH disables passwords",
    "nftables default drop",
    "auditd logging enabled",
]


def parse_args() -> argparse.Namespace:
    repo_root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(
        description="Generate checklist evidence and update checklist files."
    )
    parser.add_argument(
        "--eol-report",
        default="",
        help="Path to EOL result.json. If omitted, script auto-discovers newest report.",
    )
    parser.add_argument(
        "--eol-checklist",
        default=str(repo_root / "docs/eol/EOL_Checklist.md"),
        help="Path to EOL checklist markdown file.",
    )
    parser.add_argument(
        "--hardening-checklist",
        default=str(repo_root / "docs/HardeningChecklist.md"),
        help="Path to hardening checklist markdown file.",
    )
    parser.add_argument(
        "--evidence-dir",
        default=str(repo_root / "dist/checklist_evidence"),
        help="Directory where evidence JSON files will be written.",
    )
    parser.add_argument(
        "--unit-file",
        default="/lib/systemd/system/register-mvp.service",
        help="Path to systemd unit used for hardening evidence.",
    )
    parser.add_argument(
        "--sshd-hardening",
        default="/etc/ssh/sshd_config.d/register-mvp.conf",
        help="Path to SSH hardening drop-in file.",
    )
    parser.add_argument(
        "--nft-conf",
        default="/etc/nftables.conf",
        help="Path to nftables configuration file.",
    )
    parser.add_argument(
        "--audit-rules",
        default="/etc/audit/rules.d/register-mvp.rules",
        help="Path to auditd rules file.",
    )
    return parser.parse_args()


def discover_latest_eol_report() -> Path | None:
    roots = [Path("/var/lib/register-mvp/eol")]
    candidates: List[Path] = []
    for root in roots:
        if root.exists():
            candidates.extend(root.glob("*/result.json"))
    if not candidates:
        return None
    return max(candidates, key=lambda p: p.stat().st_mtime)


def load_eol_evidence(eol_report: Path) -> Tuple[Dict[str, bool], Dict[str, str], Dict[str, object]]:
    raw = json.loads(eol_report.read_text(encoding="utf-8"))
    steps = raw.get("steps", [])

    step_map: Dict[str, Dict[str, object]] = {}
    for step in steps:
        name = str(step.get("name", ""))
        step_map[name] = step

    alias_map = {
        "Self-check": ["Self-check"],
        "Home shutter": ["Home shutter"],
        "Dispense coins": ["Dispense coins", "Dispense"],
        "Scale verify": ["Scale verify"],
        "Present/close": ["Present/close", "Present"],
        "Telemetry seen": ["Telemetry seen"],
    }

    statuses: Dict[str, bool] = {}
    reasons: Dict[str, str] = {}
    for item, aliases in alias_map.items():
        matched = None
        for alias in aliases:
            if alias in step_map:
                matched = step_map[alias]
                break
        if matched is None:
            statuses[item] = False
            reasons[item] = "MISSING_STEP"
            continue
        statuses[item] = bool(matched.get("ok", False))
        reasons[item] = str(matched.get("reason", ""))

    summary = {
        "serial": raw.get("serial", ""),
        "device_id": raw.get("device_id", ""),
        "version": raw.get("version", ""),
        "pass": bool(raw.get("pass", False)),
        "dispensed": raw.get("dispensed", 0),
        "expected_g": raw.get("expected_g", 0),
        "measured_g": raw.get("measured_g", 0),
        "delta_g": raw.get("delta_g", 0),
        "steps": steps,
    }
    return statuses, reasons, summary


def read_text_if_exists(path: Path) -> str:
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8", errors="replace")


def collect_hardening_evidence(
    unit_file: Path, sshd_hardening: Path, nft_conf: Path, audit_rules: Path
) -> Tuple[Dict[str, bool], Dict[str, str], Dict[str, object]]:
    statuses: Dict[str, bool] = {}
    reasons: Dict[str, str] = {}
    checks: Dict[str, object] = {}

    unit_text = read_text_if_exists(unit_file)
    unit_ok = bool(re.search(r"^User=registermvp$", unit_text, flags=re.MULTILINE))
    statuses["Service runs as `registermvp` user"] = unit_ok
    reasons["Service runs as `registermvp` user"] = "" if unit_ok else "USER_NOT_REGISTERMVP"
    checks["unit_file"] = {"path": str(unit_file), "exists": unit_file.exists(), "ok": unit_ok}

    ssh_text = read_text_if_exists(sshd_hardening)
    ssh_ok = bool(
        re.search(r"^\s*PasswordAuthentication\s+no\s*$", ssh_text, flags=re.MULTILINE | re.IGNORECASE)
    )
    statuses["SSH disables passwords"] = ssh_ok
    reasons["SSH disables passwords"] = "" if ssh_ok else "PASSWORD_AUTH_NOT_DISABLED"
    checks["sshd_hardening"] = {"path": str(sshd_hardening), "exists": sshd_hardening.exists(), "ok": ssh_ok}

    nft_text = read_text_if_exists(nft_conf)
    nft_ok = bool(re.search(r"policy\s+drop", nft_text))
    statuses["nftables default drop"] = nft_ok
    reasons["nftables default drop"] = "" if nft_ok else "DEFAULT_DROP_NOT_FOUND"
    checks["nft_conf"] = {"path": str(nft_conf), "exists": nft_conf.exists(), "ok": nft_ok}

    audit_ok = audit_rules.exists()
    statuses["auditd logging enabled"] = audit_ok
    reasons["auditd logging enabled"] = "" if audit_ok else "AUDIT_RULES_MISSING"
    checks["audit_rules"] = {"path": str(audit_rules), "exists": audit_ok, "ok": audit_ok}

    return statuses, reasons, checks


def update_checklist(path: Path, status_map: Dict[str, bool]) -> Tuple[bool, List[str]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    changed = False
    seen = set()
    out: List[str] = []

    pattern = re.compile(r"^-\s\[(?: |x|X)\]\s(.+)$")
    for line in lines:
        match = pattern.match(line)
        if not match:
            out.append(line)
            continue
        item = match.group(1)
        if item not in status_map:
            out.append(line)
            continue
        mark = "x" if status_map[item] else " "
        new_line = f"- [{mark}] {item}"
        if new_line != line:
            changed = True
        out.append(new_line)
        seen.add(item)

    missing = [item for item in status_map.keys() if item not in seen]
    path.write_text("\n".join(out) + "\n", encoding="utf-8")
    return changed, missing


def write_json(path: Path, payload: Dict[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def main() -> int:
    args = parse_args()
    ts = dt.datetime.now(tz=dt.timezone.utc).isoformat()

    if args.eol_report:
        eol_report = Path(args.eol_report)
    else:
        discovered = discover_latest_eol_report()
        if discovered is None:
            print(
                "error: could not auto-discover EOL report. Provide --eol-report <path>.",
                file=sys.stderr,
            )
            return 1
        eol_report = discovered

    if not eol_report.exists():
        print(f"error: EOL report not found: {eol_report}", file=sys.stderr)
        return 1

    eol_checklist = Path(args.eol_checklist)
    hardening_checklist = Path(args.hardening_checklist)
    evidence_dir = Path(args.evidence_dir)

    eol_statuses, eol_reasons, eol_summary = load_eol_evidence(eol_report)
    hard_statuses, hard_reasons, hard_checks = collect_hardening_evidence(
        Path(args.unit_file),
        Path(args.sshd_hardening),
        Path(args.nft_conf),
        Path(args.audit_rules),
    )

    _, eol_missing = update_checklist(eol_checklist, eol_statuses)
    _, hard_missing = update_checklist(hardening_checklist, hard_statuses)

    eol_payload = {
        "generated_at": ts,
        "source_report": str(eol_report),
        "checklist_status": eol_statuses,
        "reasons": eol_reasons,
        "report_summary": eol_summary,
    }
    hard_payload = {
        "generated_at": ts,
        "checklist_status": hard_statuses,
        "reasons": hard_reasons,
        "checks": hard_checks,
    }
    merged_payload = {
        "generated_at": ts,
        "eol": eol_payload,
        "hardening": hard_payload,
        "warnings": {
            "eol_checklist_items_missing_in_markdown": eol_missing,
            "hardening_checklist_items_missing_in_markdown": hard_missing,
        },
    }

    write_json(evidence_dir / "eol_evidence.json", eol_payload)
    write_json(evidence_dir / "hardening_evidence.json", hard_payload)
    write_json(evidence_dir / "checklist_status.json", merged_payload)

    print(f"Updated: {eol_checklist}")
    print(f"Updated: {hardening_checklist}")
    print(f"Wrote evidence: {evidence_dir / 'eol_evidence.json'}")
    print(f"Wrote evidence: {evidence_dir / 'hardening_evidence.json'}")
    print(f"Wrote summary: {evidence_dir / 'checklist_status.json'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
