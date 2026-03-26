"""从 GitHub Releases 生成 changelog Markdown。"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from pathlib import Path
from urllib import request


token = os.getenv("GITHUB_TOKEN", "")
repo = os.getenv("REPO", "MeowKJ/BinaryKeyboard")
headers = {
    "Accept": "application/vnd.github+json",
    "User-Agent": "BinaryKeyboard-Changelog",
}
if token:
    headers["Authorization"] = f"Bearer {token}"


def fetch_releases():
    """获取所有 releases（分页）"""
    releases = []
    page = 1
    while page <= 10:
        url = f"https://api.github.com/repos/{repo}/releases?per_page=100&page={page}"
        req = request.Request(url, headers=headers)
        with request.urlopen(req, timeout=30) as resp:
            batch = json.load(resp)
        if not batch:
            break
        releases.extend(batch)
        page += 1
    return releases


def format_date(iso_date: str) -> str:
    """格式化日期为中文友好格式"""
    if not iso_date:
        return ""
    date = iso_date[:10]
    parts = date.split("-")
    if len(parts) == 3:
        return f"{parts[0]} 年 {int(parts[1])} 月 {int(parts[2])} 日"
    return date


def clean_body(body: str) -> str:
    """清理 release body，移除重复的标题等"""
    if not body:
        return ""
    # 移除开头的 # 标题（避免重复）
    body = re.sub(r"^#+ .+\n*", "", body.strip())
    # 移除多余空行
    body = re.sub(r"\n{3,}", "\n\n", body)
    return body.strip()


def get_version_emoji(tag: str, prerelease: bool) -> str:
    """根据版本号返回对应 emoji"""
    if prerelease:
        return "🧪"
    if "alpha" in tag.lower() or "beta" in tag.lower():
        return "🧪"
    if tag.startswith("v1.") or tag == "v1":
        return "🎉"
    if ".0" in tag and tag.count(".") == 1:
        return "🚀"
    return "✨"

def render_markdown() -> str:
    releases = fetch_releases()
    published_releases = [rel for rel in releases if not rel.get("draft")]

    lines: list[str] = [
        "---",
        "outline: [2, 3]",
        "---",
        "",
        "# 📋 更新日志",
        "",
        "::: tip 自动同步",
        f"本页内容由 GitHub Actions 自动从 [Releases](https://github.com/{repo}/releases) 同步生成。",
        ":::",
        "",
    ]

    for i, rel in enumerate(published_releases):
        name = rel.get("name") or rel.get("tag_name") or "Release"
        tag = rel.get("tag_name") or ""
        date = rel.get("published_at") or ""
        url = rel.get("html_url") or ""
        body = clean_body(rel.get("body") or "")
        prerelease = rel.get("prerelease", False)

        emoji = get_version_emoji(tag, prerelease)
        formatted_date = format_date(date)

        title = f"## {emoji} {tag}"
        if prerelease:
            title += " <Badge type=\"warning\" text=\"Pre-release\" />"
        lines.extend([title, ""])

        if formatted_date:
            lines.extend([f"📅 **发布时间**：{formatted_date}", ""])

        if url:
            lines.extend([f"🔗 [在 GitHub 上查看]({url})", ""])

        if name and name != tag:
            lines.extend([f"> **{name}**", ""])

        lines.append(body if body else "_暂无发布说明_")
        lines.append("")

        if i < len(published_releases) - 1:
            lines.extend(["---", ""])

    return "\n".join(lines).rstrip() + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", default=os.getenv("OUT_FILE"))
    args = parser.parse_args()

    content = render_markdown()
    if args.out:
        Path(args.out).write_text(content, encoding="utf-8")
    else:
        sys.stdout.write(content)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
