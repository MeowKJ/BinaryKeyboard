import os
import requests

token = os.getenv("GITHUB_TOKEN", "")
repo = os.getenv("REPO", "MeowKJ/BinaryKeyboard")  # 兼容本地跑
headers = {"Accept": "application/vnd.github+json"}
if token:
    headers["Authorization"] = f"Bearer {token}"

def fetch_releases():
    releases = []
    page = 1
    while page <= 10:  # 最多 1000 条 release，够用
        url = f"https://api.github.com/repos/{repo}/releases?per_page=100&page={page}"
        r = requests.get(url, headers=headers, timeout=30)
        r.raise_for_status()
        batch = r.json()
        if not batch:
            break
        releases.extend(batch)
        page += 1
    return releases

releases = fetch_releases()

print("# 更新日志\n")
print("> 本页由 GitHub Releases 自动同步生成。\n")

for rel in releases:
    if rel.get("draft"):
        continue  # 草稿不写入
    name = rel.get("name") or rel.get("tag_name") or "Release"
    tag = rel.get("tag_name") or ""
    date = (rel.get("published_at") or "")[:10]
    url = rel.get("html_url") or ""
    body = (rel.get("body") or "").rstrip()

    title = f"## {name}"
    if tag:
        title += f" ({tag})"
    if date:
        title += f" — {date}"
    if rel.get("prerelease"):
        title += "  _Pre-release_"
    print(title + "\n")

    if url:
        print(f"[在 GitHub 上查看]({url})\n")

    print(body if body else "_无发布说明_")
    print("\n---\n")
