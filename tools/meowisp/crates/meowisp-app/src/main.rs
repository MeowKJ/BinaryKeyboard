use std::path::PathBuf;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::thread;
use std::time::Duration;

use meowisp_core::{isp, permission};
use rfd::FileDialog;
use slint::{ComponentHandle, Weak};

slint::include_modules!();

fn set_log(app: &AppWindow, title: &str, text: &str) {
    app.set_log_title(title.into());
    app.set_log_text(text.into());
}

fn set_progress(app: &AppWindow, label: &str, value: i32) {
    app.set_progress_visible(true);
    app.set_progress_label(label.into());
    app.set_progress_value(value);
}

fn clear_progress(app: &AppWindow) {
    app.set_progress_visible(false);
    app.set_progress_label("等待操作".into());
    app.set_progress_value(0);
}

fn show_success(app: &AppWindow, title: &str, text: &str) {
    app.set_show_success(true);
    app.set_success_title(title.into());
    app.set_success_text(text.into());
}

fn hide_success(app: &AppWindow) {
    app.set_show_success(false);
}

fn set_chip(app: &AppWindow, info: &isp::ChipInfo) {
    app.set_chip_name(info.name.clone().into());
    app.set_chip_category(info.category.clone().into());
    app.set_chip_family(info.family.clone().into());
}

fn set_firmware(app: &AppWindow, path: &std::path::Path) {
    let name = path
        .file_name()
        .and_then(|n| n.to_str())
        .unwrap_or("firmware.bin");
    app.set_firmware_name(name.into());
    app.set_firmware_path(path.display().to_string().into());
    app.set_has_firmware(true);
}

fn update_permission_state(app: &AppWindow) {
    if cfg!(target_os = "linux") {
        let installed = permission::udev_rules_installed();
        app.set_udev_installed(installed);
        app.set_show_permission_bar(!installed);
    }
}

fn run_op<F>(
    weak: Weak<AppWindow>,
    op_busy: Arc<AtomicBool>,
    busy_label: &'static str,
    action: F,
)
where
    F: FnOnce() -> Result<(String, String, Option<isp::ChipInfo>), String> + Send + 'static,
{
    op_busy.store(true, Ordering::Relaxed);
    if let Some(app) = weak.upgrade() {
        app.set_busy(true);
        app.set_cat_mood(1);
        app.set_status_text("Working...".into());
        set_log(&app, busy_label, "请稍候...");
    }

    thread::spawn(move || {
        let result = action();
        let op_busy = op_busy.clone();
        let _ = slint::invoke_from_event_loop(move || {
            op_busy.store(false, Ordering::Relaxed);
            if let Some(app) = weak.upgrade() {
                app.set_busy(false);
                match result {
                    Ok((title, detail, chip_info)) => {
                        app.set_cat_mood(2);
                        app.set_status_text("Ready".into());
                        update_permission_state(&app);
                        if let Some(info) = chip_info {
                            set_chip(&app, &info);
                        }
                        set_log(&app, &title, &detail);
                    }
                    Err(ref err) if permission::is_permission_error(err) => {
                        app.set_cat_mood(3);
                        app.set_status_text("Error".into());
                        if cfg!(target_os = "linux") && !permission::udev_rules_installed() {
                            app.set_show_permission_bar(true);
                            set_log(
                                &app,
                                "USB 权限不足",
                                &format!(
                                    "{err}\n\n点击上方「自动配置」安装 udev 规则\n安装后请重新插拔设备"
                                ),
                            );
                        } else {
                            app.set_show_permission_bar(false);
                            set_log(
                                &app,
                                "USB 访问失败",
                                &format!(
                                    "{err}\n\nudev 规则看起来已经安装。\n请重新插拔设备，确认设备处于 ISP 模式，并关闭其他占用 USB 的工具。"
                                ),
                            );
                        }
                    }
                    Err(ref err) if permission::is_timeout_error(err) => {
                        app.set_cat_mood(3);
                        app.set_status_text("Error".into());
                        app.set_show_permission_bar(false);
                        set_log(
                            &app,
                            "设备连接超时",
                            &format!(
                                "{err}\n\n这通常不是权限问题。\n请重新插拔设备，重新进入 ISP 模式，再重试一次。"
                            ),
                        );
                    }
                    Err(err) => {
                        app.set_cat_mood(3);
                        app.set_status_text("Error".into());
                        set_log(&app, "操作失败", &err);
                    }
                }
            }
        });
    });
}

fn run_progress_op<F>(
    weak: Weak<AppWindow>,
    op_busy: Arc<AtomicBool>,
    busy_label: &'static str,
    celebrate: bool,
    action: F,
)
where
    F: FnOnce(Box<dyn Fn(&str, i32) + Send>) -> Result<(String, String, Option<isp::ChipInfo>), String>
        + Send
        + 'static,
{
    op_busy.store(true, Ordering::Relaxed);
    if let Some(app) = weak.upgrade() {
        app.set_busy(true);
        app.set_cat_mood(1);
        app.set_status_text("Working...".into());
        hide_success(&app);
        set_progress(&app, busy_label, 6);
        set_log(&app, busy_label, "请稍候...");
    }

    thread::spawn(move || {
        let progress_weak = weak.clone();
        let progress = Box::new(move |label: &str, value: i32| {
            let label = label.to_string();
            let progress_weak = progress_weak.clone();
            let _ = slint::invoke_from_event_loop(move || {
                if let Some(app) = progress_weak.upgrade() {
                    set_progress(&app, &label, value);
                }
            });
        });

        let result = action(progress);
        let op_busy = op_busy.clone();
        let _ = slint::invoke_from_event_loop(move || {
            op_busy.store(false, Ordering::Relaxed);
            if let Some(app) = weak.upgrade() {
                app.set_busy(false);
                match result {
                    Ok((title, detail, chip_info)) => {
                        app.set_cat_mood(2);
                        app.set_status_text("Ready".into());
                        update_permission_state(&app);
                        if let Some(info) = chip_info {
                            set_chip(&app, &info);
                        }
                        set_progress(&app, "完成", 100);
                        set_log(&app, &title, &detail);
                        if celebrate {
                            show_success(&app, "刷写完成", &detail);
                        }
                    }
                    Err(ref err) if permission::is_permission_error(err) => {
                        clear_progress(&app);
                        app.set_cat_mood(3);
                        app.set_status_text("Error".into());
                        if cfg!(target_os = "linux") && !permission::udev_rules_installed() {
                            app.set_show_permission_bar(true);
                            set_log(
                                &app,
                                "USB 权限不足",
                                &format!(
                                    "{err}\n\n点击上方「自动配置」安装 udev 规则\n安装后请重新插拔设备"
                                ),
                            );
                        } else {
                            app.set_show_permission_bar(false);
                            set_log(
                                &app,
                                "USB 访问失败",
                                &format!(
                                    "{err}\n\nudev 规则看起来已经安装。\n请重新插拔设备，确认设备处于 ISP 模式，并关闭其他占用 USB 的工具。"
                                ),
                            );
                        }
                    }
                    Err(ref err) if permission::is_timeout_error(err) => {
                        clear_progress(&app);
                        app.set_cat_mood(3);
                        app.set_status_text("Error".into());
                        app.set_show_permission_bar(false);
                        set_log(
                            &app,
                            "设备连接超时",
                            &format!(
                                "{err}\n\n这通常不是权限问题。\n请重新插拔设备，重新进入 ISP 模式，再重试一次。"
                            ),
                        );
                    }
                    Err(err) => {
                        clear_progress(&app);
                        app.set_cat_mood(3);
                        app.set_status_text("Error".into());
                        set_log(&app, "操作失败", &err);
                    }
                }
            }
        });
    });
}

fn get_firmware(app: &AppWindow) -> Option<PathBuf> {
    let p = app.get_firmware_path().to_string();
    if p.trim().is_empty() {
        None
    } else {
        Some(PathBuf::from(p))
    }
}

fn copy_log_output(app: &AppWindow) -> Result<(), String> {
    let payload = format!("{}\n\n{}", app.get_log_title(), app.get_log_text());
    let mut clipboard = arboard::Clipboard::new()
        .map_err(|e| format!("无法访问剪贴板: {e}"))?;
    clipboard
        .set_text(payload)
        .map_err(|e| format!("复制失败: {e}"))?;
    Ok(())
}

/// Background USB watch: only tracks device presence.
fn start_usb_watch(weak: Weak<AppWindow>, stop: Arc<AtomicBool>, op_busy: Arc<AtomicBool>) {
    thread::spawn(move || {
        let mut had_device = false;
        while !stop.load(Ordering::Relaxed) {
            thread::sleep(Duration::from_secs(2));
            if stop.load(Ordering::Relaxed) {
                break;
            }
            if op_busy.load(Ordering::Relaxed) {
                continue;
            }

            let result = isp::usb_device_count();
            let has_device_now = matches!(result, Ok(count) if count > 0);
            let weak2 = weak.clone();
            let previous_device_state = had_device;

            let _ = slint::invoke_from_event_loop(move || {
                if let Some(app) = weak2.upgrade() {
                    if app.get_busy() {
                        return;
                    }
                    match result {
                        Ok(count) if count > 0 => {
                            if !previous_device_state {
                                app.set_chip_name("ISP 已连接".into());
                                app.set_chip_category("点击识别芯片".into());
                                app.set_chip_family("".into());
                                app.set_cat_mood(2);
                                set_log(
                                    &app,
                                    "检测到 WCH ISP 设备",
                                    "设备已连接。\n点击“识别芯片”获取芯片类型，或直接尝试刷写。",
                                );
                            }
                        }
                        Ok(_) | Err(_) => {
                            if previous_device_state {
                                app.set_chip_name("未识别".into());
                                app.set_chip_category("等待检测".into());
                                app.set_chip_family("".into());
                                app.set_cat_mood(0);
                                set_log(&app, "设备已断开", "等待 ISP 设备连接...");
                            }
                        }
                    }
                }
            });

            had_device = has_device_now;
        }
    });
}

fn main() -> Result<(), slint::PlatformError> {
    if std::env::args().any(|arg| arg == "--probe") {
        match isp::probe() {
            Ok(info) => {
                println!("{} ({})", info.name, info.category);
                print!("{}", info.detail);
            }
            Err(e) => eprintln!("{e}"),
        }
        return Ok(());
    }

    if std::env::args().any(|arg| arg == "--install-udev") {
        match permission::install_udev_rules() {
            Ok(msg) => println!("{msg}"),
            Err(e) => eprintln!("{e}"),
        }
        return Ok(());
    }

    if std::env::args().any(|arg| arg == "--remove-udev") {
        match permission::remove_udev_rules() {
            Ok(msg) => println!("{msg}"),
            Err(e) => eprintln!("{e}"),
        }
        return Ok(());
    }

    let app = AppWindow::new()?;
    update_permission_state(&app);
    clear_progress(&app);

    // Start USB presence watch thread
    let stop_flag = Arc::new(AtomicBool::new(false));
    let op_busy = Arc::new(AtomicBool::new(false));
    start_usb_watch(app.as_weak(), stop_flag.clone(), op_busy.clone());

    // Fix permission
    let weak = app.as_weak();
    let op_busy_fix = op_busy.clone();
    app.on_request_fix_permission(move || {
        let weak2 = weak.clone();
        op_busy_fix.store(true, Ordering::Relaxed);
        if let Some(app) = weak.upgrade() {
            app.set_busy(true);
            app.set_cat_mood(1);
            set_log(&app, "正在配置权限...", "将弹出系统授权对话框");
        }
        let op_busy_finish = op_busy_fix.clone();
        thread::spawn(move || {
            let result = permission::install_udev_rules();
            let _ = slint::invoke_from_event_loop(move || {
                op_busy_finish.store(false, Ordering::Relaxed);
                if let Some(app) = weak2.upgrade() {
                    app.set_busy(false);
                    match result {
                        Ok(msg) => {
                            app.set_cat_mood(2);
                            update_permission_state(&app);
                            set_log(&app, "权限配置完成", &msg);
                        }
                        Err(err) => {
                            app.set_cat_mood(3);
                            set_log(&app, "权限配置失败", &err);
                        }
                    }
                }
            });
        });
    });

    // Remove permission
    let weak = app.as_weak();
    let op_busy_remove = op_busy.clone();
    app.on_request_remove_permission(move || {
        let weak2 = weak.clone();
        op_busy_remove.store(true, Ordering::Relaxed);
        if let Some(app) = weak.upgrade() {
            app.set_busy(true);
            app.set_cat_mood(1);
            set_log(&app, "正在清除权限...", "将弹出系统授权对话框");
        }
        let op_busy_finish = op_busy_remove.clone();
        thread::spawn(move || {
            let result = permission::remove_udev_rules();
            let _ = slint::invoke_from_event_loop(move || {
                op_busy_finish.store(false, Ordering::Relaxed);
                if let Some(app) = weak2.upgrade() {
                    app.set_busy(false);
                    match result {
                        Ok(msg) => {
                            app.set_cat_mood(0);
                            update_permission_state(&app);
                            set_log(&app, "权限已清除", &msg);
                        }
                        Err(err) => {
                            app.set_cat_mood(3);
                            set_log(&app, "清除失败", &err);
                        }
                    }
                }
            });
        });
    });

    // Pick firmware
    let weak = app.as_weak();
    app.on_request_pick_bin(move || {
        if let Some(path) = FileDialog::new()
            .add_filter("Binary firmware", &["bin", "hex"])
            .pick_file()
        {
            if let Some(app) = weak.upgrade() {
                set_firmware(&app, &path);
                app.set_cat_mood(0);
                hide_success(&app);
                set_log(&app, "已选择固件", &path.display().to_string());
            }
        }
    });

    // Copy log
    let weak = app.as_weak();
    app.on_request_copy_log(move || {
        if let Some(app) = weak.upgrade() {
            match copy_log_output(&app) {
                Ok(()) => {
                    app.set_cat_mood(2);
                    set_log(&app, "输出已复制", &app.get_log_text().to_string());
                }
                Err(err) => {
                    app.set_cat_mood(3);
                    set_log(&app, "复制失败", &err);
                }
            }
        }
    });

    let weak = app.as_weak();
    app.on_request_dismiss_success(move || {
        if let Some(app) = weak.upgrade() {
            hide_success(&app);
        }
    });

    // Manual detect
    let weak = app.as_weak();
    let op_busy_detect = op_busy.clone();
    app.on_request_detect_chip(move || {
        run_op(weak.clone(), op_busy_detect.clone(), "正在识别芯片...", || {
            let info = isp::probe()?;
            let title = format!("识别到 {}", info.name);
            let detail = info.detail.clone();
            Ok((title, detail, Some(info)))
        });
    });

    // Flash
    let weak = app.as_weak();
    let op_busy_flash = op_busy.clone();
    app.on_request_flash_bin(move || {
        if let Some(app) = weak.upgrade() {
            let Some(path) = get_firmware(&app) else {
                set_log(&app, "缺少固件", "请先选择一个固件文件");
                return;
            };
            drop(app);
            run_progress_op(weak.clone(), op_busy_flash.clone(), "准备刷写", true, move |progress| {
                let r = isp::flash_with_progress(&path, |label, value| progress(label, value))?;
                Ok((r.summary, r.detail, None))
            });
        }
    });

    // Verify
    let weak = app.as_weak();
    let op_busy_verify = op_busy.clone();
    app.on_request_verify_bin(move || {
        if let Some(app) = weak.upgrade() {
            let Some(path) = get_firmware(&app) else {
                set_log(&app, "缺少固件", "请先选择一个固件文件");
                return;
            };
            drop(app);
            run_progress_op(weak.clone(), op_busy_verify.clone(), "准备校验", false, move |progress| {
                let r = isp::verify_with_progress(&path, |label, value| progress(label, value))?;
                Ok((r.summary, r.detail, None))
            });
        }
    });

    // Erase code
    let weak = app.as_weak();
    let op_busy_erase_code = op_busy.clone();
    app.on_request_erase_code(move || {
        run_progress_op(
            weak.clone(),
            op_busy_erase_code.clone(),
            "准备清除 Codeflash",
            false,
            |progress| {
            let r = isp::erase_code_with_progress(|label, value| progress(label, value))?;
            Ok((r.summary, r.detail, None))
        });
    });

    // Erase data
    let weak = app.as_weak();
    let op_busy_erase_data = op_busy.clone();
    app.on_request_erase_data(move || {
        run_progress_op(
            weak.clone(),
            op_busy_erase_data.clone(),
            "准备清除 Dataflash",
            false,
            |progress| {
            let r = isp::erase_data_with_progress(|label, value| progress(label, value))?;
            Ok((r.summary, r.detail, None))
        });
    });

    let result = app.run();
    stop_flag.store(true, Ordering::Relaxed);
    result
}
