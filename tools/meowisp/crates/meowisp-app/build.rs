use std::env;
use std::fs;
use std::path::PathBuf;

fn main() {
    println!("cargo:rerun-if-env-changed=BK_MEOWISP_VERSION");

    let out_dir = PathBuf::from(env::var("OUT_DIR").expect("OUT_DIR not set"));
    let version = env::var("BK_MEOWISP_VERSION")
        .ok()
        .filter(|v| !v.trim().is_empty())
        .unwrap_or_else(|| "dev".to_string());
    fs::write(
        out_dir.join("app_version.rs"),
        format!("pub const APP_VERSION: &str = {:?};\n", version),
    )
    .expect("failed to write app_version.rs");

    slint_build::compile("ui/app.slint").expect("failed to compile Slint UI");
}
