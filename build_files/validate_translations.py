import argparse
import html
import json
import re
import sys
import xml.etree.ElementTree as ET
from collections import Counter
from pathlib import Path

PLACEHOLDER = re.compile(r"%(?:L)?\d+|%n|%%")
TAG = re.compile(r"<[^<>]+>")
ACCELERATOR = re.compile(r"&(?!#\d+;|#x[0-9a-fA-F]+;|[a-zA-Z]+;)([^\s&<])")
PERCENT_MNEMONIC = re.compile(r"%(?!L?\d|n|%)([A-Za-z])")
CONTEXT_BLOCK = re.compile(r"<context>.*?</context>", re.S)
MESSAGE_BLOCK = re.compile(r"<message\b.*?</message>", re.S)
SOURCE_BLOCK = re.compile(r"<source>(.*?)</source>", re.S)
NAME_BLOCK = re.compile(r"<name>(.*?)</name>", re.S)


def unescape(value):
    return html.unescape(value or "")


def text_of(element):
    if element is None:
        return ""
    return "".join(element.itertext())


def placeholders(value):
    return Counter(PLACEHOLDER.findall(value))


def tags(value):
    return Counter(TAG.findall(unescape(value)))


def accelerators(value):
    return {match.group(1).casefold() for match in ACCELERATOR.finditer(value)}


def normalized(value):
    value = unescape(value)
    value = TAG.sub("", value)
    return " ".join(value.split())


def normalize_location(filename):
    parts = filename.replace("\\", "/").split("/")
    if "src" in parts:
        parts = parts[parts.index("src") + 1:]
    return "/".join(parts)


def build_line_index(path):
    text = path.read_text(encoding="utf-8", errors="replace")
    index = {}
    for context_match in CONTEXT_BLOCK.finditer(text):
        name_match = NAME_BLOCK.search(context_match.group(0))
        if not name_match:
            continue
        context = unescape(name_match.group(1)).strip()
        block = context_match.group(0)
        offset = context_match.start()
        for message_match in MESSAGE_BLOCK.finditer(block):
            source_match = SOURCE_BLOCK.search(message_match.group(0))
            if not source_match:
                continue
            source = unescape(source_match.group(1))
            absolute = offset + message_match.start() + source_match.start()
            line = text.count("\n", 0, absolute) + 1
            index[(context, source)] = line
    return index


def parse_ts(path):
    tree = ET.parse(path)
    root = tree.getroot()
    messages = []
    for context in root.findall("context"):
        context_name = text_of(context.find("name"))
        for message in context.findall("message"):
            source = text_of(message.find("source"))
            translation_node = message.find("translation")
            translation = ""
            numerusforms = []
            if translation_node is not None:
                translation = text_of(translation_node.find("numerusform")) if translation_node.find("numerusform") is not None else (translation_node.text or "")
                numerusforms = [text_of(n) for n in translation_node.findall("numerusform")]
            messages.append({
                "context": context_name,
                "source": source,
                "translation": translation,
                "numerusforms": numerusforms,
                "type": translation_node.get("type") if translation_node is not None else None,
                "locations": [node.attrib for node in message.findall("location")],
            })
    return root.attrib, messages


def consensus_english(ts_files):
    maps = []
    for path in ts_files:
        _, messages = parse_ts(path)
        maps.append({(m["context"], m["source"]): m for m in messages})
    counts = Counter(key for mapping in maps for key in mapping)
    if not counts:
        return {}
    threshold = max(counts.values()) // 2
    keys = {key for key, count in counts.items() if count >= threshold}
    return {key: maps[0][key] for key in keys if key in maps[0]}


def source_english(source_ts):
    _, messages = parse_ts(source_ts)
    return {(m["context"], m["source"]): m for m in messages}


def source_texts(src_dir):
    texts = {}
    blob_parts = []
    for path in sorted(src_dir.rglob("*")):
        if path.suffix in {".cpp", ".h", ".hxx", ".ui"}:
            text = path.read_text(encoding="utf-8", errors="replace")
            texts[str(path.relative_to(src_dir))] = text
            blob_parts.append(text)
    blob = "\n".join(blob_parts)
    texts["__blob__"] = blob
    texts["__blob_norm__"] = normalized(blob)
    return texts


def source_present(source, texts):
    probe = normalized(source)
    if not probe:
        return True
    return probe in texts["__blob_norm__"]


def add(findings, severity, category, file_name, line, message):
    findings.append({
        "severity": severity,
        "category": category,
        "file": file_name,
        "line": line,
        "message": message,
    })


def validate_file(path, english, texts, findings):
    attrs, messages = parse_ts(path)
    file_name = path.name
    line_index = build_line_index(path)
    done = 0
    unfinished = 0
    vanished = 0
    obsolete = 0
    identical = 0
    for message in messages:
        key = (message["context"], message["source"])
        line = line_index.get(key)
        source = message["source"]
        forms = message["numerusforms"]
        values = forms if forms else [message["translation"]]
        if message["type"] == "unfinished":
            unfinished += 1
        if message["type"] == "vanished":
            vanished += 1
        if message["type"] == "obsolete":
            obsolete += 1
        if message["type"] is None and any(value.strip() for value in values):
            done += 1
        if any(value.strip() == source.strip() for value in values) and len(source.strip()) > 3:
            identical += 1
        for value in values:
            if not value:
                continue
            expected = english.get(key)
            if expected is not None:
                source_value = expected["source"]
                if placeholders(value) != placeholders(source_value):
                    add(findings, "error", "placeholder", file_name, line,
                        f'{message["context"]}: "{source}" -> placeholders differ')
                if placeholders(source_value) == Counter() and PERCENT_MNEMONIC.search(value):
                    matches = ", ".join(m.group(0) for m in PERCENT_MNEMONIC.finditer(value))
                    add(findings, "error", "accelerator-syntax", file_name, line,
                        f'{message["context"]}: "{source}" -> "%" mnemonic typo: {matches}')
                if accelerators(source_value) and not accelerators(value):
                    add(findings, "warning", "accelerator-missing", file_name, line,
                        f'{message["context"]}: "{source}" -> mnemonic missing')
                if tags(source_value) != tags(value):
                    add(findings, "warning", "html-tag", file_name, line,
                        f'{message["context"]}: "{source}" -> rich-text tags differ')
                if unescape(source_value).count("\n") != unescape(value).count("\n"):
                    add(findings, "warning", "newline", file_name, line,
                        f'{message["context"]}: "{source}" -> newline count differs')
            if value != value.rstrip() and source == source.rstrip():
                add(findings, "warning", "trailing-whitespace", file_name, line,
                    f'{message["context"]}: "{source}" -> trailing whitespace')
        if key not in english:
            add(findings, "info", "not-in-english", file_name, line,
                f'{message["context"]}: "{source}" -> not in fresh English reference')
        for location in message["locations"]:
            filename = normalize_location(location.get("filename", ""))
            line_number = location.get("line")
            if not filename:
                continue
            if filename not in texts:
                add(findings, "info", "source-file-missing", file_name, line,
                    f'{message["context"]}: "{source}" -> source file missing: {filename}')
                continue
            file_text = texts[filename]
            if line_number and line_number.isdigit():
                indexed = file_text.splitlines()
                index = int(line_number) - 1
                if 0 <= index < len(indexed) and normalized(source) in indexed[index]:
                    continue
            if not source_present(source, texts):
                add(findings, "info", "source-not-found", file_name, line,
                    f'{message["context"]}: "{source}" -> source not found in current tree')
    total = len(messages)
    return {
        "file": file_name,
        "language": attrs.get("language", ""),
        "total": total,
        "done": done,
        "unfinished": unfinished,
        "vanished": vanished,
        "obsolete": obsolete,
        "identical": identical,
        "percent": round(100 * done / total, 1) if total else 0.0,
    }


def short(message):
    message = message.replace("\n", " ")
    if len(message) > 180:
        return message[:177] + "..."
    return message


def markdown(stats, findings, english_path, ts_dir):
    lines = [
        "# Translation validation report",
        "",
        f"English reference: `{english_path}`",
        f"Translation directory: `{ts_dir}`",
        "",
        "## Coverage",
        "",
        "| Language | Done | Total | Complete | Unfinished | Same as English |",
        "|---|---:|---:|---:|---:|---:|",
    ]
    for row in stats:
        lines.append(
            f'| {row["language"]} | {row["done"]} | {row["total"]} | '
            f'{row["percent"]:.1f}% | {row["unfinished"]} | {row["identical"]} |'
        )
    counts = Counter(f["severity"] for f in findings)
    categories = Counter((f["severity"], f["category"]) for f in findings)
    lines += [
        "",
        "## Findings",
        "",
        f'- Errors: {counts["error"]}',
        f'- Warnings: {counts["warning"]}',
        f'- Info: {counts["info"]}',
        "",
        "| Severity | Category | Count |",
        "|---|---|---:|",
    ]
    for (severity, category), count in sorted(categories.items()):
        lines.append(f"| {severity} | {category} | {count} |")
    lines += [
        "",
        "## Errors and warnings",
        "",
        "| Severity | Category | File | Line | Message |",
        "|---|---|---|---:|---|",
    ]
    for finding in findings:
        if finding["severity"] == "info":
            continue
        message = short(finding["message"]).replace("|", "\\|")
        line = finding["line"] or ""
        lines.append(
            f'| {finding["severity"]} | {finding["category"]} | {finding["file"]} | '
            f'{line} | {message} |'
        )
    lines += ["", "## Informational examples", ""]
    for category in sorted({f["category"] for f in findings if f["severity"] == "info"}):
        lines.append(f"### {category}")
        lines.append("")
        for finding in [f for f in findings if f["category"] == category][:10]:
            message = short(finding["message"]).replace("|", "\\|")
            line = finding["line"] or ""
            lines.append(f'- {finding["file"]}:{line} — {message}')
        lines.append("")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--ts-dir", default="i18n", type=Path)
    parser.add_argument("--source-ts", type=Path)
    parser.add_argument("--src-dir", default="src", type=Path)
    parser.add_argument("--output", default="i18n/TRANSLATION_STATUS.md", type=Path)
    parser.add_argument("--format", choices=["text", "json"], default="text")
    parser.add_argument("--fail-on", choices=["never", "error", "warning"], default="never")
    args = parser.parse_args()

    ts_files = sorted(args.ts_dir.glob("lang_*.ts"))
    if not ts_files:
        print("No translation files found", file=sys.stderr)
        return 2

    if args.source_ts:
        english = source_english(args.source_ts)
        english_path = "generated i18n/source.ts"
    else:
        english = consensus_english(ts_files)
        english_path = "consensus of translation sources"

    texts = source_texts(args.src_dir)
    findings = []
    stats = [validate_file(path, english, texts, findings) for path in ts_files]
    stats.sort(key=lambda row: row["language"])

    report = markdown(stats, findings, english_path, str(args.ts_dir))
    if args.format == "json":
        print(json.dumps({"stats": stats, "findings": findings}, indent=2))
    else:
        print(report)
    args.output.write_text(report, encoding="utf-8")

    if args.fail_on == "error" and any(f["severity"] == "error" for f in findings):
        return 1
    if args.fail_on == "warning" and findings:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
