use anyhow::Error;
use std::fmt::Write as _;
use std::path::Path;
use std::thread::sleep;
use std::time::Duration;
use wchisp::transport::UsbTransport;

const SECTOR_SIZE: usize = 1024;

#[derive(Debug, Clone)]
pub struct ChipInfo {
    pub name: String,
    pub chip_id: String,
    pub flash_size: u32,
    pub eeprom_size: u32,
    pub category: String,
    pub family: String,
    pub detail: String,
}

#[derive(Debug, Clone)]
pub struct OpResult {
    pub summary: String,
    pub detail: String,
}

#[derive(Debug, Clone)]
pub struct SessionResult {
    pub result: OpResult,
    pub chip: ChipInfo,
}

fn categorize(name: &str) -> (String, String) {
    let u = name.to_ascii_uppercase();
    if u.starts_with("CH55") {
        ("有线款".into(), "CH55x 有线键盘系列".into())
    } else if u.starts_with("CH57") || u.starts_with("CH58") || u.starts_with("CH59") {
        ("无线款".into(), "CH57x/CH58x/CH59x 无线系列".into())
    } else if u.starts_with("CH") {
        ("其他 WCH".into(), "其他 WCH 芯片".into())
    } else {
        ("未知".into(), "未知芯片系列".into())
    }
}

fn chip_info_from_chip(chip: &wchisp::Chip) -> ChipInfo {
    let (category, family) = categorize(&chip.name);

    let mut detail = String::new();
    let _ = writeln!(detail, "Chip: {}", chip.name);
    let _ = writeln!(detail, "Chip ID: 0x{:02X}", chip.chip_id);
    let _ = writeln!(detail, "Flash: {} KB", chip.flash_size / 1024);
    let _ = writeln!(detail, "EEPROM: {} KB", chip.eeprom_size / 1024);

    ChipInfo {
        name: chip.name.clone(),
        chip_id: format!("0x{:02X}", chip.chip_id),
        flash_size: chip.flash_size,
        eeprom_size: chip.eeprom_size,
        category,
        family,
        detail,
    }
}

pub fn guess_chip_from_firmware_path(path: &Path) -> Option<ChipInfo> {
    let name = path.file_name()?.to_string_lossy().to_ascii_uppercase();
    let guessed_name = if name.contains("CH552") {
        "CH552"
    } else if name.contains("CH559") {
        "CH559"
    } else if name.contains("CH571") {
        "CH571"
    } else if name.contains("CH582") {
        "CH582"
    } else if name.contains("CH583") {
        "CH583"
    } else if name.contains("CH592") {
        "CH592"
    } else if name.contains("CH579") {
        "CH579"
    } else if name.contains("CH57") {
        "CH57x"
    } else if name.contains("CH58") {
        "CH58x"
    } else if name.contains("CH59") {
        "CH59x"
    } else if name.contains("CH55") {
        "CH55x"
    } else {
        return None;
    };

    let (category, family) = categorize(guessed_name);
    Some(ChipInfo {
        name: guessed_name.into(),
        chip_id: "推断".into(),
        flash_size: 0,
        eeprom_size: 0,
        category,
        family,
        detail: format!(
            "已根据固件文件名推断目标芯片。\n文件: {}\n目标: {}",
            path.display(),
            guessed_name
        ),
    })
}

fn pad_to_sector(data: &mut Vec<u8>) {
    let r = data.len() % SECTOR_SIZE;
    if r != 0 {
        data.resize(data.len() + SECTOR_SIZE - r, 0xFF);
    }
}

fn debug_line(message: impl AsRef<str>) {
    eprintln!("[meowisp] {}", message.as_ref());
}

fn format_error_chain(err: &Error) -> String {
    let mut out = String::new();
    let _ = writeln!(out, "error: {err}");

    let mut current = err.source();
    let mut depth = 0usize;
    while let Some(source) = current {
        depth += 1;
        let _ = writeln!(out, "caused by[{depth}]: {source}");
        current = source.source();
    }

    out.trim_end().to_string()
}

fn map_wchisp_error(context: &str, user_message: &str, err: Error) -> String {
    debug_line(format!("{context} failed"));
    for line in format_error_chain(&err).lines() {
        debug_line(format!("  {line}"));
    }
    format!("{user_message}: {err}")
}

fn log_chip_context(op: &str, flashing: &wchisp::Flashing<'_>) {
    debug_line(format!(
        "{op}: transport=usb chip={} chip_id=0x{:02X} flash={} eeprom={}",
        flashing.chip.name,
        flashing.chip.chip_id,
        flashing.chip.flash_size,
        flashing.chip.eeprom_size
    ));
}

fn log_chip_context_light(op: &str, chip: &wchisp::Chip) {
    debug_line(format!(
        "{op}: transport=usb chip={} chip_id=0x{:02X} flash={} eeprom={}",
        chip.name, chip.chip_id, chip.flash_size, chip.eeprom_size
    ));
}

pub fn usb_device_count() -> Result<usize, String> {
    UsbTransport::scan_devices()
        .map_err(|e| map_wchisp_error("scan_devices", "无法扫描 USB 设备", e))
}

pub fn probe() -> Result<ChipInfo, String> {
    debug_line("probe: opening transport=usb device=auto (light)");
    let mut transport = UsbTransport::open_any()
        .map_err(|e| map_wchisp_error("probe/open", "无法连接 USB 设备", e))?;
    let chip = wchisp::Flashing::get_chip(&mut transport)
        .map_err(|e| map_wchisp_error("probe/get_chip", "无法识别芯片", e))?;
    log_chip_context_light("probe", &chip);
    Ok(chip_info_from_chip(&chip))
}

pub fn flash(firmware_path: &Path) -> Result<OpResult, String> {
    flash_with_progress(firmware_path, |_, _| {}).map(|r| r.result)
}

pub fn flash_with_progress<F>(
    firmware_path: &Path,
    mut progress: F,
) -> Result<SessionResult, String>
where
    F: FnMut(&str, i32),
{
    debug_line(format!("flash: firmware={}", firmware_path.display()));
    progress("读取固件", 8);
    let mut data = wchisp::format::read_firmware_from_file(firmware_path)
        .map_err(|e| map_wchisp_error("flash/read_firmware", "无法读取固件文件", e))?;
    pad_to_sector(&mut data);
    debug_line(format!("flash: padded_size={} bytes", data.len()));

    progress("连接设备", 16);
    let mut flashing = wchisp::Flashing::new_from_usb(None)
        .map_err(|e| map_wchisp_error("flash/open", "无法连接 USB 设备", e))?;
    log_chip_context("flash", &flashing);
    let chip_info = chip_info_from_chip(&flashing.chip);

    let sectors = (data.len() / SECTOR_SIZE + 1) as u32;
    debug_line(format!("flash: erase_code sectors={sectors}"));
    progress("擦除 Codeflash", 28);
    flashing
        .erase_code(sectors)
        .map_err(|e| map_wchisp_error("flash/erase_code", "擦除失败", e))?;
    sleep(Duration::from_secs(1));
    debug_line("flash: program");
    progress("写入固件", 60);
    flashing
        .flash(&data)
        .map_err(|e| map_wchisp_error("flash/program", "刷写失败", e))?;
    sleep(Duration::from_millis(500));
    debug_line("flash: verify");
    progress("校验固件", 86);
    flashing
        .verify(&data)
        .map_err(|e| map_wchisp_error("flash/verify", "校验失败", e))?;
    debug_line("flash: reset");
    progress("重启设备", 96);
    let _ = flashing.reset();
    progress("刷写完成", 100);

    Ok(SessionResult {
        result: OpResult {
            summary: format!("已刷写 {}", firmware_path.display()),
            detail: format!(
                "芯片: {}\n固件大小: {} 字节\n已擦除 {} 扇区\n刷写并校验完成",
                chip_info.name,
                data.len(),
                sectors
            ),
        },
        chip: chip_info,
    })
}

pub fn verify(firmware_path: &Path) -> Result<OpResult, String> {
    verify_with_progress(firmware_path, |_, _| {}).map(|r| r.result)
}

pub fn verify_with_progress<F>(
    firmware_path: &Path,
    mut progress: F,
) -> Result<SessionResult, String>
where
    F: FnMut(&str, i32),
{
    debug_line(format!("verify: firmware={}", firmware_path.display()));
    progress("读取固件", 12);
    let mut data = wchisp::format::read_firmware_from_file(firmware_path)
        .map_err(|e| map_wchisp_error("verify/read_firmware", "无法读取固件文件", e))?;
    pad_to_sector(&mut data);
    debug_line(format!("verify: padded_size={} bytes", data.len()));

    progress("连接设备", 26);
    let mut flashing = wchisp::Flashing::new_from_usb(None)
        .map_err(|e| map_wchisp_error("verify/open", "无法连接 USB 设备", e))?;
    log_chip_context("verify", &flashing);
    let chip_info = chip_info_from_chip(&flashing.chip);
    debug_line("verify: compare");
    progress("校验中", 72);
    flashing
        .verify(&data)
        .map_err(|e| map_wchisp_error("verify/compare", "校验失败", e))?;
    progress("校验完成", 100);

    Ok(SessionResult {
        result: OpResult {
            summary: format!("已校验 {}", firmware_path.display()),
            detail: format!(
                "芯片: {}\n固件大小: {} 字节\n校验通过",
                chip_info.name,
                data.len()
            ),
        },
        chip: chip_info,
    })
}

pub fn erase_code() -> Result<OpResult, String> {
    erase_code_with_progress(|_, _| {}).map(|r| r.result)
}

pub fn erase_code_with_progress<F>(mut progress: F) -> Result<SessionResult, String>
where
    F: FnMut(&str, i32),
{
    debug_line("erase_code: opening transport=usb device=auto");
    progress("连接设备", 18);
    let mut flashing = wchisp::Flashing::new_from_usb(None)
        .map_err(|e| map_wchisp_error("erase_code/open", "无法连接 USB 设备", e))?;
    log_chip_context("erase_code", &flashing);
    let chip_info = chip_info_from_chip(&flashing.chip);
    let sectors = flashing.chip.flash_size / 1024;
    debug_line(format!("erase_code: sectors={sectors}"));
    progress("擦除 Codeflash", 68);
    flashing
        .erase_code(sectors)
        .map_err(|e| map_wchisp_error("erase_code/run", "擦除 Codeflash 失败", e))?;
    progress("擦除完成", 100);

    Ok(SessionResult {
        result: OpResult {
            summary: "Codeflash 已清除".into(),
            detail: format!("芯片: {}\n已擦除 {} 扇区", chip_info.name, sectors),
        },
        chip: chip_info,
    })
}

pub fn erase_data() -> Result<OpResult, String> {
    erase_data_with_progress(|_, _| {}).map(|r| r.result)
}

pub fn erase_data_with_progress<F>(mut progress: F) -> Result<SessionResult, String>
where
    F: FnMut(&str, i32),
{
    debug_line("erase_data: opening transport=usb device=auto");
    progress("连接设备", 18);
    let mut flashing = wchisp::Flashing::new_from_usb(None)
        .map_err(|e| map_wchisp_error("erase_data/open", "无法连接 USB 设备", e))?;
    log_chip_context("erase_data", &flashing);
    let chip_info = chip_info_from_chip(&flashing.chip);
    debug_line("erase_data: run");
    progress("擦除 Dataflash", 70);
    flashing
        .erase_data()
        .map_err(|e| map_wchisp_error("erase_data/run", "擦除 Dataflash 失败", e))?;
    progress("擦除完成", 100);

    Ok(SessionResult {
        result: OpResult {
            summary: "Dataflash 已清除".into(),
            detail: format!("芯片: {}\nEEPROM / Dataflash 已清除", chip_info.name),
        },
        chip: chip_info,
    })
}
