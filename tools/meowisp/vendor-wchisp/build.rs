use std::env;
use std::fs;
use std::path::PathBuf;

fn main() {
    println!("cargo:rerun-if-env-changed=WCHISP_CH375_DLL");
    println!("cargo:rerun-if-changed=../../.cache/windows-assets/CH375DLL64.dll");

    let out_dir = PathBuf::from(env::var("OUT_DIR").expect("OUT_DIR not set"));
    let generated = out_dir.join("ch375_embedded.rs");

    let env_path = env::var("WCHISP_CH375_DLL")
        .ok()
        .filter(|s| !s.trim().is_empty())
        .map(PathBuf::from);
    let default_cache_path = PathBuf::from("../../.cache/windows-assets/CH375DLL64.dll");
    let selected_path = env_path.or_else(|| default_cache_path.is_file().then_some(default_cache_path));

    let body = match selected_path {
        Some(path) => {
            let canonical = path.canonicalize().unwrap_or(path);
            println!("cargo:rerun-if-changed={}", canonical.display());
            format!(
                "pub const EMBEDDED_CH375_DLL: Option<&[u8]> = Some(include_bytes!(r#\"{}\"#));\n",
                canonical.display()
            )
        }
        None => "pub const EMBEDDED_CH375_DLL: Option<&[u8]> = None;\n".to_string(),
    };

    fs::write(generated, body).expect("failed to write ch375_embedded.rs");
}
