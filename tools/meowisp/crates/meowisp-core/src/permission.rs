use std::path::Path;
use std::process::Command;

const UDEV_RULES_PATH: &str = "/etc/udev/rules.d/50-wchisp.rules";

const UDEV_RULES: &str = r#"# WCH ISP bootloader devices - for MeowISP / wchisp
SUBSYSTEM=="usb", ATTRS{idVendor}=="4348", ATTRS{idProduct}=="55e0", MODE="0666"
SUBSYSTEM=="usb", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="55e0", MODE="0666"
"#;

pub fn udev_rules_installed() -> bool {
    Path::new(UDEV_RULES_PATH).is_file()
}

pub fn is_permission_error(err: &str) -> bool {
    let lower = err.to_lowercase();
    lower.contains("access denied")
        || lower.contains("libusb_error_access")
        || lower.contains("operation not permitted")
        || lower.contains("permission denied")
}

pub fn is_timeout_error(err: &str) -> bool {
    let lower = err.to_lowercase();
    lower.contains("timed out")
        || lower.contains("timeout")
        || lower.contains("entity not found")
        || lower.contains("busy")
}

fn run_pkexec_script(script: &str) -> Result<(), String> {
    let status = Command::new("pkexec")
        .args(["sh", "-eu", "-c", script])
        .status()
        .map_err(|e| format!("启动 pkexec 失败: {e}"))?;

    if status.success() {
        Ok(())
    } else {
        Err("授权被取消或失败".into())
    }
}

pub fn install_udev_rules() -> Result<String, String> {
    run_pkexec_script(&format!(
        r#"tmp="$(mktemp)"
cat > "$tmp" << 'RULES'
{UDEV_RULES}RULES
install -m 0644 "$tmp" {UDEV_RULES_PATH}
rm -f "$tmp"
udevadm control --reload-rules
udevadm trigger
"#
    ))?;

    Ok("udev 规则已安装并生效\n请重新插拔 USB 设备".into())
}

pub fn remove_udev_rules() -> Result<String, String> {
    if !udev_rules_installed() {
        return Ok("udev 规则不存在，无需清除".into());
    }

    run_pkexec_script(&format!(
        r#"rm -f {UDEV_RULES_PATH}
udevadm control --reload-rules
udevadm trigger
"#
    ))?;

    Ok("udev 规则已清除".into())
}
