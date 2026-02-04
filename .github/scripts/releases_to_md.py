"""
releases_to_md.py - 从 GitHub Releases 生成美化的 Changelog Markdown
适配 VitePress，带 frontmatter 和可爱风格
"""
import os
import re
import requests

token = os.getenv("GITHUB_TOKEN", "")
repo = os.getenv("REPO", "MeowKJ/BinaryKeyboard")
headers = {"Accept": "application/vnd.github+json"}
if token:
    headers["Authorization"] = f"Bearer {token}"


def fetch_releases():
    """获取所有 releases（分页）"""
    releases = []
    page = 1
    while page <= 10:
        url = f"https://api.github.com/repos/{repo}/releases?per_page=100&page={page}"
        r = requests.get(url, headers=headers, timeout=30)
        r.raise_for_status()
        batch = r.json()
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


releases = fetch_releases()

# VitePress frontmatter
print("---")
print("outline: [2, 3]")
print("---")
print()

# 页面标题
print("# 📋 更新日志")
print()
print("::: tip 自动同步")
print("本页内容由 GitHub Actions 自动从 [Releases](https://github.com/{}/releases) 同步生成。".format(repo))
print(":::")
print()

for i, rel in enumerate(releases):
    if rel.get("draft"):
        continue

    name = rel.get("name") or rel.get("tag_name") or "Release"
    tag = rel.get("tag_name") or ""
    date = rel.get("published_at") or ""
    url = rel.get("html_url") or ""
    body = clean_body(rel.get("body") or "")
    prerelease = rel.get("prerelease", False)

    emoji = get_version_emoji(tag, prerelease)
    formatted_date = format_date(date)

    # 版本标题
    title = f"## {emoji} {tag}"
    if prerelease:
        title += " <Badge type=\"warning\" text=\"Pre-release\" />"
    print(title)
    print()

    # 元信息
    if formatted_date:
        print(f"📅 **发布时间**：{formatted_date}")
        print()

    if url:
        print(f"🔗 [在 GitHub 上查看]({url})")
        print()

    # 发布名称（如果与 tag 不同）
    if name and name != tag:
        print(f"> **{name}**")
        print()

    # 发布内容
    if body:
        print(body)
    else:
        print("_暂无发布说明_")

    print()

    # 分隔线（最后一个不加）
    if i < len([r for r in releases if not r.get("draft")]) - 1:
        print("---")
        print()
