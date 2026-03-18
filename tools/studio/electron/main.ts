import { app, BrowserWindow, session } from 'electron'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const __dirname = path.dirname(fileURLToPath(import.meta.url))

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
    return details.deviceType === 'hid'
  })

  // Auto-select first matching HID device; web app handles its own device UI
  session.defaultSession.on('select-hid-device', (event, details, callback) => {
    event.preventDefault()
    callback(details.deviceList[0]?.deviceId ?? '')
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
