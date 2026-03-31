//! USB transportation.
use std::time::Duration;

use anyhow::Result;

use super::Transport;

const USB_TIMEOUT_MS: u64 = 5000;

#[cfg(not(target_os = "windows"))]
mod imp {
    use super::*;
    use rusb::{Context, DeviceHandle, UsbContext};

    const ENDPOINT_OUT: u8 = 0x02;
    const ENDPOINT_IN: u8 = 0x82;

    pub struct UsbTransport {
        device_handle: DeviceHandle<rusb::Context>,
    }

    impl UsbTransport {
        pub fn scan_devices() -> Result<usize> {
            let context = Context::new()?;

            let n = context
                .devices()?
                .iter()
                .filter(|device| {
                    device
                        .device_descriptor()
                        .map(|desc| {
                            (desc.vendor_id() == 0x4348 || desc.vendor_id() == 0x1a86)
                                && desc.product_id() == 0x55e0
                        })
                        .unwrap_or(false)
                })
                .enumerate()
                .map(|(i, device)| {
                    log::debug!("Found WCH ISP USB device #{}: [{:?}]", i, device);
                })
                .count();
            Ok(n)
        }

        pub fn open_nth(nth: usize) -> Result<UsbTransport> {
            log::info!("Opening USB device #{}", nth);

            let context = Context::new()?;

            let device = context
                .devices()?
                .iter()
                .filter(|device| {
                    device
                        .device_descriptor()
                        .map(|desc| {
                            (desc.vendor_id() == 0x4348 || desc.vendor_id() == 0x1a86)
                                && desc.product_id() == 0x55e0
                        })
                        .unwrap_or(false)
                })
                .nth(nth)
                .ok_or(anyhow::format_err!(
                    "No WCH ISP USB device found(4348:55e0 or 1a86:55e0 device not found at index #{})",
                    nth
                ))?;
            log::debug!("Found USB Device {:?}", device);

            let device_handle = match device.open() {
                Ok(handle) => handle,
                #[cfg(target_os = "linux")]
                Err(rusb::Error::Access) => {
                    log::error!("Failed to open USB device: {:?}", device);
                    log::warn!("It's likely the udev rules is not installed properly. Please refer to README.md for more details.");
                    anyhow::bail!("Failed to open USB device on Linux due to no enough permission");
                }
                Err(e) => {
                    log::error!("Failed to open USB device: {}", e);
                    anyhow::bail!("Failed to open USB device");
                }
            };

            let config = device.config_descriptor(0)?;

            let mut endpoint_out_found = false;
            let mut endpoint_in_found = false;
            if let Some(intf) = config.interfaces().next() {
                if let Some(desc) = intf.descriptors().next() {
                    for endpoint in desc.endpoint_descriptors() {
                        if endpoint.address() == ENDPOINT_OUT {
                            endpoint_out_found = true;
                        }
                        if endpoint.address() == ENDPOINT_IN {
                            endpoint_in_found = true;
                        }
                    }
                }
            }

            if !(endpoint_out_found && endpoint_in_found) {
                anyhow::bail!("USB Endpoints not found");
            }

            device_handle.set_active_configuration(1)?;
            let _config = device.active_config_descriptor()?;
            let _descriptor = device.device_descriptor()?;

            device_handle.claim_interface(0)?;

            Ok(UsbTransport { device_handle })
        }

        pub fn open_any() -> Result<UsbTransport> {
            Self::open_nth(0)
        }
    }

    impl Drop for UsbTransport {
        fn drop(&mut self) {
            let _ = self.device_handle.release_interface(0);
        }
    }

    impl Transport for UsbTransport {
        fn send_raw(&mut self, raw: &[u8]) -> Result<()> {
            self.device_handle
                .write_bulk(ENDPOINT_OUT, raw, Duration::from_millis(USB_TIMEOUT_MS))?;
            Ok(())
        }

        fn recv_raw(&mut self, timeout: Duration) -> Result<Vec<u8>> {
            let mut buf = [0u8; 64];
            let nread = self.device_handle.read_bulk(ENDPOINT_IN, &mut buf, timeout)?;
            Ok(buf[..nread].to_vec())
        }
    }
}

#[cfg(target_os = "windows")]
mod imp {
    use super::*;
    use std::ffi::c_void;
    use std::fs;
    use std::path::PathBuf;
    use std::sync::OnceLock;

    use anyhow::{anyhow, Context};
    use libloading::Library;

    type Handle = *mut c_void;
    type Bool = i32;
    type Ulong = u32;

    const INVALID_HANDLE_VALUE: Handle = -1isize as Handle;
    const MAX_DEVICES_TO_PROBE: usize = 16;
    const READ_BUFFER_LEN: usize = 64;

    include!(concat!(env!("OUT_DIR"), "/ch375_embedded.rs"));

    type Ch375OpenDeviceFn = unsafe extern "system" fn(Ulong) -> Handle;
    type Ch375CloseDeviceFn = unsafe extern "system" fn(Ulong) -> Bool;
    type Ch375SetTimeoutFn = unsafe extern "system" fn(Ulong, Ulong, Ulong) -> Bool;
    type Ch375ReadDataFn = unsafe extern "system" fn(Ulong, *mut c_void, *mut Ulong) -> Bool;
    type Ch375WriteDataFn = unsafe extern "system" fn(Ulong, *mut c_void, *mut Ulong) -> Bool;

    struct Ch375Api {
        _library: Library,
        open_device: Ch375OpenDeviceFn,
        close_device: Ch375CloseDeviceFn,
        set_timeout: Ch375SetTimeoutFn,
        read_data: Ch375ReadDataFn,
        write_data: Ch375WriteDataFn,
    }

    impl Ch375Api {
        fn load() -> Result<Self> {
            let mut candidates = Vec::new();
            if let Some(path) = embedded_dll_path()? {
                candidates.push(path);
            }
            candidates.push(PathBuf::from("CH375DLL64.dll"));
            if let Ok(exe) = std::env::current_exe() {
                if let Some(dir) = exe.parent() {
                    candidates.insert(0, dir.join("CH375DLL64.dll"));
                }
            }

            let mut last_err = None;
            for candidate in candidates {
                let lib = unsafe { Library::new(&candidate) };
                match lib {
                    Ok(library) => unsafe {
                        let open_device = *library
                            .get::<Ch375OpenDeviceFn>(b"CH375OpenDevice\0")
                            .context("missing CH375OpenDevice export")?;
                        let close_device = *library
                            .get::<Ch375CloseDeviceFn>(b"CH375CloseDevice\0")
                            .context("missing CH375CloseDevice export")?;
                        let set_timeout = *library
                            .get::<Ch375SetTimeoutFn>(b"CH375SetTimeout\0")
                            .context("missing CH375SetTimeout export")?;
                        let read_data = *library
                            .get::<Ch375ReadDataFn>(b"CH375ReadData\0")
                            .context("missing CH375ReadData export")?;
                        let write_data = *library
                            .get::<Ch375WriteDataFn>(b"CH375WriteData\0")
                            .context("missing CH375WriteData export")?;

                        log::debug!("Loaded CH375 DLL from {}", candidate.display());
                        return Ok(Self {
                            _library: library,
                            open_device,
                            close_device,
                            set_timeout,
                            read_data,
                            write_data,
                        });
                    },
                    Err(err) => last_err = Some((candidate, err)),
                }
            }

            if let Some((path, err)) = last_err {
                Err(anyhow!(
                    "Failed to load CH375DLL64.dll from {}: {}",
                    path.display(),
                    err
                ))
            } else {
                Err(anyhow!("Failed to load CH375DLL64.dll"))
            }
        }

        fn open_index(&self, index: usize) -> Result<Handle> {
            let handle = unsafe { (self.open_device)(index as Ulong) };
            if handle.is_null() || handle == INVALID_HANDLE_VALUE {
                anyhow::bail!("device index #{index} is not available via CH375DLL64.dll");
            }
            Ok(handle)
        }

        fn configure_device(&self, index: usize, timeout_ms: u64) -> Result<()> {
            let ok = unsafe { (self.set_timeout)(index as Ulong, timeout_ms as Ulong, timeout_ms as Ulong) };
            if ok == 0 {
                anyhow::bail!("CH375SetTimeout failed for device #{index}");
            }
            Ok(())
        }

        fn write(&self, index: usize, raw: &[u8]) -> Result<()> {
            let mut len = raw.len() as Ulong;
            let mut buf = raw.to_vec();
            let ok = unsafe {
                (self.write_data)(
                    index as Ulong,
                    buf.as_mut_ptr().cast::<c_void>(),
                    &mut len as *mut Ulong,
                )
            };
            if ok == 0 {
                anyhow::bail!("CH375WriteData failed for device #{index}");
            }
            if len as usize != raw.len() {
                anyhow::bail!(
                    "CH375WriteData short write for device #{index}: expected {}, wrote {}",
                    raw.len(),
                    len
                );
            }
            Ok(())
        }

        fn read(&self, index: usize, timeout: Duration) -> Result<Vec<u8>> {
            self.configure_device(index, timeout.as_millis().clamp(1, u32::MAX as u128) as u64)?;

            let mut len = READ_BUFFER_LEN as Ulong;
            let mut buf = [0u8; READ_BUFFER_LEN];
            let ok = unsafe {
                (self.read_data)(
                    index as Ulong,
                    buf.as_mut_ptr().cast::<c_void>(),
                    &mut len as *mut Ulong,
                )
            };
            if ok == 0 {
                anyhow::bail!("CH375ReadData failed for device #{index}");
            }
            Ok(buf[..len as usize].to_vec())
        }
    }

    fn embedded_dll_path() -> Result<Option<PathBuf>> {
        static EXTRACTED: OnceLock<Result<Option<PathBuf>, String>> = OnceLock::new();

        EXTRACTED
            .get_or_init(|| {
                let Some(bytes) = EMBEDDED_CH375_DLL else {
                    return Ok(None);
                };

                let base = std::env::temp_dir().join("meowisp");
                fs::create_dir_all(&base).map_err(|e| format!("create temp dir failed: {e}"))?;
                let path = base.join("CH375DLL64.dll");

                let should_write = match fs::metadata(&path) {
                    Ok(meta) => meta.len() != bytes.len() as u64,
                    Err(_) => true,
                };

                if should_write {
                    fs::write(&path, bytes).map_err(|e| format!("extract embedded CH375DLL64.dll failed: {e}"))?;
                }

                Ok(Some(path))
            })
            .clone()
            .map_err(anyhow::Error::msg)
    }

    pub struct UsbTransport {
        api: Ch375Api,
        device_index: usize,
    }

    impl UsbTransport {
        pub fn scan_devices() -> Result<usize> {
            let api = Ch375Api::load().context("failed to initialize CH375 DLL backend")?;
            let mut count = 0usize;
            for index in 0..MAX_DEVICES_TO_PROBE {
                if api.open_index(index).is_ok() {
                    log::debug!("Found WCH ISP USB device #{} via CH375 DLL", index);
                    let _ = unsafe { (api.close_device)(index as Ulong) };
                    count += 1;
                } else if count > 0 {
                    break;
                }
            }
            Ok(count)
        }

        pub fn open_nth(nth: usize) -> Result<UsbTransport> {
            log::info!("Opening USB device #{} via CH375 DLL", nth);
            let api = Ch375Api::load().context("failed to initialize CH375 DLL backend")?;
            let _handle = api.open_index(nth)?;
            api.configure_device(nth, USB_TIMEOUT_MS)?;
            Ok(UsbTransport {
                api,
                device_index: nth,
            })
        }

        pub fn open_any() -> Result<UsbTransport> {
            Self::open_nth(0)
        }
    }

    impl Drop for UsbTransport {
        fn drop(&mut self) {
            let _ = unsafe { (self.api.close_device)(self.device_index as Ulong) };
        }
    }

    impl Transport for UsbTransport {
        fn send_raw(&mut self, raw: &[u8]) -> Result<()> {
            self.api.write(self.device_index, raw)
        }

        fn recv_raw(&mut self, timeout: Duration) -> Result<Vec<u8>> {
            self.api.read(self.device_index, timeout)
        }
    }
}

pub use imp::UsbTransport;
