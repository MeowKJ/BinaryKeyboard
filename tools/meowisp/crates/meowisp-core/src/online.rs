use serde::Deserialize;
use std::cmp::Ordering;
use std::collections::HashMap;
use std::collections::HashSet;
use std::fs;
use std::fs::File;
use std::io;
use std::path::PathBuf;
use std::time::{SystemTime, UNIX_EPOCH};

const MANIFEST_URL: &str = "https://meowkj.github.io/BinaryKeyboard/api/release-manifest.json";
const RELEASES_API: &str =
    "https://api.github.com/repos/MeowKJ/BinaryKeyboard/releases?per_page=100";

#[derive(Debug, Clone)]
pub struct FirmwareDescriptor {
    pub chip: String,
    pub keyboard: String,
    pub version: String,
    pub flavor: String,
    pub category: String,
    pub family: String,
}

#[derive(Debug, Clone)]
pub struct ReleaseAsset {
    pub name: String,
    pub download_url: String,
    pub release_tag: String,
    pub published_at: String,
    pub size_bytes: u64,
    pub descriptor: FirmwareDescriptor,
}

#[derive(Debug, Deserialize)]
struct GitHubRelease {
    tag_name: String,
    published_at: Option<String>,
    draft: bool,
    prerelease: bool,
    assets: Vec<GitHubAsset>,
}

#[derive(Debug, Deserialize)]
struct GitHubAsset {
    name: String,
    browser_download_url: String,
    size: u64,
}

#[derive(Debug, Deserialize)]
struct ReleaseManifest {
    #[serde(rename = "generatedAt")]
    generated_at: Option<String>,
    commit: Option<String>,
    artifacts: ManifestArtifacts,
}

#[derive(Debug, Deserialize)]
struct ManifestArtifacts {
    #[serde(default)]
    ch592: HashMap<String, ManifestCh592Artifact>,
    #[serde(default)]
    ch552: HashMap<String, ManifestCh552Artifact>,
}

#[derive(Debug, Deserialize)]
struct ManifestCh592Artifact {
    version: String,
    #[serde(rename = "fullBinUrl")]
    full_bin_url: Option<String>,
    #[serde(rename = "fullHexUrl")]
    full_hex_url: Option<String>,
    #[serde(rename = "appBinUrl")]
    app_bin_url: Option<String>,
}

#[derive(Debug, Deserialize)]
struct ManifestCh552Artifact {
    version: String,
    #[serde(rename = "hexUrl")]
    hex_url: Option<String>,
}

fn categorize(chip: &str) -> (String, String) {
    let upper = chip.to_ascii_uppercase();
    if upper.starts_with("CH55") {
        ("有线款".into(), "CH55x 有线键盘系列".into())
    } else if upper.starts_with("CH57") || upper.starts_with("CH58") || upper.starts_with("CH59") {
        ("无线款".into(), "CH57x/CH58x/CH59x 无线系列".into())
    } else if upper.starts_with("CH") {
        ("其他".into(), "其他 WCH 芯片".into())
    } else {
        ("待连接".into(), "等待连接设备".into())
    }
}

fn release_filename_descriptor(name: &str) -> Option<FirmwareDescriptor> {
    let (stem, extension) = name
        .strip_suffix(".bin")
        .map(|stem| (stem, "bin"))
        .or_else(|| name.strip_suffix(".hex").map(|stem| (stem, "hex")))?;
    let parts: Vec<_> = stem.split('-').collect();
    if parts.len() < 3 {
        return None;
    }

    let chip = parts[0].to_ascii_uppercase();
    let keyboard = parts[1].to_ascii_uppercase();
    let version = parts[2].to_string();
    let flavor = parts.get(3).copied().unwrap_or("app").to_ascii_lowercase();
    let (category, family) = categorize(&chip);

    if chip == "CH552G" {
        return Some(FirmwareDescriptor {
            chip,
            keyboard,
            version,
            flavor: extension.into(),
            category,
            family,
        });
    }

    if chip == "CH592F" && flavor == "full" {
        return Some(FirmwareDescriptor {
            chip,
            keyboard,
            version,
            flavor,
            category,
            family,
        });
    }

    None
}

fn fallback_descriptor(name: &str) -> Option<FirmwareDescriptor> {
    let upper = name.to_ascii_uppercase();
    let chip = if upper.contains("CH592") {
        "CH592F"
    } else if upper.contains("CH552") {
        "CH552G"
    } else if upper.contains("CH59") {
        "CH59x"
    } else if upper.contains("CH55") {
        "CH55x"
    } else {
        return None;
    };

    let keyboard = if upper.contains("BASIC") {
        "BASIC"
    } else if upper.contains("KNOB") {
        "KNOB"
    } else if upper.contains("5KEY") {
        "5KEY"
    } else {
        "未知款式"
    };

    let version = if upper.contains("DEV") {
        "DEV"
    } else {
        "未知版本"
    };

    let flavor = if upper.contains("FULL") {
        "full"
    } else {
        "app"
    };
    let (category, family) = categorize(chip);

    Some(FirmwareDescriptor {
        chip: chip.into(),
        keyboard: keyboard.into(),
        version: version.into(),
        flavor: flavor.into(),
        category,
        family,
    })
}

fn version_key(version: &str) -> Vec<u32> {
    version
        .split('.')
        .map(|part| part.parse::<u32>().unwrap_or(0))
        .collect()
}

fn compare_versions(left: &str, right: &str) -> Ordering {
    let left_parts = version_key(left);
    let right_parts = version_key(right);
    let len = left_parts.len().max(right_parts.len());
    for index in 0..len {
        let l = *left_parts.get(index).unwrap_or(&0);
        let r = *right_parts.get(index).unwrap_or(&0);
        match l.cmp(&r) {
            Ordering::Equal => continue,
            other => return other,
        }
    }
    Ordering::Equal
}

pub fn describe_firmware_name(name: &str) -> Option<FirmwareDescriptor> {
    release_filename_descriptor(name).or_else(|| fallback_descriptor(name))
}

impl ReleaseAsset {
    pub fn list_label(&self) -> String {
        let flavor = match self.descriptor.flavor.as_str() {
            "full" => "FULL",
            "hex" => "HEX",
            _ => "BIN",
        };
        format!(
            "{} · {} · v{} · {}",
            self.descriptor.chip, self.descriptor.keyboard, self.descriptor.version, flavor
        )
    }

    pub fn subtitle(&self) -> String {
        format!(
            "{} · {} · {} KB · {}",
            self.descriptor.category,
            self.release_tag,
            self.size_bytes / 1024,
            self.published_at
        )
    }
}

fn filename_from_url(url: &str) -> String {
    url.split(['?', '#'])
        .next()
        .and_then(|path| path.rsplit('/').next())
        .filter(|name| !name.is_empty())
        .unwrap_or("firmware.bin")
        .to_string()
}

fn manifest_release_tag(manifest: &ReleaseManifest) -> String {
    manifest
        .commit
        .as_deref()
        .map(|commit| format!("manifest-{commit}"))
        .unwrap_or_else(|| "release-manifest".into())
}

fn manifest_date(manifest: &ReleaseManifest) -> String {
    manifest
        .generated_at
        .as_deref()
        .unwrap_or("")
        .chars()
        .take(10)
        .collect()
}

fn manifest_asset(
    name: String,
    download_url: String,
    release_tag: &str,
    published_at: &str,
    descriptor: FirmwareDescriptor,
) -> ReleaseAsset {
    ReleaseAsset {
        name,
        download_url,
        release_tag: release_tag.into(),
        published_at: published_at.into(),
        size_bytes: 0,
        descriptor,
    }
}

fn fetch_manifest_assets() -> Result<Vec<ReleaseAsset>, String> {
    let manifest = ureq::get(MANIFEST_URL)
        .set("Accept", "application/json")
        .set("User-Agent", "BinaryKeyboard-ISP/0.2")
        .call()
        .map_err(|err| format!("读取发布清单失败: {err}"))?
        .into_json::<ReleaseManifest>()
        .map_err(|err| format!("解析发布清单失败: {err}"))?;

    let release_tag = manifest_release_tag(&manifest);
    let published_at = manifest_date(&manifest);
    let mut assets = Vec::new();

    for (keyboard, artifact) in &manifest.artifacts.ch592 {
        let full_url = artifact
            .full_bin_url
            .as_ref()
            .or(artifact.full_hex_url.as_ref());
        let Some(url) = full_url.or(artifact.app_bin_url.as_ref()) else {
            continue;
        };
        let name = filename_from_url(url);
        let (category, family) = categorize("CH592F");
        assets.push(manifest_asset(
            name,
            url.clone(),
            &release_tag,
            &published_at,
            FirmwareDescriptor {
                chip: "CH592F".into(),
                keyboard: keyboard.to_ascii_uppercase(),
                version: artifact.version.clone(),
                flavor: if full_url.is_some() {
                    "full".into()
                } else {
                    "bin".into()
                },
                category,
                family,
            },
        ));
    }

    for (keyboard, artifact) in &manifest.artifacts.ch552 {
        let Some(url) = artifact.hex_url.as_ref() else {
            continue;
        };
        let name = filename_from_url(url);
        let (category, family) = categorize("CH552G");
        assets.push(manifest_asset(
            name,
            url.clone(),
            &release_tag,
            &published_at,
            FirmwareDescriptor {
                chip: "CH552G".into(),
                keyboard: keyboard.to_ascii_uppercase(),
                version: artifact.version.clone(),
                flavor: "hex".into(),
                category,
                family,
            },
        ));
    }

    assets.sort_by(|left, right| {
        compare_versions(&right.descriptor.version, &left.descriptor.version)
            .then_with(|| left.descriptor.chip.cmp(&right.descriptor.chip))
            .then_with(|| left.descriptor.keyboard.cmp(&right.descriptor.keyboard))
    });

    Ok(assets)
}

fn fetch_github_release_assets() -> Result<Vec<ReleaseAsset>, String> {
    let releases = ureq::get(RELEASES_API)
        .set("Accept", "application/vnd.github+json")
        .set("User-Agent", "BinaryKeyboard-ISP/0.2")
        .call()
        .map_err(|err| format!("读取 GitHub Release 失败: {err}"))?
        .into_json::<Vec<GitHubRelease>>()
        .map_err(|err| format!("解析 GitHub Release 失败: {err}"))?;

    let mut seen = HashSet::new();
    let mut assets = Vec::new();

    for release in releases {
        if release.draft || release.prerelease {
            continue;
        }
        let published_at = release
            .published_at
            .as_deref()
            .unwrap_or("")
            .chars()
            .take(10)
            .collect::<String>();
        for asset in release.assets {
            let Some(descriptor) = release_filename_descriptor(&asset.name) else {
                continue;
            };
            if !seen.insert(asset.name.clone()) {
                continue;
            }
            assets.push(ReleaseAsset {
                name: asset.name,
                download_url: asset.browser_download_url,
                release_tag: release.tag_name.clone(),
                published_at: published_at.clone(),
                size_bytes: asset.size,
                descriptor,
            });
        }
    }

    assets.sort_by(|left, right| {
        compare_versions(&right.descriptor.version, &left.descriptor.version)
            .then_with(|| right.published_at.cmp(&left.published_at))
            .then_with(|| left.descriptor.chip.cmp(&right.descriptor.chip))
            .then_with(|| left.descriptor.keyboard.cmp(&right.descriptor.keyboard))
    });

    Ok(assets)
}

pub fn fetch_release_assets() -> Result<Vec<ReleaseAsset>, String> {
    match fetch_manifest_assets() {
        Ok(assets) if !assets.is_empty() => Ok(assets),
        Ok(_) => fetch_github_release_assets(),
        Err(manifest_err) => fetch_github_release_assets()
            .map_err(|release_err| format!("{manifest_err}\n{release_err}")),
    }
}

pub fn download_release_asset(asset: &ReleaseAsset) -> Result<PathBuf, String> {
    let cache_dir = std::env::temp_dir().join("meowisp-release-cache");
    fs::create_dir_all(&cache_dir).map_err(|err| format!("无法创建缓存目录: {err}"))?;

    let millis = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_millis())
        .unwrap_or(0);
    let target = cache_dir.join(format!("{}-{}", millis, asset.name));

    let response = ureq::get(&asset.download_url)
        .set("Accept", "application/octet-stream")
        .set("User-Agent", "BinaryKeyboard-ISP/0.2")
        .call()
        .map_err(|err| format!("下载固件失败: {err}"))?;

    let mut reader = response.into_reader();
    let mut file = File::create(&target).map_err(|err| format!("无法写入缓存文件: {err}"))?;
    io::copy(&mut reader, &mut file).map_err(|err| format!("保存固件失败: {err}"))?;
    Ok(target)
}
