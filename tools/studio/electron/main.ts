import { app, BrowserWindow, session } from 'electron'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))
const BINARY_KEYBOARD_VENDOR_ID = 0x413d
const BINARY_KEYBOARD_PRODUCT_ID = 0x2107

function scoreHidDevice(device: Electron.HIDDevice): number {
  if (
    device.vendorId !== BINARY_KEYBOARD_VENDOR_ID ||
    device.productId !== BINARY_KEYBOARD_PRODUCT_ID
  ) {
    return 0
  }

  const identity = `${device.deviceId} ${device.guid ?? ''} ${device.name}`.toLowerCase()
  if (identity.includes('mi_03')) return 100
  if (identity.includes('usage_page_ff00') || identity.includes('ff00')) return 90
  if (identity.includes('col04') || identity.includes('&col04')) return 80
  if (identity.includes('col03') || identity.includes('&col03')) return 70
  return 10
}

function createWindow(): void {
  const win = new BrowserWindow({
    width: 1280,
    height: 800,
    minWidth: 960,
    minHeight: 640,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false,
    },
    title: 'BinaryKeyboard Studio',
    show: false,
  })

  // Auto-grant stored WebHID device access (triggered after first selection)
  session.defaultSession.setDevicePermissionHandler((details) => {
    if (details.deviceType !== 'hid') return false
    const device = details.device as Electron.HIDDevice
    return (
      device.vendorId === BINARY_KEYBOARD_VENDOR_ID &&
      device.productId === BINARY_KEYBOARD_PRODUCT_ID
    )
  })

  // Auto-select the BinaryKeyboard vendor HID interface. CH592 is a composite
  // device, so taking the first HID interface can open the keyboard endpoint
  // instead of the configuration endpoint.
  session.defaultSession.on('select-hid-device', (event, details, callback) => {
    event.preventDefault()
    const selected = details.deviceList
      .map((device) => ({ device, score: scoreHidDevice(device) }))
      .filter((entry) => entry.score > 0)
      .sort((a, b) => b.score - a.score)[0]?.device

    callback(selected?.deviceId ?? '')
  })

  win.loadFile(path.join(__dirname, '../dist/index.html'))

  win.once('ready-to-show', () => {
    win.show()
  })

  // Keep static title
  win.on('page-title-updated', (event) => {
    event.preventDefault()
  })
}

app.whenReady().then(() => {
  createWindow()
  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})
