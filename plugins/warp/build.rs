#![allow(unused_imports)]
use std::path::PathBuf;
use std::process::Command;
use std::path::Path;

#[cfg(feature = "test")]
fn compile_rust(file: PathBuf) -> bool {
    let out_dir = std::env::var_os("OUT_DIR").unwrap();
    let rustc = std::env::var_os("RUSTC").unwrap();
    let rustc = rustc.to_str().unwrap();
    let mut rustc = rustc.split('\x1f');
    let mut cmd = Command::new(rustc.next().unwrap());
    cmd.args(rustc)
        .arg("--crate-type=rlib")
        .arg("--out-dir")
        .arg(out_dir)
        .arg(file);
    cmd.status().expect("failed to invoke rustc").success()
}

fn main() {

    let binja_base_dir: &str = option_env!("DEP_BINARYNINJACORE_BASE_DIR").unwrap_or_else(|| {
        // If the environment variable is not set, try the default locations from
        // https://docs.binary.ninja/guide/#binary-path

        #[cfg(target_os = "macos")]
        {
            let default = "/Applications/Binary Ninja.app/";
            if Path::new(default).exists() {
                return default
            }
        }

        #[cfg(target_os = "windows")]
        {
            let default = r"C:\Program Files\Vector35\BinaryNinja";
            if Path::new(default).exists() {
                return default
            }
            // Nothing at default path, check user path
            if let Some(local_app_data) = std::env::var_os("LOCALAPPDATA") {
                let user_path = Path::new(&local_app_data).join("Vector35").join("BinaryNinja");
                if user_path.exists() {
                    return user_path.to_str().unwrap()
                }
            }
        }

        panic!("DEP_BINARYNINJACORE_BASE_DIR must be set to the base directory of the Binary Ninja installation");
    });

    if let link_path = binja_base_dir {
        println!("cargo::rustc-link-lib=dylib=binaryninjacore");
        println!("cargo::rustc-link-search={}", link_path);

        #[cfg(target_os = "macos")]
        {
            // On macOS the binaryninjacore dylib is in the MacOS directory
            println!("cargo::rustc-link-search={}/Contents/MacOS", link_path);
            println!("cargo::rustc-link-arg=-Wl,-rpath,{0}/Contents/MacOS,-L{0}/Contents/MacOS", link_path);
        }

        #[cfg(not(target_os = "windows"))]
        {
            println!("cargo::rustc-link-arg=-Wl,-rpath,{0},-L{0}", link_path);
        }
    }

    #[cfg(feature = "test")]
    {
        let out_dir = std::env::var("OUT_DIR").expect("OUT_DIR specified");
        let out_dir_path = PathBuf::from(out_dir);

        // Copy all binaries to OUT_DIR for unit tests.
        let bin_dir: PathBuf = "fixtures/bin".into();
        if let Ok(entries) = std::fs::read_dir(bin_dir) {
            for entry in entries {
                let entry = entry.unwrap();
                let path = entry.path();
                if path.is_file() {
                    let file_name = path.file_name().unwrap();
                    let dest_path = out_dir_path.join(file_name);
                    std::fs::copy(&path, &dest_path).expect("failed to copy binary to OUT_DIR");
                }
            }
        }

        // Compile all .c files in fixtures/src directory for unit tests.
        let src_dir: PathBuf = "fixtures/src".into();
        if let Ok(entries) = std::fs::read_dir(src_dir) {
            for entry in entries {
                let entry = entry.unwrap();
                let path = entry.path();
                match path.extension().map(|s| s.to_str().unwrap()) {
                    Some("c") => {
                        cc::Build::new()
                            .file(&path)
                            .compile(path.file_stem().unwrap().to_str().unwrap());
                    }
                    Some("rs") => {
                        compile_rust(path);
                    }
                    _ => {}
                }
            }
        }
    }
}
