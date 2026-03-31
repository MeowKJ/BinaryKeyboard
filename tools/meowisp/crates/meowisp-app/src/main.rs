#![cfg_attr(all(target_os = "windows", not(debug_assertions)), windows_subsystem = "windows")]

use std::cell::{Cell, RefCell};
use std::io::Cursor;
use std::path::PathBuf;
use std::rc::Rc;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;

use meowisp_core::{isp, online, permission};
use png::{BlendOp, ColorType, DisposeOp, FrameControl, Transformations};
use rfd::FileDialog;
use slint::{
    ComponentHandle, Image, Rgba8Pixel, SharedPixelBuffer, SharedString, Timer, TimerMode,
    VecModel, Weak,
};

slint::include_modules!();

#[derive(Debug, Clone)]
enum FirmwareSource {
    Local(PathBuf),
    Online(online::ReleaseAsset),
}

#[derive(Default)]
struct AppState {
    selected_firmware: Option<FirmwareSource>,
    online_assets: Vec<online::ReleaseAsset>,
}

#[derive(Clone)]
struct AnimatedFrame {
    image: Image,
    delay: Duration,
}

struct AnimatedImage {
    frames: Vec<AnimatedFrame>,
    num_plays: u32,
}

fn ui_font_family() -> &'static str {
    if cfg!(target_os = "macos") {
        "Hiragino Sans GB"
    } else if cfg!(target_os = "windows") {
        "Microsoft YaHei UI"
    } else if cfg!(target_os = "linux") {
        "Noto Sans CJK SC"
    } else {
        ""
    }
}

fn brand_font_family() -> &'static str {
    if cfg!(target_os = "macos") {
        "Avenir Next"
    } else if cfg!(target_os = "windows") {
        "Segoe UI"
    } else if cfg!(target_os = "linux") {
        "Noto Sans"
    } else {
        ""
    }
}

fn print_doctor() {
    println!("MeowISP doctor");
    println!(
        "platform: {}-{}",
        std::env::consts::ARCH,
        std::env::consts::OS
    );
    println!(
        "profile: {}",
        if cfg!(debug_assertions) {
            "debug"
        } else {
            "release"
        }
    );

    match std::env::current_exe() {
        Ok(path) => println!("binary: {}", path.display()),
        Err(err) => println!("binary: <unavailable> ({err})"),
    }
    println!("ui-font-family: {}", ui_font_family());
    println!("release-source: GitHub Releases");

    if cfg!(target_os = "linux") {
        println!(
            "udev-rules: {}",
            if permission::udev_rules_installed() {
                "installed"
            } else {
                "missing"
            }
        );
    } else if cfg!(target_os = "macos") {
        println!("usb-access: macOS does not use udev rules");
    } else {
        println!("usb-access: no platform-specific permission check");
    }

    match isp::usb_device_count() {
        Ok(count) => println!("wch-isp-devices: {count}"),
        Err(err) => println!("wch-isp-devices: error ({err})"),
    }

    println!("tips:");
    println!("  - use --probe to read the connected chip");
    println!("  - launch without flags to open the desktop UI");
}

fn set_log(app: &AppWindow, title: &str, _text: &str) {
    app.set_log_title(title.into());
    app.set_log_text(_text.into());
}

fn set_progress(app: &AppWindow, label: &str, value: i32) {
    app.set_progress_visible(true);
    app.set_progress_label(label.into());
    app.set_progress_value(value.clamp(0, 100));
}

fn clear_progress(app: &AppWindow) {
    app.set_progress_visible(false);
    app.set_progress_label("准备就绪".into());
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

fn classify_error_title(err: &str) -> &'static str {
    if err.contains("校验失败") || err.contains("Verify failed") || err.contains("mismatch") {
        "校验失败"
    } else if err.contains("无法连接 USB 设备")
        || err.contains("No such device")
        || err.contains("device not found")
    {
        "未连接设备"
    } else if err.contains("无法识别芯片") {
        "识别失败"
    } else if err.contains("刷写失败") {
        "刷写失败"
    } else if err.contains("擦除失败") || err.contains("擦除 Codeflash 失败") {
        "擦除失败"
    } else if err.contains("固件文件") {
        "固件读取失败"
    } else {
        "操作失败"
    }
}

fn show_error(app: &AppWindow, title: &str, text: &str) {
    app.set_show_error(true);
    app.set_error_title(title.into());
    app.set_error_text(text.into());
}

fn hide_error(app: &AppWindow) {
    app.set_show_error(false);
}

fn set_chip(app: &AppWindow, info: &isp::ChipInfo) {
    app.set_device_connected(true);
    app.set_device_name(info.name.clone().into());
    app.set_device_category(info.category.clone().into());
    app.set_device_family(
        format!(
            "{} · Flash {} KB · EEPROM {} KB",
            info.chip_id,
            info.flash_size / 1024,
            info.eeprom_size / 1024
        )
        .into(),
    );
}

fn set_idle_chip(app: &AppWindow) {
    app.set_device_connected(false);
    app.set_device_name("待连接设备".into());
    app.set_device_category("待连接".into());
    app.set_device_family("".into());
}

fn set_pending_chip(app: &AppWindow) {
    app.set_device_connected(true);
    app.set_device_name("识别设备中...".into());
    app.set_device_category("待识别".into());
    app.set_device_family("正在自动读取芯片信息".into());
}

fn apply_firmware_descriptor(app: &AppWindow, descriptor: &online::FirmwareDescriptor) {
    app.set_chip_name(descriptor.chip.clone().into());
    app.set_chip_category(descriptor.category.clone().into());
    app.set_chip_family(
        format!(
            "{} · v{}{}",
            descriptor.keyboard,
            descriptor.version,
            if descriptor.flavor == "full" {
                " · FULL"
            } else {
                ""
            }
        )
        .into(),
    );
}

fn update_firmware_panel(app: &AppWindow, name: &str) {
    app.set_firmware_name(name.into());
    app.set_has_firmware(true);
}

fn set_online_options(app: &AppWindow, assets: &[online::ReleaseAsset]) {
    let labels: Vec<SharedString> = assets.iter().map(|a| a.list_label().into()).collect();
    let subtitles: Vec<SharedString> = assets.iter().map(|a| a.subtitle().into()).collect();
    app.set_online_options(Rc::new(VecModel::from(labels)).into());
    app.set_online_subtitles(Rc::new(VecModel::from(subtitles)).into());
}

fn update_permission_state(app: &AppWindow) {
    if cfg!(target_os = "linux") {
        let installed = permission::udev_rules_installed();
        app.set_udev_installed(installed);
        app.set_show_permission_bar(!installed);
    } else {
        app.set_udev_installed(false);
        app.set_show_permission_bar(false);
    }
}

fn frame_delay(frame: &FrameControl) -> Duration {
    let den = if frame.delay_den == 0 {
        100
    } else {
        frame.delay_den
    } as u64;
    let num = frame.delay_num as u64;
    let millis = if num == 0 {
        80
    } else {
        ((num * 1000) / den).max(16)
    };
    Duration::from_millis(millis)
}

fn blend_pixel_over(dst: &mut [u8], src: &[u8]) {
    let src_alpha = src[3] as f32 / 255.0;
    let dst_alpha = dst[3] as f32 / 255.0;
    let out_alpha = src_alpha + dst_alpha * (1.0 - src_alpha);

    if out_alpha <= f32::EPSILON {
        dst.copy_from_slice(&[0, 0, 0, 0]);
        return;
    }

    for channel in 0..3 {
        let src_channel = src[channel] as f32 / 255.0;
        let dst_channel = dst[channel] as f32 / 255.0;
        let out_channel =
            (src_channel * src_alpha + dst_channel * dst_alpha * (1.0 - src_alpha)) / out_alpha;
        dst[channel] = (out_channel * 255.0).round().clamp(0.0, 255.0) as u8;
    }
    dst[3] = (out_alpha * 255.0).round().clamp(0.0, 255.0) as u8;
}

fn composite_subframe(
    canvas: &mut [u8],
    canvas_width: u32,
    frame: &FrameControl,
    subframe: &[u8],
) -> Result<(), String> {
    let frame_width = frame.width as usize;
    let frame_height = frame.height as usize;
    let canvas_width = canvas_width as usize;

    for y in 0..frame_height {
        for x in 0..frame_width {
            let src_index = (y * frame_width + x) * 4;
            let dst_x = frame.x_offset as usize + x;
            let dst_y = frame.y_offset as usize + y;
            let dst_index = (dst_y * canvas_width + dst_x) * 4;

            let src_pixel = &subframe[src_index..src_index + 4];
            let dst_pixel = &mut canvas[dst_index..dst_index + 4];
            match frame.blend_op {
                BlendOp::Source => dst_pixel.copy_from_slice(src_pixel),
                BlendOp::Over => blend_pixel_over(dst_pixel, src_pixel),
            }
        }
    }

    Ok(())
}

fn clear_subframe_region(canvas: &mut [u8], canvas_width: u32, frame: &FrameControl) {
    let frame_width = frame.width as usize;
    let frame_height = frame.height as usize;
    let canvas_width = canvas_width as usize;

    for y in 0..frame_height {
        for x in 0..frame_width {
            let dst_x = frame.x_offset as usize + x;
            let dst_y = frame.y_offset as usize + y;
            let dst_index = (dst_y * canvas_width + dst_x) * 4;
            canvas[dst_index..dst_index + 4].copy_from_slice(&[0, 0, 0, 0]);
        }
    }
}

fn to_slint_image(rgba: &[u8], width: u32, height: u32) -> Image {
    let buffer = SharedPixelBuffer::<Rgba8Pixel>::clone_from_slice(rgba, width, height);
    Image::from_rgba8(buffer)
}

fn load_success_animation() -> Result<AnimatedImage, String> {
    let bytes = include_bytes!("../../../assets/smiling_cat_with_heart-eyes_animated.png");
    let cursor = Cursor::new(bytes.as_slice());
    let mut decoder = png::Decoder::new(cursor);
    decoder.set_transformations(Transformations::normalize_to_color8() | Transformations::ALPHA);
    let mut reader = decoder
        .read_info()
        .map_err(|err| format!("无法读取 APNG 信息: {err}"))?;

    let info = reader.info();
    let canvas_width = info.width;
    let canvas_height = info.height;
    let Some(buffer_len) = reader.output_buffer_size() else {
        return Err("APNG 输出尺寸过大".into());
    };
    let total_frames = info
        .animation_control()
        .map(|c| c.num_frames)
        .unwrap_or(1)
        .max(1);
    let num_plays = info.animation_control().map(|c| c.num_plays).unwrap_or(0);

    let mut raw = vec![0u8; buffer_len];
    let mut canvas = vec![0u8; (canvas_width * canvas_height * 4) as usize];
    let mut previous_canvas = canvas.clone();
    let mut frames = Vec::with_capacity(total_frames as usize);

    for _ in 0..total_frames {
        let output = reader
            .next_frame(&mut raw)
            .map_err(|err| format!("无法解码 APNG 帧: {err}"))?;
        if output.color_type != ColorType::Rgba {
            return Err(format!(
                "APNG 输出颜色格式不受支持: {:?}",
                output.color_type
            ));
        }

        let frame = reader.info().frame_control.unwrap_or(FrameControl {
            width: canvas_width,
            height: canvas_height,
            ..FrameControl::default()
        });
        let subframe = &raw[..output.buffer_size()];

        if matches!(frame.dispose_op, DisposeOp::Previous) {
            previous_canvas.copy_from_slice(&canvas);
        }

        composite_subframe(&mut canvas, canvas_width, &frame, subframe)?;
        frames.push(AnimatedFrame {
            image: to_slint_image(&canvas, canvas_width, canvas_height),
            delay: frame_delay(&frame),
        });

        match frame.dispose_op {
            DisposeOp::None => {}
            DisposeOp::Background => clear_subframe_region(&mut canvas, canvas_width, &frame),
            DisposeOp::Previous => canvas.copy_from_slice(&previous_canvas),
        }
    }

    if frames.is_empty() {
        return Err("APNG 中没有可播放的帧".into());
    }

    Ok(AnimatedImage { frames, num_plays })
}

fn start_success_animation(weak: Weak<AppWindow>, animation: AnimatedImage) {
    let animation = Rc::new(animation);
    let timer = Rc::new(Timer::default());
    let frame_index = Rc::new(Cell::new(0usize));
    let completed_loops = Rc::new(Cell::new(0u32));
    let was_visible = Rc::new(Cell::new(false));

    let schedule_next: Rc<RefCell<Option<Box<dyn Fn(Duration)>>>> = Rc::new(RefCell::new(None));
    let schedule_next_handle = schedule_next.clone();
    let timer_handle = timer.clone();
    let weak_handle = weak.clone();
    let animation_handle = animation.clone();
    let frame_index_handle = frame_index.clone();
    let completed_loops_handle = completed_loops.clone();
    let was_visible_handle = was_visible.clone();

    *schedule_next.borrow_mut() = Some(Box::new(move |delay: Duration| {
        let timer = timer_handle.clone();
        let weak = weak_handle.clone();
        let animation = animation_handle.clone();
        let frame_index = frame_index_handle.clone();
        let completed_loops = completed_loops_handle.clone();
        let was_visible = was_visible_handle.clone();
        let schedule_next = schedule_next_handle.clone();

        timer.start(TimerMode::SingleShot, delay, move || {
            let Some(app) = weak.upgrade() else {
                return;
            };

            if !app.get_show_success() {
                if was_visible.replace(false) {
                    frame_index.set(0);
                    completed_loops.set(0);
                    app.set_success_animation_image(animation.frames[0].image.clone());
                }
                if let Some(schedule) = &*schedule_next.borrow() {
                    schedule(Duration::from_millis(120));
                }
                return;
            }

            if !was_visible.replace(true) {
                frame_index.set(0);
                completed_loops.set(0);
                app.set_success_animation_image(animation.frames[0].image.clone());
                if let Some(schedule) = &*schedule_next.borrow() {
                    schedule(animation.frames[0].delay);
                }
                return;
            }

            let mut next_index = frame_index.get() + 1;
            if next_index >= animation.frames.len() {
                next_index = 0;
                completed_loops.set(completed_loops.get() + 1);
                if animation.num_plays != 0 && completed_loops.get() >= animation.num_plays {
                    next_index = animation.frames.len() - 1;
                }
            }

            frame_index.set(next_index);
            app.set_success_animation_image(animation.frames[next_index].image.clone());

            let next_delay = if animation.num_plays != 0
                && completed_loops.get() >= animation.num_plays
                && next_index == animation.frames.len() - 1
            {
                Duration::from_millis(180)
            } else {
                animation.frames[next_index].delay
            };

            if let Some(schedule) = &*schedule_next.borrow() {
                schedule(next_delay);
            }
        });
    }));

    if let Some(app) = weak.upgrade() {
        app.set_success_animation_image(animation.frames[0].image.clone());
    }

    {
        let schedule_ref = schedule_next.borrow();
        if let Some(schedule) = schedule_ref.as_ref() {
            schedule(Duration::from_millis(120));
        }
    }
}

fn prepare_selected_firmware(
    selection: &FirmwareSource,
    progress: &dyn Fn(&str, i32),
) -> Result<(PathBuf, String), String> {
    match selection {
        FirmwareSource::Local(path) => {
            let display_name = path
                .file_name()
                .and_then(|name| name.to_str())
                .unwrap_or("firmware.bin")
                .to_string();
            Ok((path.clone(), display_name))
        }
        FirmwareSource::Online(asset) => {
            progress("下载固件", 12);
            let path = online::download_release_asset(asset)?;
            progress("下载完成", 28);
            Ok((path, asset.name.clone()))
        }
    }
}

fn run_progress_op<F>(
    weak: Weak<AppWindow>,
    op_busy: Arc<AtomicBool>,
    busy_label: &'static str,
    success_title: Option<&'static str>,
    action: F,
) where
    F: FnOnce(
            Box<dyn Fn(&str, i32) + Send>,
        ) -> Result<(String, String, Option<isp::ChipInfo>), String>
        + Send
        + 'static,
{
    op_busy.store(true, Ordering::Relaxed);
    if let Some(app) = weak.upgrade() {
        app.set_busy(true);
        app.set_cat_mood(1);
        app.set_status_text("处理中".into());
        hide_success(&app);
        hide_error(&app);
        set_progress(&app, busy_label, 0);
    }

    thread::spawn(move || {
        let progress_weak = weak.clone();
        let progress = Box::new(move |label: &str, value: i32| {
            let label = label.to_string();
            let w = progress_weak.clone();
            let _ = slint::invoke_from_event_loop(move || {
                if let Some(app) = w.upgrade() {
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
                        app.set_status_text("准备就绪".into());
                        hide_error(&app);
                        update_permission_state(&app);
                        if let Some(info) = chip_info {
                            set_chip(&app, &info);
                        }
                        set_progress(&app, "完成", 100);
                        set_log(&app, &title, &detail);
                        if let Some(success_title) = success_title {
                            show_success(&app, success_title, &detail);
                        }
                    }
                    Err(ref err) if permission::is_permission_error(err) => {
                        set_progress(&app, "USB 权限不足", 0);
                        app.set_cat_mood(3);
                        app.set_status_text("权限不足".into());
                        if cfg!(target_os = "linux") && !permission::udev_rules_installed() {
                            app.set_show_permission_bar(true);
                            set_log(&app, "USB 权限不足", err);
                        } else {
                            app.set_show_permission_bar(false);
                            set_log(&app, "USB 访问失败", err);
                        }
                        show_error(&app, "USB 权限不足", err);
                    }
                    Err(ref err) if permission::is_timeout_error(err) => {
                        set_progress(&app, "连接超时", 0);
                        app.set_cat_mood(3);
                        app.set_status_text("连接超时".into());
                        app.set_show_permission_bar(false);
                        set_log(&app, "连接超时", err);
                        show_error(&app, "连接超时", err);
                    }
                    Err(err) => {
                        let title = classify_error_title(&err);
                        set_progress(&app, title, 0);
                        app.set_cat_mood(3);
                        app.set_status_text(title.into());
                        set_log(&app, title, &err);
                        show_error(&app, title, &err);
                    }
                }
            }
        });
    });
}

fn copy_log_output(app: &AppWindow) -> Result<(), String> {
    let payload = format!("{}\n\n{}", app.get_log_title(), app.get_log_text());
    let mut clipboard = arboard::Clipboard::new().map_err(|e| format!("无法访问剪贴板: {e}"))?;
    clipboard
        .set_text(payload)
        .map_err(|e| format!("复制失败: {e}"))?;
    Ok(())
}

fn refresh_online_assets(weak: Weak<AppWindow>, state: Arc<Mutex<AppState>>) {
    if let Some(app) = weak.upgrade() {
        app.set_online_sheet_visible(true);
        app.set_online_loading(true);
        app.set_online_status("正在读取 GitHub Release...".into());
        set_progress(&app, "读取在线固件", 20);
    }

    thread::spawn(move || {
        let result = online::fetch_release_assets();
        let _ = slint::invoke_from_event_loop(move || {
            if let Some(app) = weak.upgrade() {
                app.set_online_loading(false);
                match result {
                    Ok(assets) => {
                        {
                            let mut guard = state.lock().expect("state lock poisoned");
                            guard.online_assets = assets.clone();
                        }
                        set_online_options(&app, &assets);
                        if assets.is_empty() {
                            app.set_online_status(
                                "当前 Release 里还没有匹配的 CH552 BIN / CH592F FULL BIN".into(),
                            );
                            set_progress(&app, "在线固件为空", 100);
                        } else {
                            app.set_online_status(
                                format!("已加载 {} 个可选在线固件", assets.len()).into(),
                            );
                            set_progress(&app, "在线固件已加载", 100);
                        }
                    }
                    Err(err) => {
                        set_online_options(&app, &[]);
                        app.set_online_status("读取失败".into());
                        set_progress(&app, "在线固件读取失败", 0);
                        set_log(&app, "在线固件读取失败", &err);
                    }
                }
            }
        });
    });
}

/// Background USB watch: detects insertion and auto-probes once.
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

            if matches!(result, Ok(count) if count > 0) && !previous_device_state {
                op_busy.store(true, Ordering::Relaxed);

                let weak_pending = weak.clone();
                let _ = slint::invoke_from_event_loop(move || {
                    if let Some(app) = weak_pending.upgrade() {
                        app.set_busy(true);
                        app.set_cat_mood(1);
                        app.set_status_text("自动识别中".into());
                        set_progress(&app, "自动识别设备", 35);
                        set_pending_chip(&app);
                        set_log(&app, "检测到 WCH ISP 设备", "正在自动识别芯片信息...");
                    }
                });

                let probe_result = isp::probe();
                let weak_probe = weak.clone();
                let op_busy_probe = op_busy.clone();
                let _ = slint::invoke_from_event_loop(move || {
                    op_busy_probe.store(false, Ordering::Relaxed);
                    if let Some(app) = weak_probe.upgrade() {
                        app.set_busy(false);
                        match probe_result {
                            Ok(info) => {
                                set_chip(&app, &info);
                                app.set_cat_mood(2);
                                app.set_status_text("设备已识别".into());
                                set_progress(&app, "设备已识别", 100);
                                set_log(
                                    &app,
                                    "已自动识别设备",
                                    &format!(
                                        "已识别到 {}。\n刷写或校验时，程序仍会在同一次 USB 会话里再次确认芯片信息。",
                                        info.name
                                    ),
                                );
                            }
                            Err(err) => {
                                app.set_device_name("待识别设备".into());
                                app.set_device_category("待识别".into());
                                app.set_device_family("自动识别失败，刷写时会再次尝试".into());
                                app.set_cat_mood(2);
                                app.set_status_text("设备已连接".into());
                                set_progress(&app, "自动识别失败", 0);
                                set_log(
                                    &app,
                                    "设备已连接",
                                    &format!(
                                        "检测到 WCH ISP 设备，但自动识别失败：{err}\n\n你仍然可以继续刷写或校验，程序会再次尝试识别。"
                                    ),
                                );
                            }
                        }
                    }
                });
            }

            let _ = slint::invoke_from_event_loop(move || {
                if let Some(app) = weak2.upgrade() {
                    if app.get_busy() {
                        return;
                    }
                    match result {
                        Ok(count) if count > 0 => {
                            if !previous_device_state {
                                app.set_cat_mood(2);
                                if app.get_status_text() == "准备就绪" {
                                    app.set_status_text("设备已连接".into());
                                }
                            }
                        }
                        Ok(_) | Err(_) => {
                            if previous_device_state {
                                set_idle_chip(&app);
                                app.set_cat_mood(0);
                                app.set_status_text("等待连接".into());
                                clear_progress(&app);
                                set_log(&app, "设备已断开", "");
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
    if std::env::args().any(|arg| arg == "--doctor") {
        print_doctor();
        return Ok(());
    }

    if std::env::args().any(|arg| arg == "--probe") {
        match isp::probe() {
            Ok(info) => {
                println!("{} ({})", info.name, info.category);
                print!("{}", info.detail);
            }
            Err(err) => eprintln!("{err}"),
        }
        return Ok(());
    }

    if std::env::args().any(|arg| arg == "--install-udev") {
        match permission::install_udev_rules() {
            Ok(msg) => println!("{msg}"),
            Err(err) => eprintln!("{err}"),
        }
        return Ok(());
    }

    if std::env::args().any(|arg| arg == "--remove-udev") {
        match permission::remove_udev_rules() {
            Ok(msg) => println!("{msg}"),
            Err(err) => eprintln!("{err}"),
        }
        return Ok(());
    }

    let app = AppWindow::new()?;
    let state = Arc::new(Mutex::new(AppState::default()));

    app.set_ui_font_family(ui_font_family().into());
    app.set_brand_font_family(brand_font_family().into());
    app.set_status_text("准备就绪".into());
    set_idle_chip(&app);
    app.set_firmware_name("未选择固件".into());
    app.set_has_firmware(false);
    app.set_online_status("".into());
    set_online_options(&app, &[]);
    update_permission_state(&app);
    clear_progress(&app);
    if let Ok(animation) = load_success_animation() {
        start_success_animation(app.as_weak(), animation);
    }

    let stop_flag = Arc::new(AtomicBool::new(false));
    let op_busy = Arc::new(AtomicBool::new(false));
    start_usb_watch(app.as_weak(), stop_flag.clone(), op_busy.clone());

    let weak = app.as_weak();
    let op_busy_fix = op_busy.clone();
    app.on_request_fix_permission(move || {
        let weak2 = weak.clone();
        op_busy_fix.store(true, Ordering::Relaxed);
        if let Some(app) = weak.upgrade() {
            app.set_busy(true);
            app.set_cat_mood(1);
            app.set_status_text("配置权限".into());
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
                            app.set_status_text("准备就绪".into());
                            update_permission_state(&app);
                            set_log(&app, "权限配置完成", &msg);
                        }
                        Err(err) => {
                            app.set_cat_mood(3);
                            app.set_status_text("权限失败".into());
                            set_log(&app, "权限配置失败", &err);
                        }
                    }
                }
            });
        });
    });

    let weak = app.as_weak();
    let op_busy_remove = op_busy.clone();
    app.on_request_remove_permission(move || {
        let weak2 = weak.clone();
        op_busy_remove.store(true, Ordering::Relaxed);
        if let Some(app) = weak.upgrade() {
            app.set_busy(true);
            app.set_cat_mood(1);
            app.set_status_text("清除权限".into());
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
                            app.set_status_text("准备就绪".into());
                            update_permission_state(&app);
                            set_log(&app, "权限已清除", &msg);
                        }
                        Err(err) => {
                            app.set_cat_mood(3);
                            app.set_status_text("清除失败".into());
                            set_log(&app, "清除失败", &err);
                        }
                    }
                }
            });
        });
    });

    let weak = app.as_weak();
    let state_pick = state.clone();
    app.on_request_pick_bin(move || {
        if let Some(path) = FileDialog::new()
            .add_filter("Binary firmware", &["bin"])
            .pick_file()
        {
            let name = path
                .file_name()
                .and_then(|item| item.to_str())
                .unwrap_or("firmware.bin")
                .to_string();

            {
                let mut guard = state_pick.lock().expect("state lock poisoned");
                guard.selected_firmware = Some(FirmwareSource::Local(path.clone()));
            }

            if let Some(app) = weak.upgrade() {
                update_firmware_panel(&app, &name);
                if let Some(descriptor) = online::describe_firmware_name(&name) {
                    apply_firmware_descriptor(&app, &descriptor);
                } else {
                    app.set_chip_name("".into());
                    app.set_chip_family("".into());
                }
                app.set_cat_mood(0);
                app.set_online_sheet_visible(false);
                hide_success(&app);
                hide_error(&app);
                set_log(&app, "已选择本地固件", &path.display().to_string());
            }
        }
    });

    let weak = app.as_weak();
    let state_open_online = state.clone();
    app.on_request_open_online(move || {
        refresh_online_assets(weak.clone(), state_open_online.clone());
    });

    let weak = app.as_weak();
    let state_refresh_online = state.clone();
    app.on_request_refresh_online(move || {
        refresh_online_assets(weak.clone(), state_refresh_online.clone());
    });

    let weak = app.as_weak();
    app.on_request_close_online(move || {
        if let Some(app) = weak.upgrade() {
            app.set_online_sheet_visible(false);
        }
    });

    let weak = app.as_weak();
    let state_select_online = state.clone();
    app.on_request_select_online(move |index| {
        let asset = {
            let guard = state_select_online.lock().expect("state lock poisoned");
            guard.online_assets.get(index as usize).cloned()
        };
        let Some(asset) = asset else {
            return;
        };

        {
            let mut guard = state_select_online.lock().expect("state lock poisoned");
            guard.selected_firmware = Some(FirmwareSource::Online(asset.clone()));
        }

        if let Some(app) = weak.upgrade() {
            update_firmware_panel(&app, &asset.name);
            apply_firmware_descriptor(&app, &asset.descriptor);
            app.set_cat_mood(0);
            app.set_online_sheet_visible(false);
            hide_success(&app);
            hide_error(&app);
            set_log(&app, "已选择在线固件", &asset.subtitle());
        }
    });

    let weak = app.as_weak();
    app.on_request_copy_log(move || {
        if let Some(app) = weak.upgrade() {
            let showing_error = app.get_show_error();
            match copy_log_output(&app) {
                Ok(()) => {
                    app.set_cat_mood(2);
                    if showing_error {
                        app.set_status_text("错误信息已复制".into());
                    } else {
                        set_log(&app, "输出已复制", &app.get_log_text().to_string());
                    }
                }
                Err(err) => {
                    app.set_cat_mood(3);
                    if showing_error {
                        show_error(&app, "复制失败", &err);
                    }
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

    let weak = app.as_weak();
    app.on_request_dismiss_error(move || {
        if let Some(app) = weak.upgrade() {
            hide_error(&app);
        }
    });

    let weak = app.as_weak();
    let op_busy_flash = op_busy.clone();
    let state_flash = state.clone();
    app.on_request_flash_bin(move || {
        if let Some(app) = weak.upgrade() {
            if !app.get_device_connected() {
                set_log(
                    &app,
                    "未连接设备",
                    "请先连接一个 WCH ISP 设备，再进行刷写。",
                );
                return;
            }
        }

        let selection = {
            let guard = state_flash.lock().expect("state lock poisoned");
            guard.selected_firmware.clone()
        };
        let Some(selection) = selection else {
            if let Some(app) = weak.upgrade() {
                set_log(
                    &app,
                    "缺少固件",
                    "请先选择一个本地 BIN，或从在线列表中选择一个 Release BIN。",
                );
            }
            return;
        };

        run_progress_op(
            weak.clone(),
            op_busy_flash.clone(),
            "准备刷写",
            Some("刷写+校验完成"),
            move |progress| {
                let (path, display_name) = prepare_selected_firmware(&selection, &*progress)?;
                let r = isp::flash_with_progress(&path, |label, value| progress(label, value))?;
                Ok((
                    format!("已刷写 {display_name}"),
                    r.result.detail,
                    Some(r.chip),
                ))
            },
        );
    });

    let weak = app.as_weak();
    let op_busy_verify = op_busy.clone();
    let state_verify = state.clone();
    app.on_request_verify_bin(move || {
        if let Some(app) = weak.upgrade() {
            if !app.get_device_connected() {
                set_log(
                    &app,
                    "未连接设备",
                    "请先连接一个 WCH ISP 设备，再进行校验。",
                );
                return;
            }
        }

        let selection = {
            let guard = state_verify.lock().expect("state lock poisoned");
            guard.selected_firmware.clone()
        };
        let Some(selection) = selection else {
            if let Some(app) = weak.upgrade() {
                set_log(
                    &app,
                    "缺少固件",
                    "请先选择一个本地 BIN，或从在线列表中选择一个 Release BIN。",
                );
            }
            return;
        };

        run_progress_op(
            weak.clone(),
            op_busy_verify.clone(),
            "准备校验",
            Some("校验成功"),
            move |progress| {
                let (path, display_name) = prepare_selected_firmware(&selection, &*progress)?;
                let r = isp::verify_with_progress(&path, |label, value| progress(label, value))?;
                Ok((
                    format!("校验通过 {display_name}"),
                    r.result.detail,
                    Some(r.chip),
                ))
            },
        );
    });

    let weak = app.as_weak();
    let op_busy_erase_code = op_busy.clone();
    app.on_request_erase_code(move || {
        if let Some(app) = weak.upgrade() {
            if !app.get_device_connected() {
                set_log(
                    &app,
                    "未连接设备",
                    "请先连接一个 WCH ISP 设备，再清空 Codeflash。",
                );
                return;
            }
        }

        run_progress_op(
            weak.clone(),
            op_busy_erase_code.clone(),
            "准备清除 Codeflash",
            None,
            |progress| {
                let r = isp::erase_code_with_progress(|label, value| progress(label, value))?;
                Ok((r.result.summary, r.result.detail, Some(r.chip)))
            },
        );
    });

    let weak = app.as_weak();
    let op_busy_erase_data = op_busy.clone();
    app.on_request_erase_data(move || {
        if let Some(app) = weak.upgrade() {
            if !app.get_device_connected() {
                set_log(
                    &app,
                    "未连接设备",
                    "请先连接一个 WCH ISP 设备，再清空 Dataflash。",
                );
                return;
            }
        }

        run_progress_op(
            weak.clone(),
            op_busy_erase_data.clone(),
            "准备清除 Dataflash",
            None,
            |progress| {
                let r = isp::erase_data_with_progress(|label, value| progress(label, value))?;
                Ok((r.result.summary, r.result.detail, Some(r.chip)))
            },
        );
    });

    let result = app.run();
    stop_flag.store(true, Ordering::Relaxed);
    result
}
